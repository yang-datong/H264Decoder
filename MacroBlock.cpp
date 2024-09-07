#include "MacroBlock.hpp"
#include "BitStream.hpp"
#include "CH264Golomb.hpp"
#include "Constants.hpp"
#include "H264Cabac.hpp"
#include "H264ResidualBlockCavlc.hpp"
#include "PictureBase.hpp"
#include "SliceHeader.hpp"
#include "Type.hpp"
#include <cstdint>

#define FREE(ptr)                                                              \
  if (ptr) {                                                                   \
    delete ptr;                                                                \
    ptr = nullptr;                                                             \
  }

MacroBlock::~MacroBlock() {
  _is_cabac = 0;
  FREE(_gb);
  FREE(_cabac);
  FREE(_bs);
  FREE(_cavlc);
}

void MacroBlock::initFromSlice(const SliceHeader &header,
                               const SliceData &slice_data) {
  field_pic_flag = header.field_pic_flag;
  bottom_field_flag = header.bottom_field_flag;
  mb_skip_flag = slice_data.mb_skip_flag;
  mb_field_decoding_flag = slice_data.mb_field_decoding_flag;
  MbaffFrameFlag = header.MbaffFrameFlag;
  disable_deblocking_filter_idc = header.disable_deblocking_filter_idc;
  CurrMbAddr = slice_data.CurrMbAddr;
  slice_id = slice_data.slice_id;
  slice_number = slice_data.slice_number;
  m_slice_type = header.slice_type;
  FilterOffsetA = header.FilterOffsetA;
  FilterOffsetB = header.FilterOffsetB;
}

// 7.3.5 Macroblock layer syntax -> page 57
/* 负责解码一个宏块（这里的宏块指的是16x16 的矩阵块，并不是真的“一个”宏块）的各种信息，包括宏块类型、预测模式、残差数据 */
int MacroBlock::macroblock_layer(BitStream &bs, PictureBase &picture,
                                 const SliceData &slice_data,
                                 CH264Cabac &cabac) {
  /* ------------------ 初始化常用变量 ------------------ */
  this->_picture = &picture;
  this->_cabac = &cabac;
  _is_cabac = _picture->m_slice.m_pps.entropy_coding_mode_flag; // 是否CABAC编码
  if (_gb == nullptr) this->_gb = new CH264Golomb();
  this->_bs = &bs;
  /* ------------------  End ------------------ */

  /* ------------------ 设置别名 ------------------ */
  SliceHeader &header = _picture->m_slice.slice_header;
  SPS &sps = _picture->m_slice.m_sps;
  PPS &pps = _picture->m_slice.m_pps;
  /* ------------------  End ------------------ */

  /* 受限帧内预测标志，这个标志决定了是否可以在帧内预测中使用非帧内编码的宏块 */
  constrained_intra_pred_flag =
      _picture->m_slice.m_pps.constrained_intra_pred_flag;
  initFromSlice(header, slice_data);
  /* ------------------  End ------------------ */
  process_mb_type(header, header.slice_type);

  /* 1. 如果宏块类型是 I_PCM，则直接从比特流中读取未压缩的 PCM 样本数据（非帧内、帧间预测，直接copy原始数据） */
  if (m_mb_type_fixed == I_PCM) {
    while (!bs.byte_aligned())
      pcm_alignment_zero_bit = bs.readUn(1);
    /* 16x16 */
    for (int i = 0; i < 256; i++)
      pcm_sample_luma[i] = bs.readUn(sps.BitDepthY);
    for (int i = 0; i < 2 * (int)(sps.MbWidthC * sps.MbHeightC); i++)
      pcm_sample_chroma[i] = bs.readUn(sps.BitDepthC);

    /* 2. 如果宏块类型不是 I_PCM，则根据宏块类型和预测模式进行子宏块预测或宏块预测，根据宏块的类型和预测模式来决定如何处理宏块的预测信息。
     * 对于I帧而言，首次进入时即为I_NxN, 对于P、B帧而言，m_mb_pred_mode,m_NumMbPart在macroblock_mb_skip()函数中已经计算过了 */
  } else {
    int32_t transform_size_8x8_flag_temp = 0;
    /* 是否所有子宏块的大小都不小于8x8 */
    bool noSubMbPartSizeLessThan8x8Flag = 1;
    // I_NxN，Intra_16x16 是一种帧内预测模式（所以这里表示的是帧间预测）
    if (m_name_of_mb_type != I_NxN && m_mb_pred_mode != Intra_16x16 &&
        m_NumMbPart == 4) {
      //------------------------------------- 帧间预测(P,B) ---------------------------------------
      /* 当前宏块是一个非Intra的宏块，并且被分成了4个8x8子宏块（一般来说4个8x8就是为了运动预测），进行子宏块预测 */
      sub_mb_pred(slice_data);
      /* 3. 子宏块大小检查 */
      for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        if (m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8) {
          if (NumSubMbPartFunc(mbPartIdx) > 1)
            noSubMbPartSizeLessThan8x8Flag = 0;
        } else if (!sps.direct_8x8_inference_flag)
          noSubMbPartSizeLessThan8x8Flag = 0;
      }

    } else {
      //------------------------------------- 帧内预测(I) ---------------------------------------
      if (pps.transform_8x8_mode_flag && m_name_of_mb_type == I_NxN)
        process_transform_size_8x8_flag(transform_size_8x8_flag_temp);
      mb_pred(slice_data);
    }

    /* 4. 处理编码块模式，在上面步骤中，不管是帧间、帧内都已经完成了运动矢量的计算 */
    if (m_mb_pred_mode != Intra_16x16) {
      //------------------------------------- 帧间预测(P,B) ---------------------------------------
      /* 对于帧内预测模式，表示整个宏块作为一个16x16的块进行预测和编码。在这种模式下，宏块不会被进一步分割成更小的块，因此不需要考虑8x8变换 */

      //处理编码块模式（Coded Block Pattern, CBP），它决定了哪些块（亮度块和色度块）包含非零系数。
      process_coded_block_pattern(sps.ChromaArrayType);

      /* - CodedBlockPatternLuma > 0: 说明宏块中至少有一个亮度块包含非零系数，因此需要对这些块进行逆变换和反量化。在这种情况下，可能需要考虑使用8x8变换;
       * - pps.transform_8x8_mode_flag：检查是否编码器允许使用8x8变换;
       * - I_NxN 是帧内预测模式的一种，表示宏块被分割成多个4x4的小块进行预测。在这种模式下，通常不会使用8x8变换，因为块的大小已经是4x4;
       * - noSubMbPartSizeLessThan8x8Flag == 1 : 如果存在任一子宏块的大小小于8x8，那么使用8x8变换就不合适，因为变换块的大小应该与子宏块的大小匹配;
       * - B_Direct_16x16 是B帧中的一种直接模式，表示整个宏块作为一个16x16的块进行直接预测*/
      if (CodedBlockPatternLuma > 0 && pps.transform_8x8_mode_flag &&
          m_name_of_mb_type != I_NxN && noSubMbPartSizeLessThan8x8Flag &&
          (m_name_of_mb_type != B_Direct_16x16 ||
           sps.direct_8x8_inference_flag))
        process_transform_size_8x8_flag(transform_size_8x8_flag_temp);
    }

    /* 3. 如果宏块有残差数据（即 CodedBlockPatternLuma 或 CodedBlockPatternChroma 不为 0），则调用 residual 函数解码残差数据 */
    if (CodedBlockPatternLuma > 0 || CodedBlockPatternChroma > 0 ||
        m_mb_pred_mode == Intra_16x16) {
      //------------------------------------- 帧内预测(I) ---------------------------------------
      process_mb_qp_delta();
      /* 处理残差数据 */
      residual(0, 15);
    }
  }

  /* 4. 根据解码的量化参数增量（mb_qp_delta），更新当前宏块的量化参数 QPY。*/
  /* 7.4.5 Macroblock layer semantics , page 105 */
  /* mb_qp_delta可以改变宏块层中QPY的值。 mb_qp_delta 的解码值应在 -( 26 + QpBdOffsetY / 2) 至 +( 25 + QpBdOffsetY / 2 ) 的范围内，包括端值。当 mb_qp_delta 对于任何宏块（包括 P_Skip 和 B_Skip 宏块类型）不存在时，应推断其等于 0。如果 mb_qp_delta 超出了范围，它会被修正到合法范围内。 */
  if (mb_qp_delta < (int32_t)(-(26 + (int32_t)sps.QpBdOffsetY / 2)) ||
      mb_qp_delta > (25 + (int32_t)sps.QpBdOffsetY / 2)) {
    /* 如果 mb_qp_delta 超出范围，使用 CLIP3 函数将其限制在合法范围内 */
    mb_qp_delta = CLIP3((-(26 + (int32_t)sps.QpBdOffsetY / 2)),
                        (25 + (int32_t)sps.QpBdOffsetY / 2), mb_qp_delta);
  }

  /* 计算当前宏块的量化参数 QPY */
  QPY = ((header.QPY_prev + mb_qp_delta + 52 + 2 * sps.QpBdOffsetY) %
         (52 + sps.QpBdOffsetY)) -
        sps.QpBdOffsetY;

  /* 还原偏移后的QP */
  QP1Y = QPY + sps.QpBdOffsetY;

  /* 更新前一个宏块的量化参数（也就是当前宏快，对于下一个宏快来说就是前一个） */
  header.QPY_prev = QPY;

  // 7.4.5 Macroblock layer semantics -> mb_qp_delta
  /* 计算出是否启用变换旁路模式 */
  TransformBypassModeFlag =
      (sps.qpprime_y_zero_transform_bypass_flag == 1 && QP1Y == 0) ? 1 : 0;

  return 0;
}

#define MB_TYPE_P_SP_Skip 5
#define MB_TYPE_B_Skip 23
/* 该函数将一个宏块进行预处理，设置对应的状态，但是并不需要进行真正的解码操作（跟macroblock_layer函数非常类似） */
int MacroBlock::macroblock_mb_skip(PictureBase &picture,
                                   const SliceData &slice_data,
                                   CH264Cabac &cabac) {
  /* ------------------ 初始化常用变量 ------------------ */
  this->_cabac = &cabac;
  this->_picture = &picture;
  if (_gb == nullptr) this->_gb = new CH264Golomb();
  /* ------------------  End ------------------ */
  int ret = 0;

  /* TODO YangJing 这里的header应该是输入，不应该存在输出 <24-09-03 21:12:33> */
  SliceHeader &header = _picture->m_slice.slice_header;
  int32_t &QPY_prev = header.QPY_prev;
  const uint32_t QpBdOffsetY = _picture->m_slice.m_sps.QpBdOffsetY;

  /* 受限帧内预测标志，这个标志决定了是否可以在帧内预测中使用非帧内编码的宏块 */
  constrained_intra_pred_flag =
      _picture->m_slice.m_pps.constrained_intra_pred_flag;
  initFromSlice(header, slice_data);

  /* 执行逆宏块扫描过程，确定当前宏块在帧中的位置。这一步通常是为了处理宏块的地址映射，特别是在使用宏块自适应帧场编码（MBAFF）时 */
  ret = _picture->inverse_macroblock_scanning_process(
      header.MbaffFrameFlag, CurrMbAddr, mb_field_decoding_flag,
      _picture->m_mbs[CurrMbAddr].m_mb_position_x,
      _picture->m_mbs[CurrMbAddr].m_mb_position_y);
  /* 比如说，这里出来的x,y应该是(0,0) (16,0) (32,0) (48,0) */
  if (ret != 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return ret;
  }

  /* 当Slice为P,SP时，5表示P_Skip，即跳过宏块处理，当Slice为B时，23表示P_Skip，即跳过宏块处理 */
  if (header.slice_type == SLICE_P || header.slice_type == SLICE_SP)
    m_mb_type_fixed = mb_type = MB_TYPE_P_SP_Skip;
  else if (header.slice_type == SLICE_B)
    m_mb_type_fixed = mb_type = MB_TYPE_B_Skip;

  m_slice_type_fixed = header.slice_type;

  /* 据宏块类型和其他参数，设置宏块的预测模式。这一步决定了如何对宏块进行预测和解码 */
  /* 7.4.5 Macroblock layer semantics -> mb_type */
  ret = MbPartPredMode(m_slice_type_fixed, transform_size_8x8_flag,
                       m_mb_type_fixed, 0, m_NumMbPart, CodedBlockPatternChroma,
                       CodedBlockPatternLuma, Intra16x16PredMode,
                       m_name_of_mb_type, m_mb_pred_mode);
  if (ret != 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return ret;
  }

  /* 计算当前宏块的量化参数（QP）。量化参数影响解码后的图像质量和压缩率*/

  /* 7.4.5 Macroblock layer semantics , page 105 */
  /* mb_qp_delta可以改变宏块层中QPY的值。 mb_qp_delta 的解码值应在 -( 26 + QpBdOffsetY / 2) 至 +( 25 + QpBdOffsetY / 2 ) 的范围内，包括端值。当 mb_qp_delta 对于任何宏块（包括 P_Skip 和 B_Skip 宏块类型）不存在时，应推断其等于 0。如果 mb_qp_delta 超出了范围，它会被修正到合法范围内。 */

  /* 这里mb_qp_delta确定为0,则不需要进行修正 */
  mb_qp_delta = 0;

  /* 计算当前宏块的量化参数 QPY */
  QPY = ((QPY_prev + mb_qp_delta + 52 + 2 * QpBdOffsetY) % (52 + QpBdOffsetY)) -
        QpBdOffsetY;

  /* 还原偏移后的QP */
  QP1Y = QPY + QpBdOffsetY;

  /* 更新前一个宏块的量化参数（也就是当前宏快，对于下一个宏快来说就是前一个） */
  QPY_prev = QPY;

  /* 将宏块类型和相关的片段信息设置到宏块中，以便后续的解码过程使用 */
  ret = MbPartPredMode();
  // 因CABAC会用到MbPartWidth/MbPartHeight信息，所以需要尽可能提前设置相关值
  if (ret) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return ret;
  }

  return 0;
}

// 7.3.5.1 Macroblock prediction syntax
/* 根据宏块的预测模式（m_mb_pred_mode），处理亮度和色度的预测模式以及运动矢量差（MVD）的计算 
 * 此处的预测模式包括帧内预测（Intra）和帧间预测（Inter）*/
int MacroBlock::mb_pred(const SliceData &slice_data) {
  /* ------------------ 设置别名 ------------------ */
  const SliceHeader &header = _picture->m_slice.slice_header;
  SPS &sps = _picture->m_slice.m_sps;
  /* ------------------  End ------------------ */

  /* --------------------------这一部分属于帧间预测-------------------------- */
  /* 获取当前宏块所使用的预测模式 */
  if (m_mb_pred_mode == Intra_4x4 || m_mb_pred_mode == Intra_8x8 ||
      m_mb_pred_mode == Intra_16x16) {

    /* 宏块的大小通常是 16x16 像素。对于 Intra_4x4 预测模式，宏块被划分为 16 个 4x4 的亮度块（luma blocks），每个 4x4 块独立进行预测。这种模式允许更细粒度的预测，从而更好地适应图像中的局部变化。 */

    // NOTE: 这里有一个规则：每个块（假如是4x4) 可以选择一个预测模式（总共有 9 种可能的模式）。然而，相邻的 4x4 块通常会选择相同或相似的预测模式。为了减少冗余信息，H.264 标准引入了一个标志位 prev_intra4x4_pred_mode_flag，用于指示当前块是否使用了与前一个块相同的预测模式。通过这种方式，编码器可以在相邻块使用相同预测模式时节省比特数，因为不需要为每个块单独编码预测模式。只有在预测模式发生变化时，才需要额外编码新的模式，从而减少了整体的编码开销。

    /* 1. 对于 Intra_4x4 模式，遍历 16 个 4x4 的亮度块: */
    if (m_mb_pred_mode == Intra_4x4) {
      for (int luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; luma4x4BlkIdx++) {
        //解码每个块的前一个 Intra 4x4 预测模式标志
        process_prev_intra4x4_pred_mode_flag(luma4x4BlkIdx);
        if (!prev_intra4x4_pred_mode_flag[luma4x4BlkIdx])
          //当前一个块不存在预测模式时，解码剩余的 Intra 4x4 预测模式
          process_rem_intra4x4_pred_mode(luma4x4BlkIdx);
      }
    }

    /* 2. 对于 Intra_8x8 模式，遍历 4 个 8x8 的亮度块: */
    if (m_mb_pred_mode == Intra_8x8) {
      for (int luma8x8BlkIdx = 0; luma8x8BlkIdx < 4; luma8x8BlkIdx++) {
        //解码每个块的前一个 Intra 8x8 预测模式标志
        process_prev_intra8x8_pred_mode_flag(luma8x8BlkIdx);
        if (!prev_intra8x8_pred_mode_flag[luma8x8BlkIdx])
          //当前一个块不存在预测模式时，解码剩余的 Intra 8x8 预测模式
          process_rem_intra8x8_pred_mode(luma8x8BlkIdx);
      }
    }

    /* 如果色度阵列类型为 YUV420 或 YUV422，则处理色度的 Intra 预测模式 */
    if (sps.ChromaArrayType == 1 || sps.ChromaArrayType == 2)
      process_intra_chroma_pred_mode();

    /* 当前预测模式不为直接模式(通过周围块的信息来推导运动矢量) */
  } else if (m_mb_pred_mode != Direct) {
    /* --------------------------这一部分属于帧间预测-------------------------- */
    int ret;
    H264_MB_PART_PRED_MODE mb_pred_mode = MB_PRED_MODE_NA;
    for (int mbPartIdx = 0; mbPartIdx < m_NumMbPart; mbPartIdx++) {
      ret = MbPartPredMode(m_name_of_mb_type, mbPartIdx,
                           transform_size_8x8_flag, mb_pred_mode);
      RET(ret);

      /* (前参考）参考帧列表 0 中有多于一个参考帧可供选择 || 当前宏块的场解码模式与整个图片的场模式不同(这种情况下需要特别处理参考帧索引) 
       * 并且它使用的是参考帧列表 0（非RefPicList1，即RefPicList0）进行预测*/
      if ((header.num_ref_idx_l0_active_minus1 + 1 > 1 ||
           mb_field_decoding_flag != field_pic_flag) &&
          mb_pred_mode != Pred_L1)
        /* 根据预测模式处理参考索引*/
        process_ref_idx_l0(mbPartIdx, header.num_ref_idx_l0_active_minus1);
    }

    for (int mbPartIdx = 0; mbPartIdx < m_NumMbPart; mbPartIdx++) {
      ret = MbPartPredMode(m_name_of_mb_type, mbPartIdx,
                           transform_size_8x8_flag, mb_pred_mode);
      RET(ret);

      /* (后参考）参考帧列表 1 中有多于一个参考帧可供选择 || 当前宏块的场解码模式与整个图片的场模式不同(这种情况下需要特别处理参考帧索引) 
       * 并且它使用的是参考帧列表 1（非RefPicList0，即RefPicList1）进行预测*/
      if ((header.num_ref_idx_l1_active_minus1 + 1 > 1 ||
           mb_field_decoding_flag != field_pic_flag) &&
          mb_pred_mode != Pred_L0)
        /* 根据预测模式处理参考索引*/
        process_ref_idx_l1(mbPartIdx, header.num_ref_idx_l1_active_minus1);
    }

    //NOTE: 预测模式可以是 Pred_L0、Pred_L1 或 BiPred，分别表示使用参考帧列表0、参考帧列表1或双向预测

    for (int mbPartIdx = 0; mbPartIdx < m_NumMbPart; mbPartIdx++) {
      ret = MbPartPredMode(m_name_of_mb_type, mbPartIdx,
                           transform_size_8x8_flag, mb_pred_mode);
      if (mb_pred_mode != Pred_L1) {
        /* 根据预测模式处理运动矢量差，运动矢量差是当前宏块部分的运动矢量与预测运动矢量之间的差值 */
        for (int compIdx = 0; compIdx < 2; compIdx++)
          /* compIdx 遍历两个分量（通常是水平方向和垂直方向），分别处理这两个方向上的运动矢量差。 */
          process_mvd_l0(mbPartIdx, compIdx);
      }
    }

    for (int mbPartIdx = 0; mbPartIdx < m_NumMbPart; mbPartIdx++) {
      ret = MbPartPredMode(m_name_of_mb_type, mbPartIdx,
                           transform_size_8x8_flag, mb_pred_mode);
      if (mb_pred_mode != Pred_L0) {
        /* 同上 */
        for (int compIdx = 0; compIdx < 2; compIdx++)
          process_mvd_l1(mbPartIdx, compIdx);
      }
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
  const SliceHeader &header = _picture->m_slice.slice_header;

  /* 1. 解析子宏块类型，至于这里为什么是固定4个，是因为在macroblock_layer()函数中，已经通过 if (m_name_of_mb_type != I_NxN && m_mb_pred_mode != Intra_16x16 && m_NumMbPart == 4)进行限定 */
  for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
    process_sub_mb_type(mbPartIdx);

  /* 2. 解析参考帧索引 */
  for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    if ((header.num_ref_idx_l0_active_minus1 > 0 ||
         mb_field_decoding_flag != field_pic_flag) &&
        m_name_of_mb_type != P_8x8ref0 &&
        m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
        m_sub_mb_pred_mode[mbPartIdx] != Pred_L1)
      process_ref_idx_l0(mbPartIdx, header.num_ref_idx_l0_active_minus1);
  }

  for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    if ((header.num_ref_idx_l1_active_minus1 > 0 ||
         mb_field_decoding_flag != field_pic_flag) &&
        m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
        m_sub_mb_pred_mode[mbPartIdx] != Pred_L0)
      process_ref_idx_l1(mbPartIdx, header.num_ref_idx_l1_active_minus1);
  }

  for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    if (m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
        m_sub_mb_pred_mode[mbPartIdx] != Pred_L1) {
      for (int subMbPartIdx = 0; subMbPartIdx < NumSubMbPart[mbPartIdx];
           subMbPartIdx++)
        for (int compIdx = 0; compIdx < 2; compIdx++)
          process_mvd_l0(mbPartIdx, compIdx, subMbPartIdx);
    }
  }

  for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    if (m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
        m_sub_mb_pred_mode[mbPartIdx] != Pred_L0) {
      for (int subMbPartIdx = 0; subMbPartIdx < NumSubMbPart[mbPartIdx];
           subMbPartIdx++)
        for (int compIdx = 0; compIdx < 2; compIdx++)
          process_mvd_l1(mbPartIdx, compIdx, subMbPartIdx);
    }
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

int MacroBlock::fix_mb_type(const int32_t slice_type_raw,
                            const int32_t mb_type_raw,
                            int32_t &slice_type_fixed, int32_t &mb_type_fixed) {
  slice_type_fixed = slice_type_raw;
  mb_type_fixed = mb_type_raw;

  if ((slice_type_raw % 5) == SLICE_I) {
    // 不需要修正
  } else if ((slice_type_raw % 5) == SLICE_SI) {
    // The macroblock types for SI slices are specified in Tables 7-12 and 7-11.
    // The mb_type value 0 is specified in Table 7-12 and the mb_type values 1
    // to 26 are specified in Table 7-11, indexed by subtracting 1 from the
    // value of mb_type.
    if (mb_type_raw == 0) {
      // 不需要修正
    } else if (mb_type_raw >= 1 && mb_type_raw <= 26) {
      slice_type_fixed = SLICE_I;
      mb_type_fixed = mb_type_raw - 1; // 说明 SI slices 中含有I宏块
    } else {
      printf("SI slices: mb_type_raw=%d; Must be in [0..26]\n", mb_type_raw);
      return -1;
    }
  } else if ((slice_type_raw % 5) == SLICE_P ||
             (slice_type_raw % 5) == SLICE_SP) {
    // The macroblock types for P and SP slices are specified in Tables 7-13 and
    // 7-11. mb_type values 0 to 4 are specified in Table 7-13 and mb_type
    // values 5 to 30 are specified in Table 7-11, indexed by subtracting 5 from
    // the value of mb_type.
    if (mb_type_raw >= 0 && mb_type_raw <= 4) {
      // 不需要修正
    } else if (mb_type_raw >= 5 && mb_type_raw <= 30) {
      slice_type_fixed = SLICE_I;
      mb_type_fixed = mb_type_raw - 5; // 说明 P and SP slices 中含有I宏块
    } else {
      printf("P and SP slices: mb_type_raw=%d; Must be in [0..30]\n",
             mb_type_raw);
      return -1;
    }
  } else if ((slice_type_raw % 5) == SLICE_B) {
    // The macroblock types for B slices are specified in Tables 7-14 and 7-11.
    // The mb_type values 0 to 22 are specified in Table 7-14 and the mb_type
    // values 23 to 48 are specified in Table 7-11, indexed by subtracting 23
    // from the value of mb_type.
    if (mb_type_raw >= 0 && mb_type_raw <= 22) {
      // 不需要修正
    } else if (mb_type_raw >= 23 && mb_type_raw <= 48) {
      slice_type_fixed = SLICE_I;
      mb_type_fixed = mb_type_raw - 23; // 说明 B slices 中含有I宏块
    } else {
      printf("B slices: mb_type_raw=%d; Must be in [0..48]\n", mb_type_raw);
      return -1;
    }
  }

  return 0;
}

/* Table 7-11 – Macroblock types for I slices
 * Table 7-12 – Macroblock type with value 0 for SI slices
 * Table 7-13 – Macroblock type values 0 to 4 for P and SP slices
 * Table 7-14 – Macroblock type values 0 to 22 for B slices */
int MacroBlock::MbPartPredMode(
    int32_t slice_type, int32_t transform_size_8x8_flag, int32_t _mb_type,
    int32_t index, int32_t &NumMbPart, int32_t &CodedBlockPatternChroma,
    int32_t &CodedBlockPatternLuma, int32_t &_Intra16x16PredMode,
    H264_MB_TYPE &name_of_mb_type, H264_MB_PART_PRED_MODE &mb_pred_mode) {

  //Table 7-11 – Macroblock types for I slices
  if ((slice_type % 5) == SLICE_I) {
    const int I_NxN = 0;
    if (_mb_type == I_NxN) {
      if (transform_size_8x8_flag == 0) {
        name_of_mb_type = mb_type_I_slices_define[0].name_of_mb_type;
        mb_pred_mode = mb_type_I_slices_define[0].MbPartPredMode;
      } else {
        name_of_mb_type = mb_type_I_slices_define[1].name_of_mb_type;
        mb_pred_mode = mb_type_I_slices_define[1].MbPartPredMode;
      }
    } else if (_mb_type >= MIN_MB_TYPE_FOR_I_SLICE &&
               _mb_type <= MAX_MB_TYPE_FOR_I_SLICE) {
      name_of_mb_type = mb_type_I_slices_define[_mb_type + 1].name_of_mb_type;
      CodedBlockPatternChroma =
          mb_type_I_slices_define[_mb_type + 1].CodedBlockPatternChroma;
      CodedBlockPatternLuma =
          mb_type_I_slices_define[_mb_type + 1].CodedBlockPatternLuma;
      _Intra16x16PredMode =
          mb_type_I_slices_define[_mb_type + 1].Intra16x16PredMode;
      mb_pred_mode = mb_type_I_slices_define[_mb_type + 1].MbPartPredMode;
    } else {
      std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
                << std::endl;
      return -1;
    }

    //Table 7-12 – Macroblock type with value 0 for SI slices
  } else if ((slice_type % 5) == SLICE_SI) {
    if (_mb_type == 0) {
      name_of_mb_type = mb_type_SI_slices_define[0].name_of_mb_type;
      mb_pred_mode = mb_type_SI_slices_define[0].MbPartPredMode;
    } else {
      std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
                << std::endl;
      return -1;
    }

    //Table 7-13 – Macroblock type values 0 to 4 for P and SP slices
  } else if ((slice_type % 5) == SLICE_P || (slice_type % 5) == SLICE_SP) {
    if (_mb_type >= 0 && _mb_type <= 5) {
      name_of_mb_type = mb_type_P_SP_slices_define[_mb_type].name_of_mb_type;
      NumMbPart = mb_type_P_SP_slices_define[_mb_type].NumMbPart;
      if (index == 0)
        mb_pred_mode = mb_type_P_SP_slices_define[_mb_type].MbPartPredMode0;
      else
        mb_pred_mode = mb_type_P_SP_slices_define[_mb_type].MbPartPredMode1;
    } else {
      std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
                << std::endl;
      return -1;
    }

    //Table 7-14 – Macroblock type values 0 to 22 for B slices
  } else if ((slice_type % 5) == SLICE_B) {
    if (_mb_type >= 0 && _mb_type <= 23) {
      name_of_mb_type = mb_type_B_slices_define[_mb_type].name_of_mb_type;
      NumMbPart = mb_type_B_slices_define[_mb_type].NumMbPart;
      if (index == 0)
        mb_pred_mode = mb_type_B_slices_define[_mb_type].MbPartPredMode0;
      else
        mb_pred_mode = mb_type_B_slices_define[_mb_type].MbPartPredMode1;
    } else {
      std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
                << std::endl;
      return -1;
    }
  } else {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

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
    if (mbPartIdx == 0) {
      if (transform_size_8x8_flag == 0)
        mb_pred_mode = mb_type_I_slices_define[0].MbPartPredMode;
      else
        mb_pred_mode = mb_type_I_slices_define[1].MbPartPredMode;
    } else
      RET(-1);

  } else if (name_of_mb_type >= MIN_MB_TYPE_FOR_I_SLICE &&
             name_of_mb_type <= MAX_MB_TYPE_FOR_I_SLICE) {
    if (mbPartIdx == 0)
      mb_pred_mode =
          mb_type_I_slices_define[mbPartIdx - MIN_MB_TYPE_FOR_I_SLICE]
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
  //Table 7-11 – Macroblock types for I slices
  if ((m_slice_type_fixed % 5) == SLICE_I) {
    if (m_mb_type_fixed == 0) {
      if (transform_size_8x8_flag == 0)
        mb_type_I_slice = mb_type_I_slices_define[0];
      else
        mb_type_I_slice = mb_type_I_slices_define[1];

    } else if (m_mb_type_fixed >= 1 && m_mb_type_fixed <= 25)
      mb_type_I_slice = mb_type_I_slices_define[m_mb_type_fixed + 1];
    else {
      std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
                << std::endl;
      return -1;
    }

    //Table 7-12 – Macroblock type with value 0 for SI slices
  } else if ((m_slice_type_fixed % 5) == SLICE_SI) {
    if (m_mb_type_fixed == 0)
      mb_type_SI_slice = mb_type_SI_slices_define[0];
    else {
      std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
                << std::endl;
      return -1;
    }

    //Table 7-13 – Macroblock type values 0 to 4 for P and SP slices
  } else if ((m_slice_type_fixed % 5) == SLICE_P ||
             (m_slice_type_fixed % 5) == SLICE_SP) {
    if (m_mb_type_fixed >= 0 && m_mb_type_fixed <= 5)
      mb_type_P_SP_slice = mb_type_P_SP_slices_define[m_mb_type_fixed];
    else {
      std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
                << std::endl;
      return -1;
    }

    MbPartWidth = mb_type_P_SP_slice.MbPartWidth;
    MbPartHeight = mb_type_P_SP_slice.MbPartHeight;
    m_NumMbPart = mb_type_P_SP_slice.NumMbPart;

    //Table 7-14 – Macroblock type values 0 to 22 for B slices
  } else if ((m_slice_type_fixed % 5) == SLICE_B) {
    if (m_mb_type_fixed >= 0 && m_mb_type_fixed <= 23)
      mb_type_B_slice = mb_type_B_slices_define[m_mb_type_fixed];
    else {
      std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
                << std::endl;
      return -1;
    }

    MbPartWidth = mb_type_B_slice.MbPartWidth;
    MbPartHeight = mb_type_B_slice.MbPartHeight;
    m_NumMbPart = mb_type_B_slice.NumMbPart;
  } else {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

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
    } else {
      std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
                << std::endl;
      return -1;
    }
  } else if (slice_type == SLICE_B) {
    if (sub_mb_type >= 0 && sub_mb_type <= 12) {
      /* Table 7-18 – Sub-macroblock types in B macroblocks */
      NumSubMbPart = sub_mb_type_B_mbs_define[sub_mb_type].NumSubMbPart;
      SubMbPredMode = sub_mb_type_B_mbs_define[sub_mb_type].SubMbPredMode;
      SubMbPartWidth = sub_mb_type_B_mbs_define[sub_mb_type].SubMbPartWidth;
      SubMbPartHeight = sub_mb_type_B_mbs_define[sub_mb_type].SubMbPartHeight;
    } else {
      std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
                << std::endl;
      return -1;
    }
  } else {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

  return 0;
}

int MacroBlock::residual_block_DC(int32_t coeffLevel[], int32_t startIdx,
                                  int32_t endIdx, int32_t maxNumCoeff,
                                  int iCbCr, int32_t BlkIdx) {
  int ret = 0;
  int TotalCoeff = 0;

  if (_is_cabac)
    ret = _cabac->residual_block_cabac(coeffLevel, startIdx, endIdx,
                                       maxNumCoeff, MB_RESIDUAL_ChromaDCLevel,
                                       BlkIdx, iCbCr, TotalCoeff);
  else {
    MB_RESIDUAL_LEVEL mb_level = (iCbCr == 0) ? MB_RESIDUAL_ChromaDCLevelCb
                                              : MB_RESIDUAL_ChromaDCLevelCr;

    ret = _cavlc->residual_block_cavlc(coeffLevel, 0, endIdx, maxNumCoeff,
                                       mb_level, m_mb_pred_mode, BlkIdx,
                                       TotalCoeff);
  }
  RET(ret);

  mb_chroma_4x4_non_zero_count_coeff[iCbCr][BlkIdx] = TotalCoeff;
  return ret;
}

int MacroBlock::residual_block_AC(int32_t coeffLevel[], int32_t startIdx,
                                  int32_t endIdx, int32_t maxNumCoeff,
                                  int iCbCr, int32_t BlkIdx) {

  int ret = 0;
  int TotalCoeff = 0;

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
  return ret;
}

// 7.3.5.3 Residual data syntax
int MacroBlock::residual(int32_t startIdx, int32_t endIdx) {

  if (!_cavlc) _cavlc = new CH264ResidualBlockCavlc(_picture, _bs);
  const uint32_t ChromaArrayType = _picture->m_slice.m_sps.ChromaArrayType;
  const int32_t SubWidthC = _picture->m_slice.m_sps.SubWidthC;
  const int32_t SubHeightC = _picture->m_slice.m_sps.SubHeightC;

  int ret = 0;
  //int32_t TotalCoeff = 0; // 该 4x4 block的残差中，总共有多少个非零系数
  _mb_residual_level_dc = MB_RESIDUAL_Intra16x16DCLevel;
  _mb_residual_level_ac = MB_RESIDUAL_Intra16x16ACLevel;
  ret = residual_luma(startIdx, endIdx);
  RET(ret);

  /* TODO YangJing 感觉这里性能有问题 <24-09-07 01:15:56> */
  memcpy(Intra16x16DCLevel, i16x16DClevel, sizeof(i16x16DClevel));
  memcpy(Intra16x16ACLevel, i16x16AClevel, sizeof(i16x16AClevel));
  memcpy(LumaLevel4x4, level4x4, sizeof(level4x4));
  memcpy(LumaLevel8x8, level8x8, sizeof(level8x8));

  if (ChromaArrayType == 1 || ChromaArrayType == 2) {
    int32_t NumC8x8 = 4 / (SubWidthC * SubHeightC);
    for (int iCbCr = 0; iCbCr < 2; iCbCr++) {
      if ((CodedBlockPatternChroma & 3) && startIdx == 0)
        residual_block_DC(ChromaDCLevel[iCbCr], 0, 4 * NumC8x8 - 1, 4 * NumC8x8,
                          iCbCr, 0);
      else
        for (int i = 0; i < 4 * NumC8x8; i++)
          ChromaDCLevel[iCbCr][i] = 0;
    }

    for (int iCbCr = 0; iCbCr < 2; iCbCr++) {
      for (int i8x8 = 0; i8x8 < NumC8x8; i8x8++)
        for (int i4x4 = 0; i4x4 < 4; i4x4++) {
          int32_t BlkIdx = i8x8 * 4 + i4x4;
          if (CodedBlockPatternChroma & 2)
            residual_block_AC(ChromaACLevel[iCbCr][BlkIdx],
                              MAX(0, startIdx - 1), endIdx - 1, 15, iCbCr,
                              BlkIdx);
          else
            for (int i = 0; i < 15; i++)
              ChromaACLevel[iCbCr][BlkIdx][i] = 0;
        }
    }
  } else if (ChromaArrayType == 3) {
    _mb_residual_level_dc = MB_RESIDUAL_CbIntra16x16DCLevel;
    _mb_residual_level_ac = MB_RESIDUAL_CbIntra16x16ACLevel;
    ret = residual_luma(startIdx, endIdx);
    RET(ret);

    /* TODO YangJing 性能 <24-09-07 01:43:24> */
    memcpy(CbIntra16x16DCLevel, i16x16DClevel, sizeof(int32_t) * 16);
    memcpy(CbIntra16x16ACLevel, i16x16AClevel, sizeof(int32_t) * 16 * 16);
    memcpy(CbLevel4x4, level4x4, sizeof(int32_t) * 16 * 16);
    memcpy(CbLevel8x8, level8x8, sizeof(int32_t) * 4 * 64);

    _mb_residual_level_dc = MB_RESIDUAL_CrIntra16x16DCLevel;
    _mb_residual_level_ac = MB_RESIDUAL_CrIntra16x16ACLevel;
    ret = residual_luma(startIdx, endIdx);
    RET(ret);

    memcpy(CrIntra16x16DCLevel, i16x16DClevel, sizeof(int32_t) * 16);
    memcpy(CrIntra16x16ACLevel, i16x16AClevel, sizeof(int32_t) * 16 * 16);
    memcpy(CrLevel4x4, level4x4, sizeof(int32_t) * 16 * 16);
    memcpy(CrLevel8x8, level8x8, sizeof(int32_t) * 4 * 64);
  }

  return 0;
}

//7.3.5.3.1 Residual luma syntax
int MacroBlock::residual_luma(int32_t startIdx, int32_t endIdx) {
  int ret = 0;
  int32_t BlkIdx = 0;
  int32_t TotalCoeff = 0; // 该 4x4 block的残差中，总共有多少个非零系数

  if (startIdx == 0 && m_mb_pred_mode == Intra_16x16) {
    BlkIdx = 0;
    TotalCoeff = 0;

    if (_is_cabac)
      ret = _cabac->residual_block_cabac(i16x16DClevel, 0, 15, 16,
                                         _mb_residual_level_dc, BlkIdx, -1,
                                         TotalCoeff);
    else
      ret = _cavlc->residual_block_cavlc(i16x16DClevel, 0, 15, 16,
                                         _mb_residual_level_dc, m_mb_pred_mode,
                                         BlkIdx, TotalCoeff);
    RET(ret);

    mb_luma_4x4_non_zero_count_coeff[BlkIdx] = TotalCoeff;
  }

  for (int i8x8 = 0; i8x8 < 4; i8x8++) {
    if (!transform_size_8x8_flag ||
        !_picture->m_slice.m_pps.entropy_coding_mode_flag) {
      for (int i4x4 = 0; i4x4 < 4; i4x4++) {
        if (CodedBlockPatternLuma & (1 << i8x8)) {
          BlkIdx = i8x8 * 4 + i4x4;
          TotalCoeff = 0;

          if (m_mb_pred_mode == Intra_16x16) {
            if (_is_cabac) {
              ret = _cabac->residual_block_cabac(
                  i16x16AClevel[i8x8 * 4 + i4x4], MAX(0, startIdx - 1),
                  endIdx - 1, 15, MB_RESIDUAL_Intra16x16ACLevel, BlkIdx, -1,
                  TotalCoeff);
              RETURN_IF_FAILED(ret != 0, ret);
            } else {
              ret = _cavlc->residual_block_cavlc(
                  i16x16AClevel[i8x8 * 4 + i4x4], MAX(0, startIdx - 1),
                  endIdx - 1, 15, MB_RESIDUAL_Intra16x16ACLevel, m_mb_pred_mode,
                  BlkIdx, TotalCoeff);
              RETURN_IF_FAILED(ret != 0, ret);
            }
          } else {
            if (_is_cabac) {
              ret = _cabac->residual_block_cabac(
                  level4x4[i8x8 * 4 + i4x4], startIdx, endIdx, 16,
                  MB_RESIDUAL_LumaLevel4x4, BlkIdx, -1, TotalCoeff);
              RETURN_IF_FAILED(ret != 0, ret);
            } else {
              ret = _cavlc->residual_block_cavlc(
                  level4x4[i8x8 * 4 + i4x4], startIdx, endIdx, 16,
                  MB_RESIDUAL_LumaLevel4x4, m_mb_pred_mode, BlkIdx, TotalCoeff);
              RETURN_IF_FAILED(ret != 0, ret);
            }
          }

          mb_luma_4x4_non_zero_count_coeff[BlkIdx] = TotalCoeff;
          mb_luma_8x8_non_zero_count_coeff[i8x8] +=
              mb_luma_4x4_non_zero_count_coeff[i8x8 * 4 + i4x4];
        } else if (m_mb_pred_mode == Intra_16x16) {
          for (int i = 0; i < 15; i++) {
            i16x16AClevel[i8x8 * 4 + i4x4][i] = 0;
          }
        } else {
          for (int i = 0; i < 16; i++) {
            level4x4[i8x8 * 4 + i4x4][i] = 0;
          }
        }

        if (!_picture->m_slice.m_pps.entropy_coding_mode_flag &&
            transform_size_8x8_flag) {
          for (int i = 0; i < 16; i++) {
            level8x8[i8x8][4 * i + i4x4] = level4x4[i8x8 * 4 + i4x4][i];
          }
          mb_luma_8x8_non_zero_count_coeff[i8x8] +=
              mb_luma_4x4_non_zero_count_coeff[i8x8 * 4 + i4x4];
        }
      }
    } else if (CodedBlockPatternLuma & (1 << i8x8)) {
      BlkIdx = i8x8;
      TotalCoeff = 0;

      if (_is_cabac) {
        ret = _cabac->residual_block_cabac(
            level8x8[i8x8], 4 * startIdx, 4 * endIdx + 3, 64,
            MB_RESIDUAL_LumaLevel8x8, BlkIdx, -1, TotalCoeff);
        RETURN_IF_FAILED(ret != 0, ret);
      } else {
        ret = _cavlc->residual_block_cavlc(
            level8x8[i8x8], 4 * startIdx, 4 * endIdx + 3, 64,
            MB_RESIDUAL_LumaLevel8x8, m_mb_pred_mode, BlkIdx, TotalCoeff);
        RETURN_IF_FAILED(ret != 0, ret);
      }

      mb_luma_8x8_non_zero_count_coeff[i8x8] = TotalCoeff;
    } else {
      for (int i = 0; i < 64; i++) {
        level8x8[i8x8][i] = 0;
      }
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

string MacroBlock::getNameOfMbTypeStr(H264_MB_TYPE name_of_mb_type) {
  switch (name_of_mb_type) {
  case MB_TYPE_NA: {
    return "MB_TYPE_NA";
    break;
  }
  case I_NxN: {
    return "I_NxN";
    break;
  }
  case I_16x16_0_0_0: {
    return "I_16x16_0_0_0";
    break;
  }
  case I_16x16_1_0_0: {
    return "I_16x16_1_0_0";
    break;
  }
  case I_16x16_2_0_0: {
    return "I_16x16_2_0_0";
    break;
  }
  case I_16x16_3_0_0: {
    return "I_16x16_3_0_0";
    break;
  }
  case I_16x16_0_1_0: {
    return "I_16x16_0_1_0";
    break;
  }
  case I_16x16_1_1_0: {
    return "I_16x16_1_1_0";
    break;
  }
  case I_16x16_2_1_0: {
    return "I_16x16_2_1_0";
    break;
  }
  case I_16x16_3_1_0: {
    return "I_16x16_3_1_0";
    break;
  }
  case I_16x16_0_2_0: {
    return "I_16x16_0_2_0";
    break;
  }
  case I_16x16_1_2_0: {
    return "I_16x16_1_2_0";
    break;
  }
  case I_16x16_2_2_0: {
    return "I_16x16_2_2_0";
    break;
  }
  case I_16x16_3_2_0: {
    return "I_16x16_3_2_0";
    break;
  }
  case I_16x16_0_0_1: {
    return "I_16x16_0_0_1";
    break;
  }
  case I_16x16_1_0_1: {
    return "I_16x16_1_0_1";
    break;
  }
  case I_16x16_2_0_1: {
    return "I_16x16_2_0_1";
    break;
  }
  case I_16x16_3_0_1: {
    return "I_16x16_3_0_1";
    break;
  }
  case I_16x16_0_1_1: {
    return "I_16x16_0_1_1";
    break;
  }
  case I_16x16_1_1_1: {
    return "I_16x16_1_1_1";
    break;
  }
  case I_16x16_2_1_1: {
    return "I_16x16_2_1_1";
    break;
  }
  case I_16x16_3_1_1: {
    return "I_16x16_3_1_1";
    break;
  }
  case I_16x16_0_2_1: {
    return "I_16x16_0_2_1";
    break;
  }
  case I_16x16_1_2_1: {
    return "I_16x16_1_2_1";
    break;
  }
  case I_16x16_2_2_1: {
    return "I_16x16_2_2_1";
    break;
  }
  case I_16x16_3_2_1: {
    return "I_16x16_3_2_1";
    break;
  }
  case I_PCM: {
    return "I_PCM";
    break;
  }
  case SI: {
    return "SI";
    break;
  }
  case P_L0_16x16: {
    return "P_L0_16x16";
    break;
  }
  case P_L0_L0_16x8: {
    return "P_L0_L0_16x8";
    break;
  }
  case P_L0_L0_8x16: {
    return "P_L0_L0_8x16";
    break;
  }
  case P_8x8: {
    return "P_8x8";
    break;
  }
  case P_8x8ref0: {
    return "P_8x8ref0";
    break;
  }
  case P_Skip: {
    return "P_Skip";
    break;
  }
  case B_Direct_16x16: {
    return "B_Direct_16x16";
    break;
  }
  case B_L0_16x16: {
    return "B_L0_16x16";
    break;
  }
  case B_L1_16x16: {
    return "B_L1_16x16";
    break;
  }
  case B_Bi_16x16: {
    return "B_Bi_16x16";
    break;
  }
  case B_L0_L0_16x8: {
    return "B_L0_L0_16x8";
    break;
  }
  case B_L0_L0_8x16: {
    return "B_L0_L0_8x16";
    break;
  }
  case B_L1_L1_16x8: {
    return "B_L1_L1_16x8";
    break;
  }
  case B_L1_L1_8x16: {
    return "B_L1_L1_8x16";
    break;
  }
  case B_L0_L1_16x8: {
    return "B_L0_L1_16x8";
    break;
  }
  case B_L0_L1_8x16: {
    return "B_L0_L1_8x16";
    break;
  }
  case B_L1_L0_16x8: {
    return "B_L1_L0_16x8";
    break;
  }
  case B_L1_L0_8x16: {
    return "B_L1_L0_8x16";
    break;
  }
  case B_L0_Bi_16x8: {
    return "B_L0_Bi_16x8";
    break;
  }
  case B_L0_Bi_8x16: {
    return "B_L0_Bi_8x16";
    break;
  }
  case B_L1_Bi_16x8: {
    return "B_L1_Bi_16x8";
    break;
  }
  case B_L1_Bi_8x16: {
    return "B_L1_Bi_8x16";
    break;
  }
  case B_Bi_L0_16x8: {
    return "B_Bi_L0_16x8";
    break;
  }
  case B_Bi_L0_8x16: {
    return "B_Bi_L0_8x16";
    break;
  }
  case B_Bi_L1_16x8: {
    return "B_Bi_L1_16x8";
    break;
  }
  case B_Bi_L1_8x16: {
    return "B_Bi_L1_8x16";
    break;
  }
  case B_Bi_Bi_16x8: {
    return "B_Bi_Bi_16x8";
    break;
  }
  case B_Bi_Bi_8x16: {
    return "B_Bi_Bi_8x16";
    break;
  }
  case B_8x8: {
    return "B_8x8";
    break;
  }
  case B_Skip: {
    return "B_Skip";
    break;
  }
  case P_L0_8x8: {
    return "P_L0_8x8";
    break;
  }
  case P_L0_8x4: {
    return "P_L0_8x4";
    break;
  }
  case P_L0_4x8: {
    return "P_L0_4x8";
    break;
  }
  case P_L0_4x4: {
    return "P_L0_4x4";
    break;
  }
  case B_Direct_8x8: {
    return "B_Direct_8x8";
    break;
  }
  case B_L0_8x8: {
    return "B_L0_8x8";
    break;
  }
  case B_L1_8x8: {
    return "B_L1_8x8";
    break;
  }
  case B_Bi_8x8: {
    return "B_Bi_8x8";
    break;
  }
  case B_L0_8x4: {
    return "B_L0_8x4";
    break;
  }
  case B_L0_4x8: {
    return "B_L0_4x8";
    break;
  }
  case B_L1_8x4: {
    return "B_L1_8x4";
    break;
  }
  case B_L1_4x8: {
    return "B_L1_4x8";
    break;
  }
  case B_Bi_8x4: {
    return "B_Bi_8x4";
    break;
  }
  case B_Bi_4x8: {
    return "B_Bi_4x8";
    break;
  }
  case B_L0_4x4: {
    return "B_L0_4x4";
    break;
  }
  case B_L1_4x4: {
    return "B_L1_4x4";
    break;
  }
  case B_Bi_4x4: {
    return "B_Bi_4x4";
    break;
  }
  default: {
    return "MB_TYPE_NA";
    break;
  }
  }

  return "MB_TYPE_NA";
}

int MacroBlock::process_mb_type(SliceHeader &header, const int32_t slice_type) {
  int ret = _picture->inverse_macroblock_scanning_process(
      header.MbaffFrameFlag, CurrMbAddr, mb_field_decoding_flag,
      _picture->m_mbs[CurrMbAddr].m_mb_position_x,
      _picture->m_mbs[CurrMbAddr].m_mb_position_y);
  RET(ret);

  if (_is_cabac)
    ret = _cabac->decode_mb_type(mb_type);
  else
    mb_type = _gb->get_ue_golomb(*_bs);
  RET(ret);

  ret = fix_mb_type(slice_type, mb_type, m_slice_type_fixed,
                    m_mb_type_fixed); // 需要立即修正mb_type的值
  RET(ret);

  // 因CABAC会用到MbPartWidth/MbPartHeight信息，所以需要尽可能提前设置相关值
  ret = MbPartPredMode();
  RET(ret);

  ret = MbPartPredMode(m_slice_type_fixed, transform_size_8x8_flag,
                       m_mb_type_fixed, 0, m_NumMbPart, CodedBlockPatternChroma,
                       CodedBlockPatternLuma, Intra16x16PredMode,
                       m_name_of_mb_type, m_mb_pred_mode);
  RET(ret);
  return 0;
}

int MacroBlock::process_sub_mb_type(const int mbPartIdx) {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_sub_mb_type(sub_mb_type[mbPartIdx]);
  else
    sub_mb_type[mbPartIdx] = _gb->get_ue_golomb(*_bs);
  RET(ret);

  /* 2. 根据子宏块类型设置预测模式等信息 */
  if (m_slice_type_fixed == SLICE_P && sub_mb_type[mbPartIdx] >= 0 &&
      sub_mb_type[mbPartIdx] <= 3)
    // 设置 P 帧子宏块信息
    set_current_mb_info(sub_mb_type_P_mbs_define[sub_mb_type[mbPartIdx]],
                        mbPartIdx);
  else if (m_slice_type_fixed == SLICE_B && sub_mb_type[mbPartIdx] >= 0 &&
           sub_mb_type[mbPartIdx] <= 12)
    // 设置 B 帧子宏块信息
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
    ret = MbPartPredMode(m_slice_type_fixed, transform_size_8x8_flag,
                         m_mb_type_fixed, 0, m_NumMbPart,
                         CodedBlockPatternChroma, CodedBlockPatternLuma,
                         Intra16x16PredMode, m_name_of_mb_type, m_mb_pred_mode);
    RET(ret);

    /* 如果当前片段是 I 片段 (SLICE_I)，并且宏块类型为 0（即 m_mb_type_fixed == 0），则根据 transform_size_8x8_flag 的值来设置 mb_type_I_slice */
    if ((m_slice_type_fixed % 5) == SLICE_I && m_mb_type_fixed == 0)
      mb_type_I_slice =
          mb_type_I_slices_define[transform_size_8x8_flag ? 1 : 0];
  }

  return 0;
}

// 编码块模式（Coded Block Pattern, CBP），用于指示哪些块（亮度块和色度块）包含非零的变换系数。CBP的值决定了在解码过程中哪些块需要进行逆变换和反量化。
int MacroBlock::process_coded_block_pattern(const uint32_t ChromaArrayType) {
  int ret = 0;
  if (_is_cabac)
    ret = _cabac->decode_coded_block_pattern(coded_block_pattern);
  else
    coded_block_pattern =
        _gb->get_me_golomb(*_bs, ChromaArrayType, m_mb_pred_mode);

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
    mb_qp_delta = _gb->get_se_golomb(*_bs);
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
    intra_chroma_pred_mode = _gb->get_ue_golomb(*_bs);
  return ret;
}

/* 指定要用于预测的参考图片的参考图片列表0中的索引。   */
int MacroBlock::process_ref_idx_l0(int mbPartIdx,
                                   uint32_t num_ref_idx_l0_active_minus1) {
  int ret = 0;
  const int32_t ref_idx_flag = 0;
  if (_is_cabac)
    ret = _cabac->decode_ref_idx_lX(ref_idx_flag, mbPartIdx,
                                    ref_idx_l0[mbPartIdx]);
  else {
    /* ref_idx_l0[ mbPartIdx ]的范围、参考图片列表 0 中的索引以及用于预测的参考图片内的字段奇偶校验（如果适用）指定如下： 
     * – 如果 MbaffFrameFlag 等于 0 或 mb_field_decoding_flag 为等于 0，ref_idx_l0[ mbPartIdx ] 的值应在 0 到 num_ref_idx_l0_active_minus1 的范围内（包括 0 和 num_ref_idx_l0_active_minus1）。  
     * — 否则（MbaffFrameFlag 等于 1 并且 mb_field_decoding_flag 等于 1），ref_idx_l0[ mbPartIdx ] 的值应在 0 到 2 * num_ref_idx_l0_active_minus1 + 1 的范围内（含）。  当仅使用一张参考图片进行帧间预测时，ref_idx_l0[mbPartIdx]的值应被推断为等于0。 */
    uint32_t size = 0;
    if (MbaffFrameFlag == 0 || mb_field_decoding_flag == 0)
      size = num_ref_idx_l0_active_minus1;
    else
      size = num_ref_idx_l0_active_minus1 * 2;

    ref_idx_l0[mbPartIdx] = _gb->get_te_golomb(*_bs, size);
  }
  return ret;
}

int MacroBlock::process_ref_idx_l1(int mbPartIdx,
                                   uint32_t num_ref_idx_l1_active_minus1) {
  int ret = 0;
  const int32_t ref_idx_flag = 1;
  if (_is_cabac)
    ret = _cabac->decode_ref_idx_lX(ref_idx_flag, mbPartIdx,
                                    ref_idx_l1[mbPartIdx]);
  else {
    uint32_t size = 0;
    if (MbaffFrameFlag == 0 || mb_field_decoding_flag == 0)
      size = num_ref_idx_l1_active_minus1;
    else
      size = num_ref_idx_l1_active_minus1 * 2;

    ref_idx_l1[mbPartIdx] = _gb->get_te_golomb(*_bs, size);
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
    mvd_l0[mbPartIdx][subMbPartIdx][compIdx] = _gb->get_se_golomb(*_bs);
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
    mvd_l1[mbPartIdx][subMbPartIdx][compIdx] = _gb->get_se_golomb(*_bs);
  return ret;
}
