#include "PictureBase.hpp"
#include <cstdint>

// 8.3.1.1 Derivation process for Intra4x4PredMode (8.3.1 Intra_4x4 prediction process for luma samples)
int PictureBase::getIntra4x4PredMode(int32_t luma4x4BlkIdx,
                                     int32_t &currMbAddrPredMode,
                                     int32_t isChroma) {
  /* ------------------ 设置别名 ------------------ */
  const bool constrained_flag =
      m_slice->slice_header->m_pps->constrained_intra_pred_flag;
  const bool MbaffFrameFlag = m_slice->slice_header->MbaffFrameFlag;
  MacroBlock &mb = m_mbs[CurrMbAddr];
  /* ------------------  End ------------------ */

  int32_t x = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
              InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
  int32_t y = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
              InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

  // 6.4.12 Derivation process for neighbouring locations
  int32_t maxW = 16, maxH = 16;
  if (isChroma) maxW = MbWidthC, maxH = MbHeightC;
  int32_t xW = 0, yW = 0, mbAddrN_A = -1, mbAddrN_B = -1;
  int32_t luma4x4BlkIdxN_A = 0, luma4x4BlkIdxN_B = 0, luma8x8BlkIdxN_A = 0,
          luma8x8BlkIdxN_B = 0;
  MB_ADDR_TYPE mbAddrN_type_A = MB_ADDR_TYPE_UNKOWN,
               mbAddrN_type_B = MB_ADDR_TYPE_UNKOWN;

  /* 计算相邻位置 (x-1, y 和 x, y-1) 来确定当前块的左侧和上方相邻块 */
  if (MbaffFrameFlag) {
    RET(neighbouring_locations_MBAFF(
        x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_type_A, mbAddrN_A,
        luma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));
    RET(neighbouring_locations_MBAFF(
        x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_type_B, mbAddrN_B,
        luma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
  } else {
    RET(neighbouring_locations_non_MBAFF(
        x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_type_A, mbAddrN_A,
        luma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));
    RET(neighbouring_locations_non_MBAFF(
        x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_type_B, mbAddrN_B,
        luma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
  }

  /* 相邻位置的块不存在（即索引为负）或者是帧间预测模式，将默认使用直流预测模式（DC Prediction），因为可能没有有效的参考数据 */
  const MacroBlock &mb1 = m_mbs[mbAddrN_A];
  const MacroBlock &mb2 = m_mbs[mbAddrN_B];

  bool dcPredModePredictedFlag = false;
  if (mbAddrN_A < 0 || mbAddrN_B < 0)
    dcPredModePredictedFlag = true;
  else if (mbAddrN_A >= 0 && constrained_flag &&
           IS_INTER_Prediction_Mode(mb1.m_mb_pred_mode))
    dcPredModePredictedFlag = true;
  else if (mbAddrN_B >= 0 && constrained_flag &&
           IS_INTER_Prediction_Mode(mb2.m_mb_pred_mode))
    dcPredModePredictedFlag = true;

  /* 如果相邻块是帧内编码模式（Intra_4x4 或 Intra_8x8），则会从相邻块继承相应的预测模式 */
  int32_t intraMxMPredModeA = 0, intraMxMPredModeB = 0;
  if (dcPredModePredictedFlag ||
      (mbAddrN_A >= 0 && mb1.m_mb_pred_mode != Intra_4x4 &&
       mb1.m_mb_pred_mode != Intra_8x8))
    intraMxMPredModeA = Prediction_Mode_Intra_4x4_DC;
  else {
    if (mbAddrN_A >= 0 && mb1.m_mb_pred_mode == Intra_4x4)
      intraMxMPredModeA = mb1.Intra4x4PredMode[luma4x4BlkIdxN_A];
    else
      intraMxMPredModeA = mb1.Intra8x8PredMode[luma4x4BlkIdxN_A >> 2];
  }

  if (dcPredModePredictedFlag ||
      (mbAddrN_B >= 0 && mb2.m_mb_pred_mode != Intra_4x4 &&
       mb2.m_mb_pred_mode != Intra_8x8))
    intraMxMPredModeB = Prediction_Mode_Intra_4x4_DC;
  else {
    if (mbAddrN_B >= 0 && mb2.m_mb_pred_mode == Intra_4x4)
      intraMxMPredModeB = mb2.Intra4x4PredMode[luma4x4BlkIdxN_B];
    else
      intraMxMPredModeB = mb2.Intra8x8PredMode[luma4x4BlkIdxN_B >> 2];
  }

  /* 对于每个4x4块，选择最小的预测模式值（intraMxMPredModeA 和 intraMxMPredModeB）作为最终预测模式。这种选择通常基于实现最小编码残差的原则*/
  int32_t predIntra4x4PredMode = MIN(intraMxMPredModeA, intraMxMPredModeB);

  /* 表明之前计算或推断的预测模式是有效的，可以直接将其应用于当前的4x4块 */
  if (mb.prev_intra4x4_pred_mode_flag[luma4x4BlkIdx])
    currMbAddrPredMode = predIntra4x4PredMode;
  else {
    /* 使用编码器提供的预测模式 */
    if (mb.rem_intra4x4_pred_mode[luma4x4BlkIdx] < predIntra4x4PredMode)
      currMbAddrPredMode = mb.rem_intra4x4_pred_mode[luma4x4BlkIdx];
    else
      currMbAddrPredMode = mb.rem_intra4x4_pred_mode[luma4x4BlkIdx] + 1;
  }
  mb.Intra4x4PredMode[luma4x4BlkIdx] = currMbAddrPredMode;
  return 0;
}

// 8.3.2.1 Derivation process for Intra8x8PredMode (8.3.2 Intra_8x8 prediction process for luma samples)
// NOTE:与getIntra4x4PredMode()逻辑一样
int PictureBase::getIntra8x8PredMode(int32_t luma8x8BlkIdx,
                                     int32_t &currMbAddrPredMode,
                                     int32_t isChroma) {

  /* ------------------ 设置别名 ------------------ */
  const bool constrained_flag =
      m_slice->slice_header->m_pps->constrained_intra_pred_flag;
  const bool MbaffFrameFlag = m_slice->slice_header->MbaffFrameFlag;
  MacroBlock &mb = m_mbs[CurrMbAddr];
  /* ------------------  End ------------------ */

  int32_t x = (luma8x8BlkIdx % 2) * 8, y = (luma8x8BlkIdx / 2) * 8;

  int32_t maxW = 16, maxH = 16;
  if (isChroma) maxW = MbWidthC, maxH = MbHeightC;
  int32_t xW = 0, yW = 0, mbAddrN_A = -1, mbAddrN_B = -1;
  int32_t luma4x4BlkIdxN_A = 0, luma4x4BlkIdxN_B = 0, luma8x8BlkIdxN_A = 0,
          luma8x8BlkIdxN_B = 0;
  MB_ADDR_TYPE mbAddrN_type_A = MB_ADDR_TYPE_UNKOWN,
               mbAddrN_type_B = MB_ADDR_TYPE_UNKOWN;

  /* 计算相邻位置 (x-1, y 和 x, y-1) 来确定当前块的左侧和上方相邻块 */
  if (MbaffFrameFlag) {
    RET(neighbouring_locations_MBAFF(
        x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_type_A, mbAddrN_A,
        luma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));
    RET(neighbouring_locations_MBAFF(
        x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_type_B, mbAddrN_B,
        luma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
  } else {
    RET(neighbouring_locations_non_MBAFF(
        x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_type_A, mbAddrN_A,
        luma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));
    RET(neighbouring_locations_non_MBAFF(
        x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_type_B, mbAddrN_B,
        luma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
  }

  /* 相邻位置的块不存在（即索引为负）或者是帧间预测模式，将默认使用直流预测模式（DC Prediction），因为可能没有有效的参考数据 */
  const MacroBlock &mb1 = m_mbs[mbAddrN_A];
  const MacroBlock &mb2 = m_mbs[mbAddrN_B];

  bool dcPredModePredictedFlag = false;
  if (mbAddrN_A < 0 || mbAddrN_B < 0)
    dcPredModePredictedFlag = true;
  else if (mbAddrN_A >= 0 && constrained_flag &&
           IS_INTER_Prediction_Mode(mb1.m_mb_pred_mode))
    dcPredModePredictedFlag = true;
  else if (mbAddrN_B >= 0 && constrained_flag &&
           IS_INTER_Prediction_Mode(mb2.m_mb_pred_mode))
    dcPredModePredictedFlag = true;

  /* 如果相邻块是帧内编码模式（Intra_4x4 或 Intra_8x8），则会从相邻块继承相应的预测模式 */
  int32_t intraMxMPredModeA = 0, intraMxMPredModeB = 0;
  if (dcPredModePredictedFlag ||
      (mbAddrN_A >= 0 && mb1.m_mb_pred_mode != Intra_4x4 &&
       mb1.m_mb_pred_mode != Intra_8x8))
    intraMxMPredModeA = Prediction_Mode_Intra_8x8_DC;
  else {
    if (mbAddrN_A >= 0 && mb1.m_mb_pred_mode == Intra_8x8)
      intraMxMPredModeA = mb1.Intra8x8PredMode[luma8x8BlkIdxN_A];
    else {
      int32_t n = (MbaffFrameFlag && mb.field_pic_flag == 0 &&
                   mb1.field_pic_flag && luma8x8BlkIdx == 2)
                      ? 3
                      : 1;
      intraMxMPredModeA = mb1.Intra4x4PredMode[luma8x8BlkIdxN_A * 4 + n];
    }
  }

  if (dcPredModePredictedFlag ||
      (mbAddrN_B >= 0 && mb2.m_mb_pred_mode != Intra_4x4 &&
       mb2.m_mb_pred_mode != Intra_8x8))
    intraMxMPredModeB = Prediction_Mode_Intra_8x8_DC;
  else {
    if (mbAddrN_B >= 0 && mb2.m_mb_pred_mode == Intra_8x8)
      intraMxMPredModeB = mb2.Intra8x8PredMode[luma8x8BlkIdxN_B];
    else
      intraMxMPredModeB = mb2.Intra4x4PredMode[luma8x8BlkIdxN_B * 4 + 2];
  }

  /* 对于每个8x8块，选择最小的预测模式值（intraMxMPredModeA 和 intraMxMPredModeB）作为最终预测模式。这种选择通常基于实现最小编码残差的原则*/
  int32_t predIntra8x8PredMode = MIN(intraMxMPredModeA, intraMxMPredModeB);

  /* 表明之前计算或推断的预测模式是有效的，可以直接将其应用于当前的8x8块 */
  if (mb.prev_intra8x8_pred_mode_flag[luma8x8BlkIdx])
    currMbAddrPredMode = predIntra8x8PredMode;
  else {
    /* 使用编码器提供的预测模式 */
    if (mb.rem_intra8x8_pred_mode[luma8x8BlkIdx] < predIntra8x8PredMode)
      currMbAddrPredMode = mb.rem_intra8x8_pred_mode[luma8x8BlkIdx];
    else
      currMbAddrPredMode = mb.rem_intra8x8_pred_mode[luma8x8BlkIdx] + 1;
  }

  mb.Intra8x8PredMode[luma8x8BlkIdx] = currMbAddrPredMode;
  return 0;
}

//8.5.15 Intra residual transform-bypass decoding process( 帧内残差变换-旁路解码过程)
/* 输入： – 两个变量 nW 和 nH， – 变量 horPredFlag， – 带有元素 rij 的 (nW)x(nH) 数组 r，它是与亮度分量的残差变换旁路块相关的数组，或者与 Cb 和 Cr 分量的残差变换旁路块相关的数组。
 * 输出: (nW)x(nH) 数组 r 的修改版本，其中元素 rij 包含帧内残差变换旁路解码过程的结果。 */
/* TODO YangJing 这个函数未测试过，可能有问题 <24-10-03 19:22:29> */
int PictureBase::intra_residual_transform_bypass_decoding(int32_t nW,
                                                          int32_t nH,
                                                          int32_t horPredFlag,
                                                          int32_t *r) {
  /* 设 f 是一个临时 (nW)x(nH) 数组，其中元素 fij 是通过以下方式导出的： */
  //int32_t f[4][4] = {{0}};
  int32_t **f = new int32_t *[nH];
  for (int i = 0; i < nH; i++)
    f[i] = new int32_t[nW];

  for (int32_t i = 0; i < nH; i++)
    for (int32_t j = 0; j < nW; j++)
      f[i][j] = r[i * nW + j];

  if (horPredFlag == 0) {
    /* 1. 对于每个元素 r[i][j]，其值被设置为其所在列上面所有元素的累加和（包括自己）。这意味着每一列的每个元素都是该列上方所有元素的和。(水平预测绕过) */
    for (int32_t i = 0; i < nH; i++)
      for (int32_t j = 0; j < nW; j++) {
        r[i * nH + j] = 0;
        for (int32_t k = 0; k <= i; k++)
          r[i * nW + j] += f[k][j];
      }
  } else {
    /* 2. 对于每个元素 r[i][j]，其值被设置为其所在行左侧所有元素的累加和（包括自己）。这意味着每一行的每个元素都是该行左侧所有元素的和。(垂直预测绕过) */
    for (int32_t i = 0; i < nH; i++)
      for (int32_t j = 0; j < nW; j++) {
        r[i * nW + j] = 0;
        for (int32_t k = 0; k <= j; k++)
          r[i * nW + j] += f[i][k];
      }
  }
  return 0;
}

// 8.5.1 Specification of transform decoding process for 4x4 luma residual
int PictureBase::transform_decoding_for_4x4_luma_residual_blocks(
    int32_t isChroma, int32_t isChromaCb, int32_t BitDepth,
    int32_t PicWidthInSamples, uint8_t *pic_buff, bool isNeedIntraPrediction) {

  /* ------------------ 设置别名 ------------------ */
  MacroBlock &mb = m_mbs[CurrMbAddr];
  bool isMbAff =
      m_slice->slice_header->MbaffFrameFlag && mb.mb_field_decoding_flag;
  /* ------------------  End ------------------ */

  /* 当当前宏块预测模式不等于Intra_16x16时，变量LumaLevel4x4包含亮度变换系数的级别。对于由 luma4x4BlkIdx = 0..15 索引的 4x4 亮度块，指定以下有序步骤： */
  if (mb.m_mb_pred_mode != Intra_16x16) {
    /* 初始化缩放举证因子 */
    RET(scaling_functions(isChroma, isChromaCb));

    // 以4x4宏块为单位，遍历亮度块
    for (int32_t luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; luma4x4BlkIdx++) {
      //1. 按Zigzag扫描顺序，将单个 4x4 残差值宏块(一维数组）放入二维数组c中
      int32_t c[4][4] = {{0}};
      RET(inverse_scanning_for_4x4_transform_coeff_and_scaling_lists(
          mb.LumaLevel4x4[luma4x4BlkIdx], c,
          mb.field_pic_flag | mb.mb_field_decoding_flag));

      //2. 对单个 4x4 残差值宏块进行反量化、逆整数变换（此处后得到一个完整的残差块）
      int32_t r[4][4] = {{0}};
      RET(scaling_and_transform_for_residual_4x4_blocks(c, r, isChroma,
                                                        isChromaCb));

      //3. 当前为变换旁路模式，宏块预测模式 Intra_4x4，且 Intra4x4PredMode[ luma4x4BlkIdx ] 等于 0 或 1 时（水平、垂直预测模式）
      if (mb.TransformBypassModeFlag && mb.m_mb_pred_mode == Intra_4x4 &&
          (mb.Intra4x4PredMode[luma4x4BlkIdx] & ~1) == 0)
        intra_residual_transform_bypass_decoding(
            4, 4, mb.Intra4x4PredMode[luma4x4BlkIdx], &r[0][0]);

      //4. 帧内预测
      if (isNeedIntraPrediction)
        RET(intra_4x4_sample_prediction(luma4x4BlkIdx, PicWidthInSamples,
                                        pic_buff, isChroma, BitDepth));

      int32_t xO = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
                   InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
      int32_t yO = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
                   InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

      //5. 原始像素值 = 值残差值 + 预测值
      int32_t u[16] = {0};
      for (int32_t i = 0; i < 4; i++)
        for (int32_t j = 0; j < 4; j++) {
          int32_t y = mb.m_mb_position_y + (yO + i) * (1 + isMbAff);
          int32_t x = mb.m_mb_position_x + (xO + j);
          u[i * 4 + j] =
              Clip1C(pic_buff[y * PicWidthInSamples + x] + r[i][j], BitDepth);
        }

      //6. 重建图像数据，写入最终数据到pic_buff，主要针对帧、场编码的不同情况
      RET(picture_construction_process_prior_to_deblocking_filter(
          u, 4, 4, luma4x4BlkIdx, isChroma, PicWidthInSamples, pic_buff));
    }
  }

  return 0;
}

// 8.5.3 Specification of transform decoding process for 8x8 luma residual blocks
// This specification applies when transform_size_8x8_flag is equal to 1.
// NOTE:与transform_decoding_for_4x4_luma_residual_blocks()逻辑一致
int PictureBase::transform_decoding_for_8x8_luma_residual_blocks(
    int32_t isChroma, int32_t isChromaCb, int32_t BitDepth,
    int32_t PicWidthInSamples, int32_t Level8x8[4][64], uint8_t *pic_buff,
    bool isNeedIntraPrediction) {

  /* ------------------ 设置别名 ------------------ */
  MacroBlock &mb = m_mbs[CurrMbAddr];
  bool isMbAff =
      m_slice->slice_header->MbaffFrameFlag && mb.mb_field_decoding_flag;
  /* ------------------  End ------------------ */

  RET(scaling_functions(isChroma, isChromaCb));

  /* 16x16拆分为4个8x8块处理 */
  for (int32_t luma8x8BlkIdx = 0; luma8x8BlkIdx < 4; luma8x8BlkIdx++) {
    // 以8x8宏块为单位，遍历亮度块
    int32_t c[8][8] = {{0}};
    RET(inverse_scanning_for_8x8_transform_coeff_and_scaling_lists(
        Level8x8[luma8x8BlkIdx], c,
        mb.field_pic_flag | mb.mb_field_decoding_flag));

    // 对单个 8x8 残差值宏块进行反量化、逆整数变换（此处后得到一个完整的残差块）
    int32_t r[8][8] = {{0}};
    RET(scaling_and_transform_for_residual_8x8_blocks(c, r, isChroma,
                                                      isChromaCb));

    if (mb.TransformBypassModeFlag && mb.m_mb_pred_mode == Intra_8x8 &&
        (mb.Intra8x8PredMode[luma8x8BlkIdx] & ~1) == 0)
      intra_residual_transform_bypass_decoding(
          8, 8, mb.Intra8x8PredMode[luma8x8BlkIdx], &r[0][0]);

    // 以8x8宏块为单位，帧内预测
    if (isNeedIntraPrediction)
      RET(intra_8x8_sample_prediction(luma8x8BlkIdx, PicWidthInSamples,
                                      pic_buff, isChroma, BitDepth));

    int32_t xO = InverseRasterScan(luma8x8BlkIdx, 8, 8, 16, 0);
    int32_t yO = InverseRasterScan(luma8x8BlkIdx, 8, 8, 16, 1);

    // 原始像素值 = 值残差值 + 预测值
    int32_t u[64] = {0};
    for (int32_t i = 0; i < 8; i++)
      for (int32_t j = 0; j < 8; j++) {
        int32_t y = mb.m_mb_position_y + (yO + i) * (1 + isMbAff);
        int32_t x = mb.m_mb_position_x + (xO + j);
        u[i * 8 + j] =
            Clip1C(pic_buff[y * PicWidthInSamples + x] + r[i][j], BitDepth);
      }

    RET(picture_construction_process_prior_to_deblocking_filter(
        u, 8, 8, luma8x8BlkIdx, isChroma, PicWidthInSamples, pic_buff));
  }

  return 0;
}

// 8.5.2 Specification of transform decoding process for luma samples of Intra_16x16 macroblock prediction mode
int PictureBase::transform_decoding_for_luma_samples_of_16x16(
    int32_t isChroma, int32_t BitDepth, int32_t QP1, int32_t PicWidthInSamples,
    int32_t Intra16x16DCLevel[16], int32_t Intra16x16ACLevel[16][16],
    uint8_t *pic_buff) {

  /* ------------------ 设置别名 ------------------ */
  MacroBlock &mb = m_mbs[CurrMbAddr];
  bool isMbAff =
      m_slice->slice_header->MbaffFrameFlag && mb.mb_field_decoding_flag;
  /* ------------------  End ------------------ */
  RET(scaling_functions(isChroma, 0));

  // -------------------- 单独处理DC系数矩阵的第二层量化、变换 --------------------

  // 按Zigzag扫描顺序，将单个 4x4 DC残差值宏块(一维数组）放入二维数组c中，注意这里是DC系数矩阵
  int32_t c[4][4] = {{0}};
  RET(inverse_scanning_for_4x4_transform_coeff_and_scaling_lists(
      Intra16x16DCLevel, c, mb.field_pic_flag | mb.mb_field_decoding_flag));

  // 对单个 4x4 DC残差值宏块进行逆整数变换、反量化（这里的顺序相反，此处后得到一个完整的残差块），第二层的量化、变换
  int32_t dcY[4][4] = {{0}};
  RET(scaling_and_transform_for_DC_Intra16x16(BitDepth, QP1, c, dcY));

  /* 分块Zigzag扫描，它按照从左到右、从上到下的顺序依次访问每个2x2子块中的元素，然后再移动到下一个2x2子块:
     +-----+-----+-----+-----+
     |  0  |  1  |  4  |  5  |
     +-----+-----+-----+-----+
     |  2  |  3  |  6  |  7  |
     +-----+-----+-----+-----+
     |  8  |  9  | 12  | 13  |
     +-----+-----+-----+-----+
     | 10  | 11  | 14  | 15  |
     +-----+-----+-----+-----+ */
  int32_t dcY_to_luma_index[16] = {
      dcY[0][0], dcY[0][1], dcY[1][0], dcY[1][1], dcY[0][2], dcY[0][3],
      dcY[1][2], dcY[1][3], dcY[2][0], dcY[2][1], dcY[3][0], dcY[3][1],
      dcY[2][2], dcY[2][3], dcY[3][2], dcY[3][3],
  };

  //------------------ 下面与transform_decoding_for_4x4_luma_residual_blocks()是一样的逻辑 ------------------
  int32_t rMb[16][16] = {{0}};
  /* 16x16拆分为16个4x4块处理（只处理量化、变换部分，之后还是16x16整块预测） */
  for (int32_t luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; luma4x4BlkIdx++) {
    /* 合并DC、AC分量：将DC分量放到4x4块的首个样本中（此时DC系数已经得到完整的残差值，不需要进行反量化、逆变换，将剩余的15个AC分量同样放到4x4块中 */
    int32_t lumaList[16] = {0};
    lumaList[0] = dcY_to_luma_index[luma4x4BlkIdx];

    // 跳过首个样本，由于分开存储DC，AC这里的DC实际上是0
    for (int32_t k = 1; k < 16; k++)
      lumaList[k] = Intra16x16ACLevel[luma4x4BlkIdx][k - 1];

    // 以4x4宏块为单位，遍历亮度块
    int32_t c[4][4] = {{0}};
    RET(inverse_scanning_for_4x4_transform_coeff_and_scaling_lists(
        lumaList, c, mb.field_pic_flag | mb.mb_field_decoding_flag));

    // 以4x4宏块为单位，反量化，逆变换
    int32_t r[4][4] = {{0}};
    RET(scaling_and_transform_for_residual_4x4_blocks(c, r, 0, 0));

    int32_t xO = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
                 InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
    int32_t yO = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
                 InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

    for (int32_t i = 0; i < 4; i++)
      for (int32_t j = 0; j < 4; j++)
        rMb[yO + i][xO + j] = r[i][j];
  }

  if (mb.TransformBypassModeFlag && (mb.Intra16x16PredMode & ~1) == 0)
    intra_residual_transform_bypass_decoding(16, 16, mb.Intra16x16PredMode,
                                             &rMb[0][0]);

  // 以16x16宏块为单位，帧内预测
  RET(intra_16x16_sample_prediction(pic_buff, PicWidthInSamples, isChroma,
                                    BitDepth));

  // 原始像素值 = 值残差值 + 预测值
  int32_t u[16 * 16] = {0};
  for (int32_t i = 0; i < 16; i++)
    for (int32_t j = 0; j < 16; j++) {
      int32_t y = mb.m_mb_position_y + (0 + i) * (1 + isMbAff);
      int32_t x = mb.m_mb_position_x + (0 + j);
      u[i * 16 + j] =
          Clip1C(pic_buff[y * PicWidthInSamples + x] + rMb[i][j], BitDepth);
    }

  RET(picture_construction_process_prior_to_deblocking_filter(
      u, 16, 16, 0, isChroma, PicWidthInSamples, pic_buff));

  return 0;
}

// 8.5.4 Specification of transform decoding process for chroma samples
/* 当 ChromaArrayType 不等于 0 时，针对每个色度分量 Cb 和 Cr 分别调用此过程。*/
int PictureBase::transform_decoding_for_chroma_samples(
    int32_t isChromaCb, int32_t PicWidthInSamples, uint8_t *pic_buff,
    bool isNeedIntraPrediction) {
  // YUV444: 8.5.5 Specification of transform decoding process for chroma samples with ChromaArrayType equal to 3
  if (m_slice->slice_header->m_sps->ChromaArrayType == 3)
    return transform_decoding_for_chroma_samples_with_YUV444(
        isChromaCb, PicWidthInSamples, pic_buff);
  // YUV420,YUV422: 8.5.4 Specification of transform decoding process for chroma samples
  else
    return transform_decoding_for_chroma_samples_with_YUV420_or_YUV422(
        isChromaCb, PicWidthInSamples, pic_buff, isNeedIntraPrediction);
}

// 8.5.5 Specification of transform decoding process for chroma samples with ChromaArrayType equal to 3
// TODO 未测试 <24-10-09 15:56:49, YangJing>
int PictureBase::transform_decoding_for_chroma_samples_with_YUV444(
    int32_t isChromaCb, int32_t PicWidthInSamples, uint8_t *pic_buff) {
  const int32_t isChroma = 1;
  const int32_t BitDepth = m_slice->slice_header->m_sps->BitDepthC;
  MacroBlock &mb = m_mbs[CurrMbAddr];

  int ret = 0;
  /* 同Luma样本一样处理 */
  if (mb.m_mb_pred_mode == Intra_16x16)
    ret = transform_decoding_for_luma_samples_of_16x16(
        isChroma, BitDepth, isChromaCb ? mb.QP1Cb : mb.QP1Cr, PicWidthInSamples,
        mb.CbIntra16x16DCLevel, mb.CbIntra16x16ACLevel, pic_buff);
  else if (mb.transform_size_8x8_flag)
    ret = transform_decoding_for_8x8_luma_residual_blocks(
        isChroma, isChromaCb, BitDepth, PicWidthInSamples,
        isChromaCb ? mb.CbLevel8x8 : mb.CrLevel8x8, pic_buff);
  else
    ret = transform_decoding_for_4x4_luma_residual_blocks(
        isChroma, isChromaCb, BitDepth, PicWidthInSamples, pic_buff);

  return ret;
}

// 8.5.4 Specification of transform decoding process for chroma samples
int PictureBase::transform_decoding_for_chroma_samples_with_YUV420_or_YUV422(
    int32_t isChromaCb, int32_t PicWidthInSamples, uint8_t *pic_buff,
    bool isNeedIntraPrediction) {

  /* ------------------ 设置别名 ------------------ */
  const SliceHeader *header = m_slice->slice_header;
  const uint32_t ChromaArrayType = header->m_sps->ChromaArrayType;
  const MacroBlock &mb = m_mbs[CurrMbAddr];
  bool isMbAff = header->MbaffFrameFlag && mb.mb_field_decoding_flag;
  uint32_t BitDepthC = header->m_sps->BitDepthC;
  /* ------------------  End ------------------ */

  // Cb 的 iCbCr 设置为 0，Cr 的 iCbCr 设置为 1
  bool iCbCr = (isChromaCb == 0);

  // YUV420
  int32_t w = 2, h = 2, c[4][2] = {{0}};
  c[0][0] = mb.ChromaDCLevel[iCbCr][0];
  c[0][1] = mb.ChromaDCLevel[iCbCr][1];
  c[1][0] = mb.ChromaDCLevel[iCbCr][2];
  c[1][1] = mb.ChromaDCLevel[iCbCr][3];

  // YUV422
  if (ChromaArrayType == 2) {
    h = 4;
    c[2][0] = mb.ChromaDCLevel[iCbCr][4];
    c[2][1] = mb.ChromaDCLevel[iCbCr][5];
    c[3][0] = mb.ChromaDCLevel[iCbCr][6];
    c[3][1] = mb.ChromaDCLevel[iCbCr][7];
  }

  // 8.5.11 Scaling and transformation process for chroma DC transform
  int32_t dcC[4][2] = {{0}};
  RET(scaling_and_transform_for_chroma_DC(isChromaCb, c, w, h, dcC));

  /* 分块Zigzag扫描，它按照从上到下、从左到右的顺序依次访问每个2x2子块中的元素，然后再移动到下一个2x2子块:
     +-----+-----+
     |  0  |  1  |
     +-----+-----+
     |  2  |  3  |
     +-----+-----+
     |  4  |  5  |
     +-----+-----+
     |  6  |  7  |
     +-----+-----+  */
  const int32_t dcC_to_chroma_index[8] = {
      dcC[0][0], dcC[0][1], dcC[1][0], dcC[1][1],
      dcC[2][0], dcC[2][1], dcC[3][0], dcC[3][1],
  };

  //------------------ 下面与transform_decoding_for_luma_samples_of_16x16()是一样的逻辑 ------------------
  int32_t rMb[16][16] = {{0}};
  int32_t numChroma4x4Blks = (MbWidthC / 4) * (MbHeightC / 4);
  /* 拆分为单个4x4块处理 */
  for (int chroma4x4BlkIdx = 0; chroma4x4BlkIdx < numChroma4x4Blks;
       chroma4x4BlkIdx++) {
    /* 合并DC、AC分量：将DC分量放到4x4块的首个样本中（此时DC系数已经得到完整的残差值，不需要进行反量化、逆变换，将剩余的15个AC分量同样放到4x4块中 */
    int32_t chromaList[16] = {0};
    chromaList[0] = dcC_to_chroma_index[chroma4x4BlkIdx];
    // 跳过首个样本，由于分开存储DC，AC这里的DC实际上是0
    for (int k = 1; k <= 15; k++)
      chromaList[k] = mb.ChromaACLevel[iCbCr][chroma4x4BlkIdx][k - 1];

    // 以4x4宏块为单位，遍历色度块
    int32_t c[4][4] = {{0}};
    RET(inverse_scanning_for_4x4_transform_coeff_and_scaling_lists(
        chromaList, c, mb.field_pic_flag | mb.mb_field_decoding_flag));

    // 以4x4宏块为单位，反量化，逆变换
    int32_t r[4][4] = {{0}};
    RET(scaling_and_transform_for_residual_4x4_blocks(c, r, 1, isChromaCb));

    int32_t xO = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 0);
    int32_t yO = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 1);

    for (int32_t i = 0; i < 4; i++)
      for (int32_t j = 0; j < 4; j++)
        rMb[yO + i][xO + j] = r[i][j];
  }

  if (mb.TransformBypassModeFlag)
    if (mb.m_mb_pred_mode == Intra_4x4 || mb.m_mb_pred_mode == Intra_8x8 ||
        ((mb.m_mb_pred_mode == Intra_16x16 && mb.intra_chroma_pred_mode == 1) ||
         mb.intra_chroma_pred_mode == 2))
      intra_residual_transform_bypass_decoding(
          MbWidthC, MbHeightC, 2 - mb.intra_chroma_pred_mode, &rMb[0][0]);

  //帧内预测
  if (isNeedIntraPrediction)
    RET(intra_chroma_sample_prediction(pic_buff, PicWidthInSamples));

  int32_t u[16 * 16] = {0};
  for (int32_t i = 0; i < MbHeightC; i++) {
    for (int32_t j = 0; j < MbWidthC; j++) {
      int32_t y = ((mb.m_mb_position_y >> 4) * MbHeightC) +
                  (mb.m_mb_position_y % 2) + i * (1 + isMbAff);
      int32_t x = (mb.m_mb_position_x >> 4) * MbWidthC + j;
      u[i * MbHeightC + j] =
          Clip1C(pic_buff[y * PicWidthInSamples + x] + rMb[i][j], BitDepthC);
    }
  }

  /* 5.使用u作为输入来调用第8.5.14节中的去块滤波器过程之前的图像构建过程。 */
  RET(picture_construction_process_prior_to_deblocking_filter(
      u, MbWidthC, MbHeightC, 0, 1, PicWidthInSamples, pic_buff));

  return 0;
}

// 8.5.12.2 Transformation process for residual 4x4 blocks
// 类似4x4 IDCT离散余弦反变换蝶形运算
/* 输入： – 变量 bitDepth， – 具有元素 dij 的缩放变换系数 d 的 4x4 数组。
 * 输出: 剩余样本值，为 4x4 数组 r，元素为 rij。*/
/* TODO YangJing 这里是不是需要从编码器的算法处去理解？而这里只是一个逆变换？ <24-10-08 23:55:29> */
int PictureBase::transform_decoding_for_residual_4x4_blocks(
    int32_t d[4][4], int32_t (&r)[4][4]) {
  int32_t f[4][4] = {{0}}, h[4][4] = {{0}};
  /* 行变换 */
  for (int32_t i = 0; i < 4; i++) {
    /* 1. 通过结合系数值，重新组合低频信息*/
    int32_t ei0 = d[i][0] + d[i][2];
    int32_t ei3 = d[i][1] + (d[i][3] >> 1);

    /* 2. 通过分离系数值，重新组合高频信息*/
    int32_t ei2 = (d[i][1] >> 1) - d[i][3];
    int32_t ei1 = d[i][0] - d[i][2];

    f[i][0] = ei0 + ei3;
    f[i][1] = ei1 + ei2;
    f[i][2] = ei1 - ei2;
    f[i][3] = ei0 - ei3;
  }

  /* 列变换：同理行变换 */
  for (int32_t j = 0; j < 4; j++) {
    int32_t g0j = f[0][j] + f[2][j];
    int32_t g1j = f[0][j] - f[2][j];
    int32_t g2j = (f[1][j] >> 1) - f[3][j];
    int32_t g3j = f[1][j] + (f[3][j] >> 1);

    h[0][j] = g0j + g3j;
    h[1][j] = g1j + g2j;
    h[2][j] = g1j - g2j;
    h[3][j] = g0j - g3j;
  }

  /* 
逆变换前，频域系数：
|0x8f0,      0xa00,      0x0,        0x100     |
|0xb00,      0xfffffc40, 0xfffffd00, 0xfffffec0|
|0x0,        0xfffffd00, 0x0,        0x100     |
|0x100,      0xfffffec0, 0x100,      0x0       |
                                               
逆变换后，像素域：
|0x1370,     0xcf0,      0x4f0,      0xfffffe70|
|0x3a0,      0xd60,      0xea0,      0xc60     |
|0xfffffd80, 0xfffffd80, 0x280,      0x280     |
|0xc0,       0xffffff60, 0xa0,       0x340     |
其中0xfffff* 表示负数
   */

  /* 加32是为了在右移（即除以64）前进行舍入。这是因为右移整数会向零舍入，加32是为了在除以64时能够四舍五入到最接近的整数，随后量化步长64 */
  for (int32_t i = 0; i < 4; i++)
    for (int32_t j = 0; j < 4; j++)
      r[i][j] = (h[i][j] + 32) >> 6; // 2^5 = 32

  return 0;
}

int PictureBase::transform_decoding_for_residual_8x8_blocks(
    int32_t d[8][8], int32_t (&r)[8][8]) {
  int32_t g[8][8] = {{0}}, m[8][8] = {{0}};

  /* 行变换 */
  for (int32_t i = 0; i < 8; i++) {
    int32_t ei0 = d[i][0] + d[i][4];
    int32_t ei1 = -d[i][3] + d[i][5] - d[i][7] - (d[i][7] >> 1);
    int32_t ei2 = d[i][0] - d[i][4];
    int32_t ei3 = d[i][1] + d[i][7] - d[i][3] - (d[i][3] >> 1);
    int32_t ei4 = (d[i][2] >> 1) - d[i][6];
    int32_t ei5 = -d[i][1] + d[i][7] + d[i][5] + (d[i][5] >> 1);
    int32_t ei6 = d[i][2] + (d[i][6] >> 1);
    int32_t ei7 = d[i][3] + d[i][5] + d[i][1] + (d[i][1] >> 1);

    int32_t fi0 = ei0 + ei6;
    int32_t fi1 = ei1 + (ei7 >> 2);
    int32_t fi2 = ei2 + ei4;
    int32_t fi3 = ei3 + (ei5 >> 2);
    int32_t fi4 = ei2 - ei4;
    int32_t fi5 = (ei3 >> 2) - ei5;
    int32_t fi6 = ei0 - ei6;
    int32_t fi7 = ei7 - (ei1 >> 2);

    g[i][0] = fi0 + fi7;
    g[i][1] = fi2 + fi5;
    g[i][2] = fi4 + fi3;
    g[i][3] = fi6 + fi1;
    g[i][4] = fi6 - fi1;
    g[i][5] = fi4 - fi3;
    g[i][6] = fi2 - fi5;
    g[i][7] = fi0 - fi7;
  }

  /* 列变换：同理行变换 */
  for (int32_t j = 0; j < 8; j++) {
    int32_t h0j = g[0][j] + g[4][j];
    int32_t h1j = -g[3][j] + g[5][j] - g[7][j] - (g[7][j] >> 1);
    int32_t h2j = g[0][j] - g[4][j];
    int32_t h3j = g[1][j] + g[7][j] - g[3][j] - (g[3][j] >> 1);
    int32_t h4j = (g[2][j] >> 1) - g[6][j];
    int32_t h5j = -g[1][j] + g[7][j] + g[5][j] + (g[5][j] >> 1);
    int32_t h6j = g[2][j] + (g[6][j] >> 1);
    int32_t h7j = g[3][j] + g[5][j] + g[1][j] + (g[1][j] >> 1);

    int32_t k0j = h0j + h6j;
    int32_t k1j = h1j + (h7j >> 2);
    int32_t k2j = h2j + h4j;
    int32_t k3j = h3j + (h5j >> 2);
    int32_t k4j = h2j - h4j;
    int32_t k5j = (h3j >> 2) - h5j;
    int32_t k6j = h0j - h6j;
    int32_t k7j = h7j - (h1j >> 2);

    m[0][j] = k0j + k7j;
    m[1][j] = k2j + k5j;
    m[2][j] = k4j + k3j;
    m[3][j] = k6j + k1j;
    m[4][j] = k6j - k1j;
    m[5][j] = k4j - k3j;
    m[6][j] = k2j - k5j;
    m[7][j] = k0j - k7j;
  }

  for (int32_t i = 0; i < 8; i++)
    for (int32_t j = 0; j < 8; j++)
      r[i][j] = (m[i][j] + 32) >> 6;

  return 0;
}

// 8.3.1 Intra_4x4 prediction process for luma samples
// 8.3.1.2 Intra_4x4 sample prediction
// NOTE:帧内预测是基于已解码的样本进行预测，后续再通过残差值之和得到原始像素值
int PictureBase::intra_4x4_sample_prediction(int32_t luma4x4BlkIdx,
                                             int32_t PicWidthInSamples,
                                             uint8_t *pic_buff_luma_pred,
                                             int32_t isChroma,
                                             int32_t BitDepth) {

  /* ------------------ 设置别名 ------------------ */
  const MacroBlock &mb = m_mbs[CurrMbAddr];
  const bool MbaffFrameFlag = m_slice->slice_header->MbaffFrameFlag;
  const bool isMbAff = MbaffFrameFlag && mb.mb_field_decoding_flag;
  /* ------------------  End ------------------ */

  // -------------------- 1. 计算当前 4x4 亮度块的顶点（xO, yO）坐标 --------------------
  int32_t xO = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
               InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
  int32_t yO = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
               InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

  // -------------------- 2. 邻域样本查找 --------------------
  // 邻域样本数组初始化，相对于当前块的位置
  const int32_t neighbouring_samples_x[13] = {-1, -1, -1, -1, -1, 0, 1,
                                              2,  3,  4,  5,  6,  7};
  const int32_t neighbouring_samples_y[13] = {-1, 0,  1,  2,  3,  -1, -1,
                                              -1, -1, -1, -1, -1, -1};
  //p用于存储当前块的 4x4 样本以及其周围的邻域样本
  int32_t p[5 * 9] = {0};
  memset(p, -1, sizeof(p));
#define P(x, y) p[((y) + 1) * 9 + ((x) + 1)]

  int32_t xW = 0, yW = 0, maxW = 16, maxH = 16, mbAddrN = -1;
  if (isChroma) maxW = MbWidthC, maxH = MbHeightC;
  int32_t luma4x4BlkIdxN = 0, luma8x8BlkIdxN = 0;
  MB_ADDR_TYPE mbAddrN_type = MB_ADDR_TYPE_UNKOWN;

  // 遍历每个相邻样本
  for (int32_t i = 0; i < 13; i++) {
    // a. 根据邻域样本查找对应所属的宏块，输出为mbAddrN
    // 6.4.12 Derivation process for neighbouring locations
    const int32_t x = neighbouring_samples_x[i], y = neighbouring_samples_y[i];
    if (MbaffFrameFlag) {
      RET(neighbouring_locations_MBAFF(xO + x, yO + y, maxW, maxH, CurrMbAddr,
                                       mbAddrN_type, mbAddrN, luma4x4BlkIdxN,
                                       luma8x8BlkIdxN, xW, yW, isChroma));
    } else
      RET(neighbouring_locations_non_MBAFF(
          xO + x, yO + y, maxW, maxH, CurrMbAddr, mbAddrN_type, mbAddrN,
          luma4x4BlkIdxN, luma8x8BlkIdxN, xW, yW, isChroma));

    // b. 邻近块的有效性判断: 当样本 p[ x, y ] = -1 时表示标记为“不适用于 Intra_4x4 预测”
    const MacroBlock &mb1 = m_mbs[mbAddrN];
    //当前没有可用的作为预测的相邻块
    if (mbAddrN < 0) P(x, y) = -1;
    //邻近块为帧内预测，但不允许使用相邻预测
    else if (IS_INTER_Prediction_Mode(mb1.m_mb_pred_mode) &&
             mb1.constrained_intra_pred_flag)
      P(x, y) = -1;
    //邻近块为切换Slice块，当前块不为切换Slice块，但不允许使用相邻预测
    else if (mb1.m_name_of_mb_type == SI && mb1.constrained_intra_pred_flag &&
             mb.m_name_of_mb_type != SI)
      P(x, y) = -1;
    //当前索引为第一行和第三行的最后一个宏块, TODO 为什么是这两个宏块？ <24-10-09 00:59:31, YangJing>
    else if (x > 3 && (luma4x4BlkIdx == 3 || luma4x4BlkIdx == 11))
      P(x, y) = -1;
    else {
      //该邻近样本，可用于 Intra_4x4 预测，用作当前4x4宏块的目标宏块 mbAddrN 的左上亮度样本的位置(xM,yM)
      int32_t xM = 0, yM = 0;
      inverse_mb_scanning_process(MbaffFrameFlag, mbAddrN,
                                  mb1.mb_field_decoding_flag, xM, yM);
      int32_t y0 = (yM + 1 * yW);
      if (MbaffFrameFlag && mb1.mb_field_decoding_flag) y0 = (yM + 2 * yW);
      P(x, y) = pic_buff_luma_pred[y0 * PicWidthInSamples + (xM + xW)];
    }
  }

  // 当 "右上角的邻近宏块样本" 被标记为“不可用于 Intra_4x4 预测”，且 "上方邻近宏块的最后一个样本" 被标记为“可用于 Intra_4x4 预测”时: "右上角的邻近宏块样本" 4个样本均替换为 "上方邻近宏块的最后一个样本" ，为了下面更好的预测，并没有改变原有的样本。
  if (P(4, -1) < 0 && P(5, -1) < 0 && P(6, -1) < 0 && P(7, -1) < 0 &&
      P(3, -1) >= 0) {
    P(4, -1) = P(3, -1);
    P(5, -1) = P(3, -1);
    P(6, -1) = P(3, -1);
    P(7, -1) = P(3, -1);
  }

  // 获取当前使用的帧间预测模式，将从下面9个预测模式中进行挑选
  int32_t currMbAddrPredMode = -1;

  RET(getIntra4x4PredMode(luma4x4BlkIdx, currMbAddrPredMode, isChroma));

  //----------9种帧内4x4预测模式----------------
#define cSL(x, y)                                                              \
  pic_buff_luma_pred[(mb.m_mb_position_y + (yO + (y)) * (1 + isMbAff)) *       \
                         PicWidthInSamples +                                   \
                     (mb.m_mb_position_x + (xO + (x)))]

  // 8.3.1.2.1 Specification of Intra_4x4_Vertical prediction mode
  if (currMbAddrPredMode == 0) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0)
      for (int32_t y = 0; y <= 3; y++)
        for (int32_t x = 0; x <= 3; x++)
          cSL(x, y) = P(x, -1);
  }
  // 8.3.1.2.2 Specification of Intra_4x4_Horizontal prediction mode
  else if (currMbAddrPredMode == 1) {
    if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0)
      for (int32_t y = 0; y <= 3; y++)
        for (int32_t x = 0; x <= 3; x++)
          cSL(x, y) = P(-1, y);
  }
  // 8.3.1.2.3 Specification of Intra_4x4_DC prediction mode
  else if (currMbAddrPredMode == 2) {
    int32_t mean_value = 0;
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0)
      mean_value = (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + P(-1, 0) +
                    P(-1, 1) + P(-1, 2) + P(-1, 3) + 4) >>
                   3;
    else if ((P(0, -1) < 0 || P(1, -1) < 0 || P(2, -1) < 0 || P(3, -1) < 0) &&
             (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0))
      mean_value = (P(-1, 0) + P(-1, 1) + P(-1, 2) + P(-1, 3) + 2) >> 2;
    else if ((P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 &&
              P(3, -1) >= 0) &&
             (P(-1, 0) < 0 || P(-1, 1) < 0 || P(-1, 2) < 0 || P(-1, 3) < 0))
      mean_value = (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + 2) >> 2;
    else
      mean_value = 1 << (BitDepth - 1);

    for (int32_t y = 0; y <= 3; y++)
      for (int32_t x = 0; x <= 3; x++)
        cSL(x, y) = mean_value;
  }
  // 8.3.1.2.4 Specification of Intra_4x4_Diagonal_Down_Left prediction mode
  else if (currMbAddrPredMode == 3) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0)
      for (int32_t y = 0; y <= 3; y++)
        for (int32_t x = 0; x <= 3; x++) {
          if (x == 3 && y == 3)
            cSL(x, y) = (P(6, -1) + 3 * P(7, -1) + 2) >> 2;
          else
            cSL(x, y) =
                (P(x + y, -1) + 2 * P(x + y + 1, -1) + P(x + y + 2, -1) + 2) >>
                2;
        }
  }
  // 8.3.1.2.5 Specification of Intra_4x4_Diagonal_Down_Right prediction mode
  else if (currMbAddrPredMode == 4) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0)
      for (int32_t y = 0; y <= 3; y++)
        for (int32_t x = 0; x <= 3; x++) {
          if (x > y)
            cSL(x, y) =
                (P(x - y - 2, -1) + 2 * P(x - y - 1, -1) + P(x - y, -1) + 2) >>
                2;
          else if (x < y)
            cSL(x, y) =
                (P(-1, y - x - 2) + 2 * P(-1, y - x - 1) + P(-1, y - x) + 2) >>
                2;
          else
            cSL(x, y) = (P(0, -1) + 2 * P(-1, -1) + P(-1, 0) + 2) >> 2;
        }
  }
  // 8.3.1.2.6 Specification of Intra_4x4_Vertical_Right prediction mode
  else if (currMbAddrPredMode == 5) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0)
      for (int32_t y = 0; y <= 3; y++)
        for (int32_t x = 0; x <= 3; x++) {
          int32_t zVR = 2 * x - y;
          if (zVR == 0 || zVR == 2 || zVR == 4 || zVR == 6)
            cSL(x, y) =
                (P(x - (y >> 1) - 1, -1) + P(x - (y >> 1), -1) + 1) >> 1;
          else if (zVR == 1 || zVR == 3 || zVR == 5)
            cSL(x, y) = (P(x - (y >> 1) - 2, -1) + 2 * P(x - (y >> 1) - 1, -1) +
                         P(x - (y >> 1), -1) + 2) >>
                        2;
          else if (zVR == -1)
            cSL(x, y) = (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >> 2;
          else
            cSL(x, y) =
                (P(-1, y - 1) + 2 * P(-1, y - 2) + P(-1, y - 3) + 2) >> 2;
        }
  }
  // 8.3.1.2.7 Specification of Intra_4x4_Horizontal_Down prediction mode
  else if (currMbAddrPredMode == 6) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0)
      for (int32_t y = 0; y <= 3; y++)
        for (int32_t x = 0; x <= 3; x++) {
          int32_t zHD = 2 * y - x;
          if (zHD == 0 || zHD == 2 || zHD == 4 || zHD == 6)
            cSL(x, y) =
                (P(-1, y - (x >> 1) - 1) + P(-1, y - (x >> 1)) + 1) >> 1;
          else if (zHD == 1 || zHD == 3 || zHD == 5)
            cSL(x, y) = (P(-1, y - (x >> 1) - 2) + 2 * P(-1, y - (x >> 1) - 1) +
                         P(-1, y - (x >> 1)) + 2) >>
                        2;
          else if (zHD == -1)
            cSL(x, y) = (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >> 2;
          else
            cSL(x, y) =
                (P(x - 1, -1) + 2 * P(x - 2, -1) + P(x - 3, -1) + 2) >> 2;
        }
  }
  // 8.3.1.2.8 Specification of Intra_4x4_Vertical_Left prediction mode
  else if (currMbAddrPredMode == 7) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0)
      for (int32_t y = 0; y <= 3; y++)
        for (int32_t x = 0; x <= 3; x++) {
          if (y == 0 || y == 2)
            cSL(x, y) =
                (P(x + (y >> 1), -1) + P(x + (y >> 1) + 1, -1) + 1) >> 1;
          else
            cSL(x, y) = (P(x + (y >> 1), -1) + 2 * P(x + (y >> 1) + 1, -1) +
                         P(x + (y >> 1) + 2, -1) + 2) >>
                        2;
        }
  }
  // 8.3.1.2.9 Specification of Intra_4x4_Horizontal_Up prediction mode
  else if (currMbAddrPredMode == 8) {
    if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0)
      for (int32_t y = 0; y <= 3; y++)
        for (int32_t x = 0; x <= 3; x++) {
          int32_t zHU = x + 2 * y;
          if (zHU == 0 || zHU == 2 || zHU == 4)
            cSL(x, y) =
                (P(-1, y + (x >> 1)) + P(-1, y + (x >> 1) + 1) + 1) >> 1;
          else if (zHU == 1 || zHU == 3)
            cSL(x, y) = (P(-1, y + (x >> 1)) + 2 * P(-1, y + (x >> 1) + 1) +
                         P(-1, y + (x >> 1) + 2) + 2) >>
                        2;
          else if (zHU == 5)
            cSL(x, y) = (P(-1, 2) + 3 * P(-1, 3) + 2) >> 2;
          else
            cSL(x, y) = P(-1, 3);
        }
  }

#undef P
#undef cSL

  return 0;
}

// 8.3.2.2 Intra_8x8 sample prediction (8.3.2 Intra_8x8 prediction process for luma sampless)
int PictureBase::intra_8x8_sample_prediction(int32_t luma8x8BlkIdx,
                                             int32_t PicWidthInSamples,
                                             uint8_t *pic_buff_luma_pred,
                                             int32_t isChroma,
                                             int32_t BitDepth) {

  /* ------------------ 设置别名 ------------------ */
  const MacroBlock &mb = m_mbs[CurrMbAddr];
  const bool MbaffFrameFlag = m_slice->slice_header->MbaffFrameFlag;
  const bool isMbAff = MbaffFrameFlag && mb.mb_field_decoding_flag;
  /* ------------------  End ------------------ */

  // 6.4.5 Inverse 8x8 luma block scanning process
  int32_t xO = InverseRasterScan(luma8x8BlkIdx, 8, 8, 16, 0);
  int32_t yO = InverseRasterScan(luma8x8BlkIdx, 8, 8, 16, 1);
  // x范围[-1,15]，y范围[-1,7]，共9行17列，原点为pp[1][1]
  int32_t p[9 * 17] = {-1};
  // x范围[-1,15]，y范围[-1,7]，共9行17列，原点为pp[1][1]
  int32_t p1[9 * 17] = {-1};
#define P(x, y) p[((y) + 1) * 17 + ((x) + 1)]
#define P1(x, y) p1[((y) + 1) * 17 + ((x) + 1)]
  memset(p, -1, sizeof(p));
  memset(p1, -1, sizeof(p1));

  const int32_t neighbouring_samples_x[25] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2, 3,
      4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15};
  const int32_t neighbouring_samples_y[25] = {
      -1, 0,  1,  2,  3,  4,  5,  6,  7,  -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  int32_t xW = 0, yW = 0, maxW = 16, maxH = 16, mbAddrN = -1;
  if (isChroma) maxW = MbWidthC, maxH = MbHeightC;
  int32_t luma4x4BlkIdxN = 0, luma8x8BlkIdxN = 0;
  MB_ADDR_TYPE mbAddrN_type = MB_ADDR_TYPE_UNKOWN;

  for (int32_t i = 0; i < 25; i++) {
    // 6.4.12 Derivation process for neighbouring locations
    const int32_t x = neighbouring_samples_x[i], y = neighbouring_samples_y[i];

    if (MbaffFrameFlag) {
      RET(neighbouring_locations_MBAFF(xO + x, yO + y, maxW, maxH, CurrMbAddr,
                                       mbAddrN_type, mbAddrN, luma4x4BlkIdxN,
                                       luma8x8BlkIdxN, xW, yW, isChroma));
    } else
      RET(neighbouring_locations_non_MBAFF(
          xO + x, yO + y, maxW, maxH, CurrMbAddr, mbAddrN_type, mbAddrN,
          luma4x4BlkIdxN, luma8x8BlkIdxN, xW, yW, isChroma));

    const MacroBlock &mb1 = m_mbs[mbAddrN];
    if (mbAddrN < 0 || (IS_INTER_Prediction_Mode(mb1.m_mb_pred_mode) &&
                        mb1.constrained_intra_pred_flag))
      P(x, y) = -1;
    else {
      int32_t xM = 0, yM = 0;
      inverse_mb_scanning_process(MbaffFrameFlag, mbAddrN,
                                  mb1.mb_field_decoding_flag, xM, yM);
      int32_t y0 = (yM + 1 * yW);
      if (MbaffFrameFlag && mb1.mb_field_decoding_flag) y0 = (yM + 2 * yW);
      P(x, y) = pic_buff_luma_pred[y0 * PicWidthInSamples + (xM + xW)];
    }
  }

  if (P(8, -1) < 0 && P(9, -1) < 0 && P(10, -1) < 0 && P(11, -1) < 0 &&
      P(12, -1) < 0 && P(13, -1) < 0 && P(14, -1) < 0 && P(15, -1) < 0 &&
      P(7, -1) >= 0) {
    P(8, -1) = P(7, -1);
    P(9, -1) = P(7, -1);
    P(10, -1) = P(7, -1);
    P(11, -1) = P(7, -1);
    P(12, -1) = P(7, -1);
    P(13, -1) = P(7, -1);
    P(14, -1) = P(7, -1);
    P(15, -1) = P(7, -1);
  }

  if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
      P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
      P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
      P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0) {
    if (P(-1, -1) >= 0)
      P1(0, -1) = (P(-1, -1) + 2 * P(0, -1) + P(1, -1) + 2) >> 2;
    else
      P1(0, -1) = (3 * P(0, -1) + P(1, -1) + 2) >> 2;

    for (int32_t x = 1; x <= 14; x++)
      P1(x, -1) = (P(x - 1, -1) + 2 * P(x, -1) + P(x + 1, -1) + 2) >> 2;

    P1(15, -1) = (P(14, -1) + 3 * P(15, -1) + 2) >> 2;
  }

  if (P(-1, -1) >= 0) {
    if (P(0, -1) < 0 || P(-1, 0) < 0) {
      if (P(0, -1) >= 0)
        P1(-1, -1) = (3 * P(-1, -1) + P(0, -1) + 2) >> 2;
      else if (P(0, -1) < 0 && P(-1, 0) >= 0)
        P1(-1, -1) = (3 * P(-1, -1) + P(-1, 0) + 2) >> 2;
      else
        P1(-1, -1) = P(-1, -1);
    } else
      P1(-1, -1) = (P(0, -1) + 2 * P(-1, -1) + P(-1, 0) + 2) >> 2;
  }

  if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
      P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0) {
    if (P(-1, -1) >= 0)
      P1(-1, 0) = (P(-1, -1) + 2 * P(-1, 0) + P(-1, 1) + 2) >> 2;
    else
      P1(-1, 0) = (3 * P(-1, 0) + P(-1, 1) + 2) >> 2;

    for (int32_t y = 1; y <= 6; y++)
      P1(-1, y) = (P(-1, y - 1) + 2 * P(-1, y) + P(-1, y + 1) + 2) >> 2;

    P1(-1, 7) = (P(-1, 6) + 3 * P(-1, 7) + 2) >> 2;
  }

  memcpy(p, p1, sizeof(p1));

  //----------9种帧内8x8预测模式----------------
  int32_t currMbAddrPredMode = -1;
  RET(getIntra8x8PredMode(luma8x8BlkIdx, currMbAddrPredMode, isChroma));

#define cSL(x, y)                                                              \
  pic_buff_luma_pred[(mb.m_mb_position_y + (yO + (y)) * (1 + isMbAff)) *       \
                         PicWidthInSamples +                                   \
                     (mb.m_mb_position_x + (xO + (x)))]
  // 8.3.2.2.2 Specification of Intra_8x8_Vertical prediction mode
  if (currMbAddrPredMode == 0) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0)
      for (int32_t y = 0; y <= 7; y++)
        for (int32_t x = 0; x <= 7; x++)
          cSL(x, y) = P(x, -1);
  }
  // 8.3.2.2.3 Specification of Intra_8x8_Horizontal prediction mode}
  else if (currMbAddrPredMode == 1) {
    if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0)
      for (int32_t y = 0; y <= 7; y++)
        for (int32_t x = 0; x <= 7; x++)
          cSL(x, y) = P(-1, y);
  }
  // 8.3.2.2.4 Specification of Intra_8x8_DC prediction mode
  else if (currMbAddrPredMode == 2) {
    int32_t mean_value = 0;

    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0)
      mean_value =
          (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + P(4, -1) + P(5, -1) +
           P(6, -1) + P(7, -1) + P(-1, 0) + P(-1, 1) + P(-1, 2) + P(-1, 3) +
           P(-1, 4) + P(-1, 5) + P(-1, 6) + P(-1, 7) + 8) >>
          4;
    else if ((P(0, -1) < 0 || P(1, -1) < 0 || P(2, -1) < 0 || P(3, -1) < 0 ||
              P(4, -1) < 0 || P(5, -1) < 0 || P(6, -1) < 0 || P(7, -1) < 0) &&
             (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
              P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 &&
              P(-1, 6) >= 0 && P(-1, 7) >= 0))
      mean_value = (P(-1, 0) + P(-1, 1) + P(-1, 2) + P(-1, 3) + P(-1, 4) +
                    P(-1, 5) + P(-1, 6) + P(-1, 7) + 4) >>
                   3;
    else if ((P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 &&
              P(3, -1) >= 0 && P(4, -1) >= 0 && P(5, -1) >= 0 &&
              P(6, -1) >= 0 && P(7, -1) >= 0) &&
             (P(-1, 0) < 0 || P(-1, 1) < 0 || P(-1, 2) < 0 || P(-1, 3) < 0 ||
              P(-1, 4) < 0 || P(-1, 5) < 0 || P(-1, 6) < 0 || P(-1, 7) < 0))
      mean_value = (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + P(4, -1) +
                    P(5, -1) + P(6, -1) + P(7, -1) + 4) >>
                   3;
    else
      mean_value = 1 << (BitDepth - 1);

    for (int32_t y = 0; y <= 7; y++)
      for (int32_t x = 0; x <= 7; x++)
        cSL(x, y) = mean_value;
  }
  // 8.3.2.2.5 Specification of Intra_8x8_Diagonal_Down_Left prediction mode
  else if (currMbAddrPredMode == 3) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
        P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0)
      for (int32_t y = 0; y <= 7; y++)
        for (int32_t x = 0; x <= 7; x++) {
          if (x == 7 && y == 7)
            cSL(x, y) = (P(14, -1) + 3 * P(15, -1) + 2) >> 2;
          else
            cSL(x, y) =
                (P(x + y, -1) + 2 * P(x + y + 1, -1) + P(x + y + 2, -1) + 2) >>
                2;
        }
  }
  // 8.3.2.2.6 Specification of Intra_8x8_Diagonal_Down_Right prediction mode
  else if (currMbAddrPredMode == 4) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 &&
        P(-1, 7) >= 0)
      for (int32_t y = 0; y <= 7; y++)
        for (int32_t x = 0; x <= 7; x++) {
          if (x > y)
            cSL(x, y) =
                (P(x - y - 2, -1) + 2 * P(x - y - 1, -1) + P(x - y, -1) + 2) >>
                2;
          else if (x < y)
            cSL(x, y) =
                (P(-1, y - x - 2) + 2 * P(-1, y - x - 1) + P(-1, y - x) + 2) >>
                2;
          else
            cSL(x, y) = (P(0, -1) + 2 * P(-1, -1) + P(-1, 0) + 2) >> 2;
        }
  }
  // 8.3.2.2.7 Specification of Intra_8x8_Vertical_Right prediction mode
  else if (currMbAddrPredMode == 5) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 &&
        P(-1, 7) >= 0)
      for (int32_t y = 0; y <= 7; y++)
        for (int32_t x = 0; x <= 7; x++) {
          int32_t zVR = 2 * x - y;

          if (zVR == 0 || zVR == 2 || zVR == 4 || zVR == 6 || zVR == 8 ||
              zVR == 10 || zVR == 12 || zVR == 14)
            cSL(x, y) =
                (P(x - (y >> 1) - 1, -1) + P(x - (y >> 1), -1) + 1) >> 1;
          else if (zVR == 1 || zVR == 3 || zVR == 5 || zVR == 7 || zVR == 9 ||
                   zVR == 11 || zVR == 13)
            cSL(x, y) = (P(x - (y >> 1) - 2, -1) + 2 * P(x - (y >> 1) - 1, -1) +
                         P(x - (y >> 1), -1) + 2) >>
                        2;
          else if (zVR == -1)
            cSL(x, y) = (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >> 2;
          else
            cSL(x, y) = (P(-1, y - 2 * x - 1) + 2 * P(-1, y - 2 * x - 2) +
                         P(-1, y - 2 * x - 3) + 2) >>
                        2;
        }
  }
  // 8.3.2.2.8 Specification of Intra_8x8_Horizontal_Down prediction mode
  else if (currMbAddrPredMode == 6) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 &&
        P(-1, 7) >= 0)
      for (int32_t y = 0; y <= 7; y++)
        for (int32_t x = 0; x <= 7; x++) {
          int32_t zHD = 2 * y - x;

          if (zHD == 0 || zHD == 2 || zHD == 4 || zHD == 6 || zHD == 8 ||
              zHD == 10 || zHD == 12 || zHD == 14)
            cSL(x, y) =
                (P(-1, y - (x >> 1) - 1) + P(-1, y - (x >> 1)) + 1) >> 1;
          else if (zHD == 1 || zHD == 3 || zHD == 5 || zHD == 7 || zHD == 9 ||
                   zHD == 11 || zHD == 13)
            cSL(x, y) = (P(-1, y - (x >> 1) - 2) + 2 * P(-1, y - (x >> 1) - 1) +
                         P(-1, y - (x >> 1)) + 2) >>
                        2;
          else if (zHD == -1)
            cSL(x, y) = (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >> 2;
          else
            cSL(x, y) = (P(x - 2 * y - 1, -1) + 2 * P(x - 2 * y - 2, -1) +
                         P(x - 2 * y - 3, -1) + 2) >>
                        2;
        }
  }
  // 8.3.2.2.9 Specification of Intra_8x8_Vertical_Left prediction mode
  else if (currMbAddrPredMode == 7) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
        P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0)
      for (int32_t y = 0; y <= 7; y++)
        for (int32_t x = 0; x <= 7; x++) {
          if (y == 0 || y == 2 || y == 4 || y == 6)
            cSL(x, y) =
                (P(x + (y >> 1), -1) + P(x + (y >> 1) + 1, -1) + 1) >> 1;
          else
            cSL(x, y) = (P(x + (y >> 1), -1) + 2 * P(x + (y >> 1) + 1, -1) +
                         P(x + (y >> 1) + 2, -1) + 2) >>
                        2;
        }
  }
  // 8.3.2.2.10 Specification of Intra_8x8_Horizontal_Up prediction mode
  else if (currMbAddrPredMode == 8) {
    if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0)
      for (int32_t y = 0; y <= 7; y++)
        for (int32_t x = 0; x <= 7; x++) {
          int32_t zHU = x + 2 * y;

          if (zHU == 0 || zHU == 2 || zHU == 4 || zHU == 6 || zHU == 8 ||
              zHU == 10 || zHU == 12)
            cSL(x, y) =
                (P(-1, y + (x >> 1)) + P(-1, y + (x >> 1) + 1) + 1) >> 1;
          else if (zHU == 1 || zHU == 3 || zHU == 5 || zHU == 7 || zHU == 9 ||
                   zHU == 11)
            cSL(x, y) = (P(-1, y + (x >> 1)) + 2 * P(-1, y + (x >> 1) + 1) +
                         P(-1, y + (x >> 1) + 2) + 2) >>
                        2;
          else if (zHU == 13)
            cSL(x, y) = (P(-1, 6) + 3 * P(-1, 7) + 2) >> 2;
          else
            cSL(x, y) = P(-1, 7);
        }
  }
#undef P
#undef P1
#undef cSL

  return 0;
}

// 8.3.3 Intra_16x16 prediction process for luma samples
int PictureBase::intra_16x16_sample_prediction(uint8_t *pic_buff_luma_pred,
                                               int32_t PicWidthInSamples,
                                               int32_t isChroma,
                                               int32_t BitDepth) {
  /* ------------------ 设置别名 ------------------ */
  const MacroBlock &mb = m_mbs[CurrMbAddr];
  const bool MbaffFrameFlag = m_slice->slice_header->MbaffFrameFlag;
  const bool isMbAff = MbaffFrameFlag && mb.mb_field_decoding_flag;
  /* ------------------  End ------------------ */

  int32_t xO = 0, yO = 0;

  // x范围[-1,15]，y范围[-1,15]，共17行17列，原点为pp[1][1]
  int32_t p[17 * 17] = {-1};
  memset(p, -1, sizeof(p));
#define P(x, y) p[((y) + 1) * 17 + ((x) + 1)]

  int32_t neighbouring_samples_x[33] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15};
  int32_t neighbouring_samples_y[33] = {
      -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  int32_t xW = 0, yW = 0, maxW = 16, maxH = 16, mbAddrN = -1;
  if (isChroma) maxW = MbWidthC, maxH = MbHeightC;
  int32_t luma4x4BlkIdxN = 0, luma8x8BlkIdxN = 0;
  MB_ADDR_TYPE mbAddrN_type = MB_ADDR_TYPE_UNKOWN;

  for (int32_t i = 0; i < 33; i++) {
    const int32_t x = neighbouring_samples_x[i], y = neighbouring_samples_y[i];
    if (MbaffFrameFlag) {
      RET(neighbouring_locations_MBAFF(xO + x, yO + y, maxW, maxH, CurrMbAddr,
                                       mbAddrN_type, mbAddrN, luma4x4BlkIdxN,
                                       luma8x8BlkIdxN, xW, yW, isChroma));
    } else
      RET(neighbouring_locations_non_MBAFF(
          xO + x, yO + y, maxW, maxH, CurrMbAddr, mbAddrN_type, mbAddrN,
          luma4x4BlkIdxN, luma8x8BlkIdxN, xW, yW, isChroma));

    const MacroBlock &mb1 = m_mbs[mbAddrN];
    //当前没有可用的作为预测的相邻块
    if (mbAddrN < 0) P(x, y) = -1;
    //邻近块为帧内预测，但不允许使用相邻预测
    else if (IS_INTER_Prediction_Mode(mb1.m_mb_pred_mode) &&
             mb1.constrained_intra_pred_flag)
      P(x, y) = -1;
    //邻近块为切换Slice块，但不允许使用相邻预测
    else if (mb1.m_name_of_mb_type == SI && mb1.constrained_intra_pred_flag)
      P(x, y) = -1;
    else {
      int32_t xM = 0, yM = 0;
      inverse_mb_scanning_process(MbaffFrameFlag, mbAddrN,
                                  mb1.mb_field_decoding_flag, xM, yM);
      int32_t y0 = (yM + 1 * yW);
      if (MbaffFrameFlag && mb1.mb_field_decoding_flag) y0 = (yM + 2 * yW);
      P(x, y) = pic_buff_luma_pred[y0 * PicWidthInSamples + (xM + xW)];
    }
  }

  //----------4种帧内16x16预测模式----------------
  int32_t currMbAddrPredMode = m_mbs[CurrMbAddr].Intra16x16PredMode;

#define cSL(x, y)                                                              \
  pic_buff_luma_pred[(mb.m_mb_position_y + (y) * (1 + isMbAff)) *              \
                         PicWidthInSamples +                                   \
                     (mb.m_mb_position_x + (x))]
  // 8.3.3.1 Specification of Intra_16x16_Vertical prediction mode
  if (currMbAddrPredMode == 0) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
        P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0)
      for (int32_t y = 0; y <= 15; y++)
        for (int32_t x = 0; x <= 15; x++)
          cSL(x, y) = P(x, -1);
  }
  // 8.3.3.2 Specification of Intra_16x16_Horizontal prediction mode
  else if (currMbAddrPredMode == 1) {
    if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0 &&
        P(-1, 8) >= 0 && P(-1, 9) >= 0 && P(-1, 10) >= 0 && P(-1, 11) >= 0 &&
        P(-1, 12) >= 0 && P(-1, 13) >= 0 && P(-1, 14) >= 0 && P(-1, 15) >= 0)
      for (int32_t y = 0; y <= 15; y++)
        for (int32_t x = 0; x <= 15; x++)
          cSL(x, y) = P(-1, y);
  }
  // 8.3.3.3 Specification of Intra_16x16_DC prediction mode
  else if (currMbAddrPredMode == 2) {
    int32_t mean_value = 0;
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
        P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0 &&
        P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0 &&
        P(-1, 8) >= 0 && P(-1, 9) >= 0 && P(-1, 10) >= 0 && P(-1, 11) >= 0 &&
        P(-1, 12) >= 0 && P(-1, 13) >= 0 && P(-1, 14) >= 0 && P(-1, 15) >= 0)
      mean_value =
          (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + P(4, -1) + P(5, -1) +
           P(6, -1) + P(7, -1) + P(8, -1) + P(9, -1) + P(10, -1) + P(11, -1) +
           P(12, -1) + P(13, -1) + P(14, -1) + P(15, -1) + P(-1, 0) + P(-1, 1) +
           P(-1, 2) + P(-1, 3) + P(-1, 4) + P(-1, 5) + P(-1, 6) + P(-1, 7) +
           P(-1, 8) + P(-1, 9) + P(-1, 10) + P(-1, 11) + P(-1, 12) + P(-1, 13) +
           P(-1, 14) + P(-1, 15) + 16) >>
          5;
    else if (!(P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 &&
               P(3, -1) >= 0 && P(4, -1) >= 0 && P(5, -1) >= 0 &&
               P(6, -1) >= 0 && P(7, -1) >= 0 && P(8, -1) >= 0 &&
               P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
               P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 &&
               P(15, -1) >= 0) &&
             (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
              P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 &&
              P(-1, 6) >= 0 && P(-1, 7) >= 0 && P(-1, 8) >= 0 &&
              P(-1, 9) >= 0 && P(-1, 10) >= 0 && P(-1, 11) >= 0 &&
              P(-1, 12) >= 0 && P(-1, 13) >= 0 && P(-1, 14) >= 0 &&
              P(-1, 15) >= 0))
      mean_value =
          (P(-1, 0) + P(-1, 1) + P(-1, 2) + P(-1, 3) + P(-1, 4) + P(-1, 5) +
           P(-1, 6) + P(-1, 7) + P(-1, 8) + P(-1, 9) + P(-1, 10) + P(-1, 11) +
           P(-1, 12) + P(-1, 13) + P(-1, 14) + P(-1, 15) + 8) >>
          4;
    else if ((P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 &&
              P(3, -1) >= 0 && P(4, -1) >= 0 && P(5, -1) >= 0 &&
              P(6, -1) >= 0 && P(7, -1) >= 0 && P(8, -1) >= 0 &&
              P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
              P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 &&
              P(15, -1) >= 0) &&
             !(P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
               P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 &&
               P(-1, 6) >= 0 && P(-1, 7) >= 0 && P(-1, 8) >= 0 &&
               P(-1, 9) >= 0 && P(-1, 10) >= 0 && P(-1, 11) >= 0 &&
               P(-1, 12) >= 0 && P(-1, 13) >= 0 && P(-1, 14) >= 0 &&
               P(-1, 15) >= 0))
      mean_value =
          (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + P(4, -1) + P(5, -1) +
           P(6, -1) + P(7, -1) + P(8, -1) + P(9, -1) + P(10, -1) + P(11, -1) +
           P(12, -1) + P(13, -1) + P(14, -1) + P(15, -1) + 8) >>
          4;
    else
      mean_value = (1 << (BitDepth - 1));

    for (int32_t y = 0; y <= 15; y++)
      for (int32_t x = 0; x <= 15; x++)
        cSL(x, y) = mean_value;
  }
  // 8.3.3.4 Specification of Intra_16x16_Plane prediction mode
  else if (currMbAddrPredMode == 3) {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
        P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0 &&
        P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0 &&
        P(-1, 8) >= 0 && P(-1, 9) >= 0 && P(-1, 10) >= 0 && P(-1, 11) >= 0 &&
        P(-1, 12) >= 0 && P(-1, 13) >= 0 && P(-1, 14) >= 0 && P(-1, 15) >= 0) {
      int32_t H = 0, V = 0;

      for (int32_t x = 0; x <= 7; x++)
        H += (x + 1) * (P(8 + x, -1) - P(6 - x, -1));
      for (int32_t y = 0; y <= 7; y++)
        V += (y + 1) * (P(-1, 8 + y) - P(-1, 6 - y));

      int32_t a = 16 * (P(-1, 15) + P(15, -1));
      int32_t b = (5 * H + 32) >> 6;
      int32_t c = (5 * V + 32) >> 6;

      for (int32_t y = 0; y <= 15; y++)
        for (int32_t x = 0; x <= 15; x++)
          cSL(x, y) = CLIP3(0, (1 << BitDepth) - 1,
                            (a + b * (x - 7) + c * (y - 7) + 16) >> 5);
    }
  }
#undef P
#undef cSL

  return 0;
}

// 8.3.4 Intra prediction process for chroma samples
int PictureBase::intra_chroma_sample_prediction(uint8_t *pic_buff_chroma_pred,
                                                int32_t PicWidthInSamples) {
  if (m_slice->slice_header->m_sps->ChromaArrayType == 3)
    return intra_chroma_sample_prediction_for_YUV444(pic_buff_chroma_pred,
                                                     PicWidthInSamples);
  else
    return intra_chroma_sample_prediction_for_YUV420_or_YUV422(
        pic_buff_chroma_pred, PicWidthInSamples);

  return 0;
}

// 8.3.4 Intra prediction process for chroma samples
// NOTE: 与intra_16x16_sample_prediction()一样的逻辑，预测模式不一样
int PictureBase::intra_chroma_sample_prediction_for_YUV420_or_YUV422(
    uint8_t *pic_buff_chroma_pred, int32_t PicWidthInSamples) {

  /* ------------------ 设置别名 ------------------ */
  const MacroBlock &mb = m_mbs[CurrMbAddr];
  const bool MbaffFrameFlag = m_slice->slice_header->MbaffFrameFlag;
  const bool isMbAff = MbaffFrameFlag && mb.mb_field_decoding_flag;
  const uint32_t ChromaArrayType =
      m_slice->slice_header->m_sps->ChromaArrayType;
  /* ------------------  End ------------------ */

  int32_t xO = 0, yO = 0;

  /* TODO YangJing  <24-10-09 18:12:15> */
  // x范围[-1,15]，y范围[-1,15]，共17行17列，原点为pp[1][1]
  //int32_t p[17 * 17] = {-1};

  // x范围[-1,7]，y范围[-1,15]，共9行17列，原点为pp[1][1]
  int32_t p[9 * 17] = {-1};
  memset(p, -1, sizeof(p));
#define P(x, y) p[((y) + 1) * (MbHeightC + 1) + ((x) + 1)]

  /* 对色度进行子采样则：
      - YUV422: 8x16像素，即宽度是亮度的一半，高度相同;
      - YUV420: 8x8像素，即水平和垂直采样率都是亮度分量的一半;
    故按最大的尺寸申请 MbHeightC=16; MbWidthC=8; 25 = 16 + 8 + 1; */
  int32_t neighbouring_samples_x[25] = {-1}, neighbouring_samples_y[25] = {-1};
  memset(neighbouring_samples_x, -1, sizeof(neighbouring_samples_x));
  memset(neighbouring_samples_y, -1, sizeof(neighbouring_samples_y));
  for (int i = -1; i < (int32_t)MbHeightC; i++)
    neighbouring_samples_y[i + 1] = i;
  for (int i = 0; i < (int32_t)MbWidthC; i++)
    neighbouring_samples_x[MbHeightC + 1 + i] = i;

  int32_t xW = 0, yW = 0, maxW = 0, maxH = 0, mbAddrN = -1;
  const int32_t isChroma = 1;
  maxW = MbWidthC, maxH = MbHeightC;
  int32_t luma4x4BlkIdxN = 0, luma8x8BlkIdxN = 0;
  MB_ADDR_TYPE mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
  int32_t neighbouring_samples_count = MbHeightC + MbWidthC + 1;

  for (int32_t i = 0; i < neighbouring_samples_count; i++) {
    // 6.4.12 Derivation process for neighbouring locations
    const int32_t x = neighbouring_samples_x[i], y = neighbouring_samples_y[i];
    if (MbaffFrameFlag) {
      RET(neighbouring_locations_MBAFF(xO + x, yO + y, maxW, maxH, CurrMbAddr,
                                       mbAddrN_type, mbAddrN, luma4x4BlkIdxN,
                                       luma8x8BlkIdxN, xW, yW, isChroma));
    } else
      RET(neighbouring_locations_non_MBAFF(
          xO + x, yO + y, maxW, maxH, CurrMbAddr, mbAddrN_type, mbAddrN,
          luma4x4BlkIdxN, luma8x8BlkIdxN, xW, yW, isChroma));

    const MacroBlock &mb1 = m_mbs[mbAddrN];
    //当前没有可用的作为预测的相邻块
    if (mbAddrN < 0) P(x, y) = -1;
    //邻近块为帧内预测，但不允许使用相邻预测
    else if (IS_INTER_Prediction_Mode(mb1.m_mb_pred_mode) &&
             mb1.constrained_intra_pred_flag)
      P(x, y) = -1;
    //邻近块为切换Slice块，当前块不为切换Slice块，但不允许使用相邻预测
    else if (mb1.m_name_of_mb_type == SI && mb1.constrained_intra_pred_flag &&
             m_mbs[CurrMbAddr].m_name_of_mb_type != SI)
      P(x, y) = -1;
    else {
      int32_t xL = 0, yL = 0;
      inverse_mb_scanning_process(MbaffFrameFlag, mbAddrN,
                                  mb1.mb_field_decoding_flag, xL, yL);
      int32_t xM = (xL >> 4) * MbWidthC;
      int32_t yM = ((yL >> 4) * MbHeightC) + (yL % 2);

      int32_t y0 = (yM + 1 * yW);
      if (MbaffFrameFlag && mb1.mb_field_decoding_flag) y0 = (yM + 2 * yW);
      P(x, y) = pic_buff_chroma_pred[y0 * PicWidthInSamples + (xM + xW)];
    }
  }

  //----------4种帧内Chroma预测模式----------------
  int32_t currMbAddrPredMode = m_mbs[CurrMbAddr].intra_chroma_pred_mode;

#define cSC(x, y)                                                              \
  pic_buff_chroma_pred[(((mb.m_mb_position_y >> 4) * MbHeightC) +              \
                        (mb.m_mb_position_y % 2) + (y) * (1 + isMbAff)) *      \
                           PicWidthInSamples +                                 \
                       ((mb.m_mb_position_x >> 4) * MbWidthC + (x))]
  // 8.3.4.1 Specification of Intra_Chroma_DC prediction mode
  if (currMbAddrPredMode == 0) {
    for (int32_t BlkIdx = 0; BlkIdx < (1 << (ChromaArrayType + 1)); BlkIdx++) {
      xO = InverseRasterScan(BlkIdx, 4, 4, 8, 0);
      yO = InverseRasterScan(BlkIdx, 4, 4, 8, 1);
      int32_t mean_value = 0;
      if ((xO == 0 && yO == 0) || (xO > 0 && yO > 0)) {
        if (P(0 + xO, -1) > 0 && P(1 + xO, -1) > 0 && P(2 + xO, -1) > 0 &&
            P(3 + xO, -1) > 0 && P(-1, 0 + yO) > 0 && P(-1, 1 + yO) > 0 &&
            P(-1, 2 + yO) > 0 && P(-1, 3 + yO) > 0)
          mean_value = (P(0 + xO, -1) + P(1 + xO, -1) + P(2 + xO, -1) +
                        P(3 + xO, -1) + P(-1, 0 + yO) + P(-1, 1 + yO) +
                        P(-1, 2 + yO) + P(-1, 3 + yO) + 4) >>
                       3;
        else if (!(P(0 + xO, -1) > 0 && P(1 + xO, -1) > 0 &&
                   P(2 + xO, -1) > 0 && P(3 + xO, -1) > 0) &&
                 (P(-1, 0 + yO) > 0 && P(-1, 1 + yO) > 0 && P(-1, 2 + yO) > 0 &&
                  P(-1, 3 + yO) > 0))
          mean_value = (P(-1, 0 + yO) + P(-1, 1 + yO) + P(-1, 2 + yO) +
                        P(-1, 3 + yO) + 2) >>
                       2;
        else if ((P(0 + xO, -1) > 0 && P(1 + xO, -1) > 0 && P(2 + xO, -1) > 0 &&
                  P(3 + xO, -1) > 0) &&
                 !(P(-1, 0 + yO) > 0 && P(-1, 1 + yO) > 0 &&
                   P(-1, 2 + yO) > 0 && P(-1, 3 + yO) > 0))
          mean_value = (P(0 + xO, -1) + P(1 + xO, -1) + P(2 + xO, -1) +
                        P(3 + xO, -1) + 2) >>
                       2;
        else
          mean_value = (1 << (m_slice->slice_header->m_sps->BitDepthC - 1));
      } else if (xO > 0 && yO == 0) {
        if (P(0 + xO, -1) > 0 && P(1 + xO, -1) > 0 && P(2 + xO, -1) > 0 &&
            P(3 + xO, -1) > 0)
          mean_value = (P(0 + xO, -1) + P(1 + xO, -1) + P(2 + xO, -1) +
                        P(3 + xO, -1) + 2) >>
                       2;
        else if (P(-1, 0 + yO) > 0 && P(-1, 1 + yO) > 0 && P(-1, 2 + yO) > 0 &&
                 P(-1, 3 + yO) > 0)
          mean_value = (P(-1, 0 + yO) + P(-1, 1 + yO) + P(-1, 2 + yO) +
                        P(-1, 3 + yO) + 2) >>
                       2;
        else
          mean_value = (1 << (m_slice->slice_header->m_sps->BitDepthC - 1));
      } else if (xO == 0 && yO > 0) {
        if (P(-1, 0 + yO) > 0 && P(-1, 1 + yO) > 0 && P(-1, 2 + yO) > 0 &&
            P(-1, 3 + yO) > 0)
          mean_value = (P(-1, 0 + yO) + P(-1, 1 + yO) + P(-1, 2 + yO) +
                        P(-1, 3 + yO) + 2) >>
                       2;
        else if (P(0 + xO, -1) > 0 && P(1 + xO, -1) > 0 && P(2 + xO, -1) > 0 &&
                 P(3 + xO, -1) > 0)
          mean_value = (P(0 + xO, -1) + P(1 + xO, -1) + P(2 + xO, -1) +
                        P(3 + xO, -1) + 2) >>
                       2;
        else
          mean_value = (1 << (m_slice->slice_header->m_sps->BitDepthC - 1));
      }

      for (int32_t y = 0; y < 4; y++)
        for (int32_t x = 0; x < 4; x++)
          cSC(x + xO, y + yO) = mean_value;
    }
  }
  // 8.3.4.2 Specification of Intra_Chroma_Horizontal prediction mode
  else if (currMbAddrPredMode == 1) {
    bool flag = false;
    for (int32_t y = 0; y < (int32_t)MbHeightC; y++)
      if (P(-1, y) < 0) {
        flag = true;
        break;
      }

    if (flag == false)
      for (int32_t y = 0; y <= (int32_t)MbHeightC - 1; y++)
        for (int32_t x = 0; x <= (int32_t)MbWidthC - 1; x++)
          cSC(x, y) = P(-1, y);
  }
  // 8.3.4.3 Specification of Intra_Chroma_Vertical prediction mode
  else if (currMbAddrPredMode == 2) {
    bool flag = false;
    for (int32_t x = 0; x < (int32_t)MbWidthC; x++)
      if (P(x, -1) < 0) {
        flag = true;
        break;
      }

    if (flag == false)
      for (int32_t y = 0; y <= (int32_t)MbHeightC - 1; y++)
        for (int32_t x = 0; x <= (int32_t)MbWidthC - 1; x++)
          cSC(x, y) = P(x, -1);
  }
  // 8.3.4.4 Specification of Intra_Chroma_Plane prediction mode
  else if (currMbAddrPredMode == 3) {
    bool flag = false;
    for (int32_t x = 0; x < (int32_t)MbWidthC; x++)
      if (P(x, -1) < 0) {
        flag = true;
        break;
      }
    for (int32_t y = -1; y < (int32_t)MbHeightC; y++)
      if (P(-1, y) < 0) {
        flag = true;
        break;
      }
    if (flag == false) {
      int32_t H = 0, V = 0, xCF = ((ChromaArrayType == 3) ? 4 : 0),
              yCF = ((ChromaArrayType != 1) ? 4 : 0);

      for (int32_t x1 = 0; x1 < 4 + xCF; x1++)
        H += (x1 + 1) * (P(4 + xCF + x1, -1) - P(2 + xCF - x1, -1));
      for (int32_t y1 = 0; y1 < 4 + yCF; y1++)
        V += (y1 + 1) * (P(-1, 4 + yCF + y1) - P(-1, 2 + yCF - y1));

      int32_t a = 16 * (P(-1, MbHeightC - 1) + P(MbWidthC - 1, -1));
      int32_t b = ((34 - 29 * (ChromaArrayType == 3)) * H + 32) >> 6;
      int32_t c = ((34 - 29 * (ChromaArrayType != 1)) * V + 32) >> 6;

      for (int32_t y = 0; y <= (int32_t)MbHeightC - 1; y++)
        for (int32_t x = 0; x <= (int32_t)MbWidthC - 1; x++)
          cSC(x, y) =
              Clip1C((a + b * (x - 3 - xCF) + c * (y - 3 - yCF) + 16) >> 5,
                     m_slice->slice_header->m_sps->BitDepthC);
    }
  }

#undef P
#undef cSC

  return 0;
}

// 8.3.4 Intra prediction process for chroma samples
// 8.3.4.5 Intra prediction for chroma samples with ChromaArrayType equal to 3
[[deprecated]]
int PictureBase::intra_chroma_sample_prediction_for_YUV444(
    uint8_t *pic_buff_chroma_pred, int32_t PicWidthInSamples) {
  const int32_t isChroma = 1;
  int32_t BitDepth = m_slice->slice_header->m_sps->BitDepthC;

  if (m_mbs[CurrMbAddr].m_mb_pred_mode == Intra_4x4)
    for (int32_t chroma4x4BlkIdx = 0; chroma4x4BlkIdx < 16; chroma4x4BlkIdx++) {
      RET(intra_4x4_sample_prediction(chroma4x4BlkIdx, PicWidthInSamples,
                                      pic_buff_chroma_pred, isChroma,
                                      BitDepth));
    }
  else if (m_mbs[CurrMbAddr].m_mb_pred_mode == Intra_8x8)
    for (int32_t chroma4x4BlkIdx = 0; chroma4x4BlkIdx < 4; chroma4x4BlkIdx++) {
      RET(intra_8x8_sample_prediction(chroma4x4BlkIdx, PicWidthInSamples,
                                      pic_buff_chroma_pred, isChroma,
                                      BitDepth));
    }

  else if (m_mbs[CurrMbAddr].m_mb_pred_mode == Intra_16x16)
    RET(intra_16x16_sample_prediction(pic_buff_chroma_pred, PicWidthInSamples,
                                      isChroma, BitDepth));
  return 0;
}

// 8.3.5 Sample construction process for I_PCM macroblocks
int PictureBase::sample_construction_for_I_PCM() {
  /* ------------------ 设置别名 ------------------ */
  const SliceHeader *header = m_slice->slice_header;
  const MacroBlock &mb = m_mbs[CurrMbAddr];

  bool MbaffFrameFlag = header->MbaffFrameFlag;
  bool isMbAff = MbaffFrameFlag && mb.mb_field_decoding_flag;

  int32_t SubWidthC = header->m_sps->SubWidthC;
  int32_t SubHeightC = header->m_sps->SubHeightC;
  uint32_t ChromaArrayType = header->m_sps->ChromaArrayType;
  /* ------------------  End ------------------ */

  int32_t xP = 0, yP = 0;
  inverse_mb_scanning_process(MbaffFrameFlag, CurrMbAddr,
                              mb.mb_field_decoding_flag, xP, yP);

  /* 亮度样本复制 */
  int LumaSize = 16 * 16;
  for (int i = 0; i < LumaSize; ++i) {
    int y = yP + (1 + isMbAff) * (i / 16);
    int x = xP + (i % 16);
    m_pic_buff_luma[y * PicWidthInSamplesL + x] = mb.pcm_sample_luma[i];
  }

  /* 存在色度样本，则复制 */
  if (ChromaArrayType != 0) {
    int ChromaSize = MbWidthC * MbHeightC;
    for (int32_t i = 0; i < ChromaSize; ++i) {
      int y =
          (yP + SubHeightC - 1) / SubHeightC + (1 + isMbAff) * (i / MbWidthC);
      int x = xP / SubWidthC + (i % MbWidthC);
      m_pic_buff_cb[y * PicWidthInSamplesC + x] = mb.pcm_sample_chroma[i];
      m_pic_buff_cr[y * PicWidthInSamplesC + x] =
          mb.pcm_sample_chroma[i + ChromaSize];
    }
  }

  return 0;
}

// 8.5.14 Picture construction process prior to deblocking filter process (去块过滤过程之前的图片构造过程)
// 重建图像数据，写入最终数据到pic_buff，主要针对帧、场编码的不同情况
/* 输入：– 包含元素 uij 的样本数组 u，它是 16x16 亮度块或 (MbWidthC)x(MbHeightC) 色度块或 4x4 亮度块或 4x4 色度块或 8x8 亮度块，或者，当 ChromaArrayType等于 3，8x8 色度块，
 * – 当 u 不是 16x16 亮度块或 (MbWidthC)x(MbHeightC) 色度块时，块索引 luma4x4BlkIdx 或 chroma4x4BlkIdx 或 luma8x8BlkIdx 或 cb4x4BlkIdx 或 cr4x4BlkIdx 或 cb8x8BlkIdx 或idx。*/
int PictureBase::picture_construction_process_prior_to_deblocking_filter(
    int32_t *u, int32_t nW, int32_t nH, int32_t BlkIdx, int32_t isChroma,
    int32_t PicWidthInSamples, uint8_t *pic_buff) {

  /* ------------------ 设置别名 ------------------ */
  const int32_t mb_field_decoding_flag =
      m_mbs[CurrMbAddr].mb_field_decoding_flag;

  const SliceHeader *header = m_slice->slice_header;
  const bool MbaffFrameFlag = header->MbaffFrameFlag;
  const uint32_t ChromaArrayType = header->m_sps->ChromaArrayType;
  const int32_t SubWidthC = header->m_sps->SubWidthC;
  const int32_t SubHeightC = header->m_sps->SubHeightC;
  bool isMbAff = header->MbaffFrameFlag && mb_field_decoding_flag;
  /* ------------------  End ------------------ */

  int32_t xP = 0, yP = 0;
  inverse_mb_scanning_process(MbaffFrameFlag, CurrMbAddr,
                              mb_field_decoding_flag, xP, yP);

  /* 当 u 是亮度块时，对于亮度块的每个样本 uij，指定以下有序步骤：*/
  if (isChroma == 0) {
    int32_t xO = 0, yO = 0, nE = 16;
    if (nW == 16 && nH == 16) {
    } else if (nW == 4 && nH == 4) {
      // 6.4.3 Inverse 4x4 luma block scanning process luma4x4BlkIdx
      xO = InverseRasterScan(BlkIdx / 4, 8, 8, 16, 0) +
           InverseRasterScan(BlkIdx % 4, 4, 4, 8, 0);
      yO = InverseRasterScan(BlkIdx / 4, 8, 8, 16, 1) +
           InverseRasterScan(BlkIdx % 4, 4, 4, 8, 1);
      nE = 4;
    } else {
      // 6.4.5 Inverse 8x8 luma block scanning process
      xO = InverseRasterScan(BlkIdx, 8, 8, 16, 0);
      yO = InverseRasterScan(BlkIdx, 8, 8, 16, 1);
      nE = 8;
    }

//#define SHOW_MB_BORDER
#ifdef SHOW_MB_BORDER
    const int32_t border_color = 255; // 边框颜色为白色
#endif
    int32_t n = (isMbAff) ? 2 : 1;
    for (int32_t i = 0; i < nE; i++)
      for (int32_t j = 0; j < nE; j++) {
        int32_t y = yP + n * (yO + i);
        int32_t x = xP + xO + j;
#ifdef SHOW_MB_BORDER
        // 添加宏块边框
        if (y == (yP + n * yO))
          pic_buff[y * PicWidthInSamples + x] = border_color;
        else if (x == (xP + xO))
          pic_buff[y * PicWidthInSamples + x] = border_color;
        else
          pic_buff[y * PicWidthInSamples + x] = u[i * nE + j];
#else
        pic_buff[y * PicWidthInSamples + x] = u[i * nE + j];
#endif
      }
  }

  /* 当 u 是色度块时，对于色度块的每个样本 uij，指定以下有序步骤：*/
  else if (isChroma) {
    int32_t xO = 0, yO = 0;
    if (nW == MbWidthC && nH == MbHeightC) {
    } else if (nW == 4 && nH == 4) {
      if (ChromaArrayType == 1 || ChromaArrayType == 2) {
        // 6.4.7 Inverse 4x4 chroma block scanning process chroma4x4BlkIdx
        xO = InverseRasterScan(BlkIdx, 4, 4, 8, 0);
        yO = InverseRasterScan(BlkIdx, 4, 4, 8, 1);
      } else {
        // 6.4.3 Inverse 4x4 luma block scanning process
        xO = InverseRasterScan(BlkIdx / 4, 8, 8, 16, 0) +
             InverseRasterScan(BlkIdx % 4, 4, 4, 8, 0);
        yO = InverseRasterScan(BlkIdx / 4, 8, 8, 16, 1) +
             InverseRasterScan(BlkIdx % 4, 4, 4, 8, 1);
      }
    } else if (ChromaArrayType == 3 && nW == 8 && nH == 8) {
      // 6.4.5 Inverse 8x8 luma block scanning process luma8x8BlkIdx
      xO = InverseRasterScan(BlkIdx, 8, 8, 16, 0);
      yO = InverseRasterScan(BlkIdx, 8, 8, 16, 1);
    }

    for (int32_t i = 0; i < nH; i++)
      for (int32_t j = 0; j < nW; j++) {
        int32_t x = (xP / SubWidthC) + xO + j;

        int32_t y;
        if (isMbAff)
          y = ((yP + SubHeightC - 1) / SubHeightC) + 2 * (yO + i);
        else
          y = (yP / SubHeightC) + yO + i;

        pic_buff[y * PicWidthInSamples + x] = u[i * nW + j];
      }
  }

  return 0;
}
