#include "Type.hpp"

class BitStream;

int LOG2(int32_t value);
int32_t POWER2(int32_t value);

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

int profile_tier_level(BitStream &bs, bool profilePresentFlag,
                       int32_t maxNumSubLayersMinus1);
