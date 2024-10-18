#include "PictureBase.hpp"

// 8.5.11 Scaling and transformation process for chroma DC transform coefficients
int PictureBase::scaling_and_transform_for_chroma_DC(int32_t isChromaCb,
                                                     int32_t c[4][2],
                                                     int32_t nW, int32_t nH,
                                                     int32_t (&dcC)[4][2]) {
  // 8.5.8 Derivation process for chroma quantisation parameters
  RET(derivation_chroma_quantisation_parameters(isChromaCb));
  int32_t qP =
      (isChromaCb == 1) ? m_mbs[CurrMbAddr].QP1Cb : m_mbs[CurrMbAddr].QP1Cr;

  if (m_mbs[CurrMbAddr].TransformBypassModeFlag) {
    for (int32_t i = 0; i < MbWidthC / 4; i++)
      for (int32_t j = 0; j < MbHeightC / 4; j++)
        dcC[i][j] = c[i][j];
  } else {
    //YUV420 的逆变换与反量化
    if (nW == 2 && nH == 2) {
      // 8.5.11.1 Transformation process for chroma DC transform coefficients
      int32_t f[2][2] = {{0}};
      int32_t e00 = c[0][0] + c[1][0];
      int32_t e01 = c[0][1] + c[1][1];
      int32_t e10 = c[0][0] - c[1][0];
      int32_t e11 = c[0][1] - c[1][1];
      f[0][0] = e00 + e01;
      f[0][1] = e00 - e01;
      f[1][0] = e10 + e11;
      f[1][1] = e10 - e11;

      // 8.5.11.2 Scaling process for chroma DC transform coefficientsu
      for (int32_t i = 0; i < 2; i++)
        for (int32_t j = 0; j < 2; j++)
          dcC[i][j] =
              ((f[i][j] * LevelScale4x4[qP % 6][0][0]) << (qP / 6)) >> 5;
    }

    //YUV422 的逆变换与反量化
    else if (nW == 2 && nH == 4) {
      // 8.5.11.1 Transformation process for chroma DC transform coefficients
      int32_t f[4][2] = {{0}};
      int32_t e00 = c[0][0] + c[1][0] + c[2][0] + c[3][0];
      int32_t e01 = c[0][1] + c[1][1] + c[2][1] + c[3][1];
      int32_t e10 = c[0][0] + c[1][0] - c[2][0] - c[3][0];
      int32_t e11 = c[0][1] + c[1][1] - c[2][1] - c[3][1];
      int32_t e20 = c[0][0] - c[1][0] - c[2][0] + c[3][0];
      int32_t e21 = c[0][1] - c[1][1] - c[2][1] + c[3][1];
      int32_t e30 = c[0][0] - c[1][0] + c[2][0] - c[3][0];
      int32_t e31 = c[0][1] - c[1][1] + c[2][1] - c[3][1];
      f[0][0] = e00 + e01;
      f[0][1] = e00 - e01;
      f[1][0] = e10 + e11;
      f[1][1] = e10 - e11;
      f[2][0] = e20 + e21;
      f[2][1] = e20 - e21;
      f[3][0] = e30 + e31;
      f[3][1] = e30 - e31;

      // 8.5.11.2 Scaling process for chroma DC transform coefficients
      int32_t qP_DC = qP + 3;
      for (int32_t i = 0; i < 4; i++)
        for (int32_t j = 0; j < 2; j++)
          if (qP_DC >= 36)
            dcC[i][j] = (f[i][j] * LevelScale4x4[qP_DC % 6][0][0])
                        << (qP_DC / 6 - 6);
          else
            dcC[i][j] = (f[i][j] * LevelScale4x4[qP_DC % 6][0][0] +
                         POWER2(5 - qP_DC / 6)) >>
                        (6 - qP / 6);
    }
  }

  return 0;
}

// 8.5.12 Scaling and transformation process for residual 4x4 blocks
/* 输入: 具有元素cij的4x4数组c，cij是与亮度分量的残差块相关的数组或与色度分量的残差块相关的数组。
 * 输出: 剩余样本值，为 4x4 数组 r，元素为 rij。*/
/* 该函数包括了反量化、反整数变换（类IDCT变换）操作 */
int PictureBase::scaling_and_transform_for_residual_4x4_blocks(
    int32_t c[4][4], int32_t (&r)[4][4], int32_t isChroma, int32_t isChromaCb) {

  const uint32_t slice_type = m_slice->slice_header->slice_type % 5;
  const MacroBlock &mb = m_mbs[CurrMbAddr];

  /* 场景切换标志，需要特殊处理量化值 */
  bool sMbFlag = false;
  if (slice_type == SLICE_SI ||
      (slice_type == SLICE_SP && IS_INTER_Prediction_Mode(mb.m_mb_pred_mode)))
    sMbFlag = true;

  /* 为色差块计算量化参数 */
  RET(derivation_chroma_quantisation_parameters(isChromaCb));

  int32_t qP = 0;
  //对于亮度块，根据是否为场景切换，使用对应的量化值
  if (isChroma == 0 && sMbFlag == false)
    qP = mb.QP1Y;
  else if (isChroma == 0 && sMbFlag)
    qP = mb.QSY;
  //对于色度块，根据是否为场景切换，使用对应的量化值
  else if (isChroma && sMbFlag == false)
    qP = isChromaCb ? mb.QP1Cb : mb.QP1Cr;
  else if (isChroma && sMbFlag)
    qP = isChromaCb ? mb.QSCb : mb.QSCr;

  //变换旁路模式，即不进行任何变换或缩放处理
  if (mb.TransformBypassModeFlag) {
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        r[i][j] = c[i][j];
  } else {
    // 8.5.12.1 Scaling process for residual 4x4 blocks (反量化)
    int32_t d[4][4] = {{0}};
    scaling_for_residual_4x4_blocks(d, c, isChroma, mb.m_mb_pred_mode, qP);

    // 8.5.12.2 Transformation process for residual 4x4 blocks （反整数变换）
    transform_decoding_for_residual_4x4_blocks(d, r);
  }

  return 0;
}

// 8.5.10 Scaling and transformation process for DC transform coefficients for Intra_16x16 macroblock type
// 对于DC系数矩阵需要进行第二层的逆变换、反量化
int PictureBase::scaling_and_transform_for_DC_Intra16x16(int32_t bitDepth,
                                                         int32_t qP,
                                                         int32_t c[4][4],
                                                         int32_t (&dcY)[4][4]) {

  if (m_mbs[CurrMbAddr].TransformBypassModeFlag) {
    for (int32_t i = 0; i < 4; i++)
      for (int32_t j = 0; j < 4; j++)
        dcY[i][j] = c[i][j];
  } else {
    int32_t f[4][4] = {{0}}, g[4][4] = {{0}};
    /* 行变换 */
    for (int32_t i = 0; i < 4; ++i) {
      g[0][i] = c[0][i] + c[1][i] + c[2][i] + c[3][i];
      g[1][i] = c[0][i] + c[1][i] - c[2][i] - c[3][i];
      g[2][i] = c[0][i] - c[1][i] - c[2][i] + c[3][i];
      g[3][i] = c[0][i] - c[1][i] + c[2][i] - c[3][i];
    }

    /* 列变换：同理行变换 */
    for (int32_t j = 0; j < 4; ++j) {
      f[j][0] = g[j][0] + g[j][1] + g[j][2] + g[j][3];
      f[j][1] = g[j][0] + g[j][1] - g[j][2] - g[j][3];
      f[j][2] = g[j][0] - g[j][1] - g[j][2] + g[j][3];
      f[j][3] = g[j][0] - g[j][1] + g[j][2] - g[j][3];
    }

    /* 反量化 */
    for (int32_t i = 0; i < 4; i++)
      for (int32_t j = 0; j < 4; j++) {
        if (qP >= 36)
          dcY[i][j] = (f[i][j] * LevelScale4x4[qP % 6][0][0]) << (qP / 6 - 6);
        else
          dcY[i][j] =
              (f[i][j] * LevelScale4x4[qP % 6][0][0] + (1 << (5 - qP / 6))) >>
              (6 - qP / 6);
      }
  }

  return 0;
}

//8.5.12.1 Scaling process for residual 4x4 blocks
/* 输入：
 * – 变量 bitDepth 和 qP
 * – 具有元素 cij 的 4x4 数组 c，它是与亮度分量的残差块相关的数组或与色度分量的残差块相关的数组。
 * 输出: 缩放变换系数 d 的 4x4 数组，其中元素为 dij。 */
// 对残差宏块进行反量化操作
int PictureBase::scaling_for_residual_4x4_blocks(
    int32_t d[4][4], int32_t c[4][4], int32_t isChroma,
    const H264_MB_PART_PRED_MODE &m_mb_pred_mode, int32_t qP) {

  /* 比特流不应包含导致 c 的任何元素 cij 的数据，其中 i, j = 0..3 超出从 −2(7 + bitDepth) 到 2(7 + bitDepth) − 1（含）的整数值范围。 */
  for (int32_t i = 0; i < 4; i++) {
    for (int32_t j = 0; j < 4; j++) {
      //对于Intra_16x16模式下的亮度残差块（DC系数已经经过了反量化处理），色度残差块的4x4残差块的首个样本（DC系数已经经过了反量化处理）直接进行复制
      if (i == 0 && j == 0 &&
          ((isChroma == 0 && m_mb_pred_mode == Intra_16x16) || isChroma))
        d[0][0] = c[0][0];
      else {
        if (qP >= 24)
          d[i][j] = (c[i][j] * LevelScale4x4[qP % 6][i][j]) << (qP / 6 - 4);
        else
          d[i][j] =
              (c[i][j] * LevelScale4x4[qP % 6][i][j] + POWER2(3 - qP / 6)) >>
              (4 - qP / 6);
      }
    }
  }
  return 0;
}

int PictureBase::scaling_for_residual_8x8_blocks(
    int32_t d[8][8], int32_t c[8][8], int32_t isChroma,
    const H264_MB_PART_PRED_MODE &m_mb_pred_mode, int32_t qP) {
  for (int32_t i = 0; i < 8; i++) {
    for (int32_t j = 0; j < 8; j++) {
      if (qP >= 36)
        d[i][j] = (c[i][j] * LevelScale8x8[qP % 6][i][j]) << (qP / 6 - 6);
      else
        d[i][j] =
            (c[i][j] * LevelScale8x8[qP % 6][i][j] + POWER2(5 - qP / 6)) >>
            (6 - qP / 6);
    }
  }
  return 0;
}

// 8.5.13 Scaling and transformation process for residual 8x8 blocks
int PictureBase::scaling_and_transform_for_residual_8x8_blocks(
    int32_t c[8][8], int32_t (&r)[8][8], int32_t isChroma, int32_t isChromaCb) {

  const MacroBlock &mb = m_mbs[CurrMbAddr];

  //对于亮度块，使用对应的量化值
  int32_t qP = 0;
  if (isChroma == 0) qP = mb.QP1Y;
  //对于色度块，使用对应的量化值
  else {
    if (isChromaCb == 1)
      qP = mb.QP1Cb;
    else if (isChromaCb == 0)
      qP = mb.QP1Cr;
  }

  //变换旁路模式，即不进行任何变换或缩放处理
  if (mb.TransformBypassModeFlag) {
    for (int32_t i = 0; i < 8; i++)
      for (int32_t j = 0; j < 8; j++)
        r[i][j] = c[i][j];
  } else {
    // 8.5.13.1 Scaling process for residual 8x8 blocks (反量化)
    int32_t d[8][8] = {{0}};
    scaling_for_residual_8x8_blocks(d, c, isChroma, mb.m_mb_pred_mode, qP);

    // 8.5.13.2 Transformation process for residual 8x8 blocks
    transform_decoding_for_residual_8x8_blocks(d, r);
  }

  return 0;
}

// 8.5.9 Derivation process for scaling functions
/*  输出为：
 *  – LevelScale4x4：4x4 块变换亮度或色度系数级别的缩放因子;
 *  – LevelScale8x8：8x8 块变换亮度或色度系数级别的缩放因子; */
int PictureBase::scaling_functions(int32_t isChroma, int32_t isChromaCb) {

  /* -------------- 设置别名，初始化变量 -------------- */
  MacroBlock &mb = m_mbs[CurrMbAddr];
  bool mbIsInterFlag = !IS_INTRA_Prediction_Mode(mb.m_mb_pred_mode);

  int32_t iYCbCr = (!isChroma) ? 0 : (isChroma == 1 && isChromaCb == 1) ? 1 : 2;
  //YUV444
  if (m_slice->slice_header->m_sps->separate_colour_plane_flag)
    iYCbCr = m_slice->slice_header->colour_plane_id;

  const uint32_t *ScalingList4x4 =
      m_slice->slice_header->ScalingList4x4[iYCbCr + (mbIsInterFlag ? 3 : 0)];
  const uint32_t *ScalingList8x8 =
      m_slice->slice_header->ScalingList8x8[2 * iYCbCr + mbIsInterFlag];
  /* ------------------  End ------------------ */

  //------------------------ 4x4 缩放矩阵 ----------------------------
  int32_t weightScale4x4[4][4] = {{0}};
  RET(inverse_scanning_for_4x4_transform_coeff_and_scaling_lists(
      (int32_t *)ScalingList4x4, weightScale4x4,
      mb.field_pic_flag | mb.mb_field_decoding_flag));

  /* 其中 v 的第一个和第二个下标分别是矩阵的行索引和列索引 */
  int32_t v4x4[6][3] = {{10, 16, 13}, {11, 18, 14}, {13, 20, 16},
                        {14, 23, 18}, {16, 25, 20}, {18, 29, 23}};
  /* m[0-5]分别表示帧内、帧间预测三个分量 */
  for (int m = 0; m < 6; m++)
    for (int m = 0; m < 6; m++)
      for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
          if (i % 2 == 0 && j % 2 == 0)
            LevelScale4x4[m][i][j] = weightScale4x4[i][j] * v4x4[m][0];
          else if (i % 2 == 1 && j % 2 == 1)
            LevelScale4x4[m][i][j] = weightScale4x4[i][j] * v4x4[m][1];
          else
            LevelScale4x4[m][i][j] = weightScale4x4[i][j] * v4x4[m][2];
        }

  //------------------------ 8x8 缩放矩阵 ----------------------------
  int32_t weightScale8x8[8][8] = {{0}};
  RET(inverse_scanning_for_8x8_transform_coeff_and_scaling_lists(
      (int32_t *)ScalingList8x8, weightScale8x8,
      mb.field_pic_flag | mb.mb_field_decoding_flag));

  /* 其中 v 的第一个和第二个下标分别是矩阵的行索引和列索引 */
  int32_t v8x8[6][6] = {
      {20, 18, 32, 19, 25, 24}, {22, 19, 35, 21, 28, 26},
      {26, 23, 42, 24, 33, 31}, {28, 25, 45, 26, 35, 33},
      {32, 28, 51, 30, 40, 38}, {36, 32, 58, 34, 46, 43},
  };
  /* m[0-5]分别表示帧内、帧间预测三个分量 */
  for (int m = 0; m < 6; m++)
    for (int i = 0; i < 8; i++)
      for (int j = 0; j < 8; j++) {
        if (i % 4 == 0 && j % 4 == 0)
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][0];
        else if (i % 2 == 1 && j % 2 == 1)
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][1];
        else if (i % 4 == 2 && j % 4 == 2)
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][2];
        else if ((i % 4 == 0 && j % 2 == 1) || (i % 2 == 1 && j % 4 == 0))
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][3];
        else if ((i % 4 == 0 && j % 4 == 2) || (i % 4 == 2 && j % 4 == 0))
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][4];
        else
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][5];
      }

  return 0;
}

// 8.5.8 Derivation process for chroma quantisation parameters (色度量化参数的推导过程)
/* 输出： – QPC：每个色度分量 Cb 和 Cr 的色度量化参数，
 * – QSC：解码 SP 和 SI 切片所需的每个色度分量 Cb 和 Cr 的附加色度量化参数（如果适用）*/
int PictureBase::derivation_chroma_quantisation_parameters(int32_t isChromaCb) {
  const SliceHeader *header = m_slice->slice_header;
  MacroBlock &mb = m_mbs[CurrMbAddr];

  int32_t qPOffset = header->m_pps->second_chroma_qp_index_offset;
  if (isChromaCb == 1) qPOffset = header->m_pps->chroma_qp_index_offset;

  int32_t qPI =
      CLIP3(-((int32_t)header->m_sps->QpBdOffsetC), 51, mb.QPY + qPOffset);

  // Table 8-15 – Specification of QPC as a function of qPI
  int32_t QPC = qPI;
  if (qPI >= 30) {
    const int32_t QPCs[] = {29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36,
                            36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 39};
    int32_t index = qPI - 30;
    QPC = QPCs[index];
  }

  int32_t QP1C = QPC + header->m_sps->QpBdOffsetC;
  if (isChromaCb == 1)
    mb.QPCb = QPC, mb.QP1Cb = QP1C;
  else
    mb.QPCr = QPC, mb.QP1Cr = QP1C;

  if (header->slice_type == SLICE_SP || header->slice_type == SLICE_SI ||
      header->slice_type == SLICE_SP2 || header->slice_type == SLICE_SI2) {
    mb.QSY = mb.QPY;
    if (isChromaCb == 1)
      mb.QSCb = mb.QPCb, mb.QS1Cb = mb.QP1Cb;
    else
      mb.QSCr = mb.QPCr, mb.QS1Cr = mb.QP1Cr;
  }

  return 0;
}

// 8.5.8 Derivation process for chroma quantisation parameters
int PictureBase::get_chroma_quantisation_parameters2(int32_t QPY,
                                                     int32_t isChromaCb,
                                                     int32_t &QPC) {
  int32_t qPOffset = 0;
  if (isChromaCb == 1)
    qPOffset = m_slice->slice_header->m_pps->chroma_qp_index_offset;
  else
    qPOffset = m_slice->slice_header->m_pps->second_chroma_qp_index_offset;

  int32_t qPI = CLIP3(-(int32_t)m_slice->slice_header->m_sps->QpBdOffsetC, 51,
                      QPY + qPOffset);

  // Table 8-15 – Specification of QPC as a function of qPI
  QPC = qPI;
  if (qPI >= 30) {
    int32_t QPCs[] = {29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36,
                      36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 39};
    QPC = QPCs[qPI - 30];
  }

  return 0;
}
