#include "Common.hpp"
#include "MacroBlock.hpp"
#include "Nalu.hpp"
#include "PictureBase.hpp"
#include "Type.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>

//Table 8-7 – Specification of PicCodingStruct( X )
#define FLD 0
#define FRM 1
#define AFRM 2

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
    // 宏块、子宏块分区运动向量计数 subMvCnt
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

      /* 1. 对每个宏块分区或子宏块分区，计算其运动向量和参考帧索引 */
      RET(derivation_motion_vector_components_and_reference_indices(
          mbPartIdx, subMbPartIdx, refIdxL0, refIdxL1, mvL0, mvL1, mvCL0, mvCL1,
          subMvCnt, predFlagL0, predFlagL1, refPicL0, refPicL1));

      /* 2. 变量MvCnt 增加subMvCnt */
      MvCnt += subMvCnt;

      /* 3. 当P Slice存在加权预测 或 B Slice使用加权双向预测时，计算预测权重 */
      //加权预测变量 logWDC、w0C、w1C、o0C、o1C，其中 C 被 L 替换
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

      // 4. 计算帧间预测样本，通过运动向量和参考帧，该函数将使用这些信息来生成预测的像素值
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

      /* 为了在解码过程中稍后调用的变量的导出过程中使用，进行以下分配： */
      mb.m_MvL0[mbPartIdx][subMbPartIdx][0] = mvL0[0];
      mb.m_MvL0[mbPartIdx][subMbPartIdx][1] = mvL0[1];

      mb.m_MvL1[mbPartIdx][subMbPartIdx][0] = mvL1[0];
      mb.m_MvL1[mbPartIdx][subMbPartIdx][1] = mvL1[1];

      mb.m_RefIdxL0[mbPartIdx] = refIdxL0;
      mb.m_RefIdxL1[mbPartIdx] = refIdxL1;

      mb.m_PredFlagL0[mbPartIdx] = predFlagL0;
      mb.m_PredFlagL1[mbPartIdx] = predFlagL1;

      // 5. 通过将宏块或子宏块分区预测样本放置在它们在宏块中的正确相对位置来形成宏块预测
      // 帧宏块,场宏块偏移
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

      /* 完成该宏块解码 */
      mb.m_isDecoded[mbPartIdx][subMbPartIdx] = true;
    }
  }

  return 0;
}

// 8.4.2 Decoding process for Inter prediction samples
/* 输入： 
   * – 宏块分区 mbPartIdx， 
   * – 子宏块分区 subMbPartIdx， 
   * – 指定亮度和色度分区宽度和高度的变量（如果可用）、partWidth、partHeight、partWidthC（如果可用）和 partHeightC（如果可用） )， 
   * – 亮度运动向量 mvL0 和 mvL1，以及当 ChromaArrayType 不等于 0 色度运动向量 mvCL0 和 mvCL1 时， 
   * – 参考索引 refIdxL0 和 refIdxL1， 
   * – 预测列表利用标志 predFlagL0 和 predFlagL1， 
   * – 加权预测变量 logWDC，w0C， w1C、o0C、o1C，其中 C 被 L 替换，并且当 ChromaArrayType 不等于 0 时，Cb 和 Cr。
 * 输出：帧间预测样本predPart，它们是预测亮度样本的(partWidth)x(partHeight)数组predPartL，并且当ChromaArrayType不等于0时，预测的两个(partWidthC)x(partHeightC)数组predPartCb、predPartCr色度样本，每个色度分量 Cb 和 Cr 各一个。 */
int PictureBase::decoding_inter_prediction_samples(
    int32_t mbPartIdx, int32_t subMbPartIdx, int32_t partWidth,
    int32_t partHeight, int32_t partWidthC, int32_t partHeightC, int32_t xAL,
    int32_t yAL, int32_t (&mvL0)[2], int32_t (&mvL1)[2], int32_t (&mvCL0)[2],
    int32_t (&mvCL1)[2], PictureBase *refPicL0, PictureBase *refPicL1,
    bool predFlagL0, bool predFlagL1, int32_t logWDL, int32_t w0L, int32_t w1L,
    int32_t o0L, int32_t o1L, int32_t logWDCb, int32_t w0Cb, int32_t w1Cb,
    int32_t o0Cb, int32_t o1Cb, int32_t logWDCr, int32_t w0Cr, int32_t w1Cr,
    int32_t o0Cr, int32_t o1Cr, uint8_t *predPartL, uint8_t *predPartCb,
    uint8_t *predPartCr) {

  /* 预测亮度样本值的 (partWidth)x(partHeight) 数组 */
  /* 最大为16x16 = 256 , 最小为4x4 = 16 */
  uint8_t predPartL0L[256] = {0}, predPartL1L[256] = {0};
  /* predPartL0Cb、predPartL1Cb、predPartL0Cr 和 predPartL1Cr 为预测色度样本值的 (partWidthC)x(partHeightC) 数组 */
  uint8_t predPartL0Cb[256] = {0}, predPartL1Cb[256] = {0},
          predPartL0Cr[256] = {0}, predPartL1Cr[256] = {0};
  int ret = -1;

  /* 对于在变量 predFlagLX、RefPicListX、refIdxLX、refPicLX、predPartLX 中用 L0 或 L1 替换 LX，指定以下内容: 
   * - 当 predFlagLX 等于 1 时，以下情况适用：*/
  if (predFlagL0) {
    /* – 参考图像由亮度样本的有序二维数组 refPicLXL 组成，并且当 ChromaArrayType 不等于 0 时，色度样本的两个有序二维数组 refPicLXCb 和 refPicLXCr 是通过使用 refIdxLX 调用第 8.4.2.1 节中规定的过程来导出的和 RefPicListX 作为输入给出。 */
    /* TODO YangJing 为什么不需要调用？ <24-09-04 17:45:18> */
    //PictureBase *refPicL0 = NULL;
    // 8.4.2.1 Reference picture selection process
    //ret = Reference_picture_selection_process(refIdxL0, m_RefPicList0,
    //m_RefPicList0Length, refPicL0);

    /* 数组 predPartLXL 以及当 ChromaArrayType 不等于 0 时，数组 predPartLXCb 和 predPartLXCr 是通过调用第 8.4.2.2 节中指定的过程以及由 mbPartIdx\subMbPartIdx 指定的当前分区、运动矢量 mvLX、mvCLX（如果可用）和参考数组，其中 refPicLXL、refPicLXCb（如果可用）和 refPicLXCr（如果可用）作为输入给出。 */
    // 8.4.2.2 Fractional sample interpolation process (分数样本插值过程)
    ret = fractional_sample_interpolation(
        mbPartIdx, subMbPartIdx, partWidth, partHeight, partWidthC, partHeightC,
        xAL, yAL, mvL0, mvCL0, refPicL0, predPartL0L, predPartL0Cb,
        predPartL0Cr);
    RET(ret);
  }

  if (predFlagL1) {
    RET(fractional_sample_interpolation(
        mbPartIdx, subMbPartIdx, partWidth, partHeight, partWidthC, partHeightC,
        xAL, yAL, mvL1, mvCL1, refPicL1, predPartL1L, predPartL1Cb,
        predPartL1Cr));
  }

  /* 对于 C 被 L、Cb（如果可用）或 Cr（如果可用）替换，分量 C 的预测样本的数组 predPartC 是通过调用第 8.4.2.3 节中指定的过程以及 mbPartIdx 指定的当前分区和subMbPartIdx、预测利用标志 predFlagL0 和 predFlagL1、数组 predPartL0C 和 predPartL1C 以及作为输入给出的加权预测变量 logWDC、w0C、w1C、o0C、o1C。 */
  RET(weighted_sample_prediction(
      /* Input： */
      mbPartIdx, subMbPartIdx, predFlagL0, predFlagL1, partWidth, partHeight,
      partWidthC, partHeightC,
      //L
      logWDL, w0L, w1L, o0L, o1L,
      //Cb
      logWDCb, w0Cb, w1Cb, o0Cb, o1Cb,
      //Cr
      logWDCr, w0Cr, w1Cr, o0Cr, o1Cr, predPartL0L, predPartL0Cb, predPartL0Cr,
      predPartL1L, predPartL1Cb, predPartL1Cr,
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

  int32_t mvpL0[2] = {0}, mvpL1[2] = {0};
  bool listSuffixFlag = false, refPicLSetFlag = false;
  H264_MB_TYPE currSubMbType = MB_TYPE_NA;

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
    int32_t NumSubMbPart = 0, SubMbPartWidth = 0, SubMbPartHeight = 0;
    H264_MB_PART_PRED_MODE mb_pred_mode = MB_PRED_MODE_NA,
                           SubMbPredMode = MB_PRED_MODE_NA;

    RET(MacroBlock::MbPartPredMode(mb_type, mbPartIdx,
                                   mb.transform_size_8x8_flag, mb_pred_mode));
    RET(MacroBlock::SubMbPredMode(header->slice_type, mb.sub_mb_type[mbPartIdx],
                                  NumSubMbPart, SubMbPredMode, SubMbPartWidth,
                                  SubMbPartHeight));

    refIdxL0 = -1, predFlagL0 = false;
    if (mb_pred_mode == Pred_L0 || mb_pred_mode == BiPred ||
        SubMbPredMode == Pred_L0 || SubMbPredMode == BiPred)
      refIdxL0 = mb.ref_idx_l0[mbPartIdx], predFlagL0 = true;

    refIdxL1 = -1, predFlagL1 = false;
    if (mb_pred_mode == Pred_L1 || mb_pred_mode == BiPred ||
        SubMbPredMode == Pred_L1 || SubMbPredMode == BiPred)
      refIdxL1 = mb.ref_idx_l1[mbPartIdx], predFlagL1 = true;

    subMvCnt = predFlagL0 + predFlagL1;
    currSubMbType = (mb_type == B_8x8) ? sub_mb_type : MB_TYPE_NA;

    /* 亮度宏块预测 */
    if (predFlagL0) {
      refPicLSetFlag = true, refPicL0 = NULL;
      RET(reference_picture_selection(refIdxL0, m_RefPicList0,
                                      m_RefPicList0Length, refPicL0));

      listSuffixFlag = 0;
      RET(derivation_luma_motion_vector_prediction(
          mbPartIdx, subMbPartIdx, currSubMbType, listSuffixFlag, refIdxL0,
          mvpL0));
      mvL0[0] = mvpL0[0] + mb.mvd_l0[mbPartIdx][subMbPartIdx][0];
      mvL0[1] = mvpL0[1] + mb.mvd_l0[mbPartIdx][subMbPartIdx][1];
    }

    if (predFlagL1) {
      refPicLSetFlag = true, refPicL1 = NULL;
      RET(reference_picture_selection(refIdxL1, m_RefPicList1,
                                      m_RefPicList1Length, refPicL1));

      listSuffixFlag = true;
      RET(derivation_luma_motion_vector_prediction(
          mbPartIdx, subMbPartIdx, currSubMbType, listSuffixFlag, refIdxL1,
          mvpL1));
      mvL1[0] = mvpL1[0] + mb.mvd_l1[mbPartIdx][subMbPartIdx][0];
      mvL1[1] = mvpL1[1] + mb.mvd_l1[mbPartIdx][subMbPartIdx][1];
    }
  }

  /* 色度宏块预测 */
  if (refPicLSetFlag == false) {
    if (predFlagL0) {
      refPicLSetFlag = true, refPicL0 = NULL;
      RET(reference_picture_selection(refIdxL0, m_RefPicList0,
                                      m_RefPicList0Length, refPicL0));
    }
    if (predFlagL1) {
      refPicLSetFlag = true, refPicL1 = NULL;
      RET(reference_picture_selection(refIdxL1, m_RefPicList1,
                                      m_RefPicList1Length, refPicL1));
    }
  }

  if (ChromaArrayType != 0) {
    if (predFlagL0)
      RET(derivation_chroma_motion_vectors(ChromaArrayType, mvL0, refPicL0,
                                           mvCL0));
    if (predFlagL1)
      RET(derivation_chroma_motion_vectors(ChromaArrayType, mvL1, refPicL1,
                                           mvCL1));
  }

  return 0;
}

// 8.4.1.4 Derivation process for chroma motion vectors
// This process is only invoked when ChromaArrayType is not equal to 0.
int PictureBase::derivation_chroma_motion_vectors(int32_t ChromaArrayType,
                                                  int32_t mvLX[2],
                                                  PictureBase *refPic,
                                                  int32_t (&mvCLX)[2]) {

  if (ChromaArrayType != 1 || m_mbs[CurrMbAddr].mb_field_decoding_flag == 0)
    mvCLX[0] = mvLX[0], mvCLX[1] = mvLX[1];
  else {
    mvCLX[0] = mvLX[0];

    // Table 8-10 – Derivation of the vertical component of the chroma vector in field coding mode
    if (refPic &&
        refPic->m_picture_coded_type == PICTURE_CODED_TYPE_TOP_FIELD &&
        (this->m_picture_coded_type == PICTURE_CODED_TYPE_BOTTOM_FIELD ||
         mb_y % 2 == 1))
      mvCLX[1] = mvLX[1] + 2;
    else if (refPic &&
             refPic->m_picture_coded_type == PICTURE_CODED_TYPE_BOTTOM_FIELD &&
             (this->m_picture_coded_type == PICTURE_CODED_TYPE_TOP_FIELD ||
              mb_y % 2 == 0))
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
  subMvCnt = 1;

  refIdxL0 = 0;
  fill_n(m_mbs[CurrMbAddr].m_PredFlagL0, 4, 1);
  int32_t mbAddrN_A = 0, mbAddrN_B = 0;
  int32_t mvL0_A[2] = {0}, mvL0_B[2] = {0};
  int32_t refIdxL0_A = 0, refIdxL0_B = 0;

  /* 空引用对象 */
  int32_t nullref, nullref2[2];

  // 根据相邻分区运动矢量进行推导当前运动矢量
  RET(derivation_motion_data_of_neighbouring_partitions(
      0, 0, MB_TYPE_NA, false, mbAddrN_A, mvL0_A, refIdxL0_A, mbAddrN_B, mvL0_B,
      refIdxL0_B, nullref, nullref2, nullref));

  /* 相邻宏块不可用 */
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
  if (m_slice->slice_header->direct_spatial_mv_pred_flag) {
    RET(derivation_spatial_direct_luma_motion_vector_and_ref_index_prediction(
        mbPartIdx, subMbPartIdx, refIdxL0, refIdxL1, mvL0, mvL1, subMvCnt,
        predFlagL0, predFlagL1));
  }
  //时间直接运动预测模式
  else {
    RET(derivation_temporal_direct_luma_motion_vector_and_ref_index_prediction(
        mbPartIdx, subMbPartIdx, refIdxL0, refIdxL1, mvL0, mvL1, subMvCnt,
        predFlagL0, predFlagL1));
    subMvCnt = (subMbPartIdx == 0) ? 2 : 0;
  }

  return 0;
}

// 8.4.1.3 Derivation process for luma motion vector prediction
int PictureBase::derivation_luma_motion_vector_prediction(
    int32_t mbPartIdx, int32_t subMbPartIdx, H264_MB_TYPE currSubMbType,
    int32_t listSuffixFlag, int32_t refIdxLX, int32_t (&mvpLX)[2]) {

  const int32_t MbPartWidth = m_mbs[CurrMbAddr].MbPartWidth;
  const int32_t MbPartHeight = m_mbs[CurrMbAddr].MbPartHeight;

  int32_t mbAddrN_A = 0, mbAddrN_B = 0, mbAddrN_C = 0;
  int32_t mvLXN_A[2] = {0}, mvLXN_B[2] = {0}, mvLXN_C[2] = {0};
  int32_t refIdxLXN_A = 0, refIdxLXN_B = 0, refIdxLXN_C = 0;

  // 根据相邻分区运动矢量进行推导当前运动矢量
  RET(derivation_motion_data_of_neighbouring_partitions(
      mbPartIdx, subMbPartIdx, currSubMbType, listSuffixFlag, mbAddrN_A,
      mvLXN_A, refIdxLXN_A, mbAddrN_B, mvLXN_B, refIdxLXN_B, mbAddrN_C, mvLXN_C,
      refIdxLXN_C));

  if (MbPartWidth == 16 && MbPartHeight == 8 && mbPartIdx == 0 &&
      refIdxLXN_B == refIdxLX)
    mvpLX[0] = mvLXN_B[0], mvpLX[1] = mvLXN_B[1];
  else if (MbPartWidth == 16 && MbPartHeight == 8 && mbPartIdx == 1 &&
           refIdxLXN_A == refIdxLX)
    mvpLX[0] = mvLXN_A[0], mvpLX[1] = mvLXN_A[1];
  else if (MbPartWidth == 8 && MbPartHeight == 16 && mbPartIdx == 0 &&
           refIdxLXN_A == refIdxLX)
    mvpLX[0] = mvLXN_A[0], mvpLX[1] = mvLXN_A[1];
  else if (MbPartWidth == 8 && MbPartHeight == 16 && mbPartIdx == 1 &&
           refIdxLXN_C == refIdxLX)
    mvpLX[0] = mvLXN_C[0], mvpLX[1] = mvLXN_C[1];
  //中值预测
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
  /* 当分区 mbAddrB\mbPartIdxB\subMbPartIdxB 和 mbAddrC\mbPartIdxC\subMbPartIdxC 都不可用且 mbAddrA\mbPartIdxA\subMbPartIdxA 可用时， */
  if (mbAddrN_B < 0 && mbAddrN_C < 0 && mbAddrN_A >= 0) {
    mvLXN_B[0] = mvLXN_A[0];
    mvLXN_B[1] = mvLXN_A[1];
    mvLXN_C[0] = mvLXN_A[0];
    mvLXN_C[1] = mvLXN_A[1];
    refIdxLXN_B = refIdxLXN_A;
    refIdxLXN_C = refIdxLXN_A;
  }

  if (refIdxLXN_A == refIdxLX && refIdxLXN_B != refIdxLX &&
      refIdxLXN_C != refIdxLX)
    mvpLX[0] = mvLXN_A[0], mvpLX[1] = mvLXN_A[1];
  else if (refIdxLXN_A != refIdxLX && refIdxLXN_B == refIdxLX &&
           refIdxLXN_C != refIdxLX)
    mvpLX[0] = mvLXN_B[0], mvpLX[1] = mvLXN_B[1];
  else if (refIdxLXN_A != refIdxLX && refIdxLXN_B != refIdxLX &&
           refIdxLXN_C == refIdxLX)
    mvpLX[0] = mvLXN_C[0], mvpLX[1] = mvLXN_C[1];
  else {
    int32_t MIN0 = MIN(mvLXN_A[0], MIN(mvLXN_B[0], mvLXN_C[0]));
    int32_t MAX0 = MAX(mvLXN_A[0], MAX(mvLXN_B[0], mvLXN_C[0]));

    int32_t MIN1 = MIN(mvLXN_A[1], MIN(mvLXN_B[1], mvLXN_C[1]));
    int32_t MAX1 = MAX(mvLXN_A[1], MAX(mvLXN_B[1], mvLXN_C[1]));

    mvpLX[0] = mvLXN_A[0] + mvLXN_B[0] + mvLXN_C[0] - MIN0 - MAX0;
    mvpLX[1] = mvLXN_A[1] + mvLXN_B[1] + mvLXN_C[1] - MIN1 - MAX1;
  }
  return 0;
}

// 8.4.1.2.1 Derivation process for the co-located 4x4 sub-macroblock partitions
// 共置 4x4 子宏块分区的推导过程
int PictureBase::derivation_the_coLocated_4x4_sub_macroblock_partitions(
    int32_t mbPartIdx, int32_t subMbPartIdx, PictureBase *&colPic,
    int32_t &mbAddrCol, int32_t (&mvCol)[2], int32_t &refIdxCol,
    int32_t &vertMvScale) {

  const SliceHeader *header = m_slice->slice_header;
  Frame *refList1_0 = m_RefPicList1[0];

  /* 当RefPicList1[0]是帧或互补字段对时，令firstRefPicL1Top和firstRefPicL1Bottom分别为RefPicList1[0]的顶字段和底字段 */
  PictureBase *firstRefPicL1Top = NULL, *firstRefPicL1Bottom = NULL;
  int32_t topAbsDiffPOC = 0, bottomAbsDiffPOC = 0;
  if (refList1_0->m_picture_coded_type_marked_as_refrence ==
          PICTURE_CODED_TYPE_FRAME ||
      refList1_0->m_picture_coded_type_marked_as_refrence ==
          PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR) {
    firstRefPicL1Top = &refList1_0->m_picture_top_filed;
    firstRefPicL1Bottom = &refList1_0->m_picture_bottom_filed;

    topAbsDiffPOC = ABS(DiffPicOrderCnt(firstRefPicL1Top, this));
    bottomAbsDiffPOC = ABS(DiffPicOrderCnt(firstRefPicL1Bottom, this));
  }

  // Table 8-6 – Specification of the variable colPic
  // 共置宏块的图像，若当前图像是编码帧、当前宏块是帧宏块且互补字段被标记为“用于长期参考”时，互补字段对的图像顺序计数值对解码过程有影响。标记为“用于长期参考”的对是参考列表 1 中的第一张图片
  colPic = nullptr;

  if (header->field_pic_flag) { // 场图像
    if (refList1_0->m_is_decode_finished &&
        (refList1_0->m_picture_coded_type_marked_as_refrence ==
             PICTURE_CODED_TYPE_TOP_FIELD ||
         refList1_0->m_picture_coded_type_marked_as_refrence ==
             PICTURE_CODED_TYPE_BOTTOM_FIELD)) {
      colPic = &refList1_0->m_picture_frame;
    } else {
      if (refList1_0->m_picture_coded_type_marked_as_refrence ==
          PICTURE_CODED_TYPE_TOP_FIELD)
        colPic = &refList1_0->m_picture_top_filed;
      else if (refList1_0->m_picture_coded_type_marked_as_refrence ==
               PICTURE_CODED_TYPE_BOTTOM_FIELD)
        colPic = &refList1_0->m_picture_bottom_filed;
    }
  } else { // 帧图像
    if (refList1_0->m_is_decode_finished &&
        refList1_0->m_picture_coded_type_marked_as_refrence ==
            PICTURE_CODED_TYPE_FRAME) {
      colPic = &refList1_0->m_picture_frame;
    } else if (refList1_0->m_picture_coded_type_marked_as_refrence ==
               PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR) {
      if (m_slice->slice_data->mb_field_decoding_flag == 0)
        colPic = (topAbsDiffPOC < bottomAbsDiffPOC) ? firstRefPicL1Top
                                                    : firstRefPicL1Bottom;
      else
        colPic = (CurrMbAddr & 1) ? firstRefPicL1Bottom : firstRefPicL1Top;
    }
  }

  RET(colPic == nullptr);

  // Table 8-7 – Specification of PicCodingStruct( X )
  int32_t PicCodingStruct_CurrPic = FLD;
  if (header->field_pic_flag) //场图像
    PicCodingStruct_CurrPic = FLD;
  else { //帧图像
    if (header->m_sps->mb_adaptive_frame_field_flag == 0)
      PicCodingStruct_CurrPic = FRM;
    else //MBAFF
      PicCodingStruct_CurrPic = AFRM;
  }

  // Table 8-7 – Specification of PicCodingStruct( X )
  int32_t PicCodingStruct_colPic = FLD;
  if (colPic->m_slice->slice_header->field_pic_flag) //场图像
    PicCodingStruct_colPic = FLD;
  else { //帧图像
    if (colPic->m_slice->slice_header->m_sps->mb_adaptive_frame_field_flag == 0)
      PicCodingStruct_colPic = FRM;
    else //MBAFF
      PicCodingStruct_colPic = AFRM;
  }

  /* CurrPic 和 colPic 图像编码类型不可能是（FRM，AFRM）或（AFRM，FRM），因为这些图像编码类型必须由 IDR 图像分隔。 */
  if ((PicCodingStruct_CurrPic == FRM && PicCodingStruct_colPic == AFRM) ||
      (PicCodingStruct_CurrPic == AFRM && PicCodingStruct_colPic == FRM))
    RET(-1);

  int32_t luma4x4BlkIdx = 5 * mbPartIdx;
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
    if (PicCodingStruct_colPic == FLD) {
      mbAddrCol = CurrMbAddr;
      yM = yCol, vertMvScale = H264_VERT_MV_SCALE_One_To_One;
    }

    else if (PicCodingStruct_colPic == FRM) {
      mbAddrCol = 2 * PicWidthInMbs * (CurrMbAddr / PicWidthInMbs) +
                  (CurrMbAddr % PicWidthInMbs) + PicWidthInMbs * (yCol / 8);
      yM = (2 * yCol) % 16, vertMvScale = H264_VERT_MV_SCALE_Frm_To_Fld;
    }

    else if (PicCodingStruct_colPic == AFRM) {
      if (colPic->m_mbs[2 * CurrMbAddr].mb_field_decoding_flag == 0) {
        mbAddrCol = 2 * CurrMbAddr + this->m_mbs[CurrMbAddr].bottom_field_flag;
        yM = yCol, vertMvScale = H264_VERT_MV_SCALE_One_To_One;
      } else {
        mbAddrCol = 2 * CurrMbAddr + (yCol / 8);
        yM = (2 * yCol) % 16, vertMvScale = H264_VERT_MV_SCALE_Frm_To_Fld;
      }
    }
  }

  else if (PicCodingStruct_CurrPic == FRM) {
    if (PicCodingStruct_colPic == FLD) {
      mbAddrCol = PicWidthInMbs * (CurrMbAddr / (2 * PicWidthInMbs)) +
                  (CurrMbAddr % PicWidthInMbs);
      yM = 8 * ((CurrMbAddr / PicWidthInMbs) % 2) + 4 * (yCol / 8);
      vertMvScale = H264_VERT_MV_SCALE_Fld_To_Frm;
    } else if (PicCodingStruct_colPic == FRM) {
      mbAddrCol = CurrMbAddr;
      yM = yCol, vertMvScale = H264_VERT_MV_SCALE_One_To_One;
    }
  } else if (PicCodingStruct_CurrPic == AFRM) {
    if (PicCodingStruct_colPic == FLD) {
      mbAddrCol = CurrMbAddr / 2;
      if (m_slice->slice_data->mb_field_decoding_flag == 0)
        yM = 8 * (CurrMbAddr % 2) + 4 * (yCol / 8),
        vertMvScale = H264_VERT_MV_SCALE_Fld_To_Frm;
      else
        yM = yCol, vertMvScale = H264_VERT_MV_SCALE_One_To_One;
    } else if (PicCodingStruct_colPic == AFRM) {
      if (m_slice->slice_data->mb_field_decoding_flag == 0) {
        if (colPic->m_mbs[CurrMbAddr].mb_field_decoding_flag == 0) {
          mbAddrCol = CurrMbAddr;
          yM = yCol, vertMvScale = H264_VERT_MV_SCALE_One_To_One;
        } else {
          mbAddrCol = 2 * (CurrMbAddr / 2) +
                      ((topAbsDiffPOC < bottomAbsDiffPOC) ? 0 : 1);
          yM = 8 * (CurrMbAddr % 2) + 4 * (yCol / 8);
          vertMvScale = H264_VERT_MV_SCALE_Fld_To_Frm;
        }
      } else {
        if (colPic->m_mbs[CurrMbAddr].mb_field_decoding_flag == 0) {
          mbAddrCol = 2 * (CurrMbAddr / 2) + (yCol / 8);
          yM = (2 * yCol) % 16, vertMvScale = H264_VERT_MV_SCALE_Frm_To_Fld;
        } else {
          mbAddrCol = CurrMbAddr;
          yM = yCol, vertMvScale = H264_VERT_MV_SCALE_One_To_One;
        }
      }
    }
  }

  /* mbTypeCol为图片colPic内地址为mbAddrCol的宏块类型 */
  H264_MB_TYPE mbTypeCol = colPic->m_mbs[mbAddrCol].m_name_of_mb_type;
  H264_MB_TYPE subMbTypeCol[4] = {MB_TYPE_NA, MB_TYPE_NA, MB_TYPE_NA,
                                  MB_TYPE_NA};
  if (mbTypeCol == P_8x8 || mbTypeCol == P_8x8ref0 || mbTypeCol == B_8x8) {
    // 令subMbTypeCol为图片colPic内地址为mbAddrCol的宏块的语法元素列表sub_mb_type
    subMbTypeCol[0] = colPic->m_mbs[mbAddrCol].m_name_of_sub_mb_type[0];
    subMbTypeCol[1] = colPic->m_mbs[mbAddrCol].m_name_of_sub_mb_type[1];
    subMbTypeCol[2] = colPic->m_mbs[mbAddrCol].m_name_of_sub_mb_type[2];
    subMbTypeCol[3] = colPic->m_mbs[mbAddrCol].m_name_of_sub_mb_type[3];
  }

  /* mbPartIdxCol为共位分区的宏块分区索引，subMbPartIdxCol为共位子宏块分区的子宏块分区索引 */
  int32_t mbPartIdxCol = 0, subMbPartIdxCol = 0;
  RET(derivation_macroblock_and_sub_macroblock_partition_indices(
      mbTypeCol, subMbTypeCol, xCol, yM, mbPartIdxCol, subMbPartIdxCol));

  refIdxCol = -1;
  // 宏块mbAddrCol以帧内宏块预测模式编码，则mvCol的两个分量被设置为等于0并且refIdxCol被设置为等于-1
  if (IS_INTRA_Prediction_Mode(colPic->m_mbs[mbAddrCol].m_mb_pred_mode))
    mvCol[0] = 0, mvCol[1] = 0, refIdxCol = -1;
  // 宏块mbAddrCol以场内宏块预测模式编码
  else {
    int32_t predFlagL0Col = colPic->m_mbs[mbAddrCol].m_PredFlagL0[mbPartIdxCol];

    if (predFlagL0Col) {
      mvCol[0] =
          colPic->m_mbs[mbAddrCol].m_MvL0[mbPartIdxCol][subMbPartIdxCol][0];
      mvCol[1] =
          colPic->m_mbs[mbAddrCol].m_MvL0[mbPartIdxCol][subMbPartIdxCol][1];
      refIdxCol = colPic->m_mbs[mbAddrCol].m_RefIdxL0[mbPartIdxCol];
    } else {
      mvCol[0] =
          colPic->m_mbs[mbAddrCol].m_MvL1[mbPartIdxCol][subMbPartIdxCol][0];
      mvCol[1] =
          colPic->m_mbs[mbAddrCol].m_MvL1[mbPartIdxCol][subMbPartIdxCol][1];
      refIdxCol = colPic->m_mbs[mbAddrCol].m_RefIdxL1[mbPartIdxCol];
    }
  }

  return 0;
}

// 8.4.1.2.2 Derivation process for spatial direct luma motion vector and reference index prediction mode
//空间直接运动矢量预测
int PictureBase::
    derivation_spatial_direct_luma_motion_vector_and_ref_index_prediction(
        int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
        int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
        int32_t &subMvCnt, bool &predFlagL0, bool &predFlagL1) {

  H264_MB_TYPE currSubMbType =
      m_mbs[CurrMbAddr].m_name_of_sub_mb_type[mbPartIdx];

  /* NOTE: 对于宏块的所有 4x4 子宏块分区，运动矢量 mvL0N、mvL1N 和参考索引 refIdxL0N、refIdxL1N 是相同的 */

  int32_t mbAddrA = 0, mbAddrB = 0, mbAddrC = 0;
  int32_t mvL0A[2] = {0}, mvL0B[2] = {0}, mvL0C[2] = {0}, mvL1A[2] = {0},
          mvL1B[2] = {0}, mvL1C[2] = {0};
  int32_t refIdxL0A = 0, refIdxL0B = 0, refIdxL0C = 0, refIdxL1A = 0,
          refIdxL1B = 0, refIdxL1C = 0;
  RET(derivation_motion_data_of_neighbouring_partitions(
      0, 0, currSubMbType, false, mbAddrA, mvL0A, refIdxL0A, mbAddrB, mvL0B,
      refIdxL0B, mbAddrC, mvL0C, refIdxL0C));
  RET(derivation_motion_data_of_neighbouring_partitions(
      0, 0, currSubMbType, true, mbAddrA, mvL1A, refIdxL1A, mbAddrB, mvL1B,
      refIdxL1B, mbAddrC, mvL1C, refIdxL1C));

// define for (8-187)
#define MinPositive(x, y)                                                      \
  ((x) >= 0 && (y) >= 0) ? (MIN((x), (y))) : (MAX((x), (y)))
  refIdxL0 = MinPositive(refIdxL0A, MinPositive(refIdxL0B, refIdxL0C));
  refIdxL1 = MinPositive(refIdxL1A, MinPositive(refIdxL1B, refIdxL1C));
#undef MinPositive

  //直接零预测标志
  bool directZeroPredictionFlag = false;
  if (refIdxL0 < 0 && refIdxL1 < 0)
    refIdxL0 = 0, refIdxL1 = 0, directZeroPredictionFlag = true;

  PictureBase *colPic = nullptr;
  int32_t mvCol[2] = {0}, refIdxCol = 0, mbAddrCol = 0, vertMvScale = 0;
  RET(derivation_the_coLocated_4x4_sub_macroblock_partitions(
      mbPartIdx, subMbPartIdx, colPic, mbAddrCol, mvCol, refIdxCol,
      vertMvScale));

  /* TODO YangJing 同位宏块是什么？ <24-10-11 04:42:13> */

  bool colZeroFlag = false;
  /* 首个后参考帧，当前被标记为“用于短期参考” */
  if (m_RefPicList1[0]->reference_marked_type ==
          PICTURE_MARKED_AS_used_short_ref &&
      refIdxCol == 0) {
    if (m_mbs[mbAddrCol].mb_field_decoding_flag == 0 &&
        (mvCol[0] >= -1 && mvCol[0] <= 1) && (mvCol[1] >= -1 && mvCol[1] <= 1))
      colZeroFlag = true;
    else if ((m_mbs[mbAddrCol].mb_field_decoding_flag &&
              (mvCol[0] >= -1 && mvCol[0] <= 1) &&
              (mvCol[1] >= -1 && mvCol[1] <= 1)))
      colZeroFlag = true;
  }

  if (directZeroPredictionFlag || refIdxL0 < 0 ||
      (refIdxL0 == 0 && colZeroFlag))
    mvL0[0] = 0, mvL0[1] = 0;
  else
    //NOTE:该函数返回的运动矢量 mvLX 对于调用该过程的宏块的所有 4x4 子宏块分区是相同的
    RET(derivation_luma_motion_vector_prediction(0, 0, currSubMbType, false,
                                                 refIdxL0, mvL0));

  if (directZeroPredictionFlag || refIdxL1 < 0 ||
      (refIdxL1 == 0 && colZeroFlag))
    mvL1[0] = 0, mvL1[1] = 0;
  else
    RET(derivation_luma_motion_vector_prediction(0, 0, currSubMbType, true,
                                                 refIdxL1, mvL1));

  //Table 8-9 – Assignment of prediction utilization flags
  if (refIdxL0 >= 0 && refIdxL1 >= 0)
    predFlagL0 = true, predFlagL1 = true;
  else if (refIdxL0 >= 0 && refIdxL1 < 0)
    predFlagL0 = true, predFlagL1 = false;
  else if (refIdxL0 < 0 && refIdxL1 >= 0)
    predFlagL0 = false, predFlagL1 = true;

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
  const SliceHeader *slice_header = m_slice->slice_header;

  PictureBase *colPic = nullptr;
  int32_t mvCol[2] = {0}, refIdxCol = 0, mbAddrCol = 0, vertMvScale = 0;
  RET(derivation_the_coLocated_4x4_sub_macroblock_partitions(
      mbPartIdx, subMbPartIdx, colPic, mbAddrCol, mvCol, refIdxCol,
      vertMvScale));

  refIdxL0 = 0, refIdxL1 = 0;
  if (refIdxCol >= 0)
    refIdxL0 = mapColToList0(refIdxCol, colPic, mbAddrCol, vertMvScale,
                             slice_header->field_pic_flag);

  if (vertMvScale == H264_VERT_MV_SCALE_Frm_To_Fld)
    mvCol[1] = mvCol[1] / 2;
  else if (vertMvScale == H264_VERT_MV_SCALE_Fld_To_Frm)
    mvCol[1] = mvCol[1] * 2;
  //else if (vertMvScale == H264_VERT_MV_SCALE_One_To_One) { }

  PictureBase *pic0 = NULL, *pic1 = NULL, *currPicOrField = NULL;
  if (slice_header->field_pic_flag == 0 &&
      m_mbs[CurrMbAddr].mb_field_decoding_flag) {
    currPicOrField = (CurrMbAddr % 2) ? &(m_parent->m_picture_bottom_filed)
                                      : &(m_parent->m_picture_top_filed);
    pic1 = (CurrMbAddr % 2) ? &(m_RefPicList1[0]->m_picture_bottom_filed)
                            : &(m_RefPicList1[0]->m_picture_top_filed);

    if (refIdxL0 % 2)
      pic0 = (CurrMbAddr % 2)
                 ? &(m_RefPicList0[refIdxL0 / 2]->m_picture_top_filed)
                 : &(m_RefPicList0[refIdxL0 / 2]->m_picture_bottom_filed);
    else
      pic0 = (CurrMbAddr % 2)
                 ? &(m_RefPicList0[refIdxL0 / 2]->m_picture_bottom_filed)
                 : &(m_RefPicList0[refIdxL0 / 2]->m_picture_top_filed);

  } else {
    currPicOrField = &(m_parent->m_picture_frame);
    pic0 = &(m_RefPicList0[refIdxL0]->m_picture_frame);
    pic1 = &(m_RefPicList1[0]->m_picture_frame);
  }

  /* 当前宏块的每个4x4子宏块分区的两个运动向量mvL0和mvL1推导如下： */
  // 参考索引 refIdxL0 为长期参考图片
  if (m_RefPicList0[refIdxL0]->reference_marked_type ==
          PICTURE_MARKED_AS_used_long_ref ||
      DiffPicOrderCnt(pic1, pic0))
    mvL0[0] = mvCol[0], mvL0[1] = mvCol[1], mvL1[0] = 0, mvL1[1] = 0;

  // 运动矢量 mvL0、mvL1 被导出为同位子宏块分区的运动矢量 mvCol 的缩放版本（见图 8-2）
  else {
    /* TODO YangJing 好好看一下图8-2 – 时间直接模式运动矢量推断示例 -> page 161 <24-10-11 05:21:45> */
    int32_t tb = CLIP3(-128, 127, DiffPicOrderCnt(currPicOrField, pic0));
    int32_t td = CLIP3(-128, 127, DiffPicOrderCnt(pic1, pic0));

    int32_t tx = (16384 + ABS(td / 2)) / td;
    int32_t DistScaleFactor = CLIP3(-1024, 1023, (tb * tx + 32) >> 6);

    mvL0[0] = (DistScaleFactor * mvCol[0] + 128) >> 8;
    mvL0[1] = (DistScaleFactor * mvCol[1] + 128) >> 8;
    mvL1[0] = mvL0[0] - mvCol[0];
    mvL1[1] = mvL0[1] - mvCol[1];
  }

  predFlagL0 = predFlagL1 = true;

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

  if (mb_type_ == MB_TYPE_NA) RET(-1);
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
 * 输出: 加权预测变量 logWDC、w0C、w1C、o0C、o1C，其中 C 被 L 替换，并且当 ChromaArrayType 不等于 0 时，为 Cb 和 Cr。*/
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
  if (weighted_bipred_idc == 2 && slice_type == SLICE_B && predFlagL0 &&
      predFlagL1)
    implicitModeFlag = true, explicitModeFlag = false;
  else if (weighted_bipred_idc == 1 && slice_type == SLICE_B &&
           (predFlagL0 + predFlagL1))
    implicitModeFlag = false, explicitModeFlag = true;
  else if (weighted_pred_flag == true &&
           (slice_type == SLICE_P || slice_type == SLICE_SP) && predFlagL0)
    implicitModeFlag = false, explicitModeFlag = true;

  /* 将C替换为L，当ChromaArrayType不等于0、Cb和Cr时，变量logWDC、w0C、w1C、o0C、o1C推导如下：
   * – 如果implicitModeFlag等于1，则使用隐式模式加权预测，如下所示：*/
  if (implicitModeFlag) {
    logWDL = 5, o0L = o1L = 0;

    if (ChromaArrayType != 0)
      logWDCb = logWDCr = 5, o0Cb = o1Cb = o0Cr = o1Cr = 0;

    /* w0C 和 w1C 是按照以下顺序步骤中指定的导出的：
     * 1. 变量 currPicOrField、pic0 和 pic1 的推导如下： */
    PictureBase *currPicOrField = nullptr, *pic0 = nullptr, *pic1 = nullptr;

    /* 帧图像，但是场宏块(一般是帧图像中，为了应对复杂场景而使用场宏块编码） */
    if (header->field_pic_flag == 0 &&
        m_mbs[CurrMbAddr].mb_field_decoding_flag) {
      //* a.currPicOrField 是当前图片CurrPic 中与当前宏块具有相同奇偶性的字段。
      if (CurrMbAddr % 2 == 0)
        currPicOrField = &(m_parent->m_picture_top_filed);
      else
        currPicOrField = &(m_parent->m_picture_bottom_filed);

      /* b.变量 pic0 的推导如下： 
            * – 如果 refIdxL0 % 2 等于 0（参考帧索引为偶数），则 pic0 是 RefPicList0[ refIdxL0 / 2 ] 中与当前宏块具有相同奇偶校验的字段。  */
      if (refIdxL0 % 2 == 0) {
        if (CurrMbAddr % 2 == 0)
          pic0 = &(m_RefPicList0[refIdxL0 / 2]->m_picture_top_filed);
        else
          pic0 = &(m_RefPicList0[refIdxL0 / 2]->m_picture_bottom_filed);

        /* – 否则（refIdxL0 % 2 不等于 0）（参考帧索引为奇数），pic0 是 RefPicList0[ refIdxL0 / 2 ] 中与当前宏块具有相反奇偶校验的字段。   */
      } else {
        if (CurrMbAddr % 2 == 0)
          pic0 = &(m_RefPicList0[refIdxL0 / 2]->m_picture_bottom_filed);
        else
          pic0 = &(m_RefPicList0[refIdxL0 / 2]->m_picture_top_filed);
      }

      /* c.变量 pic1 的推导：同pic0 */
      if (refIdxL1 % 2 == 0) {
        if (CurrMbAddr % 2 == 0)
          pic1 = &(m_RefPicList1[refIdxL1 / 2]->m_picture_top_filed);
        else
          pic1 = &(m_RefPicList1[refIdxL1 / 2]->m_picture_bottom_filed);
      } else {
        if (CurrMbAddr % 2 == 0)
          pic1 = &(m_RefPicList1[refIdxL1 / 2]->m_picture_bottom_filed);
        else
          pic1 = &(m_RefPicList1[refIdxL1 / 2]->m_picture_top_filed);
      }

      //* – 否则（field_pic_flag等于1或当前宏块是帧宏块），currPicOrField是当前图片CurrPic，pic1是RefPicList1[refIdxL1]，pic0是RefPicList0[refIdxL0]。*/
      /* 场图像，或者帧宏块 */
    } else {
      currPicOrField = &(m_parent->m_picture_frame);
      pic0 = &(m_RefPicList0[refIdxL0]->m_picture_frame);
      pic1 = &(m_RefPicList1[refIdxL1]->m_picture_frame);
    }

    /* 2-1. 变量DistScaleFactor的推导如下： */
    //8.4.1.2.3 Derivation process for temporal direct luma motion vector and reference index prediction mode -> page 161
    int32_t tb = CLIP3(-128, 127, DiffPicOrderCnt(currPicOrField, pic0));
    int32_t td = CLIP3(-128, 127, DiffPicOrderCnt(pic1, pic0));
    if (td == 0) RET(-1);
    int32_t tx = (16384 + ABS(td / 2)) / td;
    int32_t DistScaleFactor = CLIP3(-1024, 1023, (tb * tx + 32) >> 6);

    /* 2-2. 变量w0C和w1C的推导如下： */
    //– 如果 DiffPicOrderCnt( pic1, pic0 ) 等于 0 或 pic1 和 pic0 之一或两者被标记为“用于长期参考”或 ( DistScaleFactor >> 2 ) < −64 或 ( DistScaleFactor >> 2 ) > 128 、w0C 和 w1C 推导为：
    if (DiffPicOrderCnt(pic1, pic0) == 0 ||
        pic0->reference_marked_type == PICTURE_MARKED_AS_used_long_ref ||
        pic1->reference_marked_type == PICTURE_MARKED_AS_used_long_ref ||
        (DistScaleFactor >> 2) < -64 || (DistScaleFactor >> 2) > 128) {
      w0L = w1L = 32;

      if (ChromaArrayType != 0) w0Cb = w1Cb = w0Cr = w1Cr = 32;

      /* 否则，变量 tb、td、tx 和 DistScaleFactor 分别使用方程 8-201、8-202、8-197 和 8-198 从 currPicOrField、pic0 和 pic1 的值导出，权重 w0C 和w1C 导出为 */
    } else {
      w0L = 64 - (DistScaleFactor >> 2);
      w1L = DistScaleFactor >> 2;

      if (ChromaArrayType != 0) {
        w0Cb = w0Cr = 64 - (DistScaleFactor >> 2);
        w1Cb = w1Cr = DistScaleFactor >> 2;
      }
    }

    /* 使用显式模式加权预测： */
  } else if (explicitModeFlag) {
    int32_t refIdxL0WP = 0, refIdxL1WP = 0;
    /* 宏块自适应帧场，场宏块(一般是帧图像中，为了应对复杂场景而使用场宏块编码） */
    if (header->MbaffFrameFlag && m_mbs[CurrMbAddr].mb_field_decoding_flag)
      refIdxL0WP = refIdxL0 >> 1, refIdxL1WP = refIdxL1 >> 1;
    /* 非宏块自适应帧场，帧宏块(一般是帧图像中，为了应对复杂场景而使用场宏块编码） */
    else
      refIdxL0WP = refIdxL0, refIdxL1WP = refIdxL1;

    // – 对于亮度样本，如果 C 等于 L
    logWDL = header->luma_log2_weight_denom;
    w0L = header->luma_weight_l0[refIdxL0WP];
    w1L = header->luma_weight_l1[refIdxL1WP];
    o0L = header->luma_offset_l0[refIdxL0WP] *
          (1 << (header->m_sps->BitDepthY - 8));
    o1L = header->luma_offset_l1[refIdxL1WP] *
          (1 << (header->m_sps->BitDepthY - 8));

    // 对于色度样本
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
  } else {
    /* 变量logWDC、w0C、w1C、o0C、o1C不用于当前宏块的重建过程*/
  }

  /* 当explicitModeFlag等于1并且predFlagL0和predFlagL1等于1时，对于C等于L，并且当ChromaArrayType不等于0时，Cb和Cr应遵守以下约束： */
  if (explicitModeFlag && predFlagL0 && predFlagL1) {
    int32_t max = (logWDL == 7) ? 127 : 128;
    if (-128 > (w0L + w1L) || (w0L + w1L) > max) RET(-1);

    if (ChromaArrayType != 0) {
      int32_t max = (logWDCb == 7) ? 127 : 128;
      if (-128 > (w0Cb + w1Cb) || (w0Cb + w1Cb) > max) RET(-1);

      max = (logWDCr == 7) ? 127 : 128;
      if (-128 > (w0Cr + w1Cr) || (w0Cr + w1Cr) > max) RET(-1);
    }
  }

  return 0;
}

// 8.4.2.1 Reference picture selection process
//  Page 164/186/812
int PictureBase::reference_picture_selection(int32_t refIdxLX,
                                             Frame *RefPicListX[16],
                                             int32_t RefPicListXLength,
                                             PictureBase *&refPic) {
  const SliceHeader *header = m_slice->slice_header;

  if (refIdxLX < 0 || refIdxLX >= 32) RET(-1);

  if (header->field_pic_flag) {
    // each entry of RefPicListX is a reference field or a field of a reference frame.
    for (int i = 0; i < RefPicListXLength; i++) {
      if (!(RefPicListX[i]->m_picture_coded_type_marked_as_refrence ==
                PICTURE_CODED_TYPE_TOP_FIELD ||
            RefPicListX[i]->m_picture_coded_type_marked_as_refrence ==
                PICTURE_CODED_TYPE_BOTTOM_FIELD))
        RET(-1);
    }
  } else {
    // each entry of RefPicListX is a reference frame or a complementary reference field pair.
    for (int i = 0; i < RefPicListXLength; i++) {
      if (!(RefPicListX[i]->m_picture_coded_type_marked_as_refrence ==
                PICTURE_CODED_TYPE_FRAME ||
            RefPicListX[i]->m_picture_coded_type_marked_as_refrence ==
                PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR))
        RET(-1);
    }
  }

  if (header->field_pic_flag) {
    if (RefPicListX[refIdxLX]->m_picture_coded_type_marked_as_refrence ==
        PICTURE_CODED_TYPE_TOP_FIELD)
      refPic = &(RefPicListX[refIdxLX]->m_picture_top_filed);
    else if (RefPicListX[refIdxLX]->m_picture_coded_type_marked_as_refrence ==
             PICTURE_CODED_TYPE_BOTTOM_FIELD)
      refPic = &(RefPicListX[refIdxLX]->m_picture_bottom_filed);
    else
      RET(-1);
  } else {
    if (m_mbs[CurrMbAddr].mb_field_decoding_flag == 0)
      refPic = &(RefPicListX[refIdxLX]->m_picture_frame);
    else {
      if (refIdxLX % 2 == 0) {
        if (mb_y % 2 == 0)
          refPic = &(RefPicListX[refIdxLX / 2]->m_picture_top_filed);
        else
          refPic = &(RefPicListX[refIdxLX / 2]->m_picture_bottom_filed);

      } else {
        if (mb_y % 2 == 0)
          refPic = &(RefPicListX[refIdxLX / 2]->m_picture_bottom_filed);
        else
          refPic = &(RefPicListX[refIdxLX / 2]->m_picture_top_filed);
      }
    }
  }

  //  if (header->m_sps->separate_colour_plane_flag == 0) {
  //    // FIXME: 8.7 Deblocking filter process
  //  } else {
  //    if (header->colour_plane_id == 0) {
  //
  //    } else if (header->colour_plane_id == 1) {
  //
  //    } else {
  //    }
  //  }

  if (refPic == nullptr) RET(-1);
  return 0;
}

// 8.4.2.2 Fractional sample interpolation process(分数样本插值过程)
/* 输入： – 由其分区索引 mbPartIdx 及其子宏块分区索引 subMbPartIdx 给出的当前分区， 
   * – 该分区的宽度和高度部分宽度、部分高度（以亮度样本单位表示）， 
   * – 给出的亮度运动向量 mvLX四分之一亮度样本单位， 
   * – 当 ChromaArrayType 不等于 0 时，色度运动向量 mvCLX 的水平精度为第 1(4*SubWidthC) 色度样本单位，精度为第 1(4*SubHeightC) 色度单位垂直样本单位，
   * – 选定的参考图片样本数组refPicLXL，以及当ChromaArrayType不等于0时，refPicLXCb和refPicLXCr。  
 * 输出： – 预测亮度样本值的 (partWidth)x(partHeight) 数组 predPartLXL， 
 * – 当 ChromaArrayType 不等于 0 时，预测色度样本值的两个 (partWidthC)x(partHeightC) 数组 predPartLXCb 和 predPartLXCr 。*/
int PictureBase::fractional_sample_interpolation(
    int32_t mbPartIdx, int32_t subMbPartIdx, int32_t partWidth,
    int32_t partHeight, int32_t partWidthC, int32_t partHeightC, int32_t xAL,
    int32_t yAL, int32_t (&mvLX)[2], int32_t (&mvCLX)[2], PictureBase *refPicLX,
    uint8_t *predPartLXL, uint8_t *predPartLXCb, uint8_t *predPartLXCr) {
  int ret = 0;

  /* 令 ( xAL, yAL ) 为由 mbPartIdx\subMbPartIdx 给出的当前分区的左上亮度样本的完整样本单位给出的位置，相对于给定的二维亮度样本数组的左上亮度样本位置。 */
  /* 令 ( xIntL, yIntL ) 为以全样本单位给出的亮度位置，( xFracL, yFracL ) 为以四分之一样本单位给出的偏移量。这些变量仅在本子句中使用，用于指定参考样本数组 refPicLXL、refPicLXCb（如果可用）和 refPicLXCr（如果可用）内的一般分数样本位置。 */

  /* 对于预测亮度样本数组 predPartLXL 内的每个亮度样本位置 (0 <= xL < partWidth, 0 <= yL < partHeight)，按照以下有序步骤指定导出相应的预测亮度样本值 predPartLXL[ xL, yL ]： 
   * 1. 变量 xIntL、yIntL、xFracL 和 yFracL 由以下公式导出： */
  for (int32_t yL = 0; yL < partHeight; yL++)
    for (int32_t xL = 0; xL < partWidth; xL++) {
      // 全样本单位的亮度位置（ xIntL, yIntL ）
      int32_t xIntL = xAL + (mvLX[0] >> 2) + xL;
      int32_t yIntL = yAL + (mvLX[1] >> 2) + yL;
      // 小数样本单位的亮度位置偏移（ xFracL, yFracL ）
      int32_t xFracL = mvLX[0] & 3;
      int32_t yFracL = mvLX[1] & 3;

      /* 预测亮度样本值 predPartLXL[ xL, yL ] 通过调用第 8.4.2.2.1 节中指定的过程（以 ( xIntL, yIntL )、( xFracL, yFracL ) 和 refPicLXL 作为输入给出）来导出。 */
      // 8.4.2.2.1 Luma sample interpolation process (Luma样本插值过程)
      uint8_t predPartLXL_xL_yL = 0;
      ret = luma_sample_interpolation(xIntL, yIntL, xFracL, yFracL, refPicLX,
                                      predPartLXL_xL_yL);
      RET(ret);
      predPartLXL[yL * partWidth + xL] = predPartLXL_xL_yL;
    }

  /* 当 ChromaArrayType 不等于 0 时，以下情况适用。  
   * 令 ( xIntC, yIntC ) 为以全样本单位给出的色度位置，( xFracC, yFracC ) 为以一 (4*SubWidthC) 个色度样本单位水平给出的偏移量和一 (4*SubHeightC)- 给出的偏移量垂直第 th 色度样本单位。这些变量仅在本子句中使用，用于指定参考样本数组 refPicLXCb 和 refPicLXCr 内的一般分数样本位置。  */

  /* 对于预测色度样本数组 predPartLXCb 和 predPartLXCr 内的每个色度样本位置 (0 <= xC < partWidthC, 0 <= yC < partHeightC)，相应的预测色度样本值 predPartLXCb[ xC, yC ] 和 predPartLXCr[ xC, yC ] 为按照以下顺序步骤指定导出： */
  if (m_slice->slice_header->m_sps->ChromaArrayType != 0) {
    int32_t xIntC = 0, yIntC = 0;
    int32_t xFracC = 0, yFracC = 0;
    int32_t isChromaCb = 1;

    for (int32_t yC = 0; yC < partHeightC; yC++) {
      for (int32_t xC = 0; xC < partWidthC; xC++) {

        /* 根据 ChromaArrayType，变量 xIntC、yIntC、xFracC 和 yFracC 的推导如下： */
        if (m_slice->slice_header->m_sps->ChromaArrayType == 1) {
          xIntC = (xAL / m_slice->slice_header->m_sps->SubWidthC) +
                  (mvCLX[0] >> 3) + xC;
          yIntC = (yAL / m_slice->slice_header->m_sps->SubHeightC) +
                  (mvCLX[1] >> 3) + yC;
          xFracC = mvCLX[0] & 7;
          yFracC = mvCLX[1] & 7;
        } else if (m_slice->slice_header->m_sps->ChromaArrayType == 2) {
          xIntC = (xAL / m_slice->slice_header->m_sps->SubWidthC) +
                  (mvCLX[0] >> 3) + xC;
          yIntC = (yAL / m_slice->slice_header->m_sps->SubHeightC) +
                  (mvCLX[1] >> 2) + yC;
          xFracC = mvCLX[0] & 7;
          yFracC = (mvCLX[1] & 3) << 1;
        } else {
          xIntC = xAL + (mvLX[0] >> 2) + xC;
          yIntC = yAL + (mvLX[1] >> 2) + yC;
          xFracC = (mvCLX[0] & 3);
          yFracC = (mvCLX[1] & 3);
        }

        /* 根据 ChromaArrayType，以下规则适用： */
        if (m_slice->slice_header->m_sps->ChromaArrayType != 3) {
          /* 预测样本值 predPartLXCb[ xC, yC ] 通过调用第 8.4.2.2.2 节中指定的过程（以 ( xIntC, yIntC )、( xFracC, yFracC ) 和 refPicLXCb 作为输入给出）来导出。 */
          uint8_t predPartLXCb_xC_yC = 0;
          isChromaCb = 1;
          // 8.4.2.2.2 Chroma sample interpolation process
          ret = chroma_sample_interpolation(xIntC, yIntC, xFracC, yFracC,
                                            refPicLX, isChromaCb,
                                            predPartLXCb_xC_yC);
          RET(ret);
          predPartLXCb[yC * partWidthC + xC] = predPartLXCb_xC_yC;

          /* 预测样本值 predPartLXCr[ xC, yC ] 通过调用第 8.4.2.2.2 节中指定的过程（以 ( xIntC, yIntC )、( xFracC, yFracC ) 和 refPicLXCr 作为输入给出）来导出。 */
          uint8_t predPartLXCr_xC_yC = 0;
          isChromaCb = 0;
          // 8.4.2.2.2 Chroma sample interpolation process
          ret = chroma_sample_interpolation(xIntC, yIntC, xFracC, yFracC,
                                            refPicLX, isChromaCb,
                                            predPartLXCr_xC_yC);
          RET(ret);
          predPartLXCr[yC * partWidthC + xC] = predPartLXCr_xC_yC;

        } else {
          /* 此时没有UV色度，即YUV400 */

          /* 预测样本值 predPartLXCb[ xC, yC ] 通过调用第 8.4.2.2.1 节中指定的过程（以 ( xIntC, yIntC )、( xFracC, yFracC ) 和 refPicLXCb 作为输入给出）来导出。 */
          uint8_t predPartLXCb_xC_yC = 0;
          // 8.4.2.2.1 Luma sample interpolation process
          ret = luma_sample_interpolation(xIntC, yIntC, xFracC, yFracC,
                                          refPicLX, predPartLXCb_xC_yC);
          RET(ret);
          predPartLXCb[yC * partWidthC + xC] = predPartLXCb_xC_yC;

          /* 预测样本值 predPartLXCr[ xC, yC ] 通过调用第 8.4.2.2.1 节中指定的过程（以 ( xIntC, yIntC )、( xFracC, yFracC ) 和 refPicLXCr 作为输入给出）来导出。 */
          uint8_t predPartLXCr_xC_yC = 0;
          // 8.4.2.2.1 Luma sample interpolation process
          ret = luma_sample_interpolation(xIntC, yIntC, xFracC, yFracC,
                                          refPicLX, predPartLXCr_xC_yC);
          RET(ret);
          predPartLXCr[yC * partWidthC + xC] = predPartLXCr_xC_yC;
        }
      }
    }
  }

  return 0;
}

// 8.4.2.2.1 Luma sample interpolation process(Luma样本插值过程)
/* 输入：– 全样本单位的亮度位置（ xIntL, yIntL ）， – 小数样本单位的亮度位置偏移（ xFracL, yFracL ）， – 亮度样本所选参考图片 refPicLXL 的数组。  
 * 输出: 预测的亮度样本值 predPartLXL[ xL, yL ]。*/
int PictureBase::luma_sample_interpolation(int32_t xIntL, int32_t yIntL,
                                           int32_t xFracL, int32_t yFracL,
                                           PictureBase *refPic,
                                           uint8_t &predPartLXL_xL_yL) {

  const SliceHeader *slice_header = m_slice->slice_header;
  const int32_t mb_field_decoding_flag =
      m_slice->slice_data->mb_field_decoding_flag;

  /* 变量 refPicHeightEffectiveL（有效参考图像亮度数组的高度）的推导如下： 
   * – 如果 MbaffFrameFlag 等于 0 或 mb_field_decoding_flag 等于 0，则 refPicHeightEffectiveL 设置为等于 PicHeightInSamplesL。  
   * – 否则（MbaffFrameFlag 等于 1 并且 mb_field_decoding_flag 等于 1），refPicHeightEffectiveL 设置为等于 PicHeightInSamplesL / 2。 */
  int32_t refPicHeightEffectiveL = 0;
  if (slice_header->MbaffFrameFlag == 0 || mb_field_decoding_flag == 0)
    refPicHeightEffectiveL = PicHeightInSamplesL;
  else
    refPicHeightEffectiveL = PicHeightInSamplesL / 2;

    /* 在给定数组 ref PiXL 内，亮度样本的推导如下： */
#define getLumaSample(xDZL, yDZL)                                              \
  refPic                                                                       \
      ->m_pic_buff_luma[CLIP3(0, refPicHeightEffectiveL - 1, yIntL + (yDZL)) * \
                            refPic->PicWidthInSamplesL +                       \
                        CLIP3(0, refPic->PicWidthInSamplesL - 1,               \
                              xIntL + (xDZL))]

  /* 给定全样本位置（xAL，yAL）至（xUL，yUL）处的亮度样本“A”至“U”，分数样本位置处的亮度样本“a”至“s”通过以下规则导出。 */
  int32_t A = getLumaSample(0, -2);
  int32_t B = getLumaSample(1, -2);
  int32_t C = getLumaSample(0, -1);
  int32_t D = getLumaSample(1, -1);
  int32_t E = getLumaSample(-2, 0);
  int32_t F = getLumaSample(-1, 0);
  int32_t G = getLumaSample(0, 0); // 坐标原点
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

  int32_t X11 = getLumaSample(-2, -2); // A所在的行与E所在的列的交点
  int32_t X12 = getLumaSample(-1, -2);
  int32_t X13 = getLumaSample(2, -2);
  int32_t X14 = getLumaSample(3, -2);

  int32_t X21 = getLumaSample(-2, -1);
  int32_t X22 = getLumaSample(-1, -1);
  int32_t X23 = getLumaSample(2, -1);
  int32_t X24 = getLumaSample(3, -1);

  int32_t X31 = getLumaSample(-2, 2);
  int32_t X32 = getLumaSample(-1, 2);
  int32_t X33 = getLumaSample(2, 2);
  int32_t X34 = getLumaSample(3, 2);

  int32_t X41 = getLumaSample(-2, 3);
  int32_t X42 = getLumaSample(-1, 3);
  int32_t X43 = getLumaSample(2, 3);
  int32_t X44 = getLumaSample(3, 3);

  /* 通过应用具有抽头值(1,-5,20,20,-5,1)的6-tap滤波器来导出半样本位置处的亮度预测值。四分之一样本位置处的亮度预测值是通过对全样本位置和半样本位置处的样本进行平均而得出的。每个分数位置的过程如下所述。*/
#define a_6_tap_filter(v1, v2, v3, v4, v5, v6)                                 \
  ((v1) - 5 * (v2) + 20 * (v3) + 20 * (v4) - 5 * (v5) + (v6))

  /* 通过首先将 6 抽头滤波器应用于水平方向上最接近的整数位置样本来计算表示为 b1 的中间值，从而导出标记为 b 的一半样本位置处的样本。通过首先将 6 抽头滤波器应用于垂直方向上最近的整数位置样本来计算表示为 h1 的中间值，从而导出标记为 h 的一半样本位置处的样本： */
  int32_t b1 = a_6_tap_filter(E, F, G, H, I, J);
  int32_t s1 = a_6_tap_filter(K, L, M, N, P, Q);
  int32_t h1 = a_6_tap_filter(A, C, G, M, R, T);
  int32_t m1 = a_6_tap_filter(B, D, H, N, S, U);

  /* 最终预测值 b 和 h 是使用以下公式得出的 */
  int32_t b = CLIP3(0, (1 << m_slice->slice_header->m_sps->BitDepthY) - 1,
                    (b1 + 16) >> 5);
  int32_t s = CLIP3(0, (1 << m_slice->slice_header->m_sps->BitDepthY) - 1,
                    (s1 + 16) >> 5);
  int32_t h = CLIP3(0, (1 << m_slice->slice_header->m_sps->BitDepthY) - 1,
                    (h1 + 16) >> 5);
  int32_t m = CLIP3(0, (1 << m_slice->slice_header->m_sps->BitDepthY) - 1,
                    (m1 + 16) >> 5);

  int32_t cc = a_6_tap_filter(X11, X21, E, K, X31, X41);
  int32_t dd = a_6_tap_filter(X12, X22, F, L, X32, X42);
  int32_t ee = a_6_tap_filter(X13, X23, I, P, X33, X43);
  int32_t ff = a_6_tap_filter(X14, X24, J, Q, X34, X44);

  /* 标记为 j 的半样本位置处的样本是通过首先计算表示为 j1 的中间值而得出的，方法是将 6 抽头滤波器应用于水平或垂直方向上最接近的半样本位置的中间值，因为它们会产生相同的结果： */
  int32_t j1 = a_6_tap_filter(cc, dd, h1, m1, ee, ff);
  //    int32_t j2 = a_6_tap_filter(aa, bb, b1, s1, gg, hh);

  /*  其中，表示为 aa、bb、gg、s1 和 hh 的中间值是通过以与 b1 的推导相同的方式水平应用 6 抽头滤波器而导出的，表示为 cc、dd、ee、m1 和 ff 的中间值是通过以下方式导出的：以与 h1 的推导相同的方式垂直应用 6 抽头滤波器。最终预测值 j 是使用以下公式得出的*/
  int32_t j = CLIP3(0, (1 << m_slice->slice_header->m_sps->BitDepthY) - 1,
                    (j1 + 512) >> 10);

  /* – 标记为 a、c、d、n、f、i、k 和 q 的四分之一样本位置的样本是通过对整数和半样本位置处的两个最近样本进行向上舍入平均而得出的*/
  int32_t a = (G + b + 1) >> 1;
  int32_t c = (H + b + 1) >> 1;
  int32_t d = (G + h + 1) >> 1;
  int32_t n = (M + h + 1) >> 1;
  int32_t f = (b + j + 1) >> 1;
  int32_t i = (h + j + 1) >> 1;
  int32_t k = (j + m + 1) >> 1;
  int32_t q = (j + s + 1) >> 1;

  /* 标记为 e、g、p 和 r 的四分之一样本位置处的样本是通过对对角线方向上一半样本位置处的两个最近样本进行向上舍入平均而得出的： */
  int32_t e = (b + h + 1) >> 1;
  int32_t g = (b + m + 1) >> 1;
  int32_t p = (h + s + 1) >> 1;
  int32_t r = (m + s + 1) >> 1;

#undef a_6_tap_filter
#undef getLumaSample

  /* 以小数样本单位表示的亮度位置偏移 ( xFracL, yFracL ) 指定将在全样本和小数样本位置处生成的亮度样本中的哪一个分配给预测的亮度样本值 predPartLXL[ xL, yL ]。此分配根据表 8-12 完成。 predPartLXL[ xL, yL ] 的值是输出。 */
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
/* 输入：– 全样本单位的色度位置 ( xIntC, yIntC )， – 小数样本单位的色度位置偏移 ( xFracC， yFracC )， – 来自所选参考图片 refPicLXC 的色度分量样本。  
 * 输出: 预测色度样本值predPartLXC[xC，yC]。*/
int PictureBase::chroma_sample_interpolation(int32_t xIntC, int32_t yIntC,
                                             int32_t xFracC, int32_t yFracC,
                                             PictureBase *refPic,
                                             int32_t isChromaCb,
                                             uint8_t &predPartLXC_xC_yC) {

  const SliceHeader *slice_header = m_slice->slice_header;

  /* 变量 refPicHeightEffectiveC 是有效参考图像色度数组的高度，其推导如下： */
  int32_t refPicHeightEffectiveC = 0;
  if (slice_header->MbaffFrameFlag == 0 ||
      m_slice->slice_data->mb_field_decoding_flag == 0)
    refPicHeightEffectiveC = PicHeightInSamplesC;
  else
    refPicHeightEffectiveC = PicHeightInSamplesC / 2;

  /* 等式8-262至8-269中指定的样本坐标用于生成预测色度样本值predPartLXC[xC,yC]。*/
  int32_t xAC = CLIP3(0, refPic->PicWidthInSamplesC - 1, xIntC);
  int32_t xBC = CLIP3(0, refPic->PicWidthInSamplesC - 1, xIntC + 1);
  int32_t xCC = CLIP3(0, refPic->PicWidthInSamplesC - 1, xIntC);
  int32_t xDC = CLIP3(0, refPic->PicWidthInSamplesC - 1, xIntC + 1);

  int32_t yAC = CLIP3(0, refPicHeightEffectiveC - 1, yIntC);
  int32_t yBC = CLIP3(0, refPicHeightEffectiveC - 1, yIntC);
  int32_t yCC = CLIP3(0, refPicHeightEffectiveC - 1, yIntC + 1);
  int32_t yDC = CLIP3(0, refPicHeightEffectiveC - 1, yIntC + 1);

  uint8_t *refPicLC_pic_buff_cbcr =
      (isChromaCb == 1) ? refPic->m_pic_buff_cb : refPic->m_pic_buff_cr;
  int32_t A = refPicLC_pic_buff_cbcr[yAC * refPic->PicWidthInSamplesC + xAC];
  int32_t B = refPicLC_pic_buff_cbcr[yBC * refPic->PicWidthInSamplesC + xBC];
  int32_t C = refPicLC_pic_buff_cbcr[yCC * refPic->PicWidthInSamplesC + xCC];
  int32_t D = refPicLC_pic_buff_cbcr[yDC * refPic->PicWidthInSamplesC + xDC];

  /* 给定等式 8-262 至 8-269 中指定的全样本位置处的色度样本 A、B、C 和 D，预测色度样本值 predPartLXC[ xC, yC ] 推导如下： */
  predPartLXC_xC_yC =
      ((8 - xFracC) * (8 - yFracC) * A + xFracC * (8 - yFracC) * B +
       (8 - xFracC) * yFracC * C + xFracC * yFracC * D + 32) >>
      6;

  return 0;
}

// 8.4.2.3 Weighted sample prediction process
/* 输入： – mbPartIdx：由分区索引给出的当前分区， – subMbPartIdx：子宏块分区索引， – predFlagL0 和 predFlagL1：预测列表利用率标志， – predPartLXL：(partWidth)x(partHeight) 数组预测亮度样本的数量（根据 predFlagL0 和 predFlagL1 将 LX 替换为 L0 或 L1）， – 当 ChromaArrayType 不等于 0 时，predPartLXCb 和 predPartLXCr：预测色度样本的 (partWidthC)x(partHeightC) 数组，每个数组对应一个色度分量 Cb 和 Cr（其中 LX 被替换为 L0 或 L1，具体取决于 predFlagL0 和 predFlagL1）， – 加权预测变量 logWDC、w0C、w1C、o0C、o1C，其中 C 被 L 替换，并且当 ChromaArrayType 不等于0、Cb 和 Cr。  
 * 输出： – predPartL：预测亮度样本的 (partWidth)x(partHeight) 数组， – 当 ChromaArrayType 不等于 0 时，predPartCb 和 predPartCr：预测色度样本的 (partWidthC)x(partHeightC) 数组，色度分量 Cb 和 Cr 各一个。*/
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

  int ret = 0;
  const SliceHeader *slice_header = m_slice->slice_header;
  uint32_t slice_type = slice_header->slice_type % 5;

  /* 对于 P 和 SP 切片中 predFlagL0 等于 1 的宏块或分区，以下情况适用： 
   * – 如果weighted_pred_flag 等于 0，则使用相同的输入和输出调用第 8.4.2.3.1 节中描述的默认加权样本预测过程如本条中描述的过程。  
   * — 否则（weighted_pred_flag等于1），使用与本节中描述的过程相同的输入和输出来调用第8.4.2.3.2节中描述的显式加权样本预测过程。 */
  if (predFlagL0 && (slice_type == SLICE_P || slice_type == SLICE_SP)) {
    if (m_slice->slice_header->m_pps->weighted_pred_flag == 0)
      // 8.4.2.3.1 Default weighted sample prediction process
      ret = default_weighted_sample_prediction(
          predFlagL0, predFlagL1, partWidth, partHeight, partWidthC,
          partHeightC, predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L,
          predPartL1Cb, predPartL1Cr, predPartL, predPartCb, predPartCr);
    else
      // 8.4.2.3.2 Weighted sample prediction process
      ret = weighted_sample_prediction_2(
          mbPartIdx, subMbPartIdx, predFlagL0, predFlagL1, partWidth,
          partHeight, partWidthC, partHeightC, logWDL, w0L, w1L, o0L, o1L,
          logWDCb, w0Cb, w1Cb, o0Cb, o1Cb, logWDCr, w0Cr, w1Cr, o0Cr, o1Cr,
          predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L, predPartL1Cb,
          predPartL1Cr, predPartL, predPartCb, predPartCr);
    RET(ret);

    /* 对于 B 切片中 predFlagL0 或 predFlagL1 等于 1 的宏块或分区，以下情况适用：
     * – 如果weighted_bipred_idc等于0，同P
     * — 否则，如果weighted_bipred_idc等于1，同P
     * – 否则（weighted_bipred_idc 等于 2），则适用： 
        * – 如果 predFlagL0 等于 1 并且 predFlagL1 等于 1，则使用相同的输入调用第 8.4.2.3.2 节中描述的隐式加权样本预测过程，并且输出如本节中描述的过程。  
        * – 否则（predFlagL0 或 predFlagL1 等于 1，但不是两者），则使用与本节中描述的过程相同的输入和输出来调用第 8.4.2.3.1 节中描述的默认加权样本预测过程。*/
  } else if ((predFlagL0 == 1 || predFlagL1 == 1) && slice_type == SLICE_B) {
    if (m_slice->slice_header->m_pps->weighted_bipred_idc == 0) {
      // 8.4.2.3.1 Default weighted sample prediction process
      ret = default_weighted_sample_prediction(
          predFlagL0, predFlagL1, partWidth, partHeight, partWidthC,
          partHeightC, predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L,
          predPartL1Cb, predPartL1Cr, predPartL, predPartCb, predPartCr);
    } else if (m_slice->slice_header->m_pps->weighted_bipred_idc == 1) {
      // 8.4.2.3.2 Weighted sample prediction process
      ret = weighted_sample_prediction_2(
          mbPartIdx, subMbPartIdx, predFlagL0, predFlagL1, partWidth,
          partHeight, partWidthC, partHeightC, logWDL, w0L, w1L, o0L, o1L,
          logWDCb, w0Cb, w1Cb, o0Cb, o1Cb, logWDCr, w0Cr, w1Cr, o0Cr, o1Cr,
          predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L, predPartL1Cb,
          predPartL1Cr, predPartL, predPartCb, predPartCr);
    } else {
      /* 双向预测 */
      if (predFlagL0 && predFlagL1)
        // 8.4.2.3.2 Weighted sample prediction process
        ret = weighted_sample_prediction_2(
            mbPartIdx, subMbPartIdx, predFlagL0, predFlagL1, partWidth,
            partHeight, partWidthC, partHeightC, logWDL, w0L, w1L, o0L, o1L,
            logWDCb, w0Cb, w1Cb, o0Cb, o1Cb, logWDCr, w0Cr, w1Cr, o0Cr, o1Cr,
            predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L, predPartL1Cb,
            predPartL1Cr, predPartL, predPartCb, predPartCr);
      /* 非双向预测 */
      else
        // 8.4.2.3.1 Default weighted sample prediction process
        ret = default_weighted_sample_prediction(
            predFlagL0, predFlagL1, partWidth, partHeight, partWidthC,
            partHeightC, predPartL0L, predPartL0Cb, predPartL0Cr, predPartL1L,
            predPartL1Cb, predPartL1Cr, predPartL, predPartCb, predPartCr);
    }
    RET(ret);
  }

  return 0;
}

// 8.4.2.3.2 Weighted sample prediction process
/* 输入、输出同weighted_sample_prediction() */
int PictureBase::weighted_sample_prediction_2(
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

  const uint32_t BitDepthY = m_slice->slice_header->m_sps->BitDepthY;
  const uint32_t BitDepthC = m_slice->slice_header->m_sps->BitDepthC;

  /* 根据导出预测块的可用组件，以下适用： 
   * – 如果导出亮度样本预测值 predPartL[ x, y ]，则以下适用，其中 C 设置等于 L，x 设置等于 0。 partWidth - 1，y 设置为等于 0..partHeight - 1，并且 Clip1( ) 被替换为 Clip1Y( )。  
   * – 否则，如果导出色度 Cb 分量样本预测值 predPartCb[ x, y ]，则以下情况适用于 C 设置等于 Cb、x 设置等于 0..partWidthC − 1、y 设置等于 0..partHeightC − 1，并且Clip1()被Clip1C()替代。  
   * – 否则（导出色度 Cr 分量样本预测值 predPartCr[ x, y ]），以下情况适用于 C 设置等于 Cr、x 设置等于 0..partWidthC − 1、y 设置等于 0..partHeightC − 1，并且Clip1()被Clip1C()替代。 */

  if (predFlagL0 && predFlagL1 == 0) {
    /* 如果 predFlagL0 等于 1 并且 predFlagL1 等于 0，则最终预测样本值 predPartC[ x, y ] 由下式得出： */
    for (int y = 0; y <= partHeight - 1; y++) {
      for (int x = 0; x <= partWidth - 1; x++) {
        /* 预测样本值的推导如下： */
        if (logWDL >= 1)
          predPartL[y * partWidth + x] =
              Clip1C(((predPartL0L[y * partWidth + x] * w0L +
                       h264_power2(logWDL - 1)) >>
                      logWDL) +
                         o0L,
                     BitDepthY);
        else
          predPartL[y * partWidth + x] =
              Clip1C(predPartL0L[y * partWidth + x] * w0L + o0L, BitDepthY);
      }
    }

    if (m_slice->slice_header->m_sps->ChromaArrayType != 0) {
      for (int y = 0; y <= partHeightC - 1; y++) {
        for (int x = 0; x <= partWidthC - 1; x++) {
          if (logWDCb >= 1)
            predPartCb[y * partWidthC + x] =
                Clip1C(((predPartL0Cb[y * partWidthC + x] * w0Cb +
                         h264_power2(logWDCb - 1)) >>
                        logWDCb) +
                           o0Cb,
                       BitDepthC);
          else
            predPartCb[y * partWidthC + x] = Clip1C(
                predPartL0Cb[y * partWidthC + x] * w0Cb + o0Cb, BitDepthC);

          if (logWDCr >= 1)
            predPartCr[y * partWidthC + x] =
                Clip1C(((predPartL0Cr[y * partWidthC + x] * w0Cr +
                         h264_power2(logWDCr - 1)) >>
                        logWDCr) +
                           o0Cr,
                       BitDepthC);
          else
            predPartCr[y * partWidthC + x] = Clip1C(
                predPartL0Cr[y * partWidthC + x] * w0Cr + o0Cr, BitDepthC);
        }
      }
    }
  } else if (predFlagL0 == 0 && predFlagL1) {
    /* 否则，如果 predFlagL0 等于 0 且 predFlagL1 等于 1，则最终预测样本值 predPartC[ x, y ] 可由下式导出： */
    for (int y = 0; y <= partHeight - 1; y++)
      for (int x = 0; x <= partWidth - 1; x++)
        if (logWDL >= 1)
          predPartL[y * partWidth + x] =
              Clip1C(((predPartL1L[y * partWidth + x] * w0L +
                       h264_power2(logWDL - 1)) >>
                      logWDL) +
                         o0L,
                     BitDepthY);
        else
          predPartL[y * partWidth + x] =
              Clip1C(predPartL1L[y * partWidth + x] * w0L + o0L, BitDepthY);

    if (m_slice->slice_header->m_sps->ChromaArrayType != 0) {
      for (int y = 0; y <= partHeightC - 1; y++) {
        for (int x = 0; x <= partWidthC - 1; x++) {
          if (logWDCb >= 1)
            predPartCb[y * partWidthC + x] =
                Clip1C(((predPartL1Cb[y * partWidthC + x] * w1Cb +
                         h264_power2(logWDCb - 1)) >>
                        logWDCb) +
                           o1Cb,
                       BitDepthC);
          else
            predPartCb[y * partWidthC + x] = Clip1C(
                predPartL1Cb[y * partWidthC + x] * w1Cb + o1Cb, BitDepthC);

          if (logWDCr >= 1)
            predPartCr[y * partWidthC + x] =
                Clip1C(((predPartL1Cr[y * partWidthC + x] * w1Cr +
                         h264_power2(logWDCr - 1)) >>
                        logWDCr) +
                           o1Cr,
                       BitDepthC);
          else
            predPartCr[y * partWidthC + x] = Clip1C(
                predPartL1Cr[y * partWidthC + x] * w1Cr + o1Cr, BitDepthC);
        }
      }
    }

    /* 否则（predFlagL0 和 predFlagL1 都等于 1），最终预测样本值 predPartC[ x, y ] 由下式得出： */
  } else if (predFlagL0 && predFlagL1) {
    // B帧双向预测
    for (int y = 0; y <= partHeight - 1; y++)
      for (int x = 0; x <= partWidth - 1; x++)
        predPartL[y * partWidth + x] = Clip1C(
            ((predPartL0L[y * partWidth + x] * w0L +
              predPartL1L[y * partWidth + x] * w1L + h264_power2(logWDL)) >>
             (logWDL + 1)) +
                ((o0L + o1L + 1) >> 1),
            BitDepthY);

    if (m_slice->slice_header->m_sps->ChromaArrayType != 0) {
      for (int y = 0; y <= partHeightC - 1; y++) {
        for (int x = 0; x <= partWidthC - 1; x++) {
          predPartCb[y * partWidthC + x] =
              Clip1C(((predPartL0Cb[y * partWidthC + x] * w0Cb +
                       predPartL1Cb[y * partWidthC + x] * w1Cb +
                       h264_power2(logWDCb)) >>
                      (logWDCb + 1)) +
                         ((o0Cb + o1Cb + 1) >> 1),
                     BitDepthC);

          predPartCr[y * partWidthC + x] =
              Clip1C(((predPartL0Cr[y * partWidthC + x] * w0Cr +
                       predPartL1Cr[y * partWidthC + x] * w1Cr +
                       h264_power2(logWDCr)) >>
                      (logWDCr + 1)) +
                         ((o0Cr + o1Cr + 1) >> 1),
                     BitDepthC);
        }
      }
    }
  }

  return 0;
}

// 8.4.2.3.1 Default weighted sample prediction process
/* 输入、输出同weighted_sample_prediction() */
int PictureBase::default_weighted_sample_prediction(
    bool predFlagL0, bool predFlagL1, int32_t partWidth, int32_t partHeight,
    int32_t partWidthC, int32_t partHeightC, uint8_t *predPartL0L,
    uint8_t *predPartL0Cb, uint8_t *predPartL0Cr, uint8_t *predPartL1L,
    uint8_t *predPartL1Cb, uint8_t *predPartL1Cr,
    /* Output: */
    uint8_t *predPartL, uint8_t *predPartCb, uint8_t *predPartCr) {

  /* 根据导出预测块的可用组件，以下适用： 
   * – 如果导出亮度样本预测值 predPartL[ x, y ]，则以下适用，其中 C 设置等于 L，x 设置等于 0。 partWidth − 1，且 y 设置等于 0..partHeight − 1。 
   * – 否则，如果导出色度 Cb 分量样本预测值 predPartCb[ x, y ]，则以下情况适用于 C 设置等于 Cb，x 设置等于0..partWidthC − 1，且 y 设置等于 0..partHeightC − 1。 
   * – 否则（导出色度 Cr 分量样本预测值 predPartCr[ x, y ]），以下适用于设置等于 Cr、x 的 C设置等于 0..partWidthC − 1，y 设置等于 0..partHeightC − 1。 */
  if (predFlagL0 && predFlagL1 == 0) {
    for (int y = 0; y <= partHeight - 1; y++)
      for (int x = 0; x <= partWidth - 1; x++)
        predPartL[y * partWidth + x] = predPartL0L[y * partWidth + x];

    if (m_slice->slice_header->m_sps->ChromaArrayType != 0) {
      for (int y = 0; y <= partHeightC - 1; y++) {
        for (int x = 0; x <= partWidthC - 1; x++) {
          predPartCb[y * partWidthC + x] = predPartL0Cb[y * partWidthC + x];
          predPartCr[y * partWidthC + x] = predPartL0Cr[y * partWidthC + x];
        }
      }
    }
  } else if (predFlagL0 == 0 && predFlagL1) {
    for (int y = 0; y <= partHeight - 1; y++)
      for (int x = 0; x <= partWidth - 1; x++)
        predPartL[y * partWidth + x] = predPartL1L[y * partWidth + x];

    if (m_slice->slice_header->m_sps->ChromaArrayType != 0) {
      for (int y = 0; y <= partHeightC - 1; y++) {
        for (int x = 0; x <= partWidthC - 1; x++) {
          predPartCb[y * partWidthC + x] = predPartL1Cb[y * partWidthC + x];
          predPartCr[y * partWidthC + x] = predPartL1Cr[y * partWidthC + x];
        }
      }
    }
  } else {
    for (int y = 0; y <= partHeight - 1; y++)
      for (int x = 0; x <= partWidth - 1; x++)
        predPartL[y * partWidth + x] = (predPartL0L[y * partWidth + x] +
                                        predPartL1L[y * partWidth + x] + 1) >>
                                       1;

    if (m_slice->slice_header->m_sps->ChromaArrayType != 0) {
      for (int y = 0; y <= partHeightC - 1; y++) {
        for (int x = 0; x <= partWidthC - 1; x++) {
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
    }
  }

  return 0;
}

// 8.4.1.2.3 Derivation process for temporal direct luma motion vector and reference index prediction mode
//refPicCol 为在对图片 colPic 内的同位宏块 mbAddrCol 进行解码时由参考索引 refIdxCol 引用的帧、字段或互补字段对
int PictureBase::mapColToList0(int32_t refIdxCol, PictureBase *colPic,
                               int32_t mbAddrCol, int32_t vertMvScale,
                               bool field_pic_flag) {

  int32_t refIdxL0Frm = NA;
  for (int i = 0; i < H264_MAX_REF_PIC_LIST_COUNT; i++) {
    if (m_RefPicList0[i] == nullptr) break;
    if ((m_RefPicList0[i]->m_picture_coded_type == PICTURE_CODED_TYPE_FRAME &&
         (&m_RefPicList0[i]->m_picture_frame == colPic)) ||
        (m_RefPicList0[i]->m_picture_coded_type ==
             PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR &&
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
      if ((colPic->m_picture_coded_type == PICTURE_CODED_TYPE_TOP_FIELD &&
           CurrMbAddr % 2 == 0) ||
          (colPic->m_picture_coded_type == PICTURE_CODED_TYPE_BOTTOM_FIELD &&
           CurrMbAddr % 2 == 1))
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
  /* TODO YangJing 什么是共置宏块？ <24-10-11 05:14:20> */

  /* NOTE: 当在包含共置宏块的图片的解码过程中引用该解码参考图片时，该解码参考图片被标记为“用于短期参考”，可能已被修改为被标记为“用于长期参考”。参考之前被用于使用当前宏块的直接预测模式进行帧间预测的参考。 */
  return refIdxL0_temp;
}

//8.2.1 Decoding process for picture order count (8-2)
/* 两张图片相差的距离（即两图片计数器的差值） */
int PictureBase::DiffPicOrderCnt(PictureBase *picA, PictureBase *picB) {
  return picOrderCntFunc(picA) - picOrderCntFunc(picB);
}
