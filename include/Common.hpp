#ifndef COMMON_HPP_9P5ZWDMO
#define COMMON_HPP_9P5ZWDMO

#include "Type.hpp"
#include <cstdint>
#include <cstdio>
#include <iostream> // IWYU pragma: export
#include <string.h> // IWYU pragma: export
using namespace std;

// 5.7 Mathematical functions
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ABS(x) ((int)(((x) >= (0)) ? (x) : (-(x))))

#define CEIL(x) (int(x))
#define CLIP(x, low, high)                                                     \
  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define CLIP3(x, y, z) (((z) < (x)) ? (x) : (((z) > (y)) ? (y) : (z)))
#define Clip1C(x, BitDepthC) CLIP3(0, ((1 << (BitDepthC)) - 1), (x))

int LOG2(int32_t value);
int32_t POWER2(int32_t value);

#define RET(ret)                                                               \
  if (ret) {                                                                   \
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__  \
              << std::endl;                                                    \
    return ret;                                                                \
  }

#define DELETE(ptr)                                                            \
  if (ptr) {                                                                   \
    delete ptr;                                                                \
    ptr = nullptr;                                                             \
  }

#define FREE(ptr)                                                              \
  if (ptr) {                                                                   \
    free(ptr);                                                                 \
    ptr = nullptr;                                                             \
  }

#define IS_INTRA_Prediction_Mode(v)                                            \
  (v != MB_PRED_MODE_NA &&                                                     \
   (v == Intra_4x4 || v == Intra_8x8 || v == Intra_16x16))

#define IS_INTER_Prediction_Mode(v)                                            \
  (v != MB_PRED_MODE_NA && (v == Pred_L0 || v == Pred_L1 || v == BiPred))

/* 光栅扫描顺序是从左到右、从上到下逐行扫描的顺序。
    a: 线性扫描索引（通常是光栅扫描顺序中的索引）。
    b: 块的宽度或高度（取决于 e 的值）。
    c: 块的宽度或高度（与 b 相对，取决于 e 的值）。
    d: 图像的宽度或高度（取决于 e 的值）。
    e: 指定要计算的是行还是列：e == 0 时，计算列索引。e == 1 时，计算行索引。*/
// 6.4.2.2 Inverse sub-macroblock partition scanning process
#define InverseRasterScan(a, b, c, d, e)                                       \
  ((e) == 0   ? ((a) % ((d) / (b))) * (b)                                      \
   : (e) == 1 ? ((a) / ((d) / (b))) * (c)                                      \
              : 0)

class BitStream;
/* 从比特流中解析 4x4/8x8 缩放矩阵 */
void scaling_list(BitStream &bitStream, uint32_t *scalingList,
                  uint32_t sizeOfScalingList,
                  uint32_t &useDefaultScalingMatrixFlag);

string MacroBlockNmae(H264_MB_TYPE m_name_of_mb_type);
string MacroBlockPredMode(H264_MB_PART_PRED_MODE m_mb_pred_mode);

int inverse_scanning_for_4x4_transform_coeff_and_scaling_lists(
    const int32_t values[16], int32_t (&c)[4][4], int32_t field_scan_flag);
int inverse_scanning_for_8x8_transform_coeff_and_scaling_lists(
    int32_t values[64], int32_t (&c)[8][8], int32_t field_scan_flag);

#endif /* end of include guard: COMMON_HPP_9P5ZWDMO */
