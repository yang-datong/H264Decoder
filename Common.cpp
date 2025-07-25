#include "Common.hpp"
#include "BitStream.hpp"
#include "Type.hpp"
#include <assert.h>
#include <cstdint>

int32_t LOG2(int32_t value) {
  assert(value > 0);
  int32_t log2 = 0;
  while (value) {
    value >>= 1;
    log2++;
  }
  return log2;
}

int32_t POWER2(int32_t value) {
  int32_t power2 = 1;
  for (int32_t i = 0; i < value; ++i)
    power2 *= 2;
  return power2;
}

string MacroBlockNmae(H264_MB_TYPE m_name_of_mb_type) {
  string mb_type = "NA";
  if (m_name_of_mb_type == I_NxN)
    mb_type = "I_NxN";
  else if (m_name_of_mb_type == I_PCM)
    mb_type = "I_PCM";
  else if (m_name_of_mb_type == I_NxN)
    mb_type = "I_NxN";
  else if (m_name_of_mb_type == I_16x16_0_0_0)
    mb_type = "I_16x16_0_0_0";
  else if (m_name_of_mb_type == I_16x16_1_0_0)
    mb_type = "I_16x16_1_0_0";
  else if (m_name_of_mb_type == I_16x16_2_0_0)
    mb_type = "I_16x16_2_0_0";
  else if (m_name_of_mb_type == I_16x16_3_0_0)
    mb_type = "I_16x16_3_0_0";
  else if (m_name_of_mb_type == I_16x16_0_1_0)
    mb_type = "I_16x16_0_1_0";
  else if (m_name_of_mb_type == I_16x16_1_1_0)
    mb_type = "I_16x16_1_1_0";
  else if (m_name_of_mb_type == I_16x16_2_1_0)
    mb_type = "I_16x16_2_1_0";
  else if (m_name_of_mb_type == I_16x16_3_1_0)
    mb_type = "I_16x16_3_1_0";
  else if (m_name_of_mb_type == I_16x16_0_2_0)
    mb_type = "I_16x16_0_2_0";
  else if (m_name_of_mb_type == I_16x16_1_2_0)
    mb_type = "I_16x16_1_2_0";
  else if (m_name_of_mb_type == I_16x16_2_2_0)
    mb_type = "I_16x16_2_2_0";
  else if (m_name_of_mb_type == I_16x16_3_2_0)
    mb_type = "I_16x16_3_2_0";
  else if (m_name_of_mb_type == I_16x16_0_0_1)
    mb_type = "I_16x16_0_0_1";
  else if (m_name_of_mb_type == I_16x16_1_0_1)
    mb_type = "I_16x16_1_0_1";
  else if (m_name_of_mb_type == I_16x16_2_0_1)
    mb_type = "I_16x16_2_0_1";
  else if (m_name_of_mb_type == I_16x16_3_0_1)
    mb_type = "I_16x16_3_0_1";
  else if (m_name_of_mb_type == I_16x16_0_1_1)
    mb_type = "I_16x16_0_1_1";
  else if (m_name_of_mb_type == I_16x16_1_1_1)
    mb_type = "I_16x16_1_1_1";
  else if (m_name_of_mb_type == I_16x16_2_1_1)
    mb_type = "I_16x16_2_1_1";
  else if (m_name_of_mb_type == I_16x16_3_1_1)
    mb_type = "I_16x16_3_1_1";
  else if (m_name_of_mb_type == I_16x16_0_2_1)
    mb_type = "I_16x16_0_2_1";
  else if (m_name_of_mb_type == I_16x16_1_2_1)
    mb_type = "I_16x16_1_2_1";
  else if (m_name_of_mb_type == I_16x16_2_2_1)
    mb_type = "I_16x16_2_2_1";
  else if (m_name_of_mb_type == I_16x16_3_2_1)
    mb_type = "I_16x16_3_2_1";
  else if (m_name_of_mb_type == SI)
    mb_type = "SI";
  else if (m_name_of_mb_type == P_L0_16x16)
    mb_type = "P_L0_16x16";
  else if (m_name_of_mb_type == P_L0_L0_16x8)
    mb_type = "P_L0_L0_16x8";
  else if (m_name_of_mb_type == P_L0_L0_8x16)
    mb_type = "P_L0_L0_8x16";
  else if (m_name_of_mb_type == P_8x8)
    mb_type = "P_8x8";
  else if (m_name_of_mb_type == P_8x8ref0)
    mb_type = "P_8x8ref0";
  else if (m_name_of_mb_type == P_Skip)
    mb_type = "P_Skip";
  else if (m_name_of_mb_type == B_Direct_16x16)
    mb_type = "B_Direct_16x16";
  else if (m_name_of_mb_type == B_L0_16x16)
    mb_type = "B_L0_16x16";
  else if (m_name_of_mb_type == B_L0_16x16)
    mb_type = "B_L1_16x16";
  else if (m_name_of_mb_type == B_Bi_16x16)
    mb_type = "B_Bi_16x16";
  else if (m_name_of_mb_type == B_L0_L0_16x8)
    mb_type = "B_L0_L0_16x8";
  else if (m_name_of_mb_type == B_L0_L0_8x16)
    mb_type = "B_L0_L0_8x16";
  else if (m_name_of_mb_type == B_L1_L1_16x8)
    mb_type = "B_L1_L1_16x8";
  else if (m_name_of_mb_type == B_L1_L1_8x16)
    mb_type = "B_L1_L1_8x16";
  else if (m_name_of_mb_type == B_L0_L1_16x8)
    mb_type = "B_L0_L1_16x8";
  else if (m_name_of_mb_type == B_L0_L1_8x16)
    mb_type = "B_L0_L1_8x16";
  else if (m_name_of_mb_type == B_L1_L0_16x8)
    mb_type = "B_L1_L0_16x8";
  else if (m_name_of_mb_type == B_L1_L0_8x16)
    mb_type = "B_L1_L0_8x16";
  else if (m_name_of_mb_type == B_L0_Bi_16x8)
    mb_type = "B_L0_Bi_16x8";
  else if (m_name_of_mb_type == B_L0_Bi_8x16)
    mb_type = "B_L0_Bi_8x16";
  else if (m_name_of_mb_type == B_L1_Bi_16x8)
    mb_type = "B_L1_Bi_16x8";
  else if (m_name_of_mb_type == B_L1_Bi_8x16)
    mb_type = "B_L1_Bi_8x16";
  else if (m_name_of_mb_type == B_Bi_L0_16x8)
    mb_type = "B_Bi_L0_16x8";
  else if (m_name_of_mb_type == B_Bi_L0_8x16)
    mb_type = "B_Bi_L0_8x16";
  else if (m_name_of_mb_type == B_Bi_L1_16x8)
    mb_type = "B_Bi_L1_16x8";
  else if (m_name_of_mb_type == B_Bi_L1_8x16)
    mb_type = "B_Bi_L1_8x16";
  else if (m_name_of_mb_type == B_Bi_Bi_16x8)
    mb_type = "B_Bi_Bi_16x8";
  else if (m_name_of_mb_type == B_Bi_Bi_8x16)
    mb_type = "B_Bi_Bi_8x16";
  else if (m_name_of_mb_type == B_8x8)
    mb_type = "B_8x8";
  else if (m_name_of_mb_type == B_Skip)
    mb_type = "B_Skip";
  else if (m_name_of_mb_type == P_L0_8x8)
    mb_type = "P_L0_8x8";
  else if (m_name_of_mb_type == P_L0_8x4)
    mb_type = "P_L0_8x4";
  else if (m_name_of_mb_type == P_L0_4x8)
    mb_type = "P_L0_4x8";
  else if (m_name_of_mb_type == P_L0_4x4)
    mb_type = "P_L0_4x4";
  else if (m_name_of_mb_type == B_Direct_8x8)
    mb_type = "B_Direct_8x8";
  else if (m_name_of_mb_type == B_L0_8x8)
    mb_type = "B_L0_8x8";
  else if (m_name_of_mb_type == B_L1_8x8)
    mb_type = "B_L1_8x8";
  else if (m_name_of_mb_type == B_Bi_8x8)
    mb_type = "B_Bi_8x8";
  else if (m_name_of_mb_type == B_L0_8x4)
    mb_type = "B_L0_8x4";
  else if (m_name_of_mb_type == B_L0_4x8)
    mb_type = "B_L0_4x8";
  else if (m_name_of_mb_type == B_L1_8x4)
    mb_type = "B_L1_8x4";
  else if (m_name_of_mb_type == B_L1_4x8)
    mb_type = "B_L1_4x8";
  else if (m_name_of_mb_type == B_Bi_8x4)
    mb_type = "B_Bi_8x4";
  else if (m_name_of_mb_type == B_Bi_4x8)
    mb_type = "B_Bi_4x8";
  else if (m_name_of_mb_type == B_L0_4x4)
    mb_type = "B_L0_4x4";
  else if (m_name_of_mb_type == B_L1_4x4)
    mb_type = "B_L1_4x4";
  else if (m_name_of_mb_type == B_Bi_4x4)
    mb_type = "B_Bi_4x4";
  return mb_type;
}

string MacroBlockPredMode(H264_MB_PART_PRED_MODE m_mb_pred_mode) {
  string mb_pred_mode = "MB_PRED_MODE_NA";
  if (m_mb_pred_mode == Intra_NA)
    mb_pred_mode = "Intra_NA";
  else if (m_mb_pred_mode == Intra_4x4)
    mb_pred_mode = "Intra_4x4";
  else if (m_mb_pred_mode == Intra_8x8)
    mb_pred_mode = "Intra_8x8";
  else if (m_mb_pred_mode == Intra_16x16)
    mb_pred_mode = "Intra_16x16";
  else if (m_mb_pred_mode == Inter)
    mb_pred_mode = "Inter";
  else if (m_mb_pred_mode == Pred_NA)
    mb_pred_mode = "Pred_NA";
  else if (m_mb_pred_mode == Pred_L0)
    mb_pred_mode = "Pred_L0";
  else if (m_mb_pred_mode == Pred_L1)
    mb_pred_mode = "Pred_L1";
  else if (m_mb_pred_mode == BiPred)
    mb_pred_mode = "BiPred";
  else if (m_mb_pred_mode == Direct)
    mb_pred_mode = "Direct";
  return mb_pred_mode;
}

// 7.3.2.1.1.1 Scaling list syntax
void scaling_list(BitStream &bs, uint32_t *scalingList,
                  uint32_t sizeOfScalingList,
                  uint32_t &useDefaultScalingMatrixFlag) {
  int32_t lastScale = 8, nextScale = 8;

  for (int j = 0; j < (int)sizeOfScalingList; j++) {
    if (nextScale != 0) {
      int delta_scale = bs.readSE();
      nextScale = (lastScale + delta_scale + 256) % 256;
      useDefaultScalingMatrixFlag = (j == 0 && nextScale == 0);
    }
    scalingList[j] = (nextScale == 0) ? lastScale : nextScale;
    lastScale = scalingList[j];
  }
}

// 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
int inverse_scanning_for_4x4_transform_coeff_and_scaling_lists(
    const int32_t values[16], int32_t (&c)[4][4], int32_t field_scan_flag) {
  // Table 8-13 – Specification of mapping of idx to cij for zig-zag and field scan
  /* +-----+-----+-----+-----+
     |  0  |  1  |  5  |  6  |
     +-----+-----+-----+-----+
     |  2  |  4  |  7  | 12  |
     +-----+-----+-----+-----+
     |  3  |  8  | 11  | 13  |
     +-----+-----+-----+-----+
     |  9  | 10  | 14  | 15  |
     +-----+-----+-----+-----+ */
  if (field_scan_flag == 0) {
    // zig-zag scan
    c[0][0] = values[0];
    c[0][1] = values[1];
    c[1][0] = values[2];
    c[2][0] = values[3];

    c[1][1] = values[4];
    c[0][2] = values[5];
    c[0][3] = values[6];
    c[1][2] = values[7];

    c[2][1] = values[8];
    c[3][0] = values[9];
    c[3][1] = values[10];
    c[2][2] = values[11];

    c[1][3] = values[12];
    c[2][3] = values[13];
    c[3][2] = values[14];
    c[3][3] = values[15];
  } else {
    // field scan
    c[0][0] = values[0];
    c[1][0] = values[1];
    c[0][1] = values[2];
    c[2][0] = values[3];

    c[3][0] = values[4];
    c[1][1] = values[5];
    c[2][1] = values[6];
    c[3][1] = values[7];

    c[0][2] = values[8];
    c[1][2] = values[9];
    c[2][2] = values[10];
    c[3][2] = values[11];

    c[0][3] = values[12];
    c[1][3] = values[13];
    c[2][3] = values[14];
    c[3][3] = values[15];
  }

  return 0;
}

// 8.5.7 Inverse scanning process for 8x8 transform coefficients and scaling lists
int inverse_scanning_for_8x8_transform_coeff_and_scaling_lists(
    int32_t values[64], int32_t (&c)[8][8], int32_t field_scan_flag) {
  // Table 8-14 – Specification of mapping of idx to cij for 8x8 zig-zag and 8x8 field scan
  if (field_scan_flag == 0) {
    // 8x8 zig-zag scan
    c[0][0] = values[0];
    c[0][1] = values[1];
    c[1][0] = values[2];
    c[2][0] = values[3];
    c[1][1] = values[4];
    c[0][2] = values[5];
    c[0][3] = values[6];
    c[1][2] = values[7];
    c[2][1] = values[8];
    c[3][0] = values[9];
    c[4][0] = values[10];
    c[3][1] = values[11];
    c[2][2] = values[12];
    c[1][3] = values[13];
    c[0][4] = values[14];
    c[0][5] = values[15];

    c[1][4] = values[16];
    c[2][3] = values[17];
    c[3][2] = values[18];
    c[4][1] = values[19];
    c[5][0] = values[20];
    c[6][0] = values[21];
    c[5][1] = values[22];
    c[4][2] = values[23];
    c[3][3] = values[24];
    c[2][4] = values[25];
    c[1][5] = values[26];
    c[0][6] = values[27];
    c[0][7] = values[28];
    c[1][6] = values[29];
    c[2][5] = values[30];
    c[3][4] = values[31];

    c[4][3] = values[32];
    c[5][2] = values[33];
    c[6][1] = values[34];
    c[7][0] = values[35];
    c[7][1] = values[36];
    c[6][2] = values[37];
    c[5][3] = values[38];
    c[4][4] = values[39];
    c[3][5] = values[40];
    c[2][6] = values[41];
    c[1][7] = values[42];
    c[2][7] = values[43];
    c[3][6] = values[44];
    c[4][5] = values[45];
    c[5][4] = values[46];
    c[6][3] = values[47];

    c[7][2] = values[48];
    c[7][3] = values[49];
    c[6][4] = values[50];
    c[5][5] = values[51];
    c[4][6] = values[52];
    c[3][7] = values[53];
    c[4][7] = values[54];
    c[5][6] = values[55];
    c[6][5] = values[56];
    c[7][4] = values[57];
    c[7][5] = values[58];
    c[6][6] = values[59];
    c[5][7] = values[60];
    c[6][7] = values[61];
    c[7][6] = values[62];
    c[7][7] = values[63];
  } else {
    // 8x8 field scan
    c[0][0] = values[0];
    c[1][0] = values[1];
    c[2][0] = values[2];
    c[0][1] = values[3];
    c[1][1] = values[4];
    c[3][0] = values[5];
    c[4][0] = values[6];
    c[2][1] = values[7];
    c[0][2] = values[8];
    c[3][1] = values[9];
    c[5][0] = values[10];
    c[6][0] = values[11];
    c[7][0] = values[12];
    c[4][1] = values[13];
    c[1][2] = values[14];
    c[0][3] = values[15];

    c[2][2] = values[16];
    c[5][1] = values[17];
    c[6][1] = values[18];
    c[7][1] = values[19];
    c[3][2] = values[20];
    c[1][3] = values[21];
    c[0][4] = values[22];
    c[2][3] = values[23];
    c[4][2] = values[24];
    c[5][2] = values[25];
    c[6][2] = values[26];
    c[7][2] = values[27];
    c[3][3] = values[28];
    c[1][4] = values[29];
    c[0][5] = values[30];
    c[2][4] = values[31];

    c[4][3] = values[32];
    c[5][3] = values[33];
    c[6][3] = values[34];
    c[7][3] = values[35];
    c[3][4] = values[36];
    c[1][5] = values[37];
    c[0][6] = values[38];
    c[2][5] = values[39];
    c[4][4] = values[40];
    c[5][4] = values[41];
    c[6][4] = values[42];
    c[7][4] = values[43];
    c[3][5] = values[44];
    c[1][6] = values[45];
    c[2][6] = values[46];
    c[4][5] = values[47];

    c[5][5] = values[48];
    c[6][5] = values[49];
    c[7][5] = values[50];
    c[3][6] = values[51];
    c[0][7] = values[52];
    c[1][7] = values[53];
    c[4][6] = values[54];
    c[5][6] = values[55];
    c[6][6] = values[56];
    c[7][6] = values[57];
    c[2][7] = values[58];
    c[3][7] = values[59];
    c[4][7] = values[60];
    c[5][7] = values[61];
    c[6][7] = values[62];
    c[7][7] = values[63];
  }

  return 0;
}
