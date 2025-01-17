#ifndef TYPE_HPP_TPOWA9WD
#define TYPE_HPP_TPOWA9WD

#include "Constants.hpp"

// Table 7-6 – Name association to slice_type
typedef enum _H264_SLICE_TYPE {
  SLICE_UNKNOWN = -1,
  SLICE_P = 0,
  SLICE_B,
  SLICE_I,
  SLICE_SP,
  /* SP 帧是一种特殊类型的 P 帧，主要用于在编码过程中切换编码模式 */
  SLICE_SI,
  SLICE_P2,
  SLICE_B2,
  SLICE_I2,
  SLICE_SP2,
  SLICE_SI2
} H264_SLICE_TYPE;

// Table 7-9 – Memory management control operation (memory_management_control_operation) values
typedef enum _H264_PICTURE_MARKED_AS_ {
  UNKOWN = 0,
  REFERENCE,
  SHORT_REF,
  LONG_REF,
  NON_EXISTING,
  UNUSED_REF,
  OUTPUT_DISPLAY,
} PICTURE_MARKED_AS;

typedef enum _H264_PICTURE_CODED_TYPE_ {
  UNKNOWN = 0,
  FRAME = 1,                    // 帧
  FIELD = 2,                    // 场
  TOP_FIELD = 3,                // 顶场
  BOTTOM_FIELD = 4,             // 底场
  COMPLEMENTARY_FIELD_PAIR = 5, // 互补场对
} H264_PICTURE_CODED_TYPE;

typedef struct _MY_BITMAP_ {
  long bmType;
  long bmWidth;
  long bmHeight;
  long bmWidthBytes;
  unsigned short bmPlanes;
  unsigned short bmBitsPixel;
  unsigned char *bmBits;
} Bitmap;

const int MIN_MB_TYPE_FOR_I_SLICE = 1, MAX_MB_TYPE_FOR_I_SLICE = 25;

typedef enum _H264_MB_TYPE_ {
  MB_TYPE_NA = 0,
  I_NxN = 1,
  I_16x16_0_0_0,
  I_16x16_1_0_0,
  I_16x16_2_0_0,
  I_16x16_3_0_0,
  I_16x16_0_1_0,
  I_16x16_1_1_0,
  I_16x16_2_1_0,
  I_16x16_3_1_0,
  I_16x16_0_2_0,
  I_16x16_1_2_0,
  I_16x16_2_2_0,
  I_16x16_3_2_0,
  I_16x16_0_0_1,
  I_16x16_1_0_1,
  I_16x16_2_0_1,
  I_16x16_3_0_1,
  I_16x16_0_1_1,
  I_16x16_1_1_1,
  I_16x16_2_1_1,
  I_16x16_3_1_1,
  I_16x16_0_2_1,
  I_16x16_1_2_1,
  I_16x16_2_2_1,
  I_16x16_3_2_1,
  I_PCM = 26,

  SI = 27,

  P_L0_16x16 = 28,
  P_L0_L0_16x8,
  P_L0_L0_8x16,
  P_8x8,
  P_8x8ref0,
  P_Skip,

  B_Direct_16x16,
  B_L0_16x16,
  B_L1_16x16,
  B_Bi_16x16,
  B_L0_L0_16x8,
  B_L0_L0_8x16,
  B_L1_L1_16x8,
  B_L1_L1_8x16,
  B_L0_L1_16x8,
  B_L0_L1_8x16,
  B_L1_L0_16x8,
  B_L1_L0_8x16,
  B_L0_Bi_16x8,
  B_L0_Bi_8x16,
  B_L1_Bi_16x8,
  B_L1_Bi_8x16,
  B_Bi_L0_16x8,
  B_Bi_L0_8x16,
  B_Bi_L1_16x8,
  B_Bi_L1_8x16,
  B_Bi_Bi_16x8,
  B_Bi_Bi_8x16,
  B_8x8,
  B_Skip,

  // Sub-macroblock types in P macroblocks
  P_L0_8x8,
  P_L0_8x4,
  P_L0_4x8,
  P_L0_4x4,

  // Sub-macroblock types in B macroblocks
  B_Direct_8x8,
  B_L0_8x8,
  B_L1_8x8,
  B_Bi_8x8,
  B_L0_8x4,
  B_L0_4x8,
  B_L1_8x4,
  B_L1_4x8,
  B_Bi_8x4,
  B_Bi_4x8,
  B_L0_4x4,
  B_L1_4x4,
  B_Bi_4x4,
} H264_MB_TYPE;

// Figure 6-14 – Determination of the neighbouring macroblock, blocks, and partitions (informative) D    B    C A    Current Macroblock or Partition or Block
typedef enum _MB_ADDR_TYPE_ {
  MB_ADDR_TYPE_UNKOWN = 0,
  MB_ADDR_TYPE_mbAddrA,
  MB_ADDR_TYPE_mbAddrB,
  MB_ADDR_TYPE_mbAddrC,
  MB_ADDR_TYPE_mbAddrD,
  MB_ADDR_TYPE_CurrMbAddr,
  MB_ADDR_TYPE_mbAddrA_add_1,
  MB_ADDR_TYPE_mbAddrB_add_1,
  MB_ADDR_TYPE_mbAddrC_add_1,
  MB_ADDR_TYPE_mbAddrD_add_1,
  MB_ADDR_TYPE_CurrMbAddr_minus_1,
} MB_ADDR_TYPE;

typedef enum _H264_MB_PART_PRED_MODE_ {
  MB_PRED_MODE_NA,
  Intra_NA,
  Intra_4x4,
  Intra_8x8,
  Intra_16x16,
  Inter,
  Pred_NA,
  Pred_L0,
  Pred_L1,
  BiPred,
  Direct,
} H264_MB_PART_PRED_MODE;

// Table 7-11 – Macroblock types for I slices
typedef struct _MB_TYPE_I_SLICES_T_ {
  int32_t mb_type;
  H264_MB_TYPE name_of_mb_type;
  int32_t transform_size_8x8_flag;
  H264_MB_PART_PRED_MODE MbPartPredMode;
  int32_t Intra16x16PredMode;
  int32_t CodedBlockPatternChroma;
  int32_t CodedBlockPatternLuma;
} MB_TYPE_I_SLICES_T;

// Table 7-12 – Macroblock type with value 0 for SI slices
typedef struct _MB_TYPE_SI_SLICES_T_ {
  int32_t mb_type;
  H264_MB_TYPE name_of_mb_type;
  H264_MB_PART_PRED_MODE MbPartPredMode;
  int32_t Intra16x16PredMode;
  int32_t CodedBlockPatternChroma;
  int32_t CodedBlockPatternLuma;
} MB_TYPE_SI_SLICES_T;

// Table 7-13 – Macroblock type values 0 to 4 for P and SP slices
typedef struct _MB_TYPE_P_SP_SLICES_T_ {
  int32_t mb_type;
  H264_MB_TYPE name_of_mb_type;
  int32_t NumMbPart;
  H264_MB_PART_PRED_MODE MbPartPredMode0;
  H264_MB_PART_PRED_MODE MbPartPredMode1;
  int32_t MbPartWidth;
  int32_t MbPartHeight;
} MB_TYPE_P_SP_SLICES_T;

// Table 7-14 – Macroblock type values 0 to 22 for B slices
typedef struct _MB_TYPE_B_SLICES_T_ {
  int32_t mb_type;
  H264_MB_TYPE name_of_mb_type;
  int32_t NumMbPart;
  H264_MB_PART_PRED_MODE MbPartPredMode0;
  H264_MB_PART_PRED_MODE MbPartPredMode1;
  int32_t MbPartWidth;
  int32_t MbPartHeight;
} MB_TYPE_B_SLICES_T;

// Table 7-17 – Sub-macroblock types in P macroblocks
typedef struct _SUB_MB_TYPE_P_MBS_T_ {
  int32_t sub_mb_type;
  H264_MB_TYPE name_of_sub_mb_type;
  int32_t NumSubMbPart;
  H264_MB_PART_PRED_MODE SubMbPredMode;
  int32_t SubMbPartWidth;
  int32_t SubMbPartHeight;
} SUB_MB_TYPE_P_MBS_T;

// Table 7-18 – Sub-macroblock types in B macroblocks
typedef struct _SUB_MB_TYPE_B_MBS_T_ {
  int32_t sub_mb_type;
  H264_MB_TYPE name_of_sub_mb_type;
  int32_t NumSubMbPart;
  H264_MB_PART_PRED_MODE SubMbPredMode;
  int32_t SubMbPartWidth;
  int32_t SubMbPartHeight;
} SUB_MB_TYPE_B_MBS_T;

// 宏块残差幅值类型
typedef enum _MB_RESIDUAL_LEVEL_ {
  MB_RESIDUAL_UNKOWN = -1,
  MB_RESIDUAL_Intra16x16DCLevel,
  MB_RESIDUAL_Intra16x16ACLevel,
  MB_RESIDUAL_LumaLevel4x4,
  MB_RESIDUAL_ChromaDCLevel,
  MB_RESIDUAL_ChromaACLevel,
  MB_RESIDUAL_LumaLevel8x8,
  MB_RESIDUAL_CbIntra16x16DCLevel,
  MB_RESIDUAL_CbIntra16x16ACLevel,
  MB_RESIDUAL_CbLevel4x4,
  MB_RESIDUAL_CbLevel8x8,
  MB_RESIDUAL_CrIntra16x16DCLevel,
  MB_RESIDUAL_CrIntra16x16ACLevel,
  MB_RESIDUAL_CrLevel4x4,
  MB_RESIDUAL_CrLevel8x8,
  MB_RESIDUAL_ChromaDCLevelCb,
  MB_RESIDUAL_ChromaDCLevelCr,
  MB_RESIDUAL_ChromaACLevelCb,
  MB_RESIDUAL_ChromaACLevelCr,
} MB_RESIDUAL_LEVEL;

#define H264_SLIECE_TYPE_TO_STR(slice_type)                                    \
  (slice_type == SLICE_P)     ? "P"                                            \
  : (slice_type == SLICE_B)   ? "B"                                            \
  : (slice_type == SLICE_I)   ? "I"                                            \
  : (slice_type == SLICE_SP)  ? "SP"                                           \
  : (slice_type == SLICE_SI)  ? "SI"                                           \
  : (slice_type == SLICE_P2)  ? "P2"                                           \
  : (slice_type == SLICE_B2)  ? "B2"                                           \
  : (slice_type == SLICE_I2)  ? "I2"                                           \
  : (slice_type == SLICE_SP2) ? "SP2"                                          \
  : (slice_type == SLICE_SI2) ? "SI2"                                          \
                              : "UNKNOWN"

// Table 8-8 – Specification of mbAddrCol, yM, and vertMvScale
typedef enum _H264_VERT_MV_SCALE_ {
  H264_VERT_MV_SCALE_UNKNOWN = 0,
  H264_VERT_MV_SCALE_One_To_One,
  H264_VERT_MV_SCALE_Frm_To_Fld,
  H264_VERT_MV_SCALE_Fld_To_Frm,
} H264_VERT_MV_SCALE;

//Table 7-11 – Macroblock types for I slices
const MB_TYPE_I_SLICES_T mb_type_I_slices_define[27] = {
    {0, I_NxN, 0, Intra_4x4, NA, -1, -1},
    {0, I_NxN, 1, Intra_8x8, NA, -1, -1},
    {1, I_16x16_0_0_0, NA, Intra_16x16, 0, 0, 0},
    {2, I_16x16_1_0_0, NA, Intra_16x16, 1, 0, 0},
    {3, I_16x16_2_0_0, NA, Intra_16x16, 2, 0, 0},
    {4, I_16x16_3_0_0, NA, Intra_16x16, 3, 0, 0},
    {5, I_16x16_0_1_0, NA, Intra_16x16, 0, 1, 0},
    {6, I_16x16_1_1_0, NA, Intra_16x16, 1, 1, 0},
    {7, I_16x16_2_1_0, NA, Intra_16x16, 2, 1, 0},
    {8, I_16x16_3_1_0, NA, Intra_16x16, 3, 1, 0},
    {9, I_16x16_0_2_0, NA, Intra_16x16, 0, 2, 0},
    {10, I_16x16_1_2_0, NA, Intra_16x16, 1, 2, 0},
    {11, I_16x16_2_2_0, NA, Intra_16x16, 2, 2, 0},
    {12, I_16x16_3_2_0, NA, Intra_16x16, 3, 2, 0},
    {13, I_16x16_0_0_1, NA, Intra_16x16, 0, 0, 15},
    {14, I_16x16_1_0_1, NA, Intra_16x16, 1, 0, 15},
    {15, I_16x16_2_0_1, NA, Intra_16x16, 2, 0, 15},
    {16, I_16x16_3_0_1, NA, Intra_16x16, 3, 0, 15},
    {17, I_16x16_0_1_1, NA, Intra_16x16, 0, 1, 15},
    {18, I_16x16_1_1_1, NA, Intra_16x16, 1, 1, 15},
    {19, I_16x16_2_1_1, NA, Intra_16x16, 2, 1, 15},
    {20, I_16x16_3_1_1, NA, Intra_16x16, 3, 1, 15},
    {21, I_16x16_0_2_1, NA, Intra_16x16, 0, 2, 15},
    {22, I_16x16_1_2_1, NA, Intra_16x16, 1, 2, 15},
    {23, I_16x16_2_2_1, NA, Intra_16x16, 2, 2, 15},
    {24, I_16x16_3_2_1, NA, Intra_16x16, 3, 2, 15},
    {25, I_PCM, NA, Intra_NA, NA, NA, NA}};

//Table 7-12 – Macroblock type with value 0 for SI slices
const MB_TYPE_SI_SLICES_T mb_type_SI_slices_define[1] = {
    {0, SI, Intra_4x4, NA, NA, NA}};

const MB_TYPE_P_SP_SLICES_T mb_type_P_SP_slices_define[6] = {
    {0, P_L0_16x16, 1, Pred_L0, Pred_NA, 16, 16},
    {
        1,
        P_L0_L0_16x8,
        2,
        Pred_L0,
        Pred_L0,
        16,
        8,
    },
    {2, P_L0_L0_8x16, 2, Pred_L0, Pred_L0, 8, 16},
    {3, P_8x8, 4, Pred_NA, Pred_NA, 8, 8},
    {4, P_8x8ref0, 4, Pred_NA, Pred_NA, 8, 8},
    {5, P_Skip, 1, Pred_L0, Pred_NA, 16, 16}};

const MB_TYPE_B_SLICES_T mb_type_B_slices_define[24] = {
    {0, B_Direct_16x16, NA, Direct, Pred_NA, 8, 8},
    {1, B_L0_16x16, 1, Pred_L0, Pred_NA, 16, 16},
    {2, B_L1_16x16, 1, Pred_L1, Pred_NA, 16, 16},
    {3, B_Bi_16x16, 1, BiPred, Pred_NA, 16, 16},
    {4, B_L0_L0_16x8, 2, Pred_L0, Pred_L0, 16, 8},
    {5, B_L0_L0_8x16, 2, Pred_L0, Pred_L0, 8, 16},
    {6, B_L1_L1_16x8, 2, Pred_L1, Pred_L1, 16, 8},
    {7, B_L1_L1_8x16, 2, Pred_L1, Pred_L1, 8, 16},
    {8, B_L0_L1_16x8, 2, Pred_L0, Pred_L1, 16, 8},
    {9, B_L0_L1_8x16, 2, Pred_L0, Pred_L1, 8, 16},
    {10, B_L1_L0_16x8, 2, Pred_L1, Pred_L0, 16, 8},
    {11, B_L1_L0_8x16, 2, Pred_L1, Pred_L0, 8, 16},
    {12, B_L0_Bi_16x8, 2, Pred_L0, BiPred, 16, 8},
    {13, B_L0_Bi_8x16, 2, Pred_L0, BiPred, 8, 16},
    {14, B_L1_Bi_16x8, 2, Pred_L1, BiPred, 16, 8},
    {15, B_L1_Bi_8x16, 2, Pred_L1, BiPred, 8, 16},
    {16, B_Bi_L0_16x8, 2, BiPred, Pred_L0, 16, 8},
    {17, B_Bi_L0_8x16, 2, BiPred, Pred_L0, 8, 16},
    {18, B_Bi_L1_16x8, 2, BiPred, Pred_L1, 16, 8},
    {19, B_Bi_L1_8x16, 2, BiPred, Pred_L1, 8, 16},
    {20, B_Bi_Bi_16x8, 2, BiPred, BiPred, 16, 8},
    {21, B_Bi_Bi_8x16, 2, BiPred, BiPred, 8, 16},
    {22, B_8x8, 4, Pred_NA, Pred_NA, 8, 8},
    {23, B_Skip, NA, Direct, Pred_NA, 8, 8}};

//Table 7-17 – Sub-macroblock types in P macroblocks
const SUB_MB_TYPE_P_MBS_T sub_mb_type_P_mbs_define[4] = {
    {0, P_L0_8x8, 1, Pred_L0, 8, 8},
    {1, P_L0_8x4, 2, Pred_L0, 8, 4},
    {2, P_L0_4x8, 2, Pred_L0, 4, 8},
    {3, P_L0_4x4, 4, Pred_L0, 4, 4}};

const SUB_MB_TYPE_B_MBS_T sub_mb_type_B_mbs_define[13] = {
    {0, B_Direct_8x8, 4, Direct, 4, 4}, {1, B_L0_8x8, 1, Pred_L0, 8, 8},
    {2, B_L1_8x8, 1, Pred_L1, 8, 8},    {3, B_Bi_8x8, 1, BiPred, 8, 8},
    {4, B_L0_8x4, 2, Pred_L0, 8, 4},    {5, B_L0_4x8, 2, Pred_L0, 4, 8},
    {6, B_L1_8x4, 2, Pred_L1, 8, 4},    {7, B_L1_4x8, 2, Pred_L1, 4, 8},
    {8, B_Bi_8x4, 2, BiPred, 8, 4},     {9, B_Bi_4x8, 2, BiPred, 4, 8},
    {10, B_L0_4x4, 4, Pred_L0, 4, 4},   {11, B_L1_4x4, 4, Pred_L1, 4, 4},
    {12, B_Bi_4x4, 4, BiPred, 4, 4}};

#endif /* end of include guard: TYPE_HPP_TPOWA9WD */
