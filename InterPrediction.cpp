#include "Common.hpp"
#include "MacroBlock.hpp"
#include "Nalu.hpp"
#include "PictureBase.hpp"
#include "Type.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>

// 8.4 Inter prediction process
// This process is invoked when decoding P and B macroblock types.
/* 该过程的输出是当前宏块的帧间预测样本，它们是亮度样本的 16x16 数组 predL，并且当 ChromaArrayType 不等于 0 时，是色度样本的两个 (MbWidthC)x(MbHeightC) 数组 predCb 和 predCr，每个数组对应一个色度分量 Cb 和 Cr。*/
int PictureBase::inter_prediction_process() {
  /* ------------------ 设置别名 ------------------ */
  const SliceHeader *header = m_slice->slice_header;
  MacroBlock &mb = m_mbs[CurrMbAddr];
  const int32_t MbPartWidth = mb.MbPartWidth;
  const int32_t MbPartHeight = mb.MbPartHeight;

  const int32_t SubHeightC = header->m_sps->SubHeightC;
  const int32_t SubWidthC = header->m_sps->SubWidthC;
  const uint32_t ChromaArrayType = header->m_sps->ChromaArrayType;
  const uint32_t slice_type = header->slice_type % 5;
  const uint32_t weighted_bipred_idc = header->m_pps->weighted_bipred_idc;
  const bool weighted_pred_flag = header->m_pps->weighted_pred_flag;
  const bool isMbAff = header->MbaffFrameFlag && mb.mb_field_decoding_flag;

  const H264_MB_TYPE &mb_type = mb.m_name_of_mb_type;
  /* ------------------  End ------------------ */

  // 宏块分区数量，指示当前宏块被划分成的分区数量，mb.m_NumMbPart已经在macroblock_mb_skip(对于B、P帧）或macroblock_layer(对于I、SI帧）函数计算过，比如P_Skip和P/B_16x16 -> 分区为1，P/B_8x16,P/B_16x8 -> 分区为2, P/B_8x8 -> 分区为4, 注意，对于帧间预测，并不存在16个分区的I_4x4

  /* B_Skip：通常这类宏块不进行运动估计，而是直接采用邻近宏块的运动矢量或默认为零运动矢量
   * B_Direct_16x16：这类宏块使用直接模式预测，通常基于时间和空间的参考来决定运动矢量
   * P_Skip: 宏块被自动处理为一个完整的单元（16x16像素），其预测模式和运动矢量直接继承自邻近宏块，无需任何进一步的分割或详细处理，所以这里并没有出现P_Skip */
  int32_t NumMbPart =
      (mb_type == B_Skip || mb_type == B_Direct_16x16) ? 4 : mb.m_NumMbPart;

  int32_t NumSubMbPart = 0;
  int32_t SubMbPartWidth = 0, SubMbPartHeight = 0;
  H264_MB_PART_PRED_MODE SubMbPredMode = MB_PRED_MODE_NA;

  //NOTE: 宏块通常是16x16大小，子宏块通常是8x8大小，子宏块也是宏块的8x8分区

  // 遍历每个宏块分区，比如1个16x16，2个16x8/8x16，4个B_Skip/B_Direct_16x16/P/B_8x8
  for (int mbPartIdx = 0; mbPartIdx < NumMbPart; mbPartIdx++) {
    /* 1. 根据"宏块"类型，进行查表并设置"子宏块"的预测模式：比如当前宏块为P_skip，宏块分区数量为1，这里的子宏块类型推导是无效的， 因为P_Skip宏块不包含任何子宏块分区。所有的预测和复制操作都基于整个宏块的单元进行，没有进一步细分为更小的单元或子宏块 */
    // 比如，当前宏块为P_skip时，宏块分区数量为1, 当宏块分区数量为1时，会当作一个子宏块处理，即子宏块分区数量为1, 子宏块大小为8x8
    RET(MacroBlock::SubMbPredMode(header->slice_type, mb.sub_mb_type[mbPartIdx],
                                  NumSubMbPart, SubMbPredMode, SubMbPartWidth,
                                  SubMbPartHeight));

    /* 2. 每个宏块分区或子宏块分区的大小 */
    int32_t partWidth = 0, partHeight = 0;

    // a. 当前分区为子宏块(8x8块)，子宏块可能进一步划分为更小的分区（如 4x4 或 4x8）对于这种情况，需要暂时先将宏块信息设为子宏块的信息，方便进一步划分或直接操作子宏块解码
    if (mb_type == P_8x8 || mb_type == P_8x8ref0 ||
        (mb_type == B_8x8 &&
         mb.m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8))
      //比如1个8x8的子宏块
      partWidth = mb.SubMbPartWidth[mbPartIdx],
      partHeight = mb.SubMbPartHeight[mbPartIdx],
      NumSubMbPart = mb.NumSubMbPart[mbPartIdx];

    // b. 不需要显示的计算运动矢量的分区，将子宏块分区为4个4x4块
    else if (mb_type == B_Skip || mb_type == B_Direct_16x16 ||
             (mb_type == B_8x8 &&
              mb.m_name_of_sub_mb_type[mbPartIdx] == B_Direct_8x8))
      //4个4x4的子宏块分区
      NumSubMbPart = partWidth = partHeight = 4;

    // c. 当前分区无子宏块(无8x8块)，但存在宏块分区，比如一个P/B_16x16,P/B_8x16,P/B_16x8的宏块
    else
      partWidth = MbPartWidth, partHeight = MbPartHeight, NumSubMbPart = 1;

    /* 色度块宽高：YUV420则为8x8,YUV422则为8x16(宽x高) */
    int32_t partWidthC = 0, partHeightC = 0;
    if (ChromaArrayType != 0)
      partWidthC = partWidth / SubWidthC, partHeightC = partHeight / SubHeightC;

    int32_t xP = InverseRasterScan(mbPartIdx, MbPartWidth, MbPartHeight, 16, 0);
    int32_t yP = InverseRasterScan(mbPartIdx, MbPartWidth, MbPartHeight, 16, 1);

    // 二维坐标，亮度运动矢量 mvL0(x,y), mvL1(x,y)，色度运动矢量 mvCL0(x,y), mvCL1(x,y)
    int32_t mvL0[2] = {0}, mvL1[2] = {0}, mvCL0[2] = {0}, mvCL1[2] = {0};
    // 参考帧索引
    int32_t refIdxL0 = -1, refIdxL1 = -1;
    // 预测列表利用标志
    bool predFlagL0 = false, predFlagL1 = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    // 宏块、子宏块分区运动向量总数
    int32_t MvCnt = 0, subMvCnt = 0;
#pragma GCC diagnostic pop

    /* NOTE: 每个宏块分区由 mbPartIdx 引用。每个子宏块分区由 subMbPartIdx 引用：
     * a. 当宏块划分由等于子宏块的分区组成时，每个子宏块可以进一步划分为子宏块分区。即16x16 -> 4个8x8 == 8x8 -> 4个4x4的情况
     * b. 当宏块划分不由子宏块组成时，subMbPartIdx设置为等于0 */

    // 遍历每个子宏块分区，比如遍历4个4x4分区，遍历一个8x8分区，遍历1个16x16,16x8,8x16
    for (int subMbPartIdx = 0; subMbPartIdx < NumSubMbPart; subMbPartIdx++) {
      // 前后方向的参考帧指针
      PictureBase *refPicL0 = nullptr, *refPicL1 = nullptr;

      /* TODO YangJing 为什么会将1个16x8/8x16进行运动预测，而不是分为2个8x8进行? <24-10-10 19:44:28> */

      /* 1. 对每个宏块分区或子宏块分区，推导其运动向量和参考帧索引 */
      RET(derivation_motion_vector_components_and_reference_indices(
          mbPartIdx, subMbPartIdx, refIdxL0, refIdxL1, mvL0, mvL1, mvCL0, mvCL1,
          subMvCnt, predFlagL0, predFlagL1, refPicL0, refPicL1));

      /* 2. 变量MvCnt 增加subMvCnt */
      MvCnt += subMvCnt;

      /* 3. 当P Slice存在加权预测 或 B Slice使用加权双向预测时，计算预测权重因子 */
      //加权预测变量 logWDC(加权因子的对数基数)、w0C(前参考加权因子)、w1C(后参考加权因子)、o0C(前参考偏移量)、o1C(后参考偏移量)，其中 C 被 L 替换
      int32_t logWDL = 0, w0L = 1, w1L = 1, o0L = 0, o1L = 0;
      int32_t logWDCb = 0, w0Cb = 1, w1Cb = 1, o0Cb = 0, o1Cb = 0;
      int32_t logWDCr = 0, w0Cr = 1, w1Cr = 1, o0Cr = 0, o1Cr = 0;
      if ((weighted_pred_flag &&
           (slice_type == SLICE_P || slice_type == SLICE_SP)) ||
          (weighted_bipred_idc > 0 && slice_type == SLICE_B)) {
        RET(derivation_prediction_weights(refIdxL0, refIdxL1, predFlagL0,
                                          predFlagL1, logWDL, w0L, w1L, o0L,
                                          o1L, logWDCb, w0Cb, w1Cb, o0Cb, o1Cb,
                                          logWDCr, w0Cr, w1Cr, o0Cr, o1Cr));
      }

      // 宏块分区、子宏块分区的左上样本相对于宏块的左上样本的位置
      int32_t xS = 0, yS = 0;
      if (mb_type == P_8x8 || mb_type == P_8x8ref0 || mb_type == B_8x8) {
        xS = InverseRasterScan(subMbPartIdx, partWidth, partHeight, 8, 0);
        yS = InverseRasterScan(subMbPartIdx, partWidth, partHeight, 8, 1);
      } else {
        xS = InverseRasterScan(subMbPartIdx, 4, 4, 8, 0);
        yS = InverseRasterScan(subMbPartIdx, 4, 4, 8, 1);
      }

      // 4. 通过运动向量和参考帧计算帧间预测样本，生成最终预测的像素值
      int32_t xAL = mb.m_mb_position_x + xP + xS;
      int32_t yAL = (isMbAff) ? mb.m_mb_position_y / 2 + yP + yS
                              : mb.m_mb_position_y + yP + yS;
      uint8_t predPartL[256] = {0}, predPartCb[256] = {0},
              predPartCr[256] = {0};
      RET(decoding_inter_prediction_samples(
          mbPartIdx, subMbPartIdx, partWidth, partHeight, partWidthC,
          partHeightC, xAL, yAL, mvL0, mvL1, mvCL0, mvCL1, refPicL0, refPicL1,
          predFlagL0, predFlagL1, logWDL, w0L, w1L, o0L, o1L, logWDCb, w0Cb,
          w1Cb, o0Cb, o1Cb, logWDCr, w0Cr, w1Cr, o0Cr, o1Cr, predPartL,
          predPartCb, predPartCr));

      /* 为了在解码过程中稍后调用的变量的导出过程中使用，进行以下分配 */
      mb.m_MvL0[mbPartIdx][subMbPartIdx][0] = mvL0[0];
      mb.m_MvL0[mbPartIdx][subMbPartIdx][1] = mvL0[1];
      mb.m_MvL1[mbPartIdx][subMbPartIdx][0] = mvL1[0];
      mb.m_MvL1[mbPartIdx][subMbPartIdx][1] = mvL1[1];
      mb.m_RefIdxL0[mbPartIdx] = refIdxL0;
      mb.m_RefIdxL1[mbPartIdx] = refIdxL1;
      mb.m_PredFlagL0[mbPartIdx] = predFlagL0;
      mb.m_PredFlagL1[mbPartIdx] = predFlagL1;

      // 5. 通过将宏块或子宏块分区预测样本放置在它们在宏块中的正确相对位置来形成宏块预测
      // 帧宏块,场宏块：对应的偏移
      int32_t luma_offset =
          mb.mb_field_decoding_flag ? ((mb_y % 2) * PicWidthInSamplesL) : 0;
      int32_t chroma_offset =
          mb.mb_field_decoding_flag ? ((mb_y % 2) * PicWidthInSamplesC) : 0;
      int32_t n = mb.mb_field_decoding_flag + 1;

      for (int i = 0; i < partHeight; i++) {
        int32_t y = (mb_y / n * MbHeightL + yP + yS + i) * n;
        for (int j = 0; j < partWidth; j++) {
          int32_t x = mb_x * MbWidthL + xP + xS + j;
          m_pic_buff_luma[luma_offset + y * PicWidthInSamplesL + x] =
              predPartL[i * partWidth + j];
        }
      }

      if (ChromaArrayType != 0) {
        for (int i = 0; i < partHeightC; i++) {
          int32_t y =
              (mb_y / n * MbHeightC + yP / SubHeightC + yS / SubHeightC + i) *
              n;
          for (int j = 0; j < partWidthC; j++) {
            int32_t x = mb_x * MbWidthC + xP / SubWidthC + xS / SubWidthC + j;
            m_pic_buff_cb[chroma_offset + y * PicWidthInSamplesC + x] =
                predPartCb[i * partWidthC + j];
            m_pic_buff_cr[chroma_offset + y * PicWidthInSamplesC + x] =
                predPartCr[i * partWidthC + j];
          }
        }
      }

      // 完成该宏块解码
      mb.m_isDecoded[mbPartIdx][subMbPartIdx] = true;
    }
  }

  return 0;
}

// 8.4.2 Decoding process for Inter prediction samples
/* 输入： 
   * – 宏块分区 mbPartIdx，子宏块分区 subMbPartIdx， 
   * – 指定亮度和色度分区宽度和高度的变量partWidth、partHeight、partWidthC和 partHeightC， 
   * – 亮度运动向量 mvL0 和 mvL1，色度运动向量 mvCL0 和 mvCL1，
   * – 参考索引 refIdxL0 和 refIdxL1， 
   * – 预测列表利用标志 predFlagL0 和 predFlagL1， 
   * – 加权预测变量 logWDC，w0C， w1C、o0C、o1C，其中 C 被 L 替换，Cb 和 Cr,
 * 输出：帧间预测样本predPart，它们是预测亮度样本的(partWidth x partHeight)数组predPartL，预测的两个(partWidthC x partHeightC)数组predPartCb、predPartCr色度样本。 */
int PictureBase::decoding_inter_prediction_samples(
    int32_t mbPartIdx, int32_t subMbPartIdx, int32_t partWidth,
    int32_t partHeight, int32_t partWidthC, int32_t partHeightC, int32_t xAL,
    int32_t yAL, int32_t (&mvL0)[2], int32_t (&mvL1)[2], int32_t (&mvCL0)[2],
    int32_t (&mvCL1)[2], PictureBase *refPicL0, PictureBase *refPicL1,
    bool predFlagL0, bool predFlagL1, int32_t logWDL, int32_t w0L, int32_t w1L,
    int32_t o0L, int32_t o1L, int32_t logWDCb, int32_t w0Cb, int32_t w1Cb,
    int32_t o0Cb, int32_t o1Cb, int32_t logWDCr, int32_t w0Cr, int32_t w1Cr,
    int32_t o0Cr, int32_t o1Cr,
    // Output:
    uint8_t *predPartL, uint8_t *predPartCb, uint8_t *predPartCr) {

  // 预测亮、色度样本值：按最大为16x16 = 256申请, 最小为4x4 = 16
  uint8_t predPartL0L[256] = {0}, predPartL1L[256] = {0};
  uint8_t predPartL0Cb[256] = {0}, predPartL1Cb[256] = {0},
          predPartL0Cr[256] = {0}, predPartL1Cr[256] = {0};

  // 亚像素插值：从前参考帧 refPicL0 以及运动矢量预测样本
  if (predFlagL0) {
    RET(fractional_sample_interpolation(
        mbPartIdx, subMbPartIdx, partWidth, partHeight, partWidthC, partHeightC,
        xAL, yAL, mvL0, mvCL0, refPicL0, predPartL0L, predPartL0Cb,
        predPartL0Cr));
  }

  // 亚像素插值：从后参考帧 refPicL1 以及运动矢量预测样本
  if (predFlagL1) {
    RET(fractional_sample_interpolation(
        mbPartIdx, subMbPartIdx, partWidth, partHeight, partWidthC, partHeightC,
        xAL, yAL, mvL1, mvCL1, refPicL1, predPartL1L, predPartL1Cb,
        predPartL1Cr));
  }

  // 根据前参考、后参考的预测样本，以及加权预测参数，计算最终的预测样本
  RET(weighted_sample_prediction(
      mbPartIdx, subMbPartIdx, predFlagL0, predFlagL1, partWidth, partHeight,
      partWidthC, partHeightC, logWDL, w0L, w1L, o0L, o1L, logWDCb, w0Cb, w1Cb,
      o0Cb, o1Cb, logWDCr, w0Cr, w1Cr, o0Cr, o1Cr, predPartL0L, predPartL0Cb,
      predPartL0Cr, predPartL1L, predPartL1Cb, predPartL1Cr,
      /* Output: */
      predPartL, predPartCb, predPartCr));

  return 0;
}

// 8.4.1 Derivation process for motion vector components and reference indices(运动矢量分量和参考索引的推导过程)
/* 该过程的输入是： 
   * – 宏块分区 mbPartIdx， 
   * – 子宏块分区 subMbPartIdx。  
 * 该过程的输出为： 
   * – 亮度运动矢量 mvL0 和 mvL1，色度运动矢量 mvCL0 和 mvCL1 
   * – 参考索引 refIdxL0 和 refIdxL1 
   * – 预测列表利用标志 predFlagL0 和 predFlagL1 
   * – 子宏块分区运动向量计数 subMvCnt */
int PictureBase::derivation_motion_vector_components_and_reference_indices(
    int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
    int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
    int32_t (&mvCL0)[2], int32_t (&mvCL1)[2], int32_t &subMvCnt,
    bool &predFlagL0, bool &predFlagL1, PictureBase *&refPicL0,
    PictureBase *&refPicL1) {

  /* ------------------ 设置别名 ------------------ */
  const SliceHeader *header = m_slice->slice_header;
  MacroBlock &mb = m_mbs[CurrMbAddr];
  const H264_MB_TYPE &mb_type = mb.m_name_of_mb_type;
  const H264_MB_TYPE &sub_mb_type = mb.m_name_of_sub_mb_type[mbPartIdx];
  const uint32_t ChromaArrayType = header->m_sps->ChromaArrayType;
  /* ------------------  End ------------------ */

  /* ---------------------------- P_Skip 宏块的亮度运动矢量 ---------------------------- */
  if (mb_type == P_Skip) {
    RET(derivation_luma_motion_vectors_for_P_Skip(
        refIdxL0, refIdxL1, mvL0, mvL1, subMvCnt, predFlagL0, predFlagL1));
  }
  /* -------------------- B_Skip,B_Direct_16x16/8x8 宏块的亮度运动矢量 -------------------- */
  else if (mb_type == B_Skip || mb_type == B_Direct_16x16 ||
           sub_mb_type == B_Direct_8x8) {
    RET(derivation_luma_motion_vectors_for_B_Skip_or_Direct_16x16_8x8(
        mbPartIdx, subMbPartIdx, refIdxL0, refIdxL1, mvL0, mvL1, subMvCnt,
        predFlagL0, predFlagL1));
  }
  /* ---------------------------- 宏块运动预测 ---------------------------- */
  else {
    /* 获取当前宏块、子宏块预测模式 */
    int32_t NumSubMbPart = 0, SubMbPartWidth = 0, SubMbPartHeight = 0;
    H264_MB_PART_PRED_MODE mb_pred_mode = MB_PRED_MODE_NA,
                           SubMbPredMode = MB_PRED_MODE_NA;
    RET(MacroBlock::MbPartPredMode(mb_type, mbPartIdx,
                                   mb.transform_size_8x8_flag, mb_pred_mode));
    RET(MacroBlock::SubMbPredMode(header->slice_type, mb.sub_mb_type[mbPartIdx],
                                  NumSubMbPart, SubMbPredMode, SubMbPartWidth,
                                  SubMbPartHeight));

    refIdxL0 = -1, predFlagL0 = false, refIdxL1 = -1, predFlagL1 = false;

    // 当前预测模式为：向前帧间预测或双向帧间预测，则获取编码器提供的对应参考帧索引
    if (mb_pred_mode == Pred_L0 || mb_pred_mode == BiPred ||
        SubMbPredMode == Pred_L0 || SubMbPredMode == BiPred)
      refIdxL0 = mb.ref_idx_l0[mbPartIdx], predFlagL0 = true;

    // 当前预测模式为：向后帧间预测或双向帧间预测，则获取编码器提供的对应参考帧索引
    if (mb_pred_mode == Pred_L1 || mb_pred_mode == BiPred ||
        SubMbPredMode == Pred_L1 || SubMbPredMode == BiPred)
      refIdxL1 = mb.ref_idx_l1[mbPartIdx], predFlagL1 = true;

    subMvCnt = predFlagL0 + predFlagL1;

    //对于B_8x8类型，在子宏块中可能的类型有：B_Direct_8x8, B_L0/L1/Bi_8x8，所以需要通过sub_mb_type进确定。比如说：predPartWidth = (currSubMbType == B_Direct_8x8) ? 16 : SubMbPartWidth
    H264_MB_TYPE currSubMbType = (mb_type == B_8x8) ? sub_mb_type : MB_TYPE_NA;
    int32_t mvpL0[2] = {0}, mvpL1[2] = {0};

    /* 亮度宏块运动预测 */
    if (predFlagL0) {
      // 1. 负责选择参考帧
      RET(reference_picture_selection(refIdxL0, m_RefPicList0,
                                      m_RefPicList0Length, refPicL0));
      // 2. 计算运动向量
      RET(derivation_luma_motion_vector_prediction(
          mbPartIdx, subMbPartIdx, currSubMbType, false, refIdxL0, mvpL0));
      // 3. 运动矢量 = 预测的运动矢量 + 运动矢量差
      mvL0[0] = mvpL0[0] + mb.mvd_l0[mbPartIdx][subMbPartIdx][0];
      mvL0[1] = mvpL0[1] + mb.mvd_l0[mbPartIdx][subMbPartIdx][1];
    }

    if (predFlagL1) {
      RET(reference_picture_selection(refIdxL1, m_RefPicList1,
                                      m_RefPicList1Length, refPicL1));
      RET(derivation_luma_motion_vector_prediction(
          mbPartIdx, subMbPartIdx, currSubMbType, true, refIdxL1, mvpL1));
      mvL1[0] = mvpL1[0] + mb.mvd_l1[mbPartIdx][subMbPartIdx][0];
      mvL1[1] = mvpL1[1] + mb.mvd_l1[mbPartIdx][subMbPartIdx][1];
    }
  }

  /* 色度宏块运动预测 */
  if (ChromaArrayType != 0) {
    if (predFlagL0) {
      RET(reference_picture_selection(refIdxL0, m_RefPicList0,
                                      m_RefPicList0Length, refPicL0));
      RET(derivation_chroma_motion_vectors(ChromaArrayType, mvL0, refPicL0,
                                           mvCL0));
    }
    if (predFlagL1) {
      RET(reference_picture_selection(refIdxL1, m_RefPicList1,
                                      m_RefPicList1Length, refPicL1));
      RET(derivation_chroma_motion_vectors(ChromaArrayType, mvL1, refPicL1,
                                           mvCL1));
    }
  }

  return 0;
}

// 8.4.1.4 Derivation process for chroma motion vectors
int PictureBase::derivation_chroma_motion_vectors(int32_t ChromaArrayType,
                                                  int32_t mvLX[2],
                                                  PictureBase *refPic,
                                                  int32_t (&mvCLX)[2]) {

  // YUV444,YUV422 或者 YUV420的帧宏块：色度分量的分辨率与亮度分量相同，直接使用亮度块的运动矢量
  if (ChromaArrayType != 1 || m_mbs[CurrMbAddr].mb_field_decoding_flag == 0)
    mvCLX[0] = mvLX[0], mvCLX[1] = mvLX[1];
  // YUV420的场宏块
  else {
    // Table 8-10 – Derivation of the vertical component of the chroma vector in field coding mode
    // 水平方向的运动矢量不受场编码的影响，所以可以直接使用
    mvCLX[0] = mvLX[0];
    // 顶场和底场之间存在 1 行像素的垂直偏移，因此需要对垂直方向的运动矢量进行修正
    if (refPic && refPic->m_picture_coded_type == TOP_FIELD &&
        (m_picture_coded_type == BOTTOM_FIELD || mb_y % 2))
      mvCLX[1] = mvLX[1] + 2;
    else if (refPic && refPic->m_picture_coded_type == BOTTOM_FIELD &&
             (m_picture_coded_type == TOP_FIELD || mb_y % 2 == 0))
      mvCLX[1] = mvLX[1] - 2;
    else
      mvCLX[1] = mvLX[1];
  }

  return 0;
}

// 8.4.1.1 Derivation process for luma motion vectors for skipped macroblocks in P and SP slices
int PictureBase::derivation_luma_motion_vectors_for_P_Skip(
    int32_t &refIdxL0, int32_t &refIdxL1, int32_t (&mvL0)[2],
    int32_t (&mvL1)[2], int32_t &subMvCnt, bool &predFlagL0, bool &predFlagL1) {

  /* NOTE:由于P_Skip宏块的预测值等于实际运动矢量，因此输出直接分配给 mvL0,mvL1 */

  /* P宏块不存在后参考预测 */
  predFlagL0 = true, predFlagL1 = false;
  mvL1[0] = NA, mvL1[1] = NA;
  //P_Skip宏块只有一个运动矢量
  subMvCnt = 1;

  //使用第一个参考帧
  refIdxL0 = 0;
  fill_n(m_mbs[CurrMbAddr].m_PredFlagL0, 4, 1);

  //左边和上边相邻宏块的地址
  int32_t mbAddrN_A = 0, mbAddrN_B = 0;
  //左边和上边相邻宏块的L0运动矢量
  int32_t mvL0_A[2] = {0}, mvL0_B[2] = {0};
  //左边和上边相邻宏块的参考帧索引
  int32_t refIdxL0_A = 0, refIdxL0_B = 0;

  /* 空引用对象 */
  int32_t nullref, nullref2[2];

  // MVP 是基于相邻宏块的运动矢量，来预测当前宏块的运动矢量，P_Skip 宏块通常使用最近的参考帧进行预测
  RET(derivation_motion_data_of_neighbouring_partitions(
      0, 0, MB_TYPE_NA, false, mbAddrN_A, mvL0_A, refIdxL0_A, mbAddrN_B, mvL0_B,
      refIdxL0_B, nullref, nullref2, nullref));

  /* 相邻宏块不可用，对于 P_Skip 宏块，运动矢量通常被假定为零 */
  if (mbAddrN_A < 0 || mbAddrN_B < 0)
    mvL0[0] = 0, mvL0[1] = 0;
  else if ((refIdxL0_A == 0 && mvL0_A[0] == 0 && mvL0_A[1] == 0) ||
           (refIdxL0_B == 0 && mvL0_B[0] == 0 && mvL0_B[1] == 0))
    mvL0[0] = 0, mvL0[1] = 0;
  /* 亮度运动矢量预测 */
  else
    RET(derivation_luma_motion_vector_prediction(0, 0, MB_TYPE_NA, false,
                                                 refIdxL0, mvL0));
  return 0;
}

// 8.4.1.2 Derivation process for luma motion vectors for B_Skip, B_Direct_16x16, and B_Direct_8x8
int PictureBase::derivation_luma_motion_vectors_for_B_Skip_or_Direct_16x16_8x8(
    int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
    int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
    int32_t &subMvCnt, bool &predFlagL0, bool &predFlagL1) {
  /* NOTE:空间和时间直接预测模式均使用同位运动向量和参考索引 */

  //空间直接运动矢量预测
  if (m_slice->slice_header->direct_spatial_mv_pred_flag)
    return derivation_spatial_direct_luma_motion_vector_and_ref_index_prediction(
        mbPartIdx, subMbPartIdx, refIdxL0, refIdxL1, mvL0, mvL1, subMvCnt,
        predFlagL0, predFlagL1);

  //时间直接运动预测模式
  else
    return derivation_temporal_direct_luma_motion_vector_and_ref_index_prediction(
        mbPartIdx, subMbPartIdx, refIdxL0, refIdxL1, mvL0, mvL1, subMvCnt,
        predFlagL0, predFlagL1);

  return 0;
}

// 8.4.1.3 Derivation process for luma motion vector prediction
int PictureBase::derivation_luma_motion_vector_prediction(
    int32_t mbPartIdx, int32_t subMbPartIdx, H264_MB_TYPE currSubMbType,
    int32_t listSuffixFlag, int32_t refIdxLX, int32_t (&mvpLX)[2]) {

  const int32_t MbPartWidth = m_mbs[CurrMbAddr].MbPartWidth;
  const int32_t MbPartHeight = m_mbs[CurrMbAddr].MbPartHeight;

  // 相邻的三个宏块的地址（左A、上B、右上C）运动矢量、对应的参考帧索引
  int32_t mbAddrN_A = 0, mbAddrN_B = 0, mbAddrN_C = 0;
  int32_t mvLXN_A[2] = {0}, mvLXN_B[2] = {0}, mvLXN_C[2] = {0};
  int32_t refIdxLXN_A = 0, refIdxLXN_B = 0, refIdxLXN_C = 0;

  // 根据相邻分区运动矢量进行推导当前运动矢量
  RET(derivation_motion_data_of_neighbouring_partitions(
      mbPartIdx, subMbPartIdx, currSubMbType, listSuffixFlag, mbAddrN_A,
      mvLXN_A, refIdxLXN_A, mbAddrN_B, mvLXN_B, refIdxLXN_B, mbAddrN_C, mvLXN_C,
      refIdxLXN_C));

  // 1. 当前宏块被水平分为两个 16x8 的分区，当前处理的是上半分区，与上宏块参考帧索引一致
  if (MbPartWidth == 16 && MbPartHeight == 8 && mbPartIdx == 0 &&
      refIdxLXN_B == refIdxLX)
    mvpLX[0] = mvLXN_B[0], mvpLX[1] = mvLXN_B[1];
  // 2. 当前宏块被水平分为两个 16x8 的分区，当前处理的是下半分区，与左宏块参考帧索引一致
  else if (MbPartWidth == 16 && MbPartHeight == 8 && mbPartIdx == 1 &&
           refIdxLXN_A == refIdxLX)
    mvpLX[0] = mvLXN_A[0], mvpLX[1] = mvLXN_A[1];
  // 3. 当前宏块被垂直分为两个 8x16 的分区，当前处理的是左半分区，与左宏块参考帧索引一致
  else if (MbPartWidth == 8 && MbPartHeight == 16 && mbPartIdx == 0 &&
           refIdxLXN_A == refIdxLX)
    mvpLX[0] = mvLXN_A[0], mvpLX[1] = mvLXN_A[1];
  // 4. 当前宏块被垂直分为两个 8x16 的分区，当前处理的是右半分区，与右上宏块参考帧索引一致
  else if (MbPartWidth == 8 && MbPartHeight == 16 && mbPartIdx == 1 &&
           refIdxLXN_C == refIdxLX)
    mvpLX[0] = mvLXN_C[0], mvpLX[1] = mvLXN_C[1];
  // 5. 中值预测
  else
    derivation_median_luma_motion_vector_prediction(
        mbAddrN_A, mvLXN_A, refIdxLXN_A, mbAddrN_B, mvLXN_B, refIdxLXN_B,
        mbAddrN_C, mvLXN_C, refIdxLXN_C, refIdxLX, mvpLX);

  return 0;
}

// 8.4.1.3.1 Derivation process for median luma motion vector prediction
int PictureBase::derivation_median_luma_motion_vector_prediction(
    int32_t &mbAddrN_A, int32_t (&mvLXN_A)[2], int32_t &refIdxLXN_A,
    int32_t &mbAddrN_B, int32_t (&mvLXN_B)[2], int32_t &refIdxLXN_B,
    int32_t &mbAddrN_C, int32_t (&mvLXN_C)[2], int32_t &refIdxLXN_C,
    int32_t refIdxLX, int32_t (&mvpLX)[2]) {
  // 在图像的首行宏块时，即相邻宏块 上宏块 和 右上宏块 不存在，但左宏块 A 存在，则将左宏块的运动矢量和参考帧索引复制给相邻宏块。为了确保后续的中值预测有足够的运动矢量数据可用
  if (mbAddrN_B < 0 && mbAddrN_C < 0 && mbAddrN_A >= 0) {
    mvLXN_B[0] = mvLXN_A[0];
    mvLXN_B[1] = mvLXN_A[1];
    mvLXN_C[0] = mvLXN_A[0];
    mvLXN_C[1] = mvLXN_A[1];
    refIdxLXN_B = refIdxLXN_A;
    refIdxLXN_C = refIdxLXN_A;
  }

  // 处理相邻宏块的参考帧索引与当前宏块的参考帧索引的单独匹配
  if (refIdxLXN_A == refIdxLX && refIdxLXN_B != refIdxLX &&
      refIdxLXN_C != refIdxLX)
    mvpLX[0] = mvLXN_A[0], mvpLX[1] = mvLXN_A[1];
  else if (refIdxLXN_A != refIdxLX && refIdxLXN_B == refIdxLX &&
           refIdxLXN_C != refIdxLX)
    mvpLX[0] = mvLXN_B[0], mvpLX[1] = mvLXN_B[1];
  else if (refIdxLXN_A != refIdxLX && refIdxLXN_B != refIdxLX &&
           refIdxLXN_C == refIdxLX)
    mvpLX[0] = mvLXN_C[0], mvpLX[1] = mvLXN_C[1];
  // 中值预测：取相邻宏块 A、B、C 的运动矢量的中值作为当前宏块的运动矢量预测值
  else {
    int32_t min_0 = MIN(mvLXN_A[0], MIN(mvLXN_B[0], mvLXN_C[0]));
    int32_t max_0 = MAX(mvLXN_A[0], MAX(mvLXN_B[0], mvLXN_C[0]));

    int32_t min_1 = MIN(mvLXN_A[1], MIN(mvLXN_B[1], mvLXN_C[1]));
    int32_t max_1 = MAX(mvLXN_A[1], MAX(mvLXN_B[1], mvLXN_C[1]));

    mvpLX[0] = mvLXN_A[0] + mvLXN_B[0] + mvLXN_C[0] - min_0 - max_0;
    mvpLX[1] = mvLXN_A[1] + mvLXN_B[1] + mvLXN_C[1] - min_1 - max_1;
  }
  return 0;
}

//Table 8-7 – Specification of PicCodingStruct( X )
#define FLD 0
#define FRM 1
#define AFRM 2

// 8.4.1.2.1 Derivation process for the co-located 4x4 sub-macroblock partitions
// 共置 4x4 子宏块分区的推导过程
int PictureBase::derivation_the_coLocated_4x4_sub_macroblock_partitions(
    int32_t mbPartIdx, int32_t subMbPartIdx, PictureBase *&colPic,
    int32_t &mbAddrCol, int32_t (&mvCol)[2], int32_t &refIdxCol,
    int32_t &vertMvScale, bool useRefPicList1) {

  const SliceHeader *header = m_slice->slice_header;
  Frame *refPic = (useRefPicList1) ? m_RefPicList1[0] : m_RefPicList0[0];
  const H264_PICTURE_CODED_TYPE &ref_marked_type =
      refPic->m_pic_coded_type_marked_as_refrence;

  // 参考帧是帧图像或互补场对，则获取其顶场和底场，并计算当前帧与顶场和底场的POC差值
  PictureBase *firstRefPicL1Top = NULL, *firstRefPicL1Bottom = NULL;
  int32_t topAbsDiffPOC = 0, bottomAbsDiffPOC = 0;
  if (ref_marked_type == FRAME || ref_marked_type == COMPLEMENTARY_FIELD_PAIR) {
    firstRefPicL1Top = &refPic->m_picture_top_filed;
    firstRefPicL1Bottom = &refPic->m_picture_bottom_filed;

    topAbsDiffPOC = ABS(DiffPicOrderCnt(firstRefPicL1Top, this));
    bottomAbsDiffPOC = ABS(DiffPicOrderCnt(firstRefPicL1Bottom, this));
  }

  // Table 8-6 – Specification of the variable colPic
  // 共置宏块的图像，若当前图像是编码帧、当前宏块是帧宏块且互补字段被标记为“用于长期参考”时，互补字段对的图像顺序计数值对解码过程有影响。标记为“用于长期参考”的对是参考列表 1 中的第一张图片
  colPic = nullptr;
  // 如果当前帧是场图像，则根据参考帧的类型，选择顶场或底场作为共位宏块的参考帧
  if (header->field_pic_flag) {
    if (refPic->m_is_decode_finished &&
        (ref_marked_type == TOP_FIELD || ref_marked_type == BOTTOM_FIELD))
      colPic = &refPic->m_picture_frame;
    else {
      if (ref_marked_type == TOP_FIELD)
        colPic = &refPic->m_picture_top_filed;
      else if (ref_marked_type == BOTTOM_FIELD)
        colPic = &refPic->m_picture_bottom_filed;
    }
  }
  // 如果当前帧是帧图像，则根据较小的POC差值选择顶场或底场，或者直接使用帧图像作为共位宏块的参考帧
  else {
    if (refPic->m_is_decode_finished && ref_marked_type == FRAME)
      colPic = &refPic->m_picture_frame;
    else if (ref_marked_type == COMPLEMENTARY_FIELD_PAIR) {
      if (m_slice->slice_data->mb_field_decoding_flag == 0)
        colPic = (topAbsDiffPOC < bottomAbsDiffPOC) ? firstRefPicL1Top
                                                    : firstRefPicL1Bottom;
      else
        colPic = (CurrMbAddr & 1) ? firstRefPicL1Bottom : firstRefPicL1Top;
    }
  }

  // 异常处理
  RET(colPic == nullptr);

  // Table 8-7 – Specification of PicCodingStruct( X )
  // 当前帧、共位宏块所在参考帧的编码结构
  int32_t PicCodingStruct_CurrPic = FLD, PicCodingStruct_colPic = FLD;
#define info(obj, field_pic_flag, mb_adaptive_frame_field_flag)                \
  if ((field_pic_flag))                                                        \
    (obj) = FLD;                                                               \
  else {                                                                       \
    if ((mb_adaptive_frame_field_flag))                                        \
      (obj) = AFRM;                                                            \
    else                                                                       \
      (obj) = FRM;                                                             \
  }

  info(PicCodingStruct_CurrPic, header->field_pic_flag,
       header->m_sps->mb_adaptive_frame_field_flag);
  info(PicCodingStruct_colPic, colPic->m_slice->slice_header->field_pic_flag,
       colPic->m_slice->slice_header->m_sps->mb_adaptive_frame_field_flag);

#undef info

  // 异常处理：当前帧,共位图像帧的编码类型不能是一个自适应帧场以及帧类型，这些图像编码类型必须由 IDR 图像分隔
  RET((PicCodingStruct_CurrPic == FRM && PicCodingStruct_colPic == AFRM) ||
      (PicCodingStruct_CurrPic == AFRM && PicCodingStruct_colPic == FRM));

  // 共位宏块的4x4亮度块索引，以4x4为单位
  // a. 直接使用 8x8 分区进行预测，每4个4x4为一个index
  int32_t luma4x4BlkIdx = 5 * mbPartIdx;
  // b. 表示需要进一步划分 8x8 分区为 4x4 子块：当前 8x8 分区的起始 4x4 子块索引 + 当前 8x8 分区中的第几个 4x4 子块
  if (header->m_sps->direct_8x8_inference_flag == 0)
    luma4x4BlkIdx = (4 * mbPartIdx + subMbPartIdx);

  int32_t xCol = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
                 InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
  int32_t yCol = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
                 InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

  int32_t yM = 0;
  mbAddrCol = CurrMbAddr;
  vertMvScale = H264_VERT_MV_SCALE_UNKNOWN;

  // Table 8-8 – Specification of mbAddrCol, yM, and vertMvScale
  if (PicCodingStruct_CurrPic == FLD) {
    // 1. 当前帧和共位宏块都是场图像，则共位宏块的地址与当前宏块相同，且垂直运动矢量不需要缩放
    if (PicCodingStruct_colPic == FLD)
      mbAddrCol = CurrMbAddr, yM = yCol,
      vertMvScale = H264_VERT_MV_SCALE_One_To_One;
    // 2. 当前帧是场图像而共位宏块是帧图像，则需要根据宏块的垂直位置调整共位宏块的地址，并将垂直运动矢量减半
    else if (PicCodingStruct_colPic == FRM)
      mbAddrCol = 2 * PicWidthInMbs * (CurrMbAddr / PicWidthInMbs) +
                  (CurrMbAddr % PicWidthInMbs) + PicWidthInMbs * (yCol / 8),
      yM = (2 * yCol) % 16, vertMvScale = H264_VERT_MV_SCALE_Frm_To_Fld;
    // 3. 当前帧是场图像而共位宏块是自适应，
    else if (PicCodingStruct_colPic == AFRM) {
      if (colPic->m_mbs[2 * CurrMbAddr].mb_field_decoding_flag == 0)
        mbAddrCol = 2 * CurrMbAddr + this->m_mbs[CurrMbAddr].bottom_field_flag,
        yM = yCol, vertMvScale = H264_VERT_MV_SCALE_One_To_One;
      else
        mbAddrCol = 2 * CurrMbAddr + (yCol / 8), yM = (2 * yCol) % 16,
        vertMvScale = H264_VERT_MV_SCALE_Frm_To_Fld;
    }
  }

  else if (PicCodingStruct_CurrPic == FRM) {
    // 4. 当前帧是帧图像而共位宏块是场图像，
    if (PicCodingStruct_colPic == FLD)
      mbAddrCol = PicWidthInMbs * (CurrMbAddr / (2 * PicWidthInMbs)) +
                  (CurrMbAddr % PicWidthInMbs),
      yM = 8 * ((CurrMbAddr / PicWidthInMbs) % 2) + 4 * (yCol / 8),
      vertMvScale = H264_VERT_MV_SCALE_Fld_To_Frm;
    // 6. 当前帧是帧图像而共位宏块是帧图像，
    else if (PicCodingStruct_colPic == FRM)
      mbAddrCol = CurrMbAddr, yM = yCol,
      vertMvScale = H264_VERT_MV_SCALE_One_To_One;
  }

  else if (PicCodingStruct_CurrPic == AFRM) {
    // 7. 当前帧是帧、场自适应图像而共位宏块是场图像，
    if (PicCodingStruct_colPic == FLD) {
      mbAddrCol = CurrMbAddr / 2;
      if (m_slice->slice_data->mb_field_decoding_flag == 0)
        yM = 8 * (CurrMbAddr % 2) + 4 * (yCol / 8),
        vertMvScale = H264_VERT_MV_SCALE_Fld_To_Frm;
      else
        yM = yCol, vertMvScale = H264_VERT_MV_SCALE_One_To_One;
    }
    // 8. 当前帧和共位宏块都是帧、场自适应图像
    else if (PicCodingStruct_colPic == AFRM) {
      if (m_slice->slice_data->mb_field_decoding_flag == 0) {
        if (colPic->m_mbs[CurrMbAddr].mb_field_decoding_flag == 0)
          mbAddrCol = CurrMbAddr, yM = yCol,
          vertMvScale = H264_VERT_MV_SCALE_One_To_One;
        else
          mbAddrCol = 2 * (CurrMbAddr / 2) +
                      ((topAbsDiffPOC < bottomAbsDiffPOC) ? 0 : 1);
        yM = 8 * (CurrMbAddr % 2) + 4 * (yCol / 8),
        vertMvScale = H264_VERT_MV_SCALE_Fld_To_Frm;
      } else {
        if (colPic->m_mbs[CurrMbAddr].mb_field_decoding_flag == 0)
          mbAddrCol = 2 * (CurrMbAddr / 2) + (yCol / 8), yM = (2 * yCol) % 16,
          vertMvScale = H264_VERT_MV_SCALE_Frm_To_Fld;
        else
          mbAddrCol = CurrMbAddr, yM = yCol,
          vertMvScale = H264_VERT_MV_SCALE_One_To_One;
      }
    }
  }

  // 别名
  const MacroBlock &colMb = colPic->m_mbs[mbAddrCol];

  // 获取共位宏块,子宏块的类型，并推导共位宏块的宏块分区索引和子宏块分区索引
  int32_t mbPartIdxCol = 0, subMbPartIdxCol = 0;
  H264_MB_TYPE mbTypeCol = colMb.m_name_of_mb_type;
  H264_MB_TYPE subMbTypeCol[4] = {MB_TYPE_NA, MB_TYPE_NA, MB_TYPE_NA,
                                  MB_TYPE_NA};
  if (mbTypeCol == P_8x8 || mbTypeCol == P_8x8ref0 || mbTypeCol == B_8x8) {
    // 令subMbTypeCol为图片colPic内地址为mbAddrCol的宏块的语法元素列表sub_mb_type
    subMbTypeCol[0] = colMb.m_name_of_sub_mb_type[0];
    subMbTypeCol[1] = colMb.m_name_of_sub_mb_type[1];
    subMbTypeCol[2] = colMb.m_name_of_sub_mb_type[2];
    subMbTypeCol[3] = colMb.m_name_of_sub_mb_type[3];
  }
  RET(derivation_macroblock_and_sub_macroblock_partition_indices(
      mbTypeCol, subMbTypeCol, xCol, yM, mbPartIdxCol, subMbPartIdxCol));

  refIdxCol = -1;
  // 共位宏块以帧内宏块预测模式编码，则mvCol的两个分量被设置为等于0并且refIdxCol被设置为等于-1
  if (IS_INTRA_Prediction_Mode(colMb.m_mb_pred_mode))
    mvCol[0] = 0, mvCol[1] = 0, refIdxCol = -1;
  // 反之，共位宏块以帧间宏块预测模式编码，则进行复制共位宏块的运动矢量、参考帧索引
  else {
    if (colMb.m_PredFlagL0[mbPartIdxCol]) {
      mvCol[0] = colMb.m_MvL0[mbPartIdxCol][subMbPartIdxCol][0];
      mvCol[1] = colMb.m_MvL0[mbPartIdxCol][subMbPartIdxCol][1];
      refIdxCol = colMb.m_RefIdxL0[mbPartIdxCol];
    } else {
      mvCol[0] = colMb.m_MvL1[mbPartIdxCol][subMbPartIdxCol][0];
      mvCol[1] = colMb.m_MvL1[mbPartIdxCol][subMbPartIdxCol][1];
      refIdxCol = colMb.m_RefIdxL1[mbPartIdxCol];
    }
  }

  return 0;
}

#undef FLD
#undef FRM
#undef AFRM

// 8.4.1.2.2 Derivation process for spatial direct luma motion vector and reference index prediction mode
//空间直接运动矢量预测
//NOTE: 运动矢量 mvLX 对于调用该过程的宏块的所有 4x4 子宏块分区是相同的，所以只需要传入宏块的首个4x4分区，进行推导
int PictureBase::
    derivation_spatial_direct_luma_motion_vector_and_ref_index_prediction(
        int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
        int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
        int32_t &subMvCnt, bool &predFlagL0, bool &predFlagL1) {

  const H264_MB_TYPE &currSubMbType =
      m_mbs[CurrMbAddr].m_name_of_sub_mb_type[mbPartIdx];

  int32_t mbAddrA = 0, mbAddrB = 0, mbAddrC = 0;
  int32_t mvL0A[2] = {0}, mvL0B[2] = {0}, mvL0C[2] = {0}, mvL1A[2] = {0},
          mvL1B[2] = {0}, mvL1C[2] = {0};
  int32_t refIdxL0A = 0, refIdxL0B = 0, refIdxL0C = 0, refIdxL1A = 0,
          refIdxL1B = 0, refIdxL1C = 0;
  //获取相邻宏块(左边（A）和上边（B）右上方（C））的L0运动信息mvL0
  RET(derivation_motion_data_of_neighbouring_partitions(
      0, 0, currSubMbType, false, mbAddrA, mvL0A, refIdxL0A, mbAddrB, mvL0B,
      refIdxL0B, mbAddrC, mvL0C, refIdxL0C));
  //获取相邻宏块(左边（A）和上边（B）右上方（C））的L1运动信息mvL1
  RET(derivation_motion_data_of_neighbouring_partitions(
      0, 0, currSubMbType, true, mbAddrA, mvL1A, refIdxL1A, mbAddrB, mvL1B,
      refIdxL1B, mbAddrC, mvL1C, refIdxL1C));

// define for (8-187)
#define MinPositive(x, y)                                                      \
  ((x) >= 0 && (y) >= 0) ? (MIN((x), (y))) : (MAX((x), (y)))

  // 确定有效的（非负的）最小参考索引。如果所有的参考索引都是负的，则选取最大的那个
  refIdxL0 = MinPositive(refIdxL0A, MinPositive(refIdxL0B, refIdxL0C));
  refIdxL1 = MinPositive(refIdxL1A, MinPositive(refIdxL1B, refIdxL1C));

#undef MinPositive

  // 直接零预测模式，这种情况通常意味着没有有效的运动信息，因此可以假设宏块是静止的
  bool directZeroPredictionFlag = false;
  if (refIdxL0 < 0 && refIdxL1 < 0)
    refIdxL0 = 0, refIdxL1 = 0, directZeroPredictionFlag = true;

  // 获取共位宏块（即与当前宏块在参考帧中相同位置的宏块）的运动信息（同时间直接模式）
  PictureBase *colPic = nullptr;
  int32_t mvCol[2] = {0}, refIdxCol = 0, mbAddrCol = 0;
  // vertMvScale：垂直运动矢量的缩放因子
  int32_t vertMvScale = 0;
  /* TODO YangJing 是不是这里也应该传入flase???，现在md5值已经定了，后续再改动吧 <24-10-13 05:56:03> */
  RET(derivation_the_coLocated_4x4_sub_macroblock_partitions(
      mbPartIdx, subMbPartIdx, colPic, mbAddrCol, mvCol, refIdxCol, vertMvScale,
      true));

  // 判断共位宏块是否为零运动矢量：共位宏块的参考帧索引为0，并且运动矢量的水平和垂直分量都在[-1, 1]之间，则认为共位宏块的运动矢量为零(运动矢量的大小非常小，可以记为忽略)
  bool colZeroFlag = false;
  /* 首个后参考帧，当前被标记为“用于短期参考” */
  if (m_RefPicList1[0]->reference_marked_type == SHORT_REF)
    if ((mvCol[0] >= -1 && mvCol[0] <= 1) && (mvCol[1] >= -1 && mvCol[1] <= 1))
      if (refIdxCol == 0) colZeroFlag = true;

#define derivation_motion_vector(refIdxLX, mvLX, listSuffixFlag)               \
  if (directZeroPredictionFlag || (refIdxLX) < 0 ||                            \
      ((refIdxLX) == 0 && colZeroFlag))                                        \
    (mvLX)[0] = 0, (mvLX)[1] = 0;                                              \
  else                                                                         \
    RET(derivation_luma_motion_vector_prediction(                              \
        0, 0, currSubMbType, (listSuffixFlag), (refIdxLX), (mvLX)));

  // 运动矢量预测：满足直接零运动预测，或者当前宏块的参考索引是零且共位宏块静止，那么将当前宏块的"运动矢量"也设置为零(0, 0)
  derivation_motion_vector(refIdxL0, mvL0, false);
  derivation_motion_vector(refIdxL1, mvL1, true);

#undef derivation_motion_vector

  // 更新预测标志和子宏块运动矢量数量
  //Table 8-9 – Assignment of prediction utilization flags
  if (refIdxL0 >= 0 && refIdxL1 >= 0)
    predFlagL0 = true, predFlagL1 = true;
  else if (refIdxL0 >= 0 && refIdxL1 < 0)
    predFlagL0 = true, predFlagL1 = false;
  else if (refIdxL0 < 0 && refIdxL1 >= 0)
    predFlagL0 = false, predFlagL1 = true;

  // 如果是第一个子宏块分区，则计算子宏块的运动矢量数量
  subMvCnt = (subMbPartIdx == 0) ? (predFlagL0 + predFlagL1) : 0;
  return 0;
}

// 8.4.1.2.3 Derivation process for temporal direct luma motion vector and reference index prediction mode
//时间直接运动预测模式
//NOTE:如果当前宏块是字段宏块，则 refIdxL0 和 refIdxL1 索引字段列表；否则（当前宏块是帧宏块），refIdxL0 和 refIdxL1 索引帧或互补参考字段对的列表。
int PictureBase::
    derivation_temporal_direct_luma_motion_vector_and_ref_index_prediction(
        int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
        int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
        int32_t &subMvCnt, bool &predFlagL0, bool &predFlagL1) {
  const SliceHeader *header = m_slice->slice_header;

  // 获取共位宏块（即与当前宏块在参考帧中相同位置的宏块）的运动信息，（同空间域）
  PictureBase *colPic = nullptr;
  int32_t mvCol[2] = {0}, refIdxCol = 0, mbAddrCol = 0;
  // vertMvScale：垂直运动矢量的缩放因子
  int32_t vertMvScale = 0;
  RET(derivation_the_coLocated_4x4_sub_macroblock_partitions(
      mbPartIdx, subMbPartIdx, colPic, mbAddrCol, mvCol, refIdxCol, vertMvScale,
      false));

  // 将共位宏块的参考帧索引映射到当前帧的前参考帧索引
  refIdxL0 = 0, refIdxL1 = 0;
  if (refIdxCol >= 0)
    refIdxL0 = MapColToList0(refIdxCol, colPic, mbAddrCol, vertMvScale,
                             header->field_pic_flag);

  // 根据 vertMvScale 的值，对共位宏块的垂直运动矢量 mvCol[1] 进行缩放
  if (vertMvScale == H264_VERT_MV_SCALE_Frm_To_Fld)
    // 从帧到场（Frm_To_Fld），则垂直运动矢量减半
    mvCol[1] = mvCol[1] / 2;
  else if (vertMvScale == H264_VERT_MV_SCALE_Fld_To_Frm)
    // 从场到帧（Fld_To_Frm），则垂直运动矢量加倍
    mvCol[1] = mvCol[1] * 2;
  else if (vertMvScale == H264_VERT_MV_SCALE_One_To_One) {
  }

  PictureBase *pic0 = nullptr, *pic1 = nullptr, *currPicOrField = nullptr;
  // 当前帧是帧图像，但当前宏块是场解码模式，则根据宏块地址的奇偶性选择顶部场或底部场
  if (header->field_pic_flag == 0 && m_mbs[CurrMbAddr].mb_field_decoding_flag) {
    currPicOrField = (CurrMbAddr % 2) ? &(m_parent->m_picture_bottom_filed)
                                      : &(m_parent->m_picture_top_filed);
    pic1 = (CurrMbAddr % 2) ? &(m_RefPicList1[refIdxL1]->m_picture_bottom_filed)
                            : &(m_RefPicList1[refIdxL1]->m_picture_top_filed);
    if (refIdxL0 % 2)
      pic0 = (CurrMbAddr % 2)
                 ? &(m_RefPicList0[refIdxL0 / 2]->m_picture_top_filed)
                 : &(m_RefPicList0[refIdxL0 / 2]->m_picture_bottom_filed);
    else
      pic0 = (CurrMbAddr % 2)
                 ? &(m_RefPicList0[refIdxL0 / 2]->m_picture_bottom_filed)
                 : &(m_RefPicList0[refIdxL0 / 2]->m_picture_top_filed);

    // 当前帧是场图像，则直接获取参考帧图像的指针
  } else {
    currPicOrField = &(m_parent->m_picture_frame);
    pic0 = &(m_RefPicList0[refIdxL0]->m_picture_frame);
    pic1 = &(m_RefPicList1[refIdxL1]->m_picture_frame);
  }

  /* 当前宏块的每个4x4子宏块分区的两个运动向量mvL0和mvL1推导如下： */
  // L0参考帧是长参考帧，或者L0和L1参考帧的POC存在差值，则直接使用共位宏块的运动矢量 mvCol 作为L0的运动矢量，并将L1的运动矢量设置为零
  if (m_RefPicList0[refIdxL0]->reference_marked_type == LONG_REF ||
      DiffPicOrderCnt(pic1, pic0))
    mvL0[0] = mvCol[0], mvL0[1] = mvCol[1], mvL1[0] = 0, mvL1[1] = 0;

  // L0参考帧是短参考帧，或者L0和L1参考帧的POC存在差值
  else {
    // 运动矢量 mvL0、mvL1 被导出为同位子宏块分区的运动矢量 mvCol 的缩放版本（见图 8-2）：
    // L1参考帧与L0参考帧的POC差
    int32_t td = CLIP3(-128, 127, DiffPicOrderCnt(pic1, pic0));
    RET(td == 0);
    // 当前帧与L0参考帧的POC差
    int32_t tb = CLIP3(-128, 127, DiffPicOrderCnt(currPicOrField, pic0));
    // 时间比例因子，用于缩放共位宏块的运动矢量，距离当前帧较近的参考帧会有更大的权重，而距离较远的参考帧权重较小
    int32_t tx = (16384 + ABS(td / 2)) / td; // 16384 = 2^{14}
    int32_t DistScaleFactor = CLIP3(-1024, 1023, (tb * tx + 32) >> 6);

    // 使用时间比例因子 DistScaleFactor 缩放共位宏块的运动矢量 mvCol，得到L0的运动矢量 mvL0。然后通过L0和共位宏块的运动矢量差，计算L1的运动矢量 mvL1
    mvL0[0] = (DistScaleFactor * mvCol[0] + 128) >> 8;
    mvL0[1] = (DistScaleFactor * mvCol[1] + 128) >> 8;
    mvL1[0] = mvL0[0] - mvCol[0];
    mvL1[1] = mvL0[1] - mvCol[1];
  }

  predFlagL0 = predFlagL1 = true;
  // 如果是第一个子宏块分区，则计算子宏块的运动矢量数量
  subMvCnt = (subMbPartIdx == 0) ? (predFlagL0 + predFlagL1) : 0;
  return 0;
}

int derivation_mvLXN_and_refIdxLXN(const MacroBlock &mb, int mbAddrN,
                                   int mbPartIdxN, int subMbPartIdxN,
                                   bool listSuffixFlag, int &refIdxLXN,
                                   int mvLXN[2], const MacroBlock &mbCurrent) {
  /* 1. 宏块分区或子宏块分区 mbAddrN\mbPartIdxN\subMbPartIdxN 不可用或mbAddrN以帧内宏块预测模式进行编码，或者mbAddrN\mbPartIdxN\subMbPartIdxN的predFlagLX等于0 */
  if (mbAddrN < 0 || mbPartIdxN < 0 || subMbPartIdxN < 0 ||
      IS_INTRA_Prediction_Mode(mb.m_mb_pred_mode) ||
      (listSuffixFlag == false && mb.m_PredFlagL0[mbPartIdxN] == 0) ||
      (listSuffixFlag && mb.m_PredFlagL1[mbPartIdxN] == 0))
    mvLXN[0] = 0, mvLXN[1] = 0, refIdxLXN = -1;
  else {
    if (listSuffixFlag == false) {
      mvLXN[0] = mb.m_MvL0[mbPartIdxN][subMbPartIdxN][0];
      mvLXN[1] = mb.m_MvL0[mbPartIdxN][subMbPartIdxN][1];
      refIdxLXN = mb.m_RefIdxL0[mbPartIdxN];
    } else {
      mvLXN[0] = mb.m_MvL1[mbPartIdxN][subMbPartIdxN][0];
      mvLXN[1] = mb.m_MvL1[mbPartIdxN][subMbPartIdxN][1];
      refIdxLXN = mb.m_RefIdxL1[mbPartIdxN];
    }

    /* 2. 当前宏块是场宏块且宏块mbAddrN是帧宏块 */
    if (mbCurrent.mb_field_decoding_flag && mb.mb_field_decoding_flag == false)
      mvLXN[1] /= 2, refIdxLXN *= 2;
    else if (mbCurrent.mb_field_decoding_flag == false &&
             mb.mb_field_decoding_flag)
      mvLXN[1] *= 2, refIdxLXN /= 2;
  }
  return 0;
}

// 8.4.1.3.2 Derivation process for motion data of neighbouring partitions
// mvLXN为 相邻分区的运动向量，refIdxLXN为相邻分区的参考索引
// listSuffixFlag : 是否为列表的最后一个，即mvL0，还是mvL1
/*  该过程的输出为（N 被 A、B 或 C 替换）:
    – mbAddrN\mbPartIdxN\subMbPartIdxN 指定相邻分区，
    – 相邻分区的运动向量 mvLXN，
    – 相邻分区的参考索引 refIdxLXN。 */
int PictureBase::derivation_motion_data_of_neighbouring_partitions(
    int32_t mbPartIdx, int32_t subMbPartIdx, H264_MB_TYPE currSubMbType,
    int32_t listSuffixFlag, int32_t &mbAddrN_A, int32_t (&mvLXN_A)[2],
    int32_t &refIdxLXN_A, int32_t &mbAddrN_B, int32_t (&mvLXN_B)[2],
    int32_t &refIdxLXN_B, int32_t &mbAddrN_C, int32_t (&mvLXN_C)[2],
    int32_t &refIdxLXN_C) {

  const MacroBlock &mb = m_mbs[CurrMbAddr];
  const int32_t MbPartWidth = mb.MbPartWidth;
  const int32_t MbPartHeight = mb.MbPartHeight;
  const int32_t SubMbPartWidth = mb.SubMbPartWidth[mbPartIdx];
  const int32_t SubMbPartHeight = mb.SubMbPartHeight[mbPartIdx];

  /* 宏块坐标 */
  int32_t x = InverseRasterScan(mbPartIdx, MbPartWidth, MbPartHeight, 16, 0);
  int32_t y = InverseRasterScan(mbPartIdx, MbPartWidth, MbPartHeight, 16, 1);

  /* 子宏块坐标 */
  int32_t xS = 0, yS = 0;
  if (mb.m_name_of_mb_type == P_8x8 || mb.m_name_of_mb_type == P_8x8ref0 ||
      mb.m_name_of_mb_type == B_8x8) {
    xS = InverseRasterScan(subMbPartIdx, SubMbPartWidth, SubMbPartHeight, 8, 0);
    yS = InverseRasterScan(subMbPartIdx, SubMbPartWidth, SubMbPartHeight, 8, 1);
  }

  int32_t predPartWidth = 0;
  if (mb.m_name_of_mb_type == P_Skip || mb.m_name_of_mb_type == B_Skip ||
      mb.m_name_of_mb_type == B_Direct_16x16)
    predPartWidth = 16;
  else if (mb.m_name_of_mb_type == B_8x8)
    // 当 currSubMbType 等于 B_Direct_8x8 且 direct_spatial_mv_pred_flag 等于 1 时，预测运动矢量是完整宏块的预测运动矢量
    predPartWidth = (currSubMbType == B_Direct_8x8) ? 16 : SubMbPartWidth;
  else if (mb.m_name_of_mb_type == P_8x8 || mb.m_name_of_mb_type == P_8x8ref0)
    predPartWidth = SubMbPartWidth;
  else
    predPartWidth = MbPartWidth;

  int32_t mbPartIdxN_A = 0, mbPartIdxN_B = 0, mbPartIdxN_C = 0,
          mbPartIdxN_D = 0;
  int32_t subMbPartIdxN_A = 0, subMbPartIdxN_B = 0, subMbPartIdxN_C = 0,
          subMbPartIdxN_D = 0;
  int32_t mbAddrN_D = 0;
  RET(derivation_neighbouring_partitions(
      x + xS - 1, y + yS + 0, mbPartIdx, currSubMbType, subMbPartIdx, 0,
      mbAddrN_A, mbPartIdxN_A, subMbPartIdxN_A));
  RET(derivation_neighbouring_partitions(
      x + xS + 0, y + yS - 1, mbPartIdx, currSubMbType, subMbPartIdx, 0,
      mbAddrN_B, mbPartIdxN_B, subMbPartIdxN_B));
  RET(derivation_neighbouring_partitions(
      x + xS + predPartWidth, y + yS - 1, mbPartIdx, currSubMbType,
      subMbPartIdx, 0, mbAddrN_C, mbPartIdxN_C, subMbPartIdxN_C));
  RET(derivation_neighbouring_partitions(
      x + xS - 1, y + yS - 1, mbPartIdx, currSubMbType, subMbPartIdx, 0,
      mbAddrN_D, mbPartIdxN_D, subMbPartIdxN_D));

  /* 当分区 mbAddrC\mbPartIdxC\subMbPartIdxC 不可用时 */
  if (mbAddrN_C < 0 || mbPartIdxN_C < 0 || subMbPartIdxN_C < 0) {
    mbAddrN_C = mbAddrN_D, mbPartIdxN_C = mbPartIdxN_D,
    subMbPartIdxN_C = subMbPartIdxN_D;
  }

  // 运动矢量 mvLXN 和参考索引 refIdxLXN (N 为 A、B 或 C) 推导
  derivation_mvLXN_and_refIdxLXN(m_mbs[mbAddrN_A], mbAddrN_A, mbPartIdxN_A,
                                 subMbPartIdxN_A, listSuffixFlag, refIdxLXN_A,
                                 mvLXN_A, mb);
  derivation_mvLXN_and_refIdxLXN(m_mbs[mbAddrN_B], mbAddrN_B, mbPartIdxN_B,
                                 subMbPartIdxN_B, listSuffixFlag, refIdxLXN_B,
                                 mvLXN_B, mb);
  derivation_mvLXN_and_refIdxLXN(m_mbs[mbAddrN_C], mbAddrN_C, mbPartIdxN_C,
                                 subMbPartIdxN_C, listSuffixFlag, refIdxLXN_C,
                                 mvLXN_C, mb);

  return 0;
}

// 6.4.11.7 Derivation process for neighbouring partitions
int PictureBase::derivation_neighbouring_partitions(
    int32_t xN, int32_t yN, int32_t mbPartIdx, H264_MB_TYPE currSubMbType,
    int32_t subMbPartIdx, int32_t isChroma, int32_t &mbAddrN,
    int32_t &mbPartIdxN, int32_t &subMbPartIdxN) {

  const SliceHeader *slice_header = m_slice->slice_header;

  //---------------------------------------
  // mbAddrA\mbPartIdxA\subMbPartIdxA
  // 6.4.12 Derivation process for neighbouring locations
  int32_t xW = 0, yW = 0, maxW = 16, maxH = 16;
  int32_t luma4x4BlkIdxN = 0, luma8x8BlkIdxN = 0;
  MB_ADDR_TYPE mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
  if (isChroma) maxW = MbWidthC, maxH = MbHeightC;

  if (slice_header->MbaffFrameFlag) {
    RET(neighbouring_locations_MBAFF(xN, yN, maxW, maxH, CurrMbAddr,
                                     mbAddrN_type, mbAddrN, luma4x4BlkIdxN,
                                     luma8x8BlkIdxN, xW, yW, isChroma));
  } else {
    RET(neighbouring_locations_non_MBAFF(xN, yN, maxW, maxH, CurrMbAddr,
                                         mbAddrN_type, mbAddrN, luma4x4BlkIdxN,
                                         luma8x8BlkIdxN, xW, yW, isChroma));
  }

  /* mbAddrN 不可用，则宏块或子宏块分区 mbAddrN\mbPartIdxN\subMbPartIdxN 被标记为不可用 */
  if (mbAddrN < 0)
    mbAddrN = NA, mbPartIdxN = NA, subMbPartIdxN = NA;
  else {
    RET(derivation_macroblock_and_sub_macroblock_partition_indices(
        m_mbs[mbAddrN].m_name_of_mb_type, m_mbs[mbAddrN].m_name_of_sub_mb_type,
        xW, yW, mbPartIdxN, subMbPartIdxN));

    /* 当mbPartIdxN和subMbPartIdxN给出的分区尚未被解码时，宏块分区mbPartIdxN和子宏块分区subMbPartIdxN被标记为不可用：如当 mbPartIdx = 2、subMbPartIdx = 3、xD = 4、yD = −1 时的情况，即，当请求第三个子宏块的最后 4x4 亮度块的邻居 C 时的情况。*/
    if (m_mbs[mbAddrN].NumSubMbPart[mbPartIdxN] > subMbPartIdxN &&
        m_mbs[mbAddrN].m_isDecoded[mbPartIdxN][subMbPartIdxN] == 0) {
      //mbAddrN = NA, mbPartIdxN = NA,
      //subMbPartIdxN = NA;
      /* TODO YangJing 这为什么应该注释？不注释就解码报错了 <24-10-11 01:38:20> */
    }
  }

  return 0;
}

// 6.4.13.4 Derivation process for macroblock and sub-macroblock partition indices
int PictureBase::derivation_macroblock_and_sub_macroblock_partition_indices(
    H264_MB_TYPE mb_type_, H264_MB_TYPE subMbType_[4], int32_t xP, int32_t yP,
    int32_t &mbPartIdxN, int32_t &subMbPartIdxN) {

  RET(mb_type_ == MB_TYPE_NA);
  if (mb_type_ >= I_NxN && mb_type_ <= I_PCM)
    mbPartIdxN = 0;
  else {
    int32_t MbPartWidth = 0, MbPartHeight = 0;
    RET(MacroBlock::getMbPartWidthAndHeight(mb_type_, MbPartWidth,
                                            MbPartHeight));
    mbPartIdxN = (16 / MbPartWidth) * (yP / MbPartHeight) + (xP / MbPartWidth);
  }

  if (mb_type_ != P_8x8 && mb_type_ != P_8x8ref0 && mb_type_ != B_8x8 &&
      mb_type_ != B_Skip && mb_type_ != B_Direct_16x16) {
    subMbPartIdxN = 0;
  } else if (mb_type_ == B_Skip || mb_type_ == B_Direct_16x16) {
    subMbPartIdxN = 2 * ((yP % 8) / 4) + ((xP % 8) / 4);
  } else {
    int32_t SubMbPartWidth = 0, SubMbPartHeight = 0;
    RET(MacroBlock::getMbPartWidthAndHeight(subMbType_[mbPartIdxN],
                                            SubMbPartWidth, SubMbPartHeight));
    subMbPartIdxN = (8 / SubMbPartWidth) * ((yP % 8) / SubMbPartHeight) +
                    ((xP % 8) / SubMbPartWidth);
  }

  return 0;
}

// 8.4.3 Derivation process for prediction weights
/* 输入：参考索引 refIdxL0 和 refIdxL1，预测利用标志 predFlagL0 和 predFlagL1
 * 输出: 加权预测变量 logWDC、w0C、w1C、o0C、o1C，其中 C 被 L 替换 */
int PictureBase::derivation_prediction_weights(
    int32_t refIdxL0, int32_t refIdxL1, bool predFlagL0, bool predFlagL1,
    int32_t &logWDL, int32_t &w0L, int32_t &w1L, int32_t &o0L, int32_t &o1L,
    int32_t &logWDCb, int32_t &w0Cb, int32_t &w1Cb, int32_t &o0Cb,
    int32_t &o1Cb, int32_t &logWDCr, int32_t &w0Cr, int32_t &w1Cr,
    int32_t &o0Cr, int32_t &o1Cr) {

  const SliceHeader *header = m_slice->slice_header;
  const uint32_t weighted_bipred_idc = header->m_pps->weighted_bipred_idc;
  const uint32_t slice_type = header->slice_type % 5;
  const bool weighted_pred_flag = header->m_pps->weighted_pred_flag;
  const uint32_t ChromaArrayType = header->m_sps->ChromaArrayType;

  bool implicitModeFlag = false, explicitModeFlag = false;
  // 1. 双向预测条件下，且编码器指示使用"隐式加权预测"
  if (weighted_bipred_idc == 2 && slice_type == SLICE_B && predFlagL0 &&
      predFlagL1)
    implicitModeFlag = true;
  // 2. 双向或单向预测条件下，且编码器指示使用"显式加权预测"
  else if (weighted_bipred_idc == 1 && slice_type == SLICE_B &&
           (predFlagL0 + predFlagL1))
    explicitModeFlag = true;
  // 3. 单向预测条件下，且编码器允许使用"加权预测"，则使用"显式加权预测"
  else if (weighted_pred_flag &&
           (slice_type == SLICE_P || slice_type == SLICE_SP) && predFlagL0)
    explicitModeFlag = true;

  /* NOTE:隐式加权预测一定是B Slice，而P Slice只能是显示加权预测 */

  // 隐式模式加权双向预测: 根据参考帧的时间戳差异来计算加权因子
  if (implicitModeFlag) {
    logWDL = 5, o0L = o1L = 0;
    if (ChromaArrayType != 0)
      logWDCb = logWDCr = 5, o0Cb = o1Cb = o0Cr = o1Cr = 0;

    // 根据当前宏块的场编码模式选择合适的参考帧
    PictureBase *currPicOrField = nullptr, *pic0 = nullptr, *pic1 = nullptr;
    /* 帧图像，场宏块 */
    if (header->field_pic_flag == 0 &&
        m_mbs[CurrMbAddr].mb_field_decoding_flag) {
      currPicOrField = (CurrMbAddr % 2) ? &(m_parent->m_picture_bottom_filed)
                                        : &(m_parent->m_picture_top_filed);

#define picture(refIdx, pic, RefPicList)                                       \
  if (refIdx % 2 == 0)                                                         \
    pic = (CurrMbAddr % 2) ? &(RefPicList[refIdx / 2]->m_picture_bottom_filed) \
                           : &(RefPicList[refIdx / 2]->m_picture_top_filed);   \
  else                                                                         \
    pic = (CurrMbAddr % 2)                                                     \
              ? &(RefPicList[refIdx / 2]->m_picture_top_filed)                 \
              : &(RefPicList[refIdx / 2]->m_picture_bottom_filed);
      picture(refIdxL0, pic0, m_RefPicList0);
      picture(refIdxL1, pic1, m_RefPicList1);
#undef info

      /* "帧图像-帧宏块" 或 "场图像-场宏块" 或 "场图像-帧宏块" */
    } else {
      currPicOrField = &(m_parent->m_picture_frame);
      pic0 = &(m_RefPicList0[refIdxL0]->m_picture_frame);
      pic1 = &(m_RefPicList1[refIdxL1]->m_picture_frame);
    }

    // 计算距离缩放因子（同derivation_temporal_direct_luma_motion_vector_and_ref_index_prediction())
    int32_t tb = CLIP3(-128, 127, DiffPicOrderCnt(currPicOrField, pic0));
    int32_t td = CLIP3(-128, 127, DiffPicOrderCnt(pic1, pic0));
    RET(td == 0);
    int32_t tx = (16384 + ABS(td / 2)) / td;
    // 距离当前帧较近的参考帧会有更大的权重，而距离较远的参考帧权重较小
    int32_t DistScaleFactor = CLIP3(-1024, 1023, (tb * tx + 32) >> 6);

    // 计算亮度和色度分量的加权因子
    // 1. 不存在POC差，或者参考帧均为是长期参考帧，或者距离缩放因子超出范围，亮度和色度分量的前后参考加权因子被设置为 32
    if (DiffPicOrderCnt(pic1, pic0) == 0 ||
        pic0->reference_marked_type == LONG_REF ||
        pic1->reference_marked_type == LONG_REF ||
        (DistScaleFactor >> 2) < -0x40 || (DistScaleFactor >> 2) > 0x80) {
      w0L = w1L = 32;
      if (ChromaArrayType != 0) w0Cb = w1Cb = w0Cr = w1Cr = 0x20;
      // 2. 存在POC差 且 参考帧均为短期参考帧，距离缩放因子可用，亮度和色度分量的前后参考加权因子被设置为距离缩放因子的1/4
    } else {
      // 0x40 是一个基准值，表示满权重（即 100% 的贡献），即w0L + w1L = 0x40
      w0L = 0x40 - (DistScaleFactor >> 2), w1L = DistScaleFactor >> 2;
      if (ChromaArrayType != 0)
        w0Cb = w0Cr = 0x40 - (DistScaleFactor >> 2),
        w1Cb = w1Cr = DistScaleFactor >> 2;
    }
  }
  /* 使用显式模式加权预测： */
  else if (explicitModeFlag) {
    int32_t refIdxL0WP = 0, refIdxL1WP = 0;
    // 宏块自适应帧场，场宏块
    if (header->MbaffFrameFlag && m_mbs[CurrMbAddr].mb_field_decoding_flag)
      refIdxL0WP = refIdxL0 >> 1, refIdxL1WP = refIdxL1 >> 1;
    // 非宏块自适应帧场，帧宏块
    else
      refIdxL0WP = refIdxL0, refIdxL1WP = refIdxL1;

    // 对于亮度样本: 从 slice header 中获取亮度分量的加权因子和偏移量
    logWDL = header->luma_log2_weight_denom;
    w0L = header->luma_weight_l0[refIdxL0WP];
    w1L = header->luma_weight_l1[refIdxL1WP];
    o0L = header->luma_offset_l0[refIdxL0WP] *
          (1 << (header->m_sps->BitDepthY - 8));
    o1L = header->luma_offset_l1[refIdxL1WP] *
          (1 << (header->m_sps->BitDepthY - 8));

    // 对于色度样本: 从 slice header 中获取色度分量的加权因子和偏移量
    if (ChromaArrayType != 0) {
      logWDCb = logWDCr = header->chroma_log2_weight_denom;
      w0Cb = header->chroma_weight_l0[refIdxL0WP][0];
      w0Cr = header->chroma_weight_l0[refIdxL0WP][1];
      w1Cb = header->chroma_weight_l1[refIdxL1WP][0];
      w1Cr = header->chroma_weight_l1[refIdxL1WP][1];
      o0Cb = header->chroma_offset_l0[refIdxL0WP][0] *
             (1 << (header->m_sps->BitDepthC - 8));
      o0Cr = header->chroma_offset_l0[refIdxL0WP][1] *
             (1 << (header->m_sps->BitDepthC - 8));
      o1Cb = header->chroma_offset_l1[refIdxL1WP][0] *
             (1 << (header->m_sps->BitDepthC - 8));
      o1Cr = header->chroma_offset_l1[refIdxL1WP][1] *
             (1 << (header->m_sps->BitDepthC - 8));
    }
  }

  // 检查显示加权因子的合法性，确保它们在合理的范围内
  if (explicitModeFlag && predFlagL0 && predFlagL1) {
    int32_t max = (logWDL == 7) ? 127 : 128;
    RET(-128 > (w0L + w1L) || (w0L + w1L) > max);
    if (ChromaArrayType != 0) {
      max = (logWDCb == 7) ? 127 : 128;
      RET(-128 > (w0Cb + w1Cb) || (w0Cb + w1Cb) > max);

      max = (logWDCr == 7) ? 127 : 128;
      RET(-128 > (w0Cr + w1Cr) || (w0Cr + w1Cr) > max);
    }
  }

  return 0;
}

// 8.4.2.1 Reference picture selection process
int PictureBase::reference_picture_selection(int32_t refIdxLX,
                                             Frame *RefPicListX[16],
                                             int32_t RefPicListXLength,
                                             PictureBase *&refPic) {
  const SliceHeader *header = m_slice->slice_header;
  refPic = nullptr;
  RET(refIdxLX < 0 || refIdxLX >= 32);

  // RefPicListX的每个条目是参考场或参考帧的场 或者 都是参考帧或互补参考场对，不存在混合情况。
  for (int i = 0; i < RefPicListXLength; i++) {
    // 当前帧是场图像，则参考帧必须是场图像
    if (header->field_pic_flag) {
      RET((RefPicListX[i]->m_pic_coded_type_marked_as_refrence != TOP_FIELD &&
           RefPicListX[i]->m_pic_coded_type_marked_as_refrence !=
               BOTTOM_FIELD));
    }
    // 当前帧是帧图像，则参考帧必须是帧图像
    else
      RET((RefPicListX[i]->m_pic_coded_type_marked_as_refrence != FRAME &&
           RefPicListX[i]->m_pic_coded_type_marked_as_refrence !=
               COMPLEMENTARY_FIELD_PAIR));
  }

  if (header->field_pic_flag) { // Field
    // 如果参考帧是顶场（PICTURE_CODED_TYPE_TOP_FIELD），则选择 m_picture_top_filed。
    if (RefPicListX[refIdxLX]->m_pic_coded_type_marked_as_refrence == TOP_FIELD)
      refPic = &(RefPicListX[refIdxLX]->m_picture_top_filed);
    // 如果参考帧是底场（PICTURE_CODED_TYPE_BOTTOM_FIELD），则选择 m_picture_bottom_filed
    else if (RefPicListX[refIdxLX]->m_pic_coded_type_marked_as_refrence ==
             BOTTOM_FIELD)
      refPic = &(RefPicListX[refIdxLX]->m_picture_bottom_filed);
    else
      RET(-1);
  } else { // Frame
           // 当前宏块是帧模式，直接选择参考帧
    if (m_mbs[CurrMbAddr].mb_field_decoding_flag == 0)
      refPic = &(RefPicListX[refIdxLX]->m_picture_frame);
    else { //MBAFF
      Frame *ref = RefPicListX[refIdxLX / 2];
      if (refIdxLX % 2 == 0)
        refPic = (mb_y % 2) ? &(ref->m_picture_bottom_filed)
                            : &(ref->m_picture_top_filed);
      else
        refPic = (mb_y % 2) ? &(ref->m_picture_top_filed)
                            : &(ref->m_picture_bottom_filed);
    }
  }

  RET(refPic == nullptr);
  return 0;
}

// 8.4.2.2 Fractional sample interpolation process(亚像素插值过程)
/* 输入： 
   * – 分区索引 mbPartIdx, 子宏块分区索引 subMbPartIdx 
   * – 该分区的宽度和高度部分宽度、部分高度（以亮度样本单位表示）， 
   * – 给出的亮度运动向量 mvLX 四分之一亮度样本单位， 
   * – 色度运动向量 mvCLX 的水平精度为第 1(4*SubWidthC) 色度样本单位，精度为第 1(4*SubHeightC) 色度单位垂直样本单位，
   * – 参考图片样本;
 * 输出：
   * – 预测亮度样本值predPartLXL，预测色度样本值predPartLXCb 和 predPartLXCr; */
int PictureBase::fractional_sample_interpolation(
    int32_t mbPartIdx, int32_t subMbPartIdx, int32_t partWidth,
    int32_t partHeight, int32_t partWidthC, int32_t partHeightC, int32_t xAL,
    int32_t yAL, int32_t (&mvLX)[2], int32_t (&mvCLX)[2], PictureBase *refPicLX,
    uint8_t *predPartLXL, uint8_t *predPartLXCb, uint8_t *predPartLXCr) {

  const uint32_t ChromaArrayType =
      m_slice->slice_header->m_sps->ChromaArrayType;
  const int32_t SubWidthC = m_slice->slice_header->m_sps->SubWidthC;
  const int32_t SubHeightC = m_slice->slice_header->m_sps->SubHeightC;

  // 全样本单位的亮度位置（ xIntL, yIntL ），以四分之一样本单位给出的偏移量（ xFracL, yFracL ）
  int32_t xIntL = 0, yIntL = 0, xFracL = 0, yFracL = 0;
  int32_t xIntC = 0, yIntC = 0, xFracC = 0, yFracC = 0;
  // 预测亮、色度样本值
  uint8_t predPartLXL_xL_yL = 0, predPartLXCb_xC_yC = 0, predPartLXCr_xC_yC = 0;
  //NOTE: H.264 中的运动矢量以 1/4 像素精度存储时，运动矢量的值会乘以 4 存储为整数
  // ------------------------ 亮度分量预测 ------------------------
  // 当前分区的左上亮度样本的完整样本单位给出的位置，相对于给定的二维亮度样本数组的左上亮度样本位置
  for (int32_t yL = 0; yL < partHeight; yL++)
    for (int32_t xL = 0; xL < partWidth; xL++) {
      // 整数部分：通过右移 2 位（>> 2）得到运动矢量的整数部分（即1/4)
      xIntL = xAL + (mvLX[0] >> 2) + xL, yIntL = yAL + (mvLX[1] >> 2) + yL;
      // 亚像素偏移：通过按位与操作（& 3），得到运动矢量的亚像素偏移(范围是 0 到 3，分别对应 0、1/4、1/2 和 3/4 像素)
      xFracL = mvLX[0] & 3, yFracL = mvLX[1] & 3;
      // 使用参考帧 refPicLX 中的像素值和亚像素偏移进行插值，得到预测的亮度样本
      RET(luma_sample_interpolation(xIntL, yIntL, xFracL, yFracL, refPicLX,
                                    predPartLXL_xL_yL));
      predPartLXL[yL * partWidth + xL] = predPartLXL_xL_yL;
    }

  if (ChromaArrayType == 0) return 0;

#define YUV420 1
#define YUV422 2
#define YUV444 3
  // ------------------------ 色度分量预测 ------------------------
  for (int32_t yC = 0; yC < partHeightC; yC++)
    for (int32_t xC = 0; xC < partWidthC; xC++) {
      // YUV420: 运动矢量的整数部分通过右移 3 位（>> 3）得到，亚像素偏移通过 & 7 计算
      if (ChromaArrayType == YUV420)
        xIntC = (xAL / SubWidthC) + (mvCLX[0] >> 3) + xC,
        yIntC = (yAL / SubHeightC) + (mvCLX[1] >> 3) + yC,
        xFracC = mvCLX[0] & 7, yFracC = mvCLX[1] & 7;
      // YUV422: 运动矢量的整数部分通过右移 3 位（水平）和右移 2 位（垂直）得到，亚像素偏移通过 & 7 和 (mvCLX[1] & 3) << 1 计算
      else if (ChromaArrayType == YUV422)
        xIntC = (xAL / SubWidthC) + (mvCLX[0] >> 3) + xC,
        yIntC = (yAL / SubHeightC) + (mvCLX[1] >> 2) + yC,
        xFracC = mvCLX[0] & 7, yFracC = (mvCLX[1] & 3) << 1;
      else //YUV444: 同亮度
        xIntC = xAL + (mvLX[0] >> 2) + xC, yIntC = yAL + (mvLX[1] >> 2) + yC,
        xFracC = (mvCLX[0] & 3), yFracC = (mvCLX[1] & 3);

      // 存在子采样，即YUV420,YUV422
      if (ChromaArrayType != YUV422) {
        RET(chroma_sample_interpolation(xIntC, yIntC, xFracC, yFracC, refPicLX,
                                        1, predPartLXCb_xC_yC));
        predPartLXCb[yC * partWidthC + xC] = predPartLXCb_xC_yC;

        RET(chroma_sample_interpolation(xIntC, yIntC, xFracC, yFracC, refPicLX,
                                        0, predPartLXCr_xC_yC));
        predPartLXCr[yC * partWidthC + xC] = predPartLXCr_xC_yC;
      }
      // 无子采样，即YUV444
      else {
        RET(luma_sample_interpolation(xIntC, yIntC, xFracC, yFracC, refPicLX,
                                      predPartLXCb_xC_yC));
        predPartLXCb[yC * partWidthC + xC] = predPartLXCb_xC_yC;

        RET(luma_sample_interpolation(xIntC, yIntC, xFracC, yFracC, refPicLX,
                                      predPartLXCr_xC_yC));
        predPartLXCr[yC * partWidthC + xC] = predPartLXCr_xC_yC;
      }
    }

#undef YUV420
#undef YUV422
#undef YUV444

  return 0;
}

// 8.4.2.2.1 Luma sample interpolation process(Luma样本插值过程)
/* 输入：
 * – 全样本单位的亮度位置（ xIntL, yIntL ）， 
 * – 亚像素样本单位的亮度位置偏移（ xFracL, yFracL ）， 
 * – 亮度样本所选参考图片 refPicLXL 的数组。  
 * 输出: 预测的亮度样本值 predPartLXL[ xL, yL ]。*/
int PictureBase::luma_sample_interpolation(int32_t xIntL, int32_t yIntL,
                                           int32_t xFracL, int32_t yFracL,
                                           PictureBase *refPic,
                                           uint8_t &predPartLXL_xL_yL) {

  const SliceHeader *header = m_slice->slice_header;
  const uint32_t BitDepthY = header->m_sps->BitDepthY;
  const int32_t mb_field_decoding_flag =
      m_slice->slice_data->mb_field_decoding_flag;

  // 有效参考图像亮度数组的高度
  int32_t refPicHeightEffectiveL = PicHeightInSamplesL;
  if (header->MbaffFrameFlag && mb_field_decoding_flag)
    refPicHeightEffectiveL /= 2;

    // 从参考帧中获取亮度样本
#define getLumaSample(xDZL, yDZL)                                              \
  refPic                                                                       \
      ->m_pic_buff_luma[CLIP3(0, refPicHeightEffectiveL - 1, yIntL + (yDZL)) * \
                            refPic->PicWidthInSamplesL +                       \
                        CLIP3(0, refPic->PicWidthInSamplesL - 1,               \
                              xIntL + (xDZL))]

  /* 
    +----+----+----+----+----+----+
    |X11 |X12 | A  | B  |X13 |X14 |
    +----+----+----+----+----+----+
    |X21 |X22 | C  | D  |X23 |X24 |
    +----+----+----+----+----+----+
    | E  | F  | *G | H  | I  | J  |
    +----+----+----+----+----+----+
    | K  | L  | M  | N  | P  | Q  |
    +----+----+----+----+----+----+
    |X51 |X52 | R  | S  |X53 |X54 |
    +----+----+----+----+----+----+
    |X61 |X62 | T  | U  |X63 |X64 |
    +----+----+----+----+----+----+
   */

  // 提取参考帧中以 (xIntL, yIntL) 为中心的 6x6 像素块的像素值，将用于后续的 6-tap 滤波器插值（共20个样本）
  int32_t A = getLumaSample(0, -2);
  int32_t B = getLumaSample(1, -2);
  int32_t C = getLumaSample(0, -1);
  int32_t D = getLumaSample(1, -1);
  int32_t E = getLumaSample(-2, 0);
  int32_t F = getLumaSample(-1, 0);
  int32_t G = getLumaSample(0, 0); // 原点
  int32_t H = getLumaSample(1, 0);
  int32_t I = getLumaSample(2, 0);
  int32_t J = getLumaSample(3, 0);
  int32_t K = getLumaSample(-2, 1);
  int32_t L = getLumaSample(-1, 1);
  int32_t M = getLumaSample(0, 1);
  int32_t N = getLumaSample(1, 1);
  int32_t P = getLumaSample(2, 1);
  int32_t Q = getLumaSample(3, 1);
  int32_t R = getLumaSample(0, 2);
  int32_t S = getLumaSample(1, 2);
  int32_t T = getLumaSample(0, 3);
  int32_t U = getLumaSample(1, 3);

  // 6x6矩阵的首行
  int32_t X11 = getLumaSample(-2, -2);
  int32_t X12 = getLumaSample(-1, -2);
  int32_t X13 = getLumaSample(2, -2);
  int32_t X14 = getLumaSample(3, -2);

  // 6x6矩阵的第二行
  int32_t X21 = getLumaSample(-2, -1);
  int32_t X22 = getLumaSample(-1, -1);
  int32_t X23 = getLumaSample(2, -1);
  int32_t X24 = getLumaSample(3, -1);

  // 6x6矩阵的第五行
  int32_t X51 = getLumaSample(-2, 2);
  int32_t X52 = getLumaSample(-1, 2);
  int32_t X53 = getLumaSample(2, 2);
  int32_t X54 = getLumaSample(3, 2);

  // 6x6矩阵的第六行
  int32_t X61 = getLumaSample(-2, 3);
  int32_t X62 = getLumaSample(-1, 3);
  int32_t X63 = getLumaSample(2, 3);
  int32_t X64 = getLumaSample(3, 3);

  // 根据 6 个相邻的像素值进行加权求和，对于 1/4 像素插值，滤波器的权重是 [-1, 5, 20, 20, -5, 1]（越靠近中间的权重越大）
#define a_6_tap_filter(v1, v2, v3, v4, v5, v6)                                 \
  ((v1) - 5 * (v2) + 20 * (v3) + 20 * (v4) - 5 * (v5) + (v6))

  // 在水平方向上使用 6-tap 滤波器对像素进行插值。b1 是以 G 为中心的水平插值结果，s1 是以 M 为中心的水平插值结果
  int32_t b1 = a_6_tap_filter(E, F, G, H, I, J),
          s1 = a_6_tap_filter(K, L, M, N, P, Q);

  // 在垂直方向上使用 6-tap 滤波器对像素进行插值。h1 是以 G 为中心的垂直插值结果，m1 是以 H 为中心的垂直插值结果
  int32_t h1 = a_6_tap_filter(A, C, G, M, R, T),
          m1 = a_6_tap_filter(B, D, H, N, S, U);

  // 前两列、后两列进行插值， 对对角线方向的像素进行插值，计算出 j1
  int32_t cc = a_6_tap_filter(X11, X21, E, K, X51, X61),
          dd = a_6_tap_filter(X12, X22, F, L, X52, X62),
          ee = a_6_tap_filter(X13, X23, I, P, X53, X63),
          ff = a_6_tap_filter(X14, X24, J, Q, X54, X64);

  // 所有垂直方向的插值
  int32_t j1 = a_6_tap_filter(cc, dd, h1, m1, ee, ff),
          j = Clip1C((j1 + 512) >> 10, BitDepthY);

  // 将水平方向插值结果缩放，相当于除以 32，进行四舍五入
  int32_t b = Clip1C((b1 + 16) >> 5, BitDepthY),
          s = Clip1C((s1 + 16) >> 5, BitDepthY),
          h = Clip1C((h1 + 16) >> 5, BitDepthY),
          m = Clip1C((m1 + 16) >> 5, BitDepthY);

  /* {G, d, h, n}
     {a, e, i, p}  
     {b, f, j, q} s
     {c, g, k, r} 
            m       */
  // 副对角线：通过对整数和半样本位置处的两个最近样本进行向上舍入平均而得
  int32_t d = (G + h + 1) >> 1, a = (G + b + 1) >> 1;
  int32_t n = (M + h + 1) >> 1, i = (h + j + 1) >> 1, f = (b + j + 1) >> 1,
          c = (H + b + 1) >> 1;
  int32_t q = (j + s + 1) >> 1, k = (j + m + 1) >> 1;

  // 副对角线：通过对对角线方向上一半样本位置处的两个最近样本进行向上舍入平均而得
  int32_t e = (b + h + 1) >> 1, g = (b + m + 1) >> 1, p = (h + s + 1) >> 1,
          r = (m + s + 1) >> 1;

#undef a_6_tap_filter
#undef getLumaSample

  // 4x4矩阵，不同亚像素偏移位置的预测值，根据 xFracL 和 yFracL 的值，选择对应的预测值为最终的预测值
  // Table 8-12 – Assignment of the luma prediction sample predPartLXL[ xL, yL ]
  int32_t predPartLXLs[4][4] = {
      {G, d, h, n},
      {a, e, i, p},
      {b, f, j, q},
      {c, g, k, r},
  };
  predPartLXL_xL_yL = predPartLXLs[xFracL][yFracL];

  return 0;
}

// 8.4.2.2.2 Chroma sample interpolation process
//同Luma_sample_interpolation_process类似
/* 输入：
   * – 整数像素单位的色度位置 ( xIntC, yIntC )， 
   * – 亚像素单位的色度位置偏移 ( xFracC， yFracC )， 
   * – 来自所选参考图片 refPicLXC 的色度分量样本。  
 * 输出: 预测色度样本值predPartLXC[xC，yC]。*/
int PictureBase::chroma_sample_interpolation(int32_t xIntC, int32_t yIntC,
                                             int32_t xFracC, int32_t yFracC,
                                             PictureBase *refPic,
                                             int32_t isChromaCb,
                                             uint8_t &predPartLXC_xC_yC) {

  const SliceHeader *header = m_slice->slice_header;
  const int32_t mb_field_decoding_flag =
      m_slice->slice_data->mb_field_decoding_flag;

  // 有效参考图像亮度数组的高度
  int32_t refPicHeightEffectiveC = PicHeightInSamplesC;
  if (header->MbaffFrameFlag && mb_field_decoding_flag)
    refPicHeightEffectiveC /= 2;

  // 色度样本的四个整数像素位置。它们对应于参考帧中 2x2 像素块的四个顶点。
  int32_t xAC = CLIP3(0, refPic->PicWidthInSamplesC - 1, xIntC),
          yAC = CLIP3(0, refPicHeightEffectiveC - 1, yIntC);
  int32_t xBC = CLIP3(0, refPic->PicWidthInSamplesC - 1, xIntC + 1),
          yBC = CLIP3(0, refPicHeightEffectiveC - 1, yIntC);
  int32_t xCC = CLIP3(0, refPic->PicWidthInSamplesC - 1, xIntC),
          yCC = CLIP3(0, refPicHeightEffectiveC - 1, yIntC + 1);
  int32_t xDC = CLIP3(0, refPic->PicWidthInSamplesC - 1, xIntC + 1),
          yDC = CLIP3(0, refPicHeightEffectiveC - 1, yIntC + 1);

  // 从参考帧的色度样本中提取 2x2 像素块的四个顶点的色度值
  uint8_t *refPicLC_pic_buff_cbcr =
      (isChromaCb) ? refPic->m_pic_buff_cb : refPic->m_pic_buff_cr;
  int32_t A = refPicLC_pic_buff_cbcr[yAC * refPic->PicWidthInSamplesC + xAC],
          B = refPicLC_pic_buff_cbcr[yBC * refPic->PicWidthInSamplesC + xBC],
          C = refPicLC_pic_buff_cbcr[yCC * refPic->PicWidthInSamplesC + xCC],
          D = refPicLC_pic_buff_cbcr[yDC * refPic->PicWidthInSamplesC + xDC];

  // 左上角像素 A,右上角像素 B,左下角像素 C,右下角像素 D的权重因子，插值权重的总和为 0x40
  int32_t wA = (8 - xFracC) * (8 - yFracC), wB = xFracC * (8 - yFracC),
          wC = (8 - xFracC) * yFracC, wD = xFracC * yFracC;
  // 双线性插值，计算亚像素位置的预测值
  predPartLXC_xC_yC = (wA * A + wB * B + wC * C + wD * D + 32) >> 6; //除0x40

  return 0;
}

// 8.4.2.3 Weighted sample prediction process
/* 输入： 
   * – mbPartIdx：由分区索引给出的当前分区， subMbPartIdx：子宏块分区索引， 
   * – predFlagL0 和 predFlagL1：预测列表利用率标志， 
   * – predPartLXL：(partWidth)x(partHeight) 数组预测亮度样本的数量（根据 predFlagL0 和 predFlagL1 将 LX 替换为 L0 或 L1）， 
   * – predPartLXCb 和 predPartLXCr：预测色度样本的 (partWidthC)x(partHeightC) 数组，每个数组对应一个色度分量 Cb 和 Cr（其中 LX 被替换为 L0 或 L1）， 
   * – 加权预测变量 logWDC、w0C、w1C、o0C、o1C，其中 C 被 L 替换，Cb 和 Cr。  
 * 输出： 
   * – predPartL：预测亮度样本的 (partWidth)x(partHeight) 数组， 
   * – predPartCb 和 predPartCr：预测色度样本的 (partWidthC)x(partHeightC) 数组，色度分量 Cb 和 Cr 各一个。*/
int PictureBase::weighted_sample_prediction(
    /* Input: */
    int32_t mbPartIdx, int32_t subMbPartIdx, bool predFlagL0, bool predFlagL1,
    int32_t partWidth, int32_t partHeight, int32_t partWidthC,
    int32_t partHeightC, int32_t logWDL, int32_t w0L, int32_t w1L, int32_t o0L,
    int32_t o1L, int32_t logWDCb, int32_t w0Cb, int32_t w1Cb, int32_t o0Cb,
    int32_t o1Cb, int32_t logWDCr, int32_t w0Cr, int32_t w1Cr, int32_t o0Cr,
    int32_t o1Cr, uint8_t *predPartL0L, uint8_t *predPartL0Cb,
    uint8_t *predPartL0Cr, uint8_t *predPartL1L, uint8_t *predPartL1Cb,
    uint8_t *predPartL1Cr,
    /* Output: */
    uint8_t *predPartL, uint8_t *predPartCb, uint8_t *predPartCr) {

  const SliceHeader *header = m_slice->slice_header;
  uint32_t slice_type = header->slice_type % 5;

  /* 单向预测（前,P Slice） */
  if (predFlagL0 && (slice_type == SLICE_P || slice_type == SLICE_SP)) {
    if (header->m_pps->weighted_pred_flag == 0) {
      // 默认加权样本预测
      RET(weighted_sample_prediction_default(
          predFlagL0, predFlagL1, partWidth, partHeight, partWidthC,
          partHeightC, predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L,
          predPartL1Cb, predPartL1Cr, predPartL, predPartCb, predPartCr));
    } else
      // 显式加权样本预测
      RET(weighted_sample_prediction_Explicit_or_Implicit(
          mbPartIdx, subMbPartIdx, predFlagL0, predFlagL1, partWidth,
          partHeight, partWidthC, partHeightC, logWDL, w0L, w1L, o0L, o1L,
          logWDCb, w0Cb, w1Cb, o0Cb, o1Cb, logWDCr, w0Cr, w1Cr, o0Cr, o1Cr,
          predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L, predPartL1Cb,
          predPartL1Cr, predPartL, predPartCb, predPartCr));

    return 0;
  }

  /* 单、双向预测（前、后、双向,B Slide） */
  if ((predFlagL0 || predFlagL1) && slice_type == SLICE_B) {
    // 1. 不使用加权双向预测
    if (header->m_pps->weighted_bipred_idc == 0) {
      RET(weighted_sample_prediction_default(
          predFlagL0, predFlagL1, partWidth, partHeight, partWidthC,
          partHeightC, predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L,
          predPartL1Cb, predPartL1Cr, predPartL, predPartCb, predPartCr));
    }
    // 2. 显式加权样本预测
    else if (header->m_pps->weighted_bipred_idc == 1) {
      RET(weighted_sample_prediction_Explicit_or_Implicit(
          mbPartIdx, subMbPartIdx, predFlagL0, predFlagL1, partWidth,
          partHeight, partWidthC, partHeightC, logWDL, w0L, w1L, o0L, o1L,
          logWDCb, w0Cb, w1Cb, o0Cb, o1Cb, logWDCr, w0Cr, w1Cr, o0Cr, o1Cr,
          predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L, predPartL1Cb,
          predPartL1Cr, predPartL, predPartCb, predPartCr));
    }
    // 3. 隐式加权样本预测
    else {
      if (predFlagL0 && predFlagL1) { /* 双向预测 */
        RET(weighted_sample_prediction_Explicit_or_Implicit(
            mbPartIdx, subMbPartIdx, predFlagL0, predFlagL1, partWidth,
            partHeight, partWidthC, partHeightC, logWDL, w0L, w1L, o0L, o1L,
            logWDCb, w0Cb, w1Cb, o0Cb, o1Cb, logWDCr, w0Cr, w1Cr, o0Cr, o1Cr,
            predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L, predPartL1Cb,
            predPartL1Cr, predPartL, predPartCb, predPartCr));
      } else /* 单向预测 */
        RET(weighted_sample_prediction_default(
            predFlagL0, predFlagL1, partWidth, partHeight, partWidthC,
            partHeightC, predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L,
            predPartL1Cb, predPartL1Cr, predPartL, predPartCb, predPartCr));
    }
  }

  return 0;
}

// 8.4.2.3.1 Default weighted sample prediction process
/* 输入、输出同weighted_sample_prediction() */
// NOTE: 无加权
int PictureBase::weighted_sample_prediction_default(
    bool predFlagL0, bool predFlagL1, int32_t partWidth, int32_t partHeight,
    int32_t partWidthC, int32_t partHeightC, uint8_t *predPartL0L,
    uint8_t *predPartL0Cb, uint8_t *predPartL0Cr, uint8_t *predPartL1L,
    uint8_t *predPartL1Cb, uint8_t *predPartL1Cr,
    /* Output: */
    uint8_t *predPartL, uint8_t *predPartCb, uint8_t *predPartCr) {

  const uint32_t ChromaArrayType =
      m_slice->slice_header->m_sps->ChromaArrayType;

#define single_prediction(predPartLXL, predPartLXCb, predPartLXCr, n)          \
  for (int y = 0; y < partHeight; y++)                                         \
    for (int x = 0; x < partWidth; x++)                                        \
      predPartL[y * partWidth + x] = predPartLXL[y * partWidth + x];           \
  if (ChromaArrayType != 0)                                                    \
    for (int y = 0; y < partHeightC; y++)                                      \
      for (int x = 0; x < partWidthC; x++) {                                   \
        predPartCb[y * partWidthC + x] =                                       \
            (predPartLXCb[y * partWidthC + x] + n) >> n;                       \
        predPartCr[y * partWidthC + x] =                                       \
            (predPartLXCr[y * partWidthC + x] + n) >> n;                       \
      }

  // 向前预测
  if (predFlagL0 && predFlagL1 == false) {
    single_prediction(predPartL0L, predPartL0Cb, predPartL0Cr, 0);
  }
  // 向后预测
  else if (predFlagL0 == false && predFlagL1) {
    single_prediction(predPartL1L, predPartL1Cb, predPartL1Cr, 0);
  }

#undef single_prediction
  // 双向预测（外部调用已经排除了无预测情况）
  else {
    for (int y = 0; y < partHeight; y++)
      for (int x = 0; x < partWidth; x++)
        //(前预测的样本 + 后预测的样本)取平均值
        predPartL[y * partWidth + x] = (predPartL0L[y * partWidth + x] +
                                        predPartL1L[y * partWidth + x] + 1) >>
                                       1;

    if (ChromaArrayType != 0)
      for (int y = 0; y < partHeightC; y++)
        for (int x = 0; x < partWidthC; x++) {
          //(前预测的样本 + 后预测的样本)取平均值
          predPartCb[y * partWidthC + x] =
              (predPartL0Cb[y * partWidthC + x] +
               predPartL1Cb[y * partWidthC + x] + 1) >>
              1;
          predPartCr[y * partWidthC + x] =
              (predPartL0Cr[y * partWidthC + x] +
               predPartL1Cr[y * partWidthC + x] + 1) >>
              1;
        }
  }

  return 0;
}

// 8.4.2.3.2 Weighted sample prediction process
// 用于显式、隐式加权预测
/* 输入、输出同weighted_sample_prediction()，且逻辑一致，只不过一个有权重，一个无权重 */
int PictureBase::weighted_sample_prediction_Explicit_or_Implicit(
    int32_t mbPartIdx, int32_t subMbPartIdx, bool predFlagL0, bool predFlagL1,
    int32_t partWidth, int32_t partHeight, int32_t partWidthC,
    int32_t partHeightC, int32_t logWDL, int32_t w0L, int32_t w1L, int32_t o0L,
    int32_t o1L, int32_t logWDCb, int32_t w0Cb, int32_t w1Cb, int32_t o0Cb,
    int32_t o1Cb, int32_t logWDCr, int32_t w0Cr, int32_t w1Cr, int32_t o0Cr,
    int32_t o1Cr, uint8_t *predPartL0L, uint8_t *predPartL0Cb,
    uint8_t *predPartL0Cr, uint8_t *predPartL1L, uint8_t *predPartL1Cb,
    uint8_t *predPartL1Cr,
    /* Output: */
    uint8_t *predPartL, uint8_t *predPartCb, uint8_t *predPartCr) {

  const uint32_t ChromaArrayType =
      m_slice->slice_header->m_sps->ChromaArrayType;
  const uint32_t BitDepthY = m_slice->slice_header->m_sps->BitDepthY;
  const uint32_t BitDepthC = m_slice->slice_header->m_sps->BitDepthC;

#define single_prediction(predPartLXL, predPartLXCb, predPartLXCr, wXCb, oXCb, \
                          wXCr, oXCr)                                          \
  for (int y = 0; y < partHeight; y++)                                         \
    for (int x = 0; x < partWidth; x++)                                        \
      if (logWDL >= 1)                                                         \
        predPartL[y * partWidth + x] = Clip1C(                                 \
            ((predPartLXL[y * partWidth + x] * w0L + POWER2(logWDL - 1)) >>    \
             logWDL) +                                                         \
                o0L,                                                           \
            BitDepthY);                                                        \
      else                                                                     \
        predPartL[y * partWidth + x] =                                         \
            Clip1C(predPartLXL[y * partWidth + x] * w0L + o0L, BitDepthY);     \
  if (ChromaArrayType != 0) {                                                  \
    for (int y = 0; y < partHeightC; y++) {                                    \
      for (int x = 0; x < partWidthC; x++) {                                   \
        if (logWDCb >= 1)                                                      \
          predPartCb[y * partWidthC + x] =                                     \
              Clip1C(((predPartLXCb[y * partWidthC + x] * wXCb +               \
                       POWER2(logWDCb - 1)) >>                                 \
                      logWDCb) +                                               \
                         oXCb,                                                 \
                     BitDepthC);                                               \
        else                                                                   \
          predPartCb[y * partWidthC + x] = Clip1C(                             \
              predPartLXCb[y * partWidthC + x] * wXCb + oXCb, BitDepthC);      \
        if (logWDCr >= 1)                                                      \
          predPartCr[y * partWidthC + x] =                                     \
              Clip1C(((predPartLXCr[y * partWidthC + x] * wXCr +               \
                       POWER2(logWDCr - 1)) >>                                 \
                      logWDCr) +                                               \
                         oXCr,                                                 \
                     BitDepthC);                                               \
        else                                                                   \
          predPartCr[y * partWidthC + x] = Clip1C(                             \
              predPartLXCr[y * partWidthC + x] * wXCr + oXCr, BitDepthC);      \
      }                                                                        \
    }                                                                          \
  }

  // 向前预测
  if (predFlagL0 && predFlagL1 == false) {
    single_prediction(predPartL0L, predPartL0Cb, predPartL0Cr, w0Cb, o0Cb, w0Cr,
                      o0Cr);
  }
  // 向后预测
  else if (predFlagL0 == false && predFlagL1) {
    single_prediction(predPartL1L, predPartL1Cb, predPartL1Cr, w1Cb, o1Cb, w1Cr,
                      o1Cr);
  }
#undef single_prediction

  // 双向预测（外部调用已经排除了无预测情况）
  else {
    for (int y = 0; y < partHeight; y++)
      for (int x = 0; x < partWidth; x++)
        predPartL[y * partWidth + x] =
            Clip1C(((predPartL0L[y * partWidth + x] * w0L +
                     predPartL1L[y * partWidth + x] * w1L + POWER2(logWDL)) >>
                    (logWDL + 1)) +
                       ((o0L + o1L + 1) >> 1),
                   BitDepthY);

    if (ChromaArrayType != 0) {
      for (int y = 0; y < partHeightC; y++) {
        for (int x = 0; x < partWidthC; x++) {
          predPartCb[y * partWidthC + x] = Clip1C(
              ((predPartL0Cb[y * partWidthC + x] * w0Cb +
                predPartL1Cb[y * partWidthC + x] * w1Cb + POWER2(logWDCb)) >>
               (logWDCb + 1)) +
                  ((o0Cb + o1Cb + 1) >> 1),
              BitDepthC);

          predPartCr[y * partWidthC + x] = Clip1C(
              ((predPartL0Cr[y * partWidthC + x] * w0Cr +
                predPartL1Cr[y * partWidthC + x] * w1Cr + POWER2(logWDCr)) >>
               (logWDCr + 1)) +
                  ((o0Cr + o1Cr + 1) >> 1),
              BitDepthC);
        }
      }
    }
  }

  return 0;
}

// 8.4.1.2.3 Derivation process for temporal direct luma motion vector and reference index prediction mode
//refPicCol 为在对图片 colPic 内的同位宏块 mbAddrCol 进行解码时由参考索引 refIdxCol 引用的帧、字段或互补字段对
int PictureBase::MapColToList0(int32_t refIdxCol, PictureBase *colPic,
                               int32_t mbAddrCol, int32_t vertMvScale,
                               bool field_pic_flag) {

  int32_t refIdxL0Frm = NA;
  for (int i = 0; i < H264_MAX_REF_PIC_LIST_COUNT; i++) {
    if (m_RefPicList0[i] == nullptr) break;
    if ((m_RefPicList0[i]->m_picture_coded_type == FRAME &&
         (&m_RefPicList0[i]->m_picture_frame == colPic)) ||
        (m_RefPicList0[i]->m_picture_coded_type == COMPLEMENTARY_FIELD_PAIR &&
         (&m_RefPicList0[i]->m_picture_top_filed == colPic ||
          &m_RefPicList0[i]->m_picture_bottom_filed == colPic))) {
      refIdxL0Frm = i;
      break;
    }
  }

  if (refIdxL0Frm == NA) RET(-1);

  //NOTE: refPicCol 引用的字段包含帧或互补字段对。 RefPicList0 应包含一个帧或包含字段 refPicCol 的互补字段对。
  int32_t refIdxL0_temp = 0;
  if (vertMvScale == H264_VERT_MV_SCALE_One_To_One) {
    if (field_pic_flag == 0 && m_mbs[CurrMbAddr].mb_field_decoding_flag) {
      // 引用refIdxCol 引用的字段与当前宏块具有相同的奇偶校验
      if ((colPic->m_picture_coded_type == TOP_FIELD && CurrMbAddr % 2 == 0) ||
          (colPic->m_picture_coded_type == BOTTOM_FIELD && CurrMbAddr % 2 == 1))
        refIdxL0_temp = refIdxL0Frm << 1;
      // 引用refIdxCol 引用的字段具有与当前宏块相反的奇偶校验
      else
        refIdxL0_temp = (refIdxL0Frm << 1) + 1;
    }
    // 引用refPicCol 的当前参考图片列表 RefPicList0 中的最低值参考索引 refIdxL0
    else
      refIdxL0_temp = refIdxL0Frm;

  } else if (vertMvScale == H264_VERT_MV_SCALE_Frm_To_Fld) {
    // 引用refPicCol的当前参考图片列表RefPicList0中的最低值参考索引，RefPicList0 应包含 refPicCol
    if (m_mbs[mbAddrCol].field_pic_flag == 0)
      refIdxL0_temp = refIdxL0Frm << 1;
    else {
      // 引用refPicCol的当前参考图片列表RefPicList0中的最低值参考索引，该索引参考与当前图片 CurrPic 具有相同奇偶校验的 refPicCol 字段
      if (colPic->m_picture_coded_type == this->m_picture_coded_type)
        refIdxL0_temp = refIdxL0Frm;
    }

  } else if (vertMvScale == H264_VERT_MV_SCALE_Fld_To_Frm)
    refIdxL0_temp = refIdxL0Frm;

  /* NOTE: 当在包含共置宏块的图片的解码过程中引用该解码参考图片时，该解码参考图片被标记为“用于短期参考”，可能已被修改为被标记为“用于长期参考”。参考之前被用于使用当前宏块的直接预测模式进行帧间预测的参考。 */
  return refIdxL0_temp;
}

//8.2.1 Decoding process for picture order count (8-2)
/* 两张图片相差的距离（即两图片计数器的差值） */
int PictureBase::DiffPicOrderCnt(PictureBase *picA, PictureBase *picB) {
  return picOrderCntFunc(picA) - picOrderCntFunc(picB);
}
