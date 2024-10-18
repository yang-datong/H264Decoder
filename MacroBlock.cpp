#include "MacroBlock.hpp"
#include "BitStream.hpp"
#include "Cabac.hpp"
#include "Cavlc.hpp"
#include "Constants.hpp"
#include "PictureBase.hpp"
#include "SliceHeader.hpp"
#include "Type.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>

MacroBlock::~MacroBlock() {
  _is_cabac = 0;
  FREE(_cabac);
  FREE(_bs);
  FREE(_cavlc);
}

void MacroBlock::initFromSlice(const SliceHeader &header,
                               const SliceData &slice_data) {
  mb_field_decoding_flag = slice_data.mb_field_decoding_flag;
  MbaffFrameFlag = header.MbaffFrameFlag;
  field_pic_flag = header.field_pic_flag;
  CurrMbAddr = slice_data.CurrMbAddr;

  slice_number = slice_data.slice_number;
  m_slice_type = header.slice_type;
  mb_skip_flag = slice_data.mb_skip_flag;
  bottom_field_flag = header.bottom_field_flag;

  //去块滤波器是否使用，应该是与宏块为单位进行判断，因为去块滤波器可能只会应用于Slice内部的宏块边缘，而不应用与slice外部的宏块边缘
  disable_deblocking_filter_idc = header.disable_deblocking_filter_idc;
  FilterOffsetA = header.FilterOffsetA;
  FilterOffsetB = header.FilterOffsetB;

  constrained_intra_pred_flag = header.m_pps->constrained_intra_pred_flag;
}

// 7.3.5 Macroblock layer syntax -> page 57
/* 负责解码一个宏块（这里的宏块指的是16x16 的矩阵块，并不是真的“一个”宏块）的各种信息，包括宏块类型、预测模式、残差数据 */
int MacroBlock::decode(BitStream &bs, PictureBase &picture,
                       const SliceData &slice_data, Cabac &cabac) {
  /* ------------------ 初始化常用变量 ------------------ */
  _pic = &picture;
  _cabac = &cabac;
  _bs = &bs;
  _is_cabac = _pic->m_slice->slice_header->m_pps
                  ->entropy_coding_mode_flag; // 是否CABAC编码
  /* ------------------  End ------------------ */

  SliceHeader *header = _pic->m_slice->slice_header;
  const SPS *sps = _pic->m_slice->slice_header->m_sps;
  const PPS *pps = _pic->m_slice->slice_header->m_pps;
  initFromSlice(*header, slice_data);

  // 计算宏块的左上角亮度样本相对于图片左上角样本的位置 (x,y)，如x,y应该是(0,0) (16,0) (32,0) (48,0) */
  int32_t &x = _pic->m_mbs[CurrMbAddr].m_mb_position_x;
  int32_t &y = _pic->m_mbs[CurrMbAddr].m_mb_position_y;
  _pic->inverse_mb_scanning_process(MbaffFrameFlag, CurrMbAddr,
                                    mb_field_decoding_flag, x, y);

  /* 解码当前宏块类型 */
  process_mb_type(*header, header->slice_type);

  /* 1. 如果宏块类型是 I_PCM，直接从比特流中读取未压缩的 LCM 样本数据（直接copy原始数据） */
  if (m_mb_type_fixed == I_PCM) pcm_sample_copy(sps);

  /* 2. 非I_PCM，根据宏块类型和预测模式进行子宏块预测或宏块预测 */
  else {
    int32_t transform_size_8x8_flag_temp = 0;
    /* 是否所有子宏块的大小都不小于8x8 */
    bool noSubMbSizeLessThan8x8 = true;

    //------------------------- 3.宏块预测信息解码 -------------------------
    // 对于P_8x8,B_8x8宏块，且为无预测模式时，说明需要进行子宏块预测（一般子宏块预测主要用于P和B帧中)
    if (m_name_of_mb_type != I_NxN && m_mb_pred_mode != Intra_16x16 &&
        m_NumMbPart == 4) {
      //  解析4个8x8子宏块类型
      for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
        process_sub_mb_type(mbPartIdx);

      // 解码子宏块预测(或帧间，或帧内)信息，如获取预测模式（帧内）、运动矢量差（帧间）
      sub_mb_pred(slice_data);

      // 检查是否所有子宏块的大小都不小于8x8，用于后面的8x8变换模式
      check_sub_mb_size(noSubMbSizeLessThan8x8, sps->direct_8x8_inference_flag);
    }
    // 其他情况均为整宏块预测
    else {
      //若为I_8x8,I_4x4预测模式，且编码器支持8x8变换，则进一步判断是否使用8x8变换
      if (pps->transform_8x8_mode_flag && m_name_of_mb_type == I_NxN)
        process_transform_size_8x8_flag(transform_size_8x8_flag_temp);

      // 解码整宏块预测(或帧间，或帧内)信息，如获取预测模式（帧内）、运动矢量差（帧间）
      mb_pred(slice_data);
    }

    //------------------------- 4.编码块模式和残差数据 -------------------------
    /* 1. 非Intra_16x16模式（如I_4x4、I_8x8,P,B等）的宏块可能包含部分子块不包含重要信息，也可能涉及更复杂或更灵活的编码块结构，需要动态CBP决策处理。*/
    if (m_mb_pred_mode != Intra_16x16) {
      //std::cout << "mb_type:" <<mb_type << ",m_name_of_mb_type:" << m_name_of_mb_type << ",m_mb_pred_mode:" << m_mb_pred_mode << std::endl;
      //解码CBP，它决定了哪些块（亮度块和色度块）包含非零系数。
      process_coded_block_pattern(sps->ChromaArrayType);

      /* 宏块中存在亮度块包含非零系数，需要对这些块进行逆变换和反量化，且宏块大小至少有8x8的大小(8x8,16x8,8x16,16x16)，在这种情况下，可能需要考虑使用8x8变换 */
      if (CodedBlockPatternLuma > 0 && noSubMbSizeLessThan8x8 &&
          (m_name_of_mb_type != B_Direct_16x16 ||
           sps->direct_8x8_inference_flag))
        //注，这里I_NxN在上面步骤中已经进行了一次解码
        if (pps->transform_8x8_mode_flag && m_name_of_mb_type != I_NxN)
          process_transform_size_8x8_flag(transform_size_8x8_flag_temp);
    }

    /* 2. Intra_16x16模式的整个宏块的残差都需要被处理。由于整个宏块被统一处理，残差编码是整个块的一部分，不需要分别确定哪些子块包含重要信息 */
    // 宏块中存在非零系数(或亮度或色度）或当前为整块预测(简单区域一般压缩数据较多)，处理残差数据
    if (CodedBlockPatternLuma > 0 || CodedBlockPatternChroma > 0 ||
        m_mb_pred_mode == Intra_16x16) {
      process_mb_qp_delta();
      /* 处理残差数据，startIdx 和 endIdx 是相对于整个 4x4 块的残差系数的最大索引，比如I_16x16宏块被分成了16个4x4的子块（索引从0到15），又或者I_8x8宏块分为8个I_4x4，同时也可以以8x8的宏块为单位处理 */
      residual(0, 15);
      //TODO 没搞清楚这里的startIdx,endIdx具体是用在哪里 <24-10-03 16:08:51, YangJing>
    }
  }

  /* 4. 根据解码的量化参数增量（mb_qp_delta），更新当前宏块的量化参数 QPY。*/
  const int32_t max = 26 + sps->QpBdOffsetY / 2;
  process_mb_qp(header, -(max - 1), max);

  // 7.4.5 Macroblock layer semantics -> mb_qp_delta
  /* 计算出是否启用变换旁路模式 */
  TransformBypassModeFlag = sps->qpprime_y_zero_transform_bypass_flag && !QP1Y;
  return 0;
}

#define MB_TYPE_P_SP_Skip 5
#define MB_TYPE_B_Skip 23
/* 该函数将一个宏块进行预处理，设置宏块跳过的状态，但是并不需要进行真正的解码操作 */
int MacroBlock::decode_skip(PictureBase &picture, const SliceData &slice_data,
                            Cabac &cabac) {
  /* ------------------ 初始化常用变量 ------------------ */
  this->_cabac = &cabac;
  this->_pic = &picture;
  /* ------------------  End ------------------ */
  SliceHeader *header = _pic->m_slice->slice_header;
  m_slice_type_fixed = header->slice_type;
  initFromSlice(*header, slice_data);

  // 计算宏块的左上角亮度样本相对于图片左上角样本的位置 ( x, y )，比如说x,y应该是(0,0) (16,0) (32,0) (48,0) */
  int32_t &x = _pic->m_mbs[CurrMbAddr].m_mb_position_x;
  int32_t &y = _pic->m_mbs[CurrMbAddr].m_mb_position_y;
  _pic->inverse_mb_scanning_process(MbaffFrameFlag, CurrMbAddr,
                                    mb_field_decoding_flag, x, y);

  /* 宏块类型处理：将宏块类型设置为跳过解码类型 */
  if (header->slice_type == SLICE_P || header->slice_type == SLICE_SP)
    m_mb_type_fixed = mb_type = MB_TYPE_P_SP_Skip;
  else if (header->slice_type == SLICE_B)
    m_mb_type_fixed = mb_type = MB_TYPE_B_Skip;

  /* 宏块预测模式处理：据宏块类型，设置宏块的预测模式。这一步决定了如何对宏块进行预测和解码 */
  int ret =
      MbPartPredMode(m_mb_type_fixed, 0, m_name_of_mb_type, m_mb_pred_mode);
  RET(ret);

  /* 计算当前宏块的量化参数（QP）。量化参数影响解码后的图像质量和压缩率*/
  process_mb_qp(header);

  /* 将宏块类型和相关的片段信息设置到宏块中，以便后续的解码过程使用 */
  ret = MbPartPredMode();

  // 因CABAC会用到MbPartWidth/MbPartHeight信息，所以需要尽可能提前设置相关值
  RET(ret);
  return 0;
}

int MacroBlock::process_mb_qp(SliceHeader *&header, int32_t min, int32_t max) {
  const uint32_t QpBdOffsetY = header->m_sps->QpBdOffsetY; //输入
  int32_t &QPY_prev = header->QPY_prev;                    //输出

  /* 7.4.5 Macroblock layer semantics(page 105)：mb_qp_delta可以改变宏块层中QPY的值。 mb_qp_delta 的解码值应在 -( 26 + QpBdOffsetY / 2) 至 +( 25 + QpBdOffsetY / 2 ) 的范围内，包括端值。当 mb_qp_delta 对于任何宏块（包括 P_Skip 和 B_Skip 宏块类型）不存在时，应推断其等于 0。如果 mb_qp_delta 超出了范围，它会被修正到合法范围内。 */
  if (min == 0 && max == 0)
    //宏块被跳过时，量化参数差值（mb_qp_delta）为0
    mb_qp_delta = 0;
  else
    mb_qp_delta = CLIP3(min, max, mb_qp_delta);

  /*  计算当前宏块的量化参数 QPY，同时更新前宏块的量化参数（即当前宏快，对于下一个宏快来说就是前一个）
   *  52: 在H.264标准中，量化参数的范围是0到51。这里的52用于处理循环。
   *  QpBdOffsetY: 色度量化参数偏移量，通常与色度位深有关。在更高的色度位深（比如10bit或12bit）时，该偏移量使得量化参数可以在更宽的范围内调整。
   *  其中 (QPY_prev + mb_qp_delta + 52 + 2 * QpBdOffsetY) 是为了确保即使在 (mb_qp_delta) 为负数的情况下，加法的结果也保持正值，从而有效地执行模运算。
   *  模运算: (x % (52 + QpBdOffsetY)) 确保结果在合法的量化参数范围内。
   *  调整值：最后从上一步的结果中减去 ( QpBdOffsetY )，将量化参数值重新调整回标准范围。这步确保即使在使用色度偏移的情况下，最终的量化参数仍然是有效的。*/
  // $QP_y = (QP_{y0} + \Delta{QP} + 52 + 2 \cdot QP_{offset_y}) \bmod ( 52 + QP_{offset_y} ) - QP_{offset_y}$
  QPY_prev = QPY =
      ((QPY_prev + mb_qp_delta + 52 + 2 * QpBdOffsetY) % (52 + QpBdOffsetY)) -
      QpBdOffsetY;

  // 还原偏移后的QP
  QP1Y = QPY + QpBdOffsetY;
  return 0;
}

int MacroBlock::pcm_sample_copy(const SPS *sps) {
  while (!_bs->byte_aligned())
    pcm_alignment_zero_bit = _bs->readUn(1);

#define LUMA_MB_SIZE 256 // 16x16
  for (uint32_t i = 0; i < LUMA_MB_SIZE; i++)
    pcm_sample_luma[i] = _bs->readUn(sps->BitDepthY);
  for (uint32_t i = 0; i < 2 * sps->MbWidthC * sps->MbHeightC; i++)
    pcm_sample_chroma[i] = _bs->readUn(sps->BitDepthC);
  return 0;
}

// 7.3.5.1 Macroblock prediction syntax
/* 根据宏块的预测模式（m_mb_pred_mode），处理亮度和色度的预测模式以及运动矢量差（MVD）的计算。此处的预测模式包括帧内预测（Intra）和帧间预测（Inter）*/
int MacroBlock::mb_pred(const SliceData &slice_data) {
  /* ------------------ 设置别名 ------------------ */
  const SliceHeader *header = _pic->m_slice->slice_header;
  const SPS *sps = _pic->m_slice->slice_header->m_sps;
  /* ------------------  End ------------------ */

  /* --------------------------这一部分属于帧内预测-------------------------- */
  /* 获取当前宏块所使用的预测模式 */
  if (m_mb_pred_mode == Intra_4x4 || m_mb_pred_mode == Intra_8x8 ||
      m_mb_pred_mode == Intra_16x16) {
    /* 宏块的大小通常是 16x16 像素。对于 Intra_4x4 预测模式，宏块被分区为 16 个 4x4 的亮度块（luma blocks），每个 4x4 块独立进行预测。这种模式允许更细粒度的预测，从而更好地适应图像中的局部变化。 */

    // NOTE: 这里有一个规则：每个块（假如是4x4) 可以选择一个预测模式（总共有 9 种可能的模式）。然而，相邻的 4x4 块通常会选择相同或相似的预测模式。为了减少冗余信息，H.264 标准引入了一个标志位 prev_intra4x4_pred_mode_flag，用于指示当前块是否使用了与前一个块相同的预测模式。通过这种方式，编码器可以在相邻块使用相同预测模式时节省比特数，因为不需要为每个块单独编码预测模式。只有在预测模式发生变化时，才需要额外编码新的模式，从而减少了整体的编码开销。

    /* 1. 对于 Intra_4x4 模式，遍历 16 个 4x4 的亮度块: */
    if (m_mb_pred_mode == Intra_4x4) {
      for (int luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; luma4x4BlkIdx++) {
        //解码每个块的前一个 Intra 4x4 预测模式标志
        process_prev_intra4x4_pred_mode_flag(luma4x4BlkIdx);
        //当前一个块不存在预测模式时，解码当前的 Intra 4x4 预测模式
        if (prev_intra4x4_pred_mode_flag[luma4x4BlkIdx] == 0)
          process_rem_intra4x4_pred_mode(luma4x4BlkIdx);
      }
    }

    /* 2. 对于 Intra_8x8 模式，遍历 4 个 8x8 的亮度块: */
    if (m_mb_pred_mode == Intra_8x8) {
      for (int luma8x8BlkIdx = 0; luma8x8BlkIdx < 4; luma8x8BlkIdx++) {
        //解码每个块的前一个 Intra 8x8 预测模式标志
        process_prev_intra8x8_pred_mode_flag(luma8x8BlkIdx);
        //当前一个块不存在预测模式时，解码当前的 Intra 8x8 预测模式
        if (!prev_intra8x8_pred_mode_flag[luma8x8BlkIdx])
          process_rem_intra8x8_pred_mode(luma8x8BlkIdx);
      }
    }

    /* 3. 对于 Intra_16x16 模式，这里不需要显示的解码，后续直接使用 */
    if (m_mb_pred_mode == Intra_16x16) {
    }

    /* 4. 色度阵列类型为 YUV420 或 YUV422时，则解码intra_chroma_pred_mode，色度通常有自己的帧内预测模式 */
    if (sps->ChromaArrayType == 1 || sps->ChromaArrayType == 2)
      process_intra_chroma_pred_mode();

    /* --------------------------这一部分属于帧间预测-------------------------- */
  } else if (m_mb_pred_mode != Direct) {
    int ret;
    H264_MB_PART_PRED_MODE mb_pred_mode = MB_PRED_MODE_NA;

    //------------------------- 运动矢量参考帧索引 ----------------------------
    //m_NumMbPart为宏块分区：一般为1,2，即为1时：P_16x16, B_16x16, 为2时：P_16x8, P_8x16, B_16x8, B_8x16
    for (int mbPartIdx = 0; mbPartIdx < m_NumMbPart; mbPartIdx++) {
      ret = MbPartPredMode(m_name_of_mb_type, mbPartIdx,
                           transform_size_8x8_flag, mb_pred_mode);
      RET(ret);
      /* 前参考帧列表中有多于一个参考帧可供选择 || 当前宏块的场解码模式与整个图片的场模式不同(这种情况下需要特别处理参考帧索引) */
      if ((header->num_ref_idx_l0_active_minus1 > 0 ||
           mb_field_decoding_flag != field_pic_flag) &&
          //(mb_pred_mode == Pred_L0 || mb_pred_mode == BiPred))
          mb_pred_mode != Pred_L1)
        /* 根据预测模式处理参考索引，如P_L0_16x16 -> Pred_L0 , P_L0_L0_8x16 -> Pred_L0 , B_Bi_16x16 -> BiPred*/
        process_ref_idx_l0(mbPartIdx, header->num_ref_idx_l0_active_minus1);

      /* 后参考帧列表中有多于一个参考帧可供选择*/
      if ((header->num_ref_idx_l1_active_minus1 > 0 ||
           mb_field_decoding_flag != field_pic_flag) &&
          //(mb_pred_mode == Pred_L1 || mb_pred_mode == BiPred))
          mb_pred_mode != Pred_L0)
        /* 根据预测模式处理参考索引，如B_L1_16x16 -> Pred_L1 , B_L1_Bi_8x16 -> BiPred */
        process_ref_idx_l1(mbPartIdx, header->num_ref_idx_l1_active_minus1);
    }

    //------------------------- 运动矢量 ----------------------------
    //前预测模式，或双向预测
    for (int mbPartIdx = 0; mbPartIdx < m_NumMbPart; mbPartIdx++) {
      ret = MbPartPredMode(m_name_of_mb_type, mbPartIdx,
                           transform_size_8x8_flag, mb_pred_mode);
      RET(ret);
      //if (mb_pred_mode == Pred_L0 || mb_pred_mode == BiPred)
      if (mb_pred_mode != Pred_L1)
        /* 分别处理水平方向和垂直方向上的运动矢量差 */
        for (int compIdx = 0; compIdx < 2; compIdx++)
          /* 根据预测模式处理运动矢量差 */
          process_mvd_l0(mbPartIdx, compIdx);
    }

    //后预测模式，或双向预测
    for (int mbPartIdx = 0; mbPartIdx < m_NumMbPart; mbPartIdx++) {
      ret = MbPartPredMode(m_name_of_mb_type, mbPartIdx,
                           transform_size_8x8_flag, mb_pred_mode);
      RET(ret);
      //if (mb_pred_mode == Pred_L1 || mb_pred_mode == BiPred)
      if (mb_pred_mode != Pred_L0)
        for (int compIdx = 0; compIdx < 2; compIdx++)
          process_mvd_l1(mbPartIdx, compIdx);
    }
  }
  return 0;
}

void MacroBlock::set_current_mb_info(SUB_MB_TYPE_P_MBS_T type, int mbPartIdx) {
  m_name_of_sub_mb_type[mbPartIdx] = type.name_of_sub_mb_type;
  m_sub_mb_pred_mode[mbPartIdx] = type.SubMbPredMode;
  NumSubMbPart[mbPartIdx] = type.NumSubMbPart;
  SubMbPartWidth[mbPartIdx] = type.SubMbPartWidth;
  SubMbPartHeight[mbPartIdx] = type.SubMbPartHeight;
}

void MacroBlock::set_current_mb_info(SUB_MB_TYPE_B_MBS_T type, int mbPartIdx) {
  m_name_of_sub_mb_type[mbPartIdx] = type.name_of_sub_mb_type;
  m_sub_mb_pred_mode[mbPartIdx] = type.SubMbPredMode;
  NumSubMbPart[mbPartIdx] = type.NumSubMbPart;
  SubMbPartWidth[mbPartIdx] = type.SubMbPartWidth;
  SubMbPartHeight[mbPartIdx] = type.SubMbPartHeight;
}

// 7.3.5.2 Sub-macroblock prediction syntax （该函数与mb_pred非常相似）
/* 一般来说在处理 P 帧和 B 帧时会分割为子宏块处理，这样才能更好的进行运动预测（帧间预测） */
/* 作用：计算出子宏块的预测值。这些预测值将用于后续的残差计算和解码过程。 */
int MacroBlock::sub_mb_pred(const SliceData &slice_data) {
  const SliceHeader *header = _pic->m_slice->slice_header;
  /* NOTE:由于是子宏块，所以没有帧内预测信息 */

  //------------------------- 运动矢量参考帧索引 ----------------------------
  /* TODO：这里进行了合并，还没有测试有没有问题 */
  for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
    if (m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8) {

      /* 前参考帧列表中有多于一个参考帧可供选择，NOTE:ref0表明对于所有这些子宏块，运动矢量的参考索引固定为0 */
      if (m_name_of_mb_type != P_8x8ref0 &&
          (header->num_ref_idx_l0_active_minus1 > 0 ||
           mb_field_decoding_flag != field_pic_flag) &&
          m_sub_mb_pred_mode[mbPartIdx] != Pred_L1)
        /* 根据预测模式处理参考索引，如P_8x8 -> Pred_L0 */
        process_ref_idx_l0(mbPartIdx, header->num_ref_idx_l0_active_minus1);

      /* 后参考帧列表中有多于一个参考帧可供选择*/
      if ((header->num_ref_idx_l1_active_minus1 > 0 ||
           mb_field_decoding_flag != field_pic_flag) &&
          m_sub_mb_pred_mode[mbPartIdx] != Pred_L0)
        process_ref_idx_l1(mbPartIdx, header->num_ref_idx_l1_active_minus1);
    }

  //------------------------- 运动矢量 ----------------------------
  //前预测模式，或双向预测
  for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    if (m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
        m_sub_mb_pred_mode[mbPartIdx] != Pred_L1)
      //m_NumMbPart[mbPartIdx]为宏块分区：一般为1,2，即为1时：P_8x8, B_8x8, 为2时：P_4x8, P_8x4, B_4x8, B_8x4
      for (int subMbIdx = 0; subMbIdx < NumSubMbPart[mbPartIdx]; subMbIdx++)
        /* 分别处理水平方向和垂直方向上的运动矢量差 */
        for (int compIdx = 0; compIdx < 2; compIdx++)
          /* 根据预测模式处理运动矢量差 */
          process_mvd_l0(mbPartIdx, compIdx, subMbIdx);
  }

  //后预测模式，或双向预测
  for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    if (m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
        m_sub_mb_pred_mode[mbPartIdx] != Pred_L0)
      for (int subMbIdx = 0; subMbIdx < NumSubMbPart[mbPartIdx]; subMbIdx++)
        for (int compIdx = 0; compIdx < 2; compIdx++)
          process_mvd_l1(mbPartIdx, compIdx, subMbIdx);
  }

  return 0;
}

int MacroBlock::NumSubMbPartFunc(int mbPartIdx) {
  int32_t NumSubMbPart = -1;
  const int32_t type = sub_mb_type[mbPartIdx];
  if (m_slice_type_fixed == SLICE_P && type >= 0 && type <= 3)
    /* Table 7-17 – Sub-macroblock types in P macroblocks */
    NumSubMbPart = sub_mb_type_P_mbs_define[type].NumSubMbPart;
  /* TODO YangJing 这里不应该是[0,12]? 为什么是[0,3]?先用12,有问题再改回来 <24-09-07 00:01:26> */
  else if (m_slice_type_fixed == SLICE_B && type >= 0 && type <= 12)
    /* Table 7-18 – Sub-macroblock types in B macroblocks */
    NumSubMbPart = sub_mb_type_B_mbs_define[type].NumSubMbPart;
  else
    RET(-1);
  return NumSubMbPart;
}

int MacroBlock::fix_mb_type(int32_t slice_type_raw, int32_t mb_type_raw,
                            int32_t &slice_type_fixed, int32_t &mb_type_fixed) {
  slice_type_fixed = slice_type_raw;
  mb_type_fixed = mb_type_raw;

  /* SI Slice的宏块类型在表 7-12 和 7-11 中指定。 mb_type 值 0 在表 7-12 中指定，mb_type 值 1 到 26 在表 7-11 中指定，通过从 mb_type 值减 1 进行索引。 */
  if ((slice_type_raw % 5) == SLICE_SI) {
    if (mb_type_raw == 0) {
      // 不需要修正
    } else if (mb_type_raw >= 1 && mb_type_raw <= 26) {
      // 说明 SI slices 中含有I宏块,
      slice_type_fixed = SLICE_I;
      mb_type_fixed = mb_type_raw - 1;
    } else {
      std::cerr << "SI Slice must be in [0..26]. " << __FUNCTION__
                << "():" << __LINE__ << std::endl;
      return -1;
    }
  }

  /* P 和 SP 片的宏块类型在表 7-13 和 7-11 中指定。 mb_type 值 0 到 4 在表 7-13 中指定，mb_type 值 5 到 30 在表 7-11 中指定，通过从 mb_type 值减去 5 进行索引。 */
  else if ((slice_type_raw % 5) == SLICE_P ||
           (slice_type_raw % 5) == SLICE_SP) {
    if (mb_type_raw >= 0 && mb_type_raw <= 4) {
      // 不需要修正
    } else if (mb_type_raw >= 5 && mb_type_raw <= 30) {
      // 说明 P and SP slices 中含有I宏块
      slice_type_fixed = SLICE_I;
      mb_type_fixed = mb_type_raw - 5;
    } else {
      std::cerr << "P,SP Slice must be in [0..30]. " << __FUNCTION__
                << "():" << __LINE__ << std::endl;
      return -1;
    }
  }

  /* B Slice的宏块类型在表 7-14 和 7-11 中指定。 mb_type 值 0 到 22 在表 7-14 中指定，mb_type 值 23 到 48 在表 7-11 中指定，通过从 mb_type 值减去 23 进行索引。 */
  else if ((slice_type_raw % 5) == SLICE_B) {
    if (mb_type_raw >= 0 && mb_type_raw <= 22) {
      // 不需要修正
    } else if (mb_type_raw >= 23 && mb_type_raw <= 48) {
      // 说明 B slices 中含有I宏块
      slice_type_fixed = SLICE_I;
      mb_type_fixed = mb_type_raw - 23;
    } else {
      std::cerr << "B Slice must be in [0..48]. " << __FUNCTION__
                << "():" << __LINE__ << std::endl;
      return -1;
    }
  }

  return 0;
}

/* 7.4.5 Macroblock layer semantics -> mb_type */
/* Table 7-11 – Macroblock types for I slices
 * Table 7-12 – Macroblock type with value 0 for SI slices
 * Table 7-13 – Macroblock type values 0 to 4 for P and SP slices
 * Table 7-14 – Macroblock type values 0 to 22 for B slices */
int MacroBlock::MbPartPredMode(int32_t _mb_type, int32_t index,
                               H264_MB_TYPE &name_of_mb_type,
                               H264_MB_PART_PRED_MODE &mb_pred_mode) {

  //Table 7-11 – Macroblock types for I slices
  if ((m_slice_type_fixed % 5) == SLICE_I) {
    const int I_NxN = 0;
    if (_mb_type == I_NxN) {
      if (transform_size_8x8_flag) {
        name_of_mb_type = mb_type_I_slices_define[1].name_of_mb_type;
        mb_pred_mode = mb_type_I_slices_define[1].MbPartPredMode;
      } else {
        name_of_mb_type = mb_type_I_slices_define[0].name_of_mb_type;
        mb_pred_mode = mb_type_I_slices_define[0].MbPartPredMode;
      }
    } else if (_mb_type >= MIN_MB_TYPE_FOR_I_SLICE &&
               _mb_type <= MAX_MB_TYPE_FOR_I_SLICE) {
      name_of_mb_type = mb_type_I_slices_define[_mb_type + 1].name_of_mb_type;
      CodedBlockPatternChroma =
          mb_type_I_slices_define[_mb_type + 1].CodedBlockPatternChroma;
      CodedBlockPatternLuma =
          mb_type_I_slices_define[_mb_type + 1].CodedBlockPatternLuma;
      Intra16x16PredMode =
          mb_type_I_slices_define[_mb_type + 1].Intra16x16PredMode;
      mb_pred_mode = mb_type_I_slices_define[_mb_type + 1].MbPartPredMode;
    } else
      RET(-1);

    //Table 7-12 – Macroblock type with value 0 for SI slices
  } else if ((m_slice_type_fixed % 5) == SLICE_SI) {
    if (_mb_type == 0) {
      name_of_mb_type = mb_type_SI_slices_define[0].name_of_mb_type;
      mb_pred_mode = mb_type_SI_slices_define[0].MbPartPredMode;
    } else
      RET(-1);

    //Table 7-13 – Macroblock type values 0 to 4 for P and SP slices
  } else if ((m_slice_type_fixed % 5) == SLICE_P ||
             (m_slice_type_fixed % 5) == SLICE_SP) {
    if (_mb_type >= 0 && _mb_type <= 5) {
      name_of_mb_type = mb_type_P_SP_slices_define[_mb_type].name_of_mb_type;
      m_NumMbPart = mb_type_P_SP_slices_define[_mb_type].NumMbPart;
      if (index == 0)
        mb_pred_mode = mb_type_P_SP_slices_define[_mb_type].MbPartPredMode0;
      else
        mb_pred_mode = mb_type_P_SP_slices_define[_mb_type].MbPartPredMode1;
    } else
      RET(-1);

    //Table 7-14 – Macroblock type values 0 to 22 for B slices
  } else if ((m_slice_type_fixed % 5) == SLICE_B) {
    if (_mb_type >= 0 && _mb_type <= 23) {
      name_of_mb_type = mb_type_B_slices_define[_mb_type].name_of_mb_type;
      m_NumMbPart = mb_type_B_slices_define[_mb_type].NumMbPart;
      if (index == 0)
        mb_pred_mode = mb_type_B_slices_define[_mb_type].MbPartPredMode0;
      else
        mb_pred_mode = mb_type_B_slices_define[_mb_type].MbPartPredMode1;
    } else
      RET(-1);
  } else
    RET(-1);

  return 0;
}

/* Table 7-11 – Macroblock types for I slices
 * Table 7-12 – Macroblock type with value 0 for SI slices
 * Table 7-13 – Macroblock type values 0 to 4 for P and SP slices
 * Table 7-14 – Macroblock type values 0 to 22 for B slices */
int MacroBlock::MbPartPredMode(H264_MB_TYPE name_of_mb_type, int32_t mbPartIdx,
                               int32_t transform_size_8x8_flag,
                               H264_MB_PART_PRED_MODE &mb_pred_mode) {
  //Table 7-11 – Macroblock types for I slices
  if (name_of_mb_type == I_NxN) {
    if (mbPartIdx == 0)
      mb_pred_mode =
          mb_type_I_slices_define[transform_size_8x8_flag].MbPartPredMode;
    else
      RET(-1);

  } else if (name_of_mb_type >= I_16x16_0_0_0 &&
             name_of_mb_type <= I_16x16_3_2_1) {
    if (mbPartIdx == 0)
      // FIXME: mbPartIdx -> name_of_mb_type
      mb_pred_mode = mb_type_I_slices_define[name_of_mb_type - I_16x16_0_0_0]
                         .MbPartPredMode;
    else
      RET(-1);

    //Table 7-12 – Macroblock type with value 0 for SI slices
  } else if (name_of_mb_type == SI) {
    if (mbPartIdx == 0)
      mb_pred_mode = mb_type_SI_slices_define[0].MbPartPredMode;
    else
      RET(-1);

    //Table 7-13 – Macroblock type values 0 to 4 for P and SP slices
  } else if (name_of_mb_type == P_L0_16x16 || name_of_mb_type == P_Skip) {
    if (mbPartIdx == 0)
      mb_pred_mode = Pred_L0;
    else
      RET(-1);
  } else if (name_of_mb_type >= P_L0_L0_16x8 &&
             name_of_mb_type <= P_L0_L0_8x16) {
    if (mbPartIdx == 0)
      mb_pred_mode = mb_type_P_SP_slices_define[name_of_mb_type - P_L0_16x16]
                         .MbPartPredMode0;
    else if (mbPartIdx == 1)
      mb_pred_mode = mb_type_P_SP_slices_define[name_of_mb_type - P_L0_16x16]
                         .MbPartPredMode1;
    else
      RET(-1);
  } else if (name_of_mb_type >= P_8x8 && name_of_mb_type <= P_8x8ref0) {
    if (mbPartIdx >= 0 && mbPartIdx <= 3)
      mb_pred_mode = Pred_L0;
    else
      RET(-1);
  }

  //Table 7-14 – Macroblock type values 0 to 22 for B slices
  else if (name_of_mb_type == B_L0_16x16) {
    if (mbPartIdx == 0)
      mb_pred_mode = Pred_L0;
    else
      RET(-1);
  } else if (name_of_mb_type == B_L1_16x16) {
    if (mbPartIdx == 0)
      mb_pred_mode = Pred_L1;
    else
      RET(-1);
  } else if (name_of_mb_type == B_Bi_16x16) {
    if (mbPartIdx == 0)
      mb_pred_mode = BiPred;
    else
      RET(-1);
  } else if (name_of_mb_type >= B_Direct_16x16 && name_of_mb_type <= B_Skip) {
    if (mbPartIdx == 0)
      mb_pred_mode = mb_type_B_slices_define[name_of_mb_type - B_Direct_16x16]
                         .MbPartPredMode0;
    else if (mbPartIdx == 1)
      mb_pred_mode = mb_type_B_slices_define[name_of_mb_type - B_Direct_16x16]
                         .MbPartPredMode1;
    else if (mbPartIdx == 2 || mbPartIdx == 3)
      mb_pred_mode = MB_PRED_MODE_NA;
    else
      RET(-1);
  } else
    RET(-1);

  return 0;
}

int MacroBlock::MbPartPredMode() {
  const int32_t slice_type = m_slice_type_fixed % 5;
  //Table 7-11 – Macroblock types for I slices
  if (slice_type == SLICE_I) {
    if (m_mb_type_fixed == 0) {
      if (transform_size_8x8_flag == 0)
        mb_type_I_slice = mb_type_I_slices_define[0];
      else
        mb_type_I_slice = mb_type_I_slices_define[1];

    } else if (m_mb_type_fixed >= 1 && m_mb_type_fixed <= 25)
      mb_type_I_slice = mb_type_I_slices_define[m_mb_type_fixed + 1];
    else
      RET(-1);

    //Table 7-12 – Macroblock type with value 0 for SI slices
  } else if (slice_type == SLICE_SI) {
    if (m_mb_type_fixed == 0)
      mb_type_SI_slice = mb_type_SI_slices_define[0];
    else
      RET(-1);

    //Table 7-13 – Macroblock type values 0 to 4 for P and SP slices
  } else if (slice_type == SLICE_P || slice_type == SLICE_SP) {
    if (m_mb_type_fixed >= 0 && m_mb_type_fixed <= 5)
      mb_type_P_SP_slice = mb_type_P_SP_slices_define[m_mb_type_fixed];
    else
      RET(-1);

    MbPartWidth = mb_type_P_SP_slice.MbPartWidth;
    MbPartHeight = mb_type_P_SP_slice.MbPartHeight;
    m_NumMbPart = mb_type_P_SP_slice.NumMbPart;

    //Table 7-14 – Macroblock type values 0 to 22 for B slices
  } else if (slice_type == SLICE_B) {
    if (m_mb_type_fixed >= 0 && m_mb_type_fixed <= 23)
      mb_type_B_slice = mb_type_B_slices_define[m_mb_type_fixed];
    else
      RET(-1);

    MbPartWidth = mb_type_B_slice.MbPartWidth;
    MbPartHeight = mb_type_B_slice.MbPartHeight;
    m_NumMbPart = mb_type_B_slice.NumMbPart;
  } else
    RET(-1);

  return 0;
}

/* 7.4.5.2 Sub-macroblock prediction semantics */
/* 输出为子宏块的sub_mb_type,NumSubMbPart,SubMbPredMode,SubMbPartWidth,SubMbPartHeight，类似于一个查表操作*/
int MacroBlock::SubMbPredMode(int32_t slice_type, int32_t sub_mb_type,
                              int32_t &NumSubMbPart,
                              H264_MB_PART_PRED_MODE &SubMbPredMode,
                              int32_t &SubMbPartWidth,
                              int32_t &SubMbPartHeight) {
  if (slice_type == SLICE_P) {
    if (sub_mb_type >= 0 && sub_mb_type <= 3) {
      /* Table 7-17 – Sub-macroblock types in P macroblocks */
      NumSubMbPart = sub_mb_type_P_mbs_define[sub_mb_type].NumSubMbPart;
      SubMbPredMode = sub_mb_type_P_mbs_define[sub_mb_type].SubMbPredMode;
      SubMbPartWidth = sub_mb_type_P_mbs_define[sub_mb_type].SubMbPartWidth;
      SubMbPartHeight = sub_mb_type_P_mbs_define[sub_mb_type].SubMbPartHeight;
    } else
      RET(-1);
  } else if (slice_type == SLICE_B) {
    if (sub_mb_type >= 0 && sub_mb_type <= 12) {
      /* Table 7-18 – Sub-macroblock types in B macroblocks */
      NumSubMbPart = sub_mb_type_B_mbs_define[sub_mb_type].NumSubMbPart;
      SubMbPredMode = sub_mb_type_B_mbs_define[sub_mb_type].SubMbPredMode;
      SubMbPartWidth = sub_mb_type_B_mbs_define[sub_mb_type].SubMbPartWidth;
      SubMbPartHeight = sub_mb_type_B_mbs_define[sub_mb_type].SubMbPartHeight;
    } else
      RET(-1);
  } else
    RET(-1);

  return 0;
}

int MacroBlock::residual_block_DC(int32_t coeffLevel[], int32_t startIdx,
                                  int32_t endIdx, int32_t maxNumCoeff,
                                  int iCbCr, int32_t BlkIdx) {
  int ret = 0, TotalCoeff = 0;
  MB_RESIDUAL_LEVEL mb_level =
      (iCbCr == 0) ? MB_RESIDUAL_ChromaDCLevelCb : MB_RESIDUAL_ChromaDCLevelCr;

  if (_is_cabac)
    ret = _cabac->residual_block_cabac(coeffLevel, startIdx, endIdx,
                                       maxNumCoeff, MB_RESIDUAL_ChromaDCLevel,
                                       BlkIdx, iCbCr, TotalCoeff);
  else

    ret = _cavlc->residual_block_cavlc(coeffLevel, startIdx, endIdx,
                                       maxNumCoeff, mb_level, m_mb_pred_mode,
                                       BlkIdx, TotalCoeff);
  RET(ret);

  mb_chroma_4x4_non_zero_count_coeff[iCbCr][BlkIdx] = TotalCoeff;
  return 0;
}

int MacroBlock::residual_block_AC(int32_t coeffLevel[], int32_t startIdx,
                                  int32_t endIdx, int32_t maxNumCoeff,
                                  int iCbCr, int32_t BlkIdx) {

  int ret = 0, TotalCoeff = 0;
  MB_RESIDUAL_LEVEL mb_level =
      (iCbCr == 0) ? MB_RESIDUAL_ChromaACLevelCb : MB_RESIDUAL_ChromaACLevelCr;

  if (_is_cabac)
    ret = _cabac->residual_block_cabac(coeffLevel, startIdx, endIdx,
                                       maxNumCoeff, MB_RESIDUAL_ChromaACLevel,
                                       BlkIdx, iCbCr, TotalCoeff);
  else
    ret = _cavlc->residual_block_cavlc(coeffLevel, startIdx, endIdx,
                                       maxNumCoeff, mb_level, m_mb_pred_mode,
                                       BlkIdx, TotalCoeff);

  RET(ret);
  mb_chroma_4x4_non_zero_count_coeff[iCbCr][BlkIdx] = TotalCoeff;
  return 0;
}

int MacroBlock::residual_block2(int32_t coeffLevel[], int32_t startIdx,
                                int32_t endIdx, int32_t maxNumCoeff,
                                MB_RESIDUAL_LEVEL mb_block_level, int iCbCr,
                                int32_t BlkIdx, int &TotalCoeff) {
  int ret = 0;
  if (_is_cabac)
    ret =
        _cabac->residual_block_cabac(coeffLevel, startIdx, endIdx, maxNumCoeff,
                                     mb_block_level, BlkIdx, iCbCr, TotalCoeff);
  else
    ret = _cavlc->residual_block_cavlc(coeffLevel, startIdx, endIdx,
                                       maxNumCoeff, mb_block_level,
                                       m_mb_pred_mode, BlkIdx, TotalCoeff);

  RET(ret);
  return ret;
}

// 7.3.5.3 Residual data syntax
/* 残差数据是指编码过程中预测值与实际值之间的差异，它在解码时需要被重建，以恢复原始图像。这个函数的主要任务是处理亮度（Luma）和色度（Chroma）的残差数据。*/
/* 对于帧内预测（I帧）：相邻的已解码像素生成预测值 -> 残差解码 -> 重建块 -> 去块效应滤波，其中残差解码有反量化残差系数，逆变换残差系数（DCT）*/
/* 对于帧间预测（B、P帧）：运动补偿预测 -> 残差解码 -> 重建块，其中残差数据用于修正预测值 */
int MacroBlock::residual(int32_t startIdx, int32_t endIdx) {
  if (!_cavlc) _cavlc = new Cavlc(_pic, _bs);
  const uint32_t ChromaArrayType =
      _pic->m_slice->slice_header->m_sps->ChromaArrayType;
  const int32_t SubWidthC = _pic->m_slice->slice_header->m_sps->SubWidthC;
  const int32_t SubHeightC = _pic->m_slice->slice_header->m_sps->SubHeightC;

  //----------------------------- 处理 Luma 信息 --------------------------------------
  /*帧内残差：对于整个 16x16 宏块使用一个预测模式,残差数据分为 DC 和 AC 两部分
    DC 残差：表示整个 16x16 宏块的平均亮度值。
    AC 残差：表示宏块内的细节变化。*/

  // 设置宏块的 DC 和 AC 残差级别，然后调用 residual_luma 函数处理亮度残差。residual_luma 函数负责解码亮度残差系数。
  _mb_residual_level_dc = MB_RESIDUAL_Intra16x16DCLevel;
  _mb_residual_level_ac = MB_RESIDUAL_Intra16x16ACLevel;
  residual_luma(startIdx, endIdx);

  /* 将解码后的残差值(亮度)保存，以16x16个数据量为单位处理 */
  memcpy(Intra16x16DCLevel, i16x16DClevel, sizeof(i16x16DClevel));
  memcpy(Intra16x16ACLevel, i16x16AClevel, sizeof(i16x16AClevel));
  memcpy(LumaLevel4x4, level4x4, sizeof(level4x4));
  memcpy(LumaLevel8x8, level8x8, sizeof(level8x8));

  //----------------------------- 处理 Chroma 信息 --------------------------------------
  /* 在YUV420,YUV422中，色度分量通常被下采样，意味着色度的分辨率比亮度低 */
  if (ChromaArrayType == 1 || ChromaArrayType == 2)
    //表示色度块的数量，8x8宏块为一个色度块，NumC8x8 = 1则表示色度块为8x8，NumC8x8 = 2则表示色度块为16x8或8x16
    residual_chroma(startIdx, endIdx, 4 / (SubWidthC * SubHeightC));

  /* 对于YUV444，此处跟Luma（亮度）块一样的处理（因为没有对色度进行子采样）*/
  else if (ChromaArrayType == 3) {
    /* 处理U(Cb)数据 */
    _mb_residual_level_dc = MB_RESIDUAL_CbIntra16x16DCLevel;
    _mb_residual_level_ac = MB_RESIDUAL_CbIntra16x16ACLevel;
    residual_luma(startIdx, endIdx);

    memcpy(CbIntra16x16DCLevel, i16x16DClevel, sizeof(i16x16DClevel));
    memcpy(CbIntra16x16ACLevel, i16x16AClevel, sizeof(i16x16AClevel));
    memcpy(CbLevel4x4, level4x4, sizeof(level4x4));
    memcpy(CbLevel8x8, level8x8, sizeof(level8x8));

    /* 处理V(Cr)数据 */
    _mb_residual_level_dc = MB_RESIDUAL_CrIntra16x16DCLevel;
    _mb_residual_level_ac = MB_RESIDUAL_CrIntra16x16ACLevel;
    residual_luma(startIdx, endIdx);

    memcpy(CrIntra16x16DCLevel, i16x16DClevel, sizeof(i16x16DClevel));
    memcpy(CrIntra16x16ACLevel, i16x16AClevel, sizeof(i16x16AClevel));
    memcpy(CrLevel4x4, level4x4, sizeof(level4x4));
    memcpy(CrLevel8x8, level8x8, sizeof(level8x8));
  }

  return 0;
}

//7.3.5.3.1 Residual luma syntax （当Cb、Cr分量未被降低采样时，也会进入到这个函数处理，即YUV444）
/* 残差系数解码。通过该函数，宏块的残差系数被解码并存储系数 */
int MacroBlock::residual_luma(int32_t startIdx, int32_t endIdx) {
  // 当前的宏块是16x16整宏块帧内预测，且为首个块（包含DC系数），则先处理 16x16 DC 残差系数
  if (startIdx == 0 && m_mb_pred_mode == Intra_16x16) {
    /* 该 4x4 block的残差中，总共有多少个非零系数 */
    int32_t TotalCoeff = 0;
    /* 调用 residual_block2 函数来解码 DC 残差系数，并将非零系数的总数存储在 TotalCoeff 中 */
    residual_block2(i16x16DClevel, 0, 15, 16, _mb_residual_level_dc, -1, 0,
                    TotalCoeff);
    mb_luma_4x4_non_zero_count_coeff[0] = TotalCoeff;
  }

  /* 宏块被分为4个8x8的子宏块，以8x8宏块为单位遍历 */
  for (int i8x8 = 0; i8x8 < 4; i8x8++) {
    /* 当未使用 8x8 变换 或者 为CAVLC编码模式（这里主要是表示为传统级别的编码） */
    if (transform_size_8x8_flag == 0 || _is_cabac == 0) {
      /* 对于传统4x4变换，需要增加遍历次数，达到遍历16个4x4 子块目的 */
      for (int i4x4 = 0; i4x4 < 4; i4x4++) {
        int BlkIdx = i8x8 * 4 + i4x4; // 当前4x4子块的索引（以4x4宏块为单位）

        /* NOTE:最外层的逻辑控制是基于8x8子宏块的CBP位的，即使代码内部是通过遍历每个4x4子块来处理，意味着至少有一个在这个8x8子宏块内的4x4子块包含非零残差系数 */
        if (CodedBlockPatternLuma & (1 << i8x8)) {
          int TotalCoeff = 0;
          /* MAX(0, startIdx - 1)表示，上面已经单独处理了一次DC系数，由于上面单独经过了DC的处理，那么startIdx肯定还是0，所以，这里经过MAX后为MAX(0,-1) = 0，但是endIdx却实实在在的-1了，比如变为了15 - 1 = 14*/
          if (m_mb_pred_mode == Intra_16x16)
            residual_block2(i16x16AClevel[BlkIdx], MAX(0, startIdx - 1),
                            endIdx - 1, 15, MB_RESIDUAL_Intra16x16ACLevel, -1,
                            BlkIdx, TotalCoeff);
          /* 在其他模式下（如 Intra_NxN 或 Inter 模式），每个 4x4 子块的残差系数是独立处理的，没有像 Intra_16x16 那样的特殊 DC/AC 分离，残差系数直接存储在 level4x4 中，包含完整的 DC 和 AC 分量 */
          else
            residual_block2(level4x4[BlkIdx], startIdx, endIdx, 16,
                            MB_RESIDUAL_LumaLevel4x4, -1, BlkIdx, TotalCoeff);

          mb_luma_4x4_non_zero_count_coeff[BlkIdx] = TotalCoeff;
          mb_luma_8x8_non_zero_count_coeff[i8x8] += TotalCoeff;
        }
        /* 当前亮度块不包含亮度残差数据，但为Intra_16x16预测模式，由于上面单独经过了DC的处理，那么这里只需要处理后面15个AC系数 */
        else if (m_mb_pred_mode == Intra_16x16)
          std::fill_n(i16x16AClevel[BlkIdx], 15, 0);
        /* 当前亮度块不包含亮度残差数据，DC,AC系数全部置空为0 */
        else
          std::fill_n(level4x4[BlkIdx], 16, 0);

        /* 当在CAVLC编码模式下使用 8x8 变换：需要将已经处理的4x4变换系数重新映射（即组合）成8x8变换系数 */
        if (transform_size_8x8_flag && _is_cabac == 0) {
          for (int i = 0; i < 16; i++)
            level8x8[i8x8][4 * i + i4x4] = level4x4[BlkIdx][i];
          mb_luma_8x8_non_zero_count_coeff[i8x8] +=
              mb_luma_4x4_non_zero_count_coeff[BlkIdx];
        }
      }

      /* 当使用 8x8 变换 且为CABAC编码模式（目前主流编码），且亮度块包含亮度残差数据 */
    } else if (CodedBlockPatternLuma & (1 << i8x8)) {
      const int &BlkIdx = i8x8;
      int TotalCoeff = 0;
      residual_block2(level8x8[BlkIdx], 4 * startIdx, 4 * endIdx + 3, 64,
                      MB_RESIDUAL_LumaLevel8x8, -1, BlkIdx, TotalCoeff);
      mb_luma_8x8_non_zero_count_coeff[BlkIdx] = TotalCoeff;
    }

    /* 当前亮度块不包含亮度残差数据，直接将其系数清零 */
    else
      std::fill_n(level8x8[i8x8], 64, 0);
  }

  return 0;
}

int MacroBlock::residual_chroma(int32_t startIdx, int32_t endIdx,
                                int32_t NumC8x8) {
  // -------------------- 处理残差DC系数 --------------------
  // 两个色度分量：Cb 和 Cr
  for (int iCbCr = 0; iCbCr < 2; iCbCr++) {
    /* DC,AC系数分开存储，首个块为DC系数块，且至少有一些色度信息需要编码 */
    if (startIdx == 0 && (CodedBlockPatternChroma & 3))
      residual_block_DC(ChromaDCLevel[iCbCr], 0, 4 * NumC8x8 - 1, 4 * NumC8x8,
                        iCbCr, 0);

    /* 如不存在DC残差值，则将所有色度块的DC置为零，这里是以4x4宏块为单位处理 */
    else
      std::fill_n(ChromaDCLevel[iCbCr], 4 * NumC8x8, 0);
  }

  // -------------------- 处理残差AC系数 --------------------
  for (int iCbCr = 0; iCbCr < 2; iCbCr++) {
    for (int i8x8 = 0; i8x8 < NumC8x8; i8x8++) // 遍历每个8x8色度块
      for (int i4x4 = 0; i4x4 < 4; i4x4++) { // 遍历每个色度块中的4x4子块
        // BlkIdx 是当前4x4子块的索引（以4x4宏块为单位）
        int32_t BlkIdx = i8x8 * 4 + i4x4;
        // 存在AC系数需要处理
        if (CodedBlockPatternChroma & 2)
          residual_block_AC(ChromaACLevel[iCbCr][BlkIdx], MAX(0, startIdx - 1),
                            endIdx - 1, 15, iCbCr, BlkIdx);

        /* 若当前4x4宏块不存在AC非零系数，则将当前的AC系数全部置空为0(出去DC系数就是16-1 = 15个） */
        else
          std::fill_n(ChromaACLevel[iCbCr][BlkIdx], 16 - 1, 0);
      }
  }
  return 0;
}

int MacroBlock::getMbPartWidthAndHeight(H264_MB_TYPE name_of_mb_type,
                                        int32_t &_MbPartWidth,
                                        int32_t &_MbPartHeight) {
  int ret = 0;

  if (name_of_mb_type >= P_L0_16x16 && name_of_mb_type <= P_Skip) {
    _MbPartWidth =
        mb_type_P_SP_slices_define[name_of_mb_type - P_L0_16x16].MbPartWidth;
    _MbPartHeight =
        mb_type_P_SP_slices_define[name_of_mb_type - P_L0_16x16].MbPartHeight;
  } else if (name_of_mb_type >= B_Direct_16x16 && name_of_mb_type <= B_Skip) {
    _MbPartWidth =
        mb_type_B_slices_define[name_of_mb_type - B_Direct_16x16].MbPartWidth;
    _MbPartHeight =
        mb_type_B_slices_define[name_of_mb_type - B_Direct_16x16].MbPartHeight;
  } else if (name_of_mb_type >= P_L0_8x8 && name_of_mb_type <= P_L0_4x4) {
    _MbPartWidth =
        sub_mb_type_P_mbs_define[name_of_mb_type - P_L0_8x8].SubMbPartWidth;
    _MbPartHeight =
        sub_mb_type_P_mbs_define[name_of_mb_type - P_L0_8x8].SubMbPartHeight;
  } else if (name_of_mb_type >= B_Direct_8x8 && name_of_mb_type <= B_Bi_4x4) {
    _MbPartWidth =
        sub_mb_type_B_mbs_define[name_of_mb_type - B_Direct_8x8].SubMbPartWidth;
    _MbPartHeight = sub_mb_type_B_mbs_define[name_of_mb_type - B_Direct_8x8]
                        .SubMbPartHeight;
  } else
    ret = -1;

  return ret;
}

int MacroBlock::check_sub_mb_size(bool &noSubMbPartSizeLessThan8x8Flag,
                                  bool direct_8x8_inference_flag) {
  for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    if (m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8) {
      if (NumSubMbPartFunc(mbPartIdx) > 1)
        noSubMbPartSizeLessThan8x8Flag = false;
    } else if (!direct_8x8_inference_flag)
      noSubMbPartSizeLessThan8x8Flag = false;
  }
  return 0;
}

int MacroBlock::process_mb_type(const SliceHeader &header, int32_t slice_type) {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_mb_type(mb_type);
  else
    mb_type = _bs->readUE();
  RET(ret);

  // 调整该宏块的mb_type、slice_type的值，为了更好的归属宏块类型，进行不同的类型预测
  ret = fix_mb_type(slice_type, mb_type, m_slice_type_fixed, m_mb_type_fixed);
  RET(ret);

  // 因CABAC会用到MbPartWidth/MbPartHeight信息，所以需要尽可能提前设置相关值
  ret = MbPartPredMode();
  RET(ret);

  ret = MbPartPredMode(m_mb_type_fixed, 0, m_name_of_mb_type, m_mb_pred_mode);
  RET(ret);
  return 0;
}

int MacroBlock::process_sub_mb_type(const int mbPartIdx) {
  if (_is_cabac) {
    RET(_cabac->decode_sub_mb_type(sub_mb_type[mbPartIdx]));
  } else
    sub_mb_type[mbPartIdx] = _bs->readUE();

  /* 2. 根据子宏块类型设置预测模式等信息 */
  // 设置 P 帧子宏块信息
  if (m_slice_type_fixed == SLICE_P && sub_mb_type[mbPartIdx] >= 0 &&
      sub_mb_type[mbPartIdx] <= 3)
    set_current_mb_info(sub_mb_type_P_mbs_define[sub_mb_type[mbPartIdx]],
                        mbPartIdx);
  // 设置 B 帧子宏块信息
  else if (m_slice_type_fixed == SLICE_B && sub_mb_type[mbPartIdx] >= 0 &&
           sub_mb_type[mbPartIdx] <= 12)
    set_current_mb_info(sub_mb_type_B_mbs_define[sub_mb_type[mbPartIdx]],
                        mbPartIdx);
  else
    RET(-1);
  return 0;
}

/* 在解码过程中是否使用 8x8 的变换块大小，而不是默认的 4x4 变换块大小 */
int MacroBlock::process_transform_size_8x8_flag(int32_t &is_8x8_flag) {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_transform_size_8x8_flag(is_8x8_flag);
  else
    is_8x8_flag = _bs->readUn(1);
  RET(ret);

  /* 如果解码得到的 transform_size_8x8_flag_temp 与当前的 transform_size_8x8_flag 不同，则更新 transform_size_8x8_flag */
  if (is_8x8_flag != transform_size_8x8_flag) {
    transform_size_8x8_flag = is_8x8_flag;
    /* 重新计算宏块的预测模式 */
    ret = MbPartPredMode(m_mb_type_fixed, 0, m_name_of_mb_type, m_mb_pred_mode);
    RET(ret);

    /* 如果当前片段是 I 片段 (SLICE_I)，并且宏块类型为 0（即 m_mb_type_fixed == 0），则根据 transform_size_8x8_flag 的值来设置 mb_type_I_slice */
    if ((m_slice_type_fixed % 5) == SLICE_I && m_mb_type_fixed == 0)
      mb_type_I_slice = mb_type_I_slices_define[transform_size_8x8_flag];
  }

  return 0;
}

// 编码块模式（Coded Block Pattern, CBP），用于指示哪些块（亮度块和色度块）包含非零的变换系数。CBP的值决定了在解码过程中哪些块需要进行逆变换和反量化。
int MacroBlock::process_coded_block_pattern(const uint32_t ChromaArrayType) {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_coded_block_pattern(coded_block_pattern);
  else
    coded_block_pattern = _bs->readME(ChromaArrayType, m_mb_pred_mode);

  /* 亮度块模式的值范围是0到15，表示宏块中哪些4x4的亮度块包含非零系数。例如，CodedBlockPatternLuma = 5 表示宏块中第1和第3个4x4亮度块包含非零系数。 */
  CodedBlockPatternLuma = coded_block_pattern % 16;
  /* 色度块模式的值范围是0到3，表示宏块中哪些色度块包含非零系数。例如，CodedBlockPatternChroma = 2 表示宏块中第2个色度块包含非零系数。 */
  CodedBlockPatternChroma = coded_block_pattern / 16;
  return ret;
}

int MacroBlock::process_mb_qp_delta() {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_mb_qp_delta(mb_qp_delta);
  else
    mb_qp_delta = _bs->readSE();
  return ret;
}

/* 每个块的前一个 Intra 4x4 预测模式标志 */
int MacroBlock::process_prev_intra4x4_pred_mode_flag(const int luma4x4BlkIdx) {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_prev_intra4x4_or_intra8x8_pred_mode_flag(
        prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]);
  else
    prev_intra4x4_pred_mode_flag[luma4x4BlkIdx] = _bs->readUn(1);
  return ret;
}

/* 剩余的 Intra 4x4 预测模式 */
int MacroBlock::process_rem_intra4x4_pred_mode(const int luma4x4BlkIdx) {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_rem_intra4x4_or_intra8x8_pred_mode(
        rem_intra4x4_pred_mode[luma4x4BlkIdx]);
  else
    rem_intra4x4_pred_mode[luma4x4BlkIdx] = _bs->readUn(3);
  return ret;
}

int MacroBlock::process_prev_intra8x8_pred_mode_flag(const int luma8x8BlkIdx) {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_prev_intra4x4_or_intra8x8_pred_mode_flag(
        prev_intra8x8_pred_mode_flag[luma8x8BlkIdx]);
  else
    prev_intra8x8_pred_mode_flag[luma8x8BlkIdx] = _bs->readUn(1);
  return ret;
}

int MacroBlock::process_rem_intra8x8_pred_mode(const int luma8x8BlkIdx) {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_rem_intra4x4_or_intra8x8_pred_mode(
        rem_intra8x8_pred_mode[luma8x8BlkIdx]);
  else
    rem_intra8x8_pred_mode[luma8x8BlkIdx] = _bs->readUn(3);
  return ret;
}

int MacroBlock::process_intra_chroma_pred_mode() {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_intra_chroma_pred_mode(intra_chroma_pred_mode);
  else
    intra_chroma_pred_mode = _bs->readUE();
  return ret;
}

/* 指定要用于预测的参考图片的参考图片列表0中的索引。   */
int MacroBlock::process_ref_idx_l0(int mbPartIdx,
                                   uint32_t num_ref_idx_l0_active_minus1) {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_ref_idx_lX(0, mbPartIdx, ref_idx_l0[mbPartIdx]);
  else {
    /* 如果当前不是MBAFF帧或宏块不是场模式解码，size就直接设置为num_ref_idx_l0_active_minus1
     * 如果这两个标志表示当前宏块是在MBAFF帧的场模式下，size被设置为num_ref_idx_l0_active_minus1 * 2
     * 目的是在场模式下，每个场可能都需要独立的参考帧，因此参考列表的实际长度可能是帧模式的两倍。 */
    uint32_t size = 0;
    if (MbaffFrameFlag && mb_field_decoding_flag)
      size = num_ref_idx_l0_active_minus1 * 2;
    else
      size = num_ref_idx_l0_active_minus1;

    ref_idx_l0[mbPartIdx] = _bs->readTE(size);
  }
  return ret;
}

int MacroBlock::process_ref_idx_l1(int mbPartIdx,
                                   uint32_t num_ref_idx_l1_active_minus1) {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_ref_idx_lX(1, mbPartIdx, ref_idx_l1[mbPartIdx]);
  else {
    uint32_t size = 0;
    if (MbaffFrameFlag && mb_field_decoding_flag)
      size = num_ref_idx_l1_active_minus1 * 2;
    else
      size = num_ref_idx_l1_active_minus1;

    ref_idx_l1[mbPartIdx] = _bs->readTE(size);
  }

  return ret;
}

/* 如果是宏块调用则subMbPartIdx默认为0;反之，子宏块调用需要传入subMbPartIdx */
int MacroBlock::process_mvd_l0(const int mbPartIdx, const int compIdx,
                               int32_t subMbPartIdx) {
  int ret = 0;
  int32_t isChroma = 0;
  int32_t mvd_flag = compIdx;
  if (_is_cabac)
    ret = _cabac->decode_mvd_lX(mvd_flag, mbPartIdx, subMbPartIdx, isChroma,
                                mvd_l0[mbPartIdx][subMbPartIdx][compIdx]);
  else
    mvd_l0[mbPartIdx][subMbPartIdx][compIdx] = _bs->readSE();
  return ret;
}

/* 如果是宏块调用则subMbPartIdx默认为0;反之，子宏块调用需要传入subMbPartIdx */
int MacroBlock::process_mvd_l1(const int mbPartIdx, const int compIdx,
                               int32_t subMbPartIdx) {
  int ret = 0;
  int32_t isChroma = 0;
  int32_t mvd_flag = 2 + compIdx;
  if (_is_cabac)
    ret = _cabac->decode_mvd_lX(mvd_flag, mbPartIdx, subMbPartIdx, isChroma,
                                mvd_l1[mbPartIdx][subMbPartIdx][compIdx]);
  else
    mvd_l1[mbPartIdx][subMbPartIdx][compIdx] = _bs->readSE();
  return ret;
}
