#ifndef TYPE_HPP_TPOWA9WD
#define TYPE_HPP_TPOWA9WD

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <iostream> // IWYU pragma: export
#include <string.h> // IWYU pragma: export

using namespace std;

// 5.7 Mathematical functions
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define CEIL(x) (int(x))

#define CLIP(x, low, high)                                                     \
  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define CLIP3(x, y, z) (((z) < (x)) ? (x) : (((z) > (y)) ? (y) : (z)))

//(5-6)
#define Clip1C(x, BitDepthC) CLIP3(0, ((1 << (BitDepthC)) - 1), (x))

/* 
光栅扫描顺序是从左到右、从上到下逐行扫描的顺序。
    a: 线性扫描索引（通常是光栅扫描顺序中的索引）。
    b: 块的宽度或高度（取决于 e 的值）。
    c: 块的宽度或高度（与 b 相对，取决于 e 的值）。
    d: 图像的宽度或高度（取决于 e 的值）。
    e: 指定要计算的是行还是列：
        e == 0 时，计算列索引。
        e == 1 时，计算行索引。
 */
// 6.4.2.2 Inverse sub-macroblock partition scanning process
#define InverseRasterScan(a, b, c, d, e)                                       \
  ((e) == 0   ? ((a) % ((d) / (b))) * (b)                                      \
   : (e) == 1 ? ((a) / ((d) / (b))) * (c)                                      \
              : 0)

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

#define ROUND(x) ((int)((x) + 0.5))
#define ABS(x) ((int)(((x) >= (0)) ? (x) : (-(x))))
#define RETURN_IF_FAILED(condition, ret)                                       \
  do {                                                                         \
    if (condition) {                                                           \
      printf("%s(%d): %s: Error: ret=%d;\n", __FILE__, __LINE__, __FUNCTION__, \
             ret);                                                             \
      return ret;                                                              \
    }                                                                          \
  } while (0)

// Table 7-9 – Memory management control operation (memory_management_control_operation) values
typedef enum _H264_PICTURE_MARKED_AS_ {
  UNKOWN = 0,
  REFERENCE = 1,
  SHORT_REF = 2,
  LONG_REF = 3,
  NON_EXISTING = 4,
  UNUSED_REF = 5,
  OUTPUT_DISPLAY = 6,
} H264_PICTURE_MARKED_AS;

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
  void *bmBits;
} MY_BITMAP;

const int MIN_MB_TYPE_FOR_I_SLICE = 1, MAX_MB_TYPE_FOR_I_SLICE = 25;

typedef enum _H264_MB_TYPE_ {
  MB_TYPE_NA,

  // Macroblock types for I slices
  I_NxN,         //    0 -> 1
  I_16x16_0_0_0, //    1
  I_16x16_1_0_0, //    2
  I_16x16_2_0_0, //    3
  I_16x16_3_0_0, //    4
  I_16x16_0_1_0, //    5
  I_16x16_1_1_0, //    6
  I_16x16_2_1_0, //    7
  I_16x16_3_1_0, //    8
  I_16x16_0_2_0, //    9
  I_16x16_1_2_0, //    10
  I_16x16_2_2_0, //    11
  I_16x16_3_2_0, //    12
  I_16x16_0_0_1, //    13
  I_16x16_1_0_1, //    14
  I_16x16_2_0_1, //    15
  I_16x16_3_0_1, //    16
  I_16x16_0_1_1, //    17
  I_16x16_1_1_1, //    18
  I_16x16_2_1_1, //    19
  I_16x16_3_1_1, //    20
  I_16x16_0_2_1, //    21
  I_16x16_1_2_1, //    22
  I_16x16_2_2_1, //    23
  I_16x16_3_2_1, //    24
  I_PCM,         //    25

  // Macroblock type with value 0 for SI slices
  SI, //              0 -> 27

  // Macroblock type values 0 to 4 for P and SP slices
  P_L0_16x16,   //    0 -> 28
  P_L0_L0_16x8, //    1
  P_L0_L0_8x16, //    2
  P_8x8,        //    3
  P_8x8ref0,    //    4
  P_Skip,       //   -1

  // Macroblock type values 0 to 22 for B slices
  B_Direct_16x16, //  0 -> 34
  B_L0_16x16,     //  1
  B_L1_16x16,     //  2
  B_Bi_16x16,     //  3
  B_L0_L0_16x8,   //  4
  B_L0_L0_8x16,   //  5
  B_L1_L1_16x8,   //  6
  B_L1_L1_8x16,   //  7
  B_L0_L1_16x8,   //  8
  B_L0_L1_8x16,   //  9
  B_L1_L0_16x8,   //  10
  B_L1_L0_8x16,   //  11
  B_L0_Bi_16x8,   //  12
  B_L0_Bi_8x16,   //  13
  B_L1_Bi_16x8,   //  14
  B_L1_Bi_8x16,   //  15
  B_Bi_L0_16x8,   //  16
  B_Bi_L0_8x16,   //  17
  B_Bi_L1_16x8,   //  18
  B_Bi_L1_8x16,   //  19
  B_Bi_Bi_16x8,   //  20
  B_Bi_Bi_8x16,   //  21
  B_8x8,          //  22 -> 56
  B_Skip,         //  -1

  // Sub-macroblock types in P macroblocks
  P_L0_8x8, //    0
  P_L0_8x4, //    1
  P_L0_4x8, //    2
  P_L0_4x4, //    3

  // Sub-macroblock types in B macroblocks
  B_Direct_8x8, //    0
  B_L0_8x8,     //    1
  B_L1_8x8,     //    2
  B_Bi_8x8,     //    3
  B_L0_8x4,     //    4
  B_L0_4x8,     //    5
  B_L1_8x4,     //    6
  B_L1_4x8,     //    7
  B_Bi_8x4,     //    8
  B_Bi_4x8,     //    9
  B_L0_4x4,     //    10
  B_L1_4x4,     //    11
  B_Bi_4x4,     //    12
} H264_MB_TYPE;

/*
 * Figure 6-14 – Determination of the neighbouring macroblock, blocks, and
 * partitions (informative) D    B    C A    Current Macroblock or Partition or
 * Block
 */
typedef enum _MB_ADDR_TYPE_ {
  MB_ADDR_TYPE_UNKOWN = 0,
  MB_ADDR_TYPE_mbAddrA = 1,
  MB_ADDR_TYPE_mbAddrB = 2,
  MB_ADDR_TYPE_mbAddrC = 3,
  MB_ADDR_TYPE_mbAddrD = 4,
  MB_ADDR_TYPE_CurrMbAddr = 5,
  MB_ADDR_TYPE_mbAddrA_add_1 = 6,
  MB_ADDR_TYPE_mbAddrB_add_1 = 7,
  MB_ADDR_TYPE_mbAddrC_add_1 = 8,
  MB_ADDR_TYPE_mbAddrD_add_1 = 9,
  MB_ADDR_TYPE_CurrMbAddr_minus_1 = 10,
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
// Name of mb_type    transform_size_8x8_flag    MbPartPredMode(mb_type, 0)
// Intra16x16PredMode    CodedBlockPatternChroma    CodedBlockPatternLuma
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
// mb_type    Name of mb_type     MbPartPredMode(mb_type, 0) Intra16x16PredMode
// CodedBlockPatternChroma    CodedBlockPatternLuma
typedef struct _MB_TYPE_SI_SLICES_T_ {
  int32_t mb_type;
  H264_MB_TYPE name_of_mb_type;
  H264_MB_PART_PRED_MODE MbPartPredMode;
  int32_t Intra16x16PredMode;
  int32_t CodedBlockPatternChroma;
  int32_t CodedBlockPatternLuma;
} MB_TYPE_SI_SLICES_T;

// Table 7-13 – Macroblock type values 0 to 4 for P and SP slices
// mb_type    Name of mb_type    NumMbPart(mb_type)    MbPartPredMode(mb_type,
// 0)    MbPartPredMode(mb_type, 1)    MbPartWidth(mb_type)
// MbPartHeight(mb_type)
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
// mb_type    Name of mb_type    NumMbPart(mb_type)    MbPartPredMode(mb_type,
// 0)    MbPartPredMode(mb_type, 1)    MbPartWidth(mb_type)
// MbPartHeight(mb_type)
typedef struct _MB_TYPE_B_SLICES_T_ {
  int32_t mb_type;
  H264_MB_TYPE name_of_mb_type;
  int32_t NumMbPart;
  H264_MB_PART_PRED_MODE MbPartPredMode0;
  H264_MB_PART_PRED_MODE MbPartPredMode1;
  int32_t MbPartWidth;
  int32_t MbPartHeight;
} MB_TYPE_B_SLICES_T;

//--------------------------------------
// Table 7-17 – Sub-macroblock types in P macroblocks
// sub_mb_type[mbPartIdx]    Name of sub_mb_type[mbPartIdx]
// NumSubMbPart(sub_mb_type[mbPartIdx])    SubMbPredMode(sub_mb_type[mbPartIdx])
// SubMbPartWidth(sub_mb_type[ mbPartIdx])
// SubMbPartHeight(sub_mb_type[mbPartIdx])
typedef struct _SUB_MB_TYPE_P_MBS_T_ {
  int32_t sub_mb_type;
  H264_MB_TYPE name_of_sub_mb_type;
  int32_t NumSubMbPart;
  H264_MB_PART_PRED_MODE SubMbPredMode;
  int32_t SubMbPartWidth;
  int32_t SubMbPartHeight;
} SUB_MB_TYPE_P_MBS_T;

// Table 7-18 – Sub-macroblock types in B macroblocks
// sub_mb_type[mbPartIdx]    Name of sub_mb_type[mbPartIdx]
// NumSubMbPart(sub_mb_type[mbPartIdx])    SubMbPredMode(sub_mb_type[mbPartIdx])
// SubMbPartWidth(sub_mb_type[mbPartIdx])
// SubMbPartHeight(sub_mb_type[mbPartIdx])
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
  MB_RESIDUAL_Intra16x16DCLevel = 0,
  MB_RESIDUAL_Intra16x16ACLevel = 1,
  MB_RESIDUAL_LumaLevel4x4 = 2,
  MB_RESIDUAL_ChromaDCLevel = 3,
  MB_RESIDUAL_ChromaACLevel = 4,
  MB_RESIDUAL_LumaLevel8x8 = 5,
  MB_RESIDUAL_CbIntra16x16DCLevel = 6,
  MB_RESIDUAL_CbIntra16x16ACLevel = 7,
  MB_RESIDUAL_CbLevel4x4 = 8,
  MB_RESIDUAL_CbLevel8x8 = 9,
  MB_RESIDUAL_CrIntra16x16DCLevel = 10,
  MB_RESIDUAL_CrIntra16x16ACLevel = 11,
  MB_RESIDUAL_CrLevel4x4 = 12,
  MB_RESIDUAL_CrLevel8x8 = 13,
  MB_RESIDUAL_ChromaDCLevelCb = 14,
  MB_RESIDUAL_ChromaDCLevelCr = 15,
  MB_RESIDUAL_ChromaACLevelCb = 16,
  MB_RESIDUAL_ChromaACLevelCr = 17,
} MB_RESIDUAL_LEVEL;

#define NA -1
#define MB_WIDTH 16
#define MB_HEIGHT 16

//--------------------------
// Table 8-2 – Specification of Intra4x4PredMode[ luma4x4BlkIdx ] and associated names
#define Prediction_Mode_Intra_4x4_Vertical 0
#define Prediction_Mode_Intra_4x4_Horizontal 1
#define Prediction_Mode_Intra_4x4_DC 2
#define Prediction_Mode_Intra_4x4_Diagonal_Down_Left 3
#define Prediction_Mode_Intra_4x4_Diagonal_Down_Right 4
#define Prediction_Mode_Intra_4x4_Vertical_Right 5
#define Prediction_Mode_Intra_4x4_Horizontal_Down 6
#define Prediction_Mode_Intra_4x4_Vertical_Left 7
#define Prediction_Mode_Intra_4x4_Horizontal_Up 8

//--------------------------
// Table 8-3 – Specification of Intra8x8PredMode[ luma8x8BlkIdx ] and associated names
#define Prediction_Mode_Intra_8x8_Vertical 0
#define Prediction_Mode_Intra_8x8_Horizontal 1
#define Prediction_Mode_Intra_8x8_DC 2
#define Prediction_Mode_Intra_8x8_Diagonal_Down_Left 3
#define Prediction_Mode_Intra_8x8_Diagonal_Down_Right 4
#define Prediction_Mode_Intra_8x8_Vertical_Right 5
#define Prediction_Mode_Intra_8x8_Horizontal_Down 6
#define Prediction_Mode_Intra_8x8_Vertical_Left 7
#define Prediction_Mode_Intra_8x8_Horizontal_Up 8

//--------------------------
// Table 8-4 – Specification of Intra16x16PredMode and associated names
#define Prediction_Mode_Intra_16x16_Vertical 0
#define Prediction_Mode_Intra_16x16_Horizontal 1
#define Prediction_Mode_Intra_16x16_DC 2
#define Prediction_Mode_Intra_16x16_Plane 3

//--------------------------
// Table 8-5 – Specification of Intra chroma prediction modes and associated
// names
#define Prediction_Mode_Intra_Chroma_DC 0
#define Prediction_Mode_Intra_Chroma_Horizontal 1
#define Prediction_Mode_Intra_Chroma_Vertical 2
#define Prediction_Mode_Intra_Chroma_Plane 3

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

#define H264_MB_PART_PRED_MODE_TO_STR(pred_mode)                               \
  (pred_mode == MB_PRED_MODE_NA) ? "MB_PRED_MODE_NA"                           \
  : (pred_mode == Intra_NA)      ? "Intra_NA"                                  \
  : (pred_mode == Intra_4x4)     ? "Intra_4x4"                                 \
  : (pred_mode == Intra_8x8)     ? "Intra_8x8"                                 \
  : (pred_mode == Intra_16x16)   ? "Intra_16x16"                               \
  : (pred_mode == Pred_NA)       ? "Pred_NA"                                   \
  : (pred_mode == Pred_L0)       ? "Pred_L0"                                   \
  : (pred_mode == Pred_L1)       ? "Pred_L1"                                   \
  : (pred_mode == BiPred)        ? "BiPred"                                    \
  : (pred_mode == BiPred)        ? "BiPred"                                    \
  : (pred_mode == Direct)        ? "Direct"                                    \
                                 : "UNKNOWN"

// Table 8-8 – Specification of mbAddrCol, yM, and vertMvScale
typedef enum _H264_VERT_MV_SCALE_ {
  H264_VERT_MV_SCALE_UNKNOWN = 0,
  H264_VERT_MV_SCALE_One_To_One = 1,
  H264_VERT_MV_SCALE_Frm_To_Fld = 2,
  H264_VERT_MV_SCALE_Fld_To_Frm = 3,
} H264_VERT_MV_SCALE;


#define MAX_DPB 16                     // DPB[16]
#define H264_MAX_REF_PIC_LIST_COUNT 16 // RefPicList0[16]

#endif /* end of include guard: TYPE_HPP_TPOWA9WD */
