#include "Type.hpp"

class BitStream;

int h264_log2(int32_t value);

int32_t h264_power2(int32_t value);

#define RET(ret)                                                               \
  if (ret) {                                                                   \
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__  \
              << std::endl;                                                    \
    return ret;                                                                \
  }

#define FREE(ptr)                                                              \
  if (ptr) {                                                                   \
    delete ptr;                                                                \
    ptr = nullptr;                                                             \
  }

#define IS_INTRA_Prediction_Mode(v)                                            \
  (v != MB_PRED_MODE_NA &&                                                     \
   (v == Intra_4x4 || v == Intra_8x8 || v == Intra_16x16))

#define IS_INTER_Prediction_Mode(v)                                            \
  (v != MB_PRED_MODE_NA && (v == Pred_L0 || v == Pred_L1 || v == BiPred))

#define LOG_INFO(pszFormat, ...)                                               \
  printf("[INFO] %s:(%d) %s: " pszFormat, __FILE__, __LINE__, __FUNCTION__,    \
         ##__VA_ARGS__)
#define LOG_ERROR(pszFormat, ...)                                              \
  printf("[ERR] %s:(%d) %s: " pszFormat, __FILE__, __LINE__, __FUNCTION__,     \
         ##__VA_ARGS__)
#define LOG_WARN(pszFormat, ...)                                               \
  printf("[WARN] %s:(%d) %s: " pszFormat, __FILE__, __LINE__, __FUNCTION__,    \
         ##__VA_ARGS__)

/* 从比特流中解析 4x4/8x8 缩放矩阵 */
void scaling_list(BitStream &bitStream, uint32_t *scalingList,
                  uint32_t sizeOfScalingList,
                  uint32_t &useDefaultScalingMatrixFlag);

int inverse_scanning_for_4x4_transform_coeff_and_scaling_lists(
    const int32_t values[16], int32_t (&c)[4][4], int32_t field_scan_flag);

int inverse_scanning_for_8x8_transform_coeff_and_scaling_lists(
    int32_t values[64], int32_t (&c)[8][8], int32_t field_scan_flag);
