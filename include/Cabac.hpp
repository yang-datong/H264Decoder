#ifndef H264CABAC_HPP_YF2ZLNUA
#define H264CABAC_HPP_YF2ZLNUA

#include "BitStream.hpp"
#include "ContextModel3DBuffer.h"
#include "SliceHeader.hpp"
#include "Type.hpp"
#include <stdint.h>
#include <stdlib.h>

class PictureBase;
class Cabac {
 private:
  ContextModel3DBuffer *m_cSaoMergeSCModel;
  //上下文变量
  uint8_t pStateIdxs[1024] = {0};
  bool valMPSs[1024] = {0};

  //上下文引擎
  int32_t codIRange = 0;
  int32_t codIOffset = 0;

  /* 声明为引用，如果Cabac消费了bs流，对应的外层也需要同样被消费 */
  BitStream &bs;
  PictureBase &picture;

 public:
  Cabac(BitStream &bitStream, PictureBase &pic) : bs(bitStream), picture(pic){};

  /* ============== 9.3.1 Initialization process ============== */
 public:
  int init_of_context_variables(H264_SLICE_TYPE slice_type,
                                int32_t cabac_init_idc, int32_t SliceQPY);
  int init_of_decoding_engine();
  int get_cabac_inline(uint8_t * const state);

    int ivlCurrRange = 0;
  int ivlOffset = 0;

 private:
  int init_m_n(int32_t ctxIdx, H264_SLICE_TYPE slice_type,
               int32_t cabac_init_idc, int32_t &m, int32_t &n);

  int derivation_ctxIdxInc_for_mb_skip_flag(int32_t currMbAddr,
                                            int32_t &ctxIdxInc);
  int derivation_ctxIdxInc_for_mb_field_decoding_flag(int32_t &ctxIdxInc);
  int derivation_ctxIdxInc_for_mb_type(int32_t ctxIdxOffset,
                                       int32_t &ctxIdxInc);
  int derivation_ctxIdxInc_for_coded_block_pattern(int32_t binIdx,
                                                   int32_t binValues,
                                                   int32_t ctxIdxOffset,
                                                   int32_t &ctxIdxInc);
  int derivation_ctxIdxInc_for_mb_qp_delta(int32_t &ctxIdxInc);
  int derivation_ctxIdxInc_for_ref_idx_lX(int32_t is_ref_idx_10,
                                          int32_t mbPartIdx,
                                          int32_t &ctxIdxInc);
  int derivation_ctxIdxInc_for_mvd_lX(int32_t is_mvd_10, int32_t mbPartIdx,
                                      int32_t subMbPartIdx, int32_t isChroma,
                                      int32_t ctxIdxOffset, int32_t &ctxIdxInc);
  int derivation_ctxIdxInc_for_intra_chroma_pred_mode(int32_t &ctxIdxInc);
  int derivation_ctxIdxInc_for_coded_block_flag(int32_t ctxBlockCat,
                                                int32_t BlkIdx, int32_t iCbCr,
                                                int32_t &ctxIdxInc);
  int derivation_ctxIdxInc_for_transform_size_8x8_flag(int32_t &ctxIdxInc);

  int decodeBin(int32_t bypassFlag, int32_t ctxIdx, int32_t &bin);
  void decodeBin(int &ruiBin, ContextModel &rcCtxModel);
  int decodeDecision(int32_t ctxIdx, int32_t &binVal);
  int decodeBypass(int32_t &binVal);
  int decodeTerminate(int32_t &binVal);
  int renormD();

  int decode_mb_type_in_I_slices(int32_t ctxIdxOffset, int32_t &synElVal);
  int decode_mb_type_in_SI_slices(int32_t &synElVal);
  int decode_mb_type_in_P_SP_slices(int32_t &synElVal);
  int decode_mb_type_in_B_slices(int32_t &synElVal);
  int decode_sub_mb_type_in_P_SP_slices(int32_t &synElVal);
  int decode_sub_mb_type_in_B_slices(int32_t &synElVal);

 public:
  int decode_mb_skip_flag(int32_t currMbAddr, int32_t &synElVal);
  int decode_mb_field_decoding_flag(int32_t &synElVal);
  int decode_mb_type(int32_t &synElVal);
  int decode_sub_mb_type(int32_t &synElVal);
  int decode_mvd_lX(int32_t mvd_flag, int32_t mbPartIdx, int32_t subMbPartIdx,
                    int32_t isChroma, int32_t &synElVal);
  int decode_ref_idx_lX(int32_t ref_idx_flag, int32_t mbPartIdx,
                        int32_t &synElVal);
  int decode_mb_qp_delta(int32_t &synElVal);
  int decode_intra_chroma_pred_mode(int32_t &synElVal);
  int decode_prev_intra4x4_or_intra8x8_pred_mode_flag(int32_t &synElVal);
  int decode_rem_intra4x4_or_intra8x8_pred_mode(int32_t &synElVal);
  int decode_coded_block_pattern(int32_t &synElVal);

 private:
  int decode_coded_block_flag(MB_RESIDUAL_LEVEL mb_block_level, int32_t BlkIdx,
                              int32_t iCbCr, int32_t &synElVal);
  int decode_significant_coeff_flag(MB_RESIDUAL_LEVEL mb_block_level,
                                    int32_t levelListIdx, int32_t last_flag,
                                    int32_t &synElVal);
  int decode_coeff_abs_level_minus1(MB_RESIDUAL_LEVEL mb_block_level,
                                    int32_t numDecodAbsLevelEq1,
                                    int32_t numDecodAbsLevelGt1,
                                    int32_t &synElVal);
  int decode_coeff_sign_flag(int32_t &synElVal);

 public:
  int decode_transform_size_8x8_flag(int32_t &synElVal);
  int decode_end_of_slice_flag(int32_t &synElVal);
  int residual_block_cabac(int32_t coeffLevel[], int32_t startIdx,
                           int32_t endIdx, int32_t maxNumCoeff,
                           MB_RESIDUAL_LEVEL mb_block_level, int32_t BlkIdx,
                           int32_t iCbCr, int32_t &TotalCoeff);

 public:
int initialization_decoding_engine();
  int initialization_context_variables(SliceHeader *header);
  int initialization_palette_predictor_entries(SPS *sps, PPS *pps);
  int preCtxState[HEVC_CONTEXTS] = {0};
  int valMps[HEVC_CONTEXTS] = {0};
  int pStateIdx[HEVC_CONTEXTS] = {0};

  //typedef struct CABACContext {
    //int low;
    //int range;
    //const uint8_t *bytestream_start;
    //const uint8_t *bytestream;
    //const uint8_t *bytestream_end;
  //} CABACContext;

  //CABACContext *c = nullptr;
  int deocde_sao_merge_left_flag();
  //int ff_hevc_sao_merge_flag_decode();
  //int get_cabac(uint8_t *const state);
  void refill2();
  //uint8_t cabac_state[HEVC_CONTEXTS];
};

const int8_t num_bins_in_se[] = {
    1,  // sao_merge_flag
    1,  // sao_type_idx
    0,  // sao_eo_class
    0,  // sao_band_position
    0,  // sao_offset_abs
    0,  // sao_offset_sign
    0,  // end_of_slice_flag
    3,  // split_coding_unit_flag
    1,  // cu_transquant_bypass_flag
    3,  // skip_flag
    3,  // cu_qp_delta
    1,  // pred_mode
    4,  // part_mode
    0,  // pcm_flag
    1,  // prev_intra_luma_pred_mode
    0,  // mpm_idx
    0,  // rem_intra_luma_pred_mode
    2,  // intra_chroma_pred_mode
    1,  // merge_flag
    1,  // merge_idx
    5,  // inter_pred_idc
    2,  // ref_idx_l0
    2,  // ref_idx_l1
    2,  // abs_mvd_greater0_flag
    2,  // abs_mvd_greater1_flag
    0,  // abs_mvd_minus2
    0,  // mvd_sign_flag
    1,  // mvp_lx_flag
    1,  // no_residual_data_flag
    3,  // split_transform_flag
    2,  // cbf_luma
    5,  // cbf_cb, cbf_cr
    2,  // transform_skip_flag[][]
    2,  // explicit_rdpcm_flag[][]
    2,  // explicit_rdpcm_dir_flag[][]
    18, // last_significant_coeff_x_prefix
    18, // last_significant_coeff_y_prefix
    0,  // last_significant_coeff_x_suffix
    0,  // last_significant_coeff_y_suffix
    4,  // significant_coeff_group_flag
    44, // significant_coeff_flag
    24, // coeff_abs_level_greater1_flag
    6,  // coeff_abs_level_greater2_flag
    0,  // coeff_abs_level_remaining
    0,  // coeff_sign_flag
    8,  // log2_res_scale_abs
    2,  // res_scale_sign_flag
    1,  // cu_chroma_qp_offset_flag
    1,  // cu_chroma_qp_offset_idx
};
const int elem_offset[sizeof(num_bins_in_se)] = {
    0,   // sao_merge_flag
    1,   // sao_type_idx
    2,   // sao_eo_class
    2,   // sao_band_position
    2,   // sao_offset_abs
    2,   // sao_offset_sign
    2,   // end_of_slice_flag
    2,   // split_coding_unit_flag
    5,   // cu_transquant_bypass_flag
    6,   // skip_flag
    9,   // cu_qp_delta
    12,  // pred_mode
    13,  // part_mode
    17,  // pcm_flag
    17,  // prev_intra_luma_pred_mode
    18,  // mpm_idx
    18,  // rem_intra_luma_pred_mode
    18,  // intra_chroma_pred_mode
    20,  // merge_flag
    21,  // merge_idx
    22,  // inter_pred_idc
    27,  // ref_idx_l0
    29,  // ref_idx_l1
    31,  // abs_mvd_greater0_flag
    33,  // abs_mvd_greater1_flag
    35,  // abs_mvd_minus2
    35,  // mvd_sign_flag
    35,  // mvp_lx_flag
    36,  // no_residual_data_flag
    37,  // split_transform_flag
    40,  // cbf_luma
    42,  // cbf_cb, cbf_cr
    47,  // transform_skip_flag[][]
    49,  // explicit_rdpcm_flag[][]
    51,  // explicit_rdpcm_dir_flag[][]
    53,  // last_significant_coeff_x_prefix
    71,  // last_significant_coeff_y_prefix
    89,  // last_significant_coeff_x_suffix
    89,  // last_significant_coeff_y_suffix
    89,  // significant_coeff_group_flag
    93,  // significant_coeff_flag
    137, // coeff_abs_level_greater1_flag
    161, // coeff_abs_level_greater2_flag
    167, // coeff_abs_level_remaining
    167, // coeff_sign_flag
    167, // log2_res_scale_abs
    175, // res_scale_sign_flag
    177, // cu_chroma_qp_offset_flag
    178, // cu_chroma_qp_offset_idx
};

#define AV_GCC_VERSION_AT_LEAST(x, y)                                          \
  (__GNUC__ > (x) || __GNUC__ == (x) && __GNUC_MINOR__ >= (y))
#if AV_GCC_VERSION_AT_LEAST(3, 1) || defined(__clang__)
#define av_used __attribute__((used))
#else
#define av_used
#endif

#define DECLARE_ASM_ALIGNED(n, t, v) t av_used __attribute__((aligned(n))) v
DECLARE_ASM_ALIGNED(1, const uint8_t, ff_h264_cabac_tables)
[512 + 4 * 2 * 64 + 4 * 64 + 63] = {
    9, 8, 7, 7, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // LPS range
    //    static_cast<uint8_t>(-128)
    static_cast<uint8_t>(-128), static_cast<uint8_t>(-128),
    static_cast<uint8_t>(-128), static_cast<uint8_t>(-128),
    static_cast<uint8_t>(-128), static_cast<uint8_t>(-128), 123, 123, 116, 116,
    111, 111, 105, 105, 100, 100, 95, 95, 90, 90, 85, 85, 81, 81, 77, 77, 73,
    73, 69, 69, 66, 66, 62, 62, 59, 59, 56, 56, 53, 53, 51, 51, 48, 48, 46, 46,
    43, 43, 41, 41, 39, 39, 37, 37, 35, 35, 33, 33, 32, 32, 30, 30, 29, 29, 27,
    27, 26, 26, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20, 19, 19, 18, 18, 17, 17,
    16, 16, 15, 15, 14, 14, 14, 14, 13, 13, 12, 12, 12, 12, 11, 11, 11, 11, 10,
    10, 10, 10, 9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6, 6, 2,
    2, static_cast<uint8_t>(-80), static_cast<uint8_t>(-80),
    static_cast<uint8_t>(-89), static_cast<uint8_t>(-89),
    static_cast<uint8_t>(-98), static_cast<uint8_t>(-98),
    static_cast<uint8_t>(-106), static_cast<uint8_t>(-106),
    static_cast<uint8_t>(-114), static_cast<uint8_t>(-114),
    static_cast<uint8_t>(-121), static_cast<uint8_t>(-121),
    static_cast<uint8_t>(-128), static_cast<uint8_t>(-128), 122, 122, 116, 116,
    110, 110, 104, 104, 99, 99, 94, 94, 89, 89, 85, 85, 80, 80, 76, 76, 72, 72,
    69, 69, 65, 65, 62, 62, 59, 59, 56, 56, 53, 53, 50, 50, 48, 48, 45, 45, 43,
    43, 41, 41, 39, 39, 37, 37, 35, 35, 33, 33, 31, 31, 30, 30, 28, 28, 27, 27,
    26, 26, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20, 19, 19, 18, 18, 17, 17, 16,
    16, 15, 15, 14, 14, 14, 14, 13, 13, 12, 12, 12, 12, 11, 11, 11, 11, 10, 10,
    9, 9, 9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 2, 2, static_cast<uint8_t>(-48),
    static_cast<uint8_t>(-48), static_cast<uint8_t>(-59),
    static_cast<uint8_t>(-59), static_cast<uint8_t>(-69),
    static_cast<uint8_t>(-69), static_cast<uint8_t>(-78),
    static_cast<uint8_t>(-78), static_cast<uint8_t>(-87),
    static_cast<uint8_t>(-87), static_cast<uint8_t>(-96),
    static_cast<uint8_t>(-96), static_cast<uint8_t>(-104),
    static_cast<uint8_t>(-104), static_cast<uint8_t>(-112),
    static_cast<uint8_t>(-112), static_cast<uint8_t>(-119),
    static_cast<uint8_t>(-119), static_cast<uint8_t>(-126),
    static_cast<uint8_t>(-126), 123, 123, 117, 117, 111, 111, 105, 105, 100,
    100, 95, 95, 90, 90, 86, 86, 81, 81, 77, 77, 73, 73, 69, 69, 66, 66, 63, 63,
    59, 59, 56, 56, 54, 54, 51, 51, 48, 48, 46, 46, 43, 43, 41, 41, 39, 39, 37,
    37, 35, 35, 33, 33, 32, 32, 30, 30, 29, 29, 27, 27, 26, 26, 25, 25, 23, 23,
    22, 22, 21, 21, 20, 20, 19, 19, 18, 18, 17, 17, 16, 16, 15, 15, 15, 15, 14,
    14, 13, 13, 12, 12, 12, 12, 11, 11, 11, 11, 10, 10, 10, 10, 9, 9, 9, 9, 8,
    8, 2, 2, static_cast<uint8_t>(-16), static_cast<uint8_t>(-16),
    static_cast<uint8_t>(-29), static_cast<uint8_t>(-29),
    static_cast<uint8_t>(-40), static_cast<uint8_t>(-40),
    static_cast<uint8_t>(-51), static_cast<uint8_t>(-51),
    static_cast<uint8_t>(-61), static_cast<uint8_t>(-61),
    static_cast<uint8_t>(-71), static_cast<uint8_t>(-71),
    static_cast<uint8_t>(-81), static_cast<uint8_t>(-81),
    static_cast<uint8_t>(-90), static_cast<uint8_t>(-90),
    static_cast<uint8_t>(-98), static_cast<uint8_t>(-98),
    static_cast<uint8_t>(-106), static_cast<uint8_t>(-106),
    static_cast<uint8_t>(-114), static_cast<uint8_t>(-114),
    static_cast<uint8_t>(-121), static_cast<uint8_t>(-121),
    static_cast<uint8_t>(-128), static_cast<uint8_t>(-128), 122, 122, 116, 116,
    110, 110, 104, 104, 99, 99, 94, 94, 89, 89, 85, 85, 80, 80, 76, 76, 72, 72,
    69, 69, 65, 65, 62, 62, 59, 59, 56, 56, 53, 53, 50, 50, 48, 48, 45, 45, 43,
    43, 41, 41, 39, 39, 37, 37, 35, 35, 33, 33, 31, 31, 30, 30, 28, 28, 27, 27,
    25, 25, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20, 19, 19, 18, 18, 17, 17, 16,
    16, 15, 15, 14, 14, 14, 14, 13, 13, 12, 12, 12, 12, 11, 11, 11, 11, 10, 10,
    9, 9, 2, 2,
    // mlps state
    127, 126, 77, 76, 77, 76, 75, 74, 75, 74, 75, 74, 73, 72, 73, 72, 73, 72,
    71, 70, 71, 70, 71, 70, 69, 68, 69, 68, 67, 66, 67, 66, 67, 66, 65, 64, 65,
    64, 63, 62, 61, 60, 61, 60, 61, 60, 59, 58, 59, 58, 57, 56, 55, 54, 55, 54,
    53, 52, 53, 52, 51, 50, 49, 48, 49, 48, 47, 46, 45, 44, 45, 44, 43, 42, 43,
    42, 39, 38, 39, 38, 37, 36, 37, 36, 33, 32, 33, 32, 31, 30, 31, 30, 27, 26,
    27, 26, 25, 24, 23, 22, 23, 22, 19, 18, 19, 18, 17, 16, 15, 14, 13, 12, 11,
    10, 9, 8, 9, 8, 5, 4, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
    68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
    87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 124, 125, 126, 127,
    // last_coeff_flag_offset_8x8
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5,
    5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8};

#define CABAC_ELEMS(ELEM)                                                      \
  ELEM(SAO_MERGE_FLAG, 1)                                                      \
  ELEM(SAO_TYPE_IDX, 1)                                                        \
  ELEM(SAO_EO_CLASS, 0)                                                        \
  ELEM(SAO_BAND_POSITION, 0)                                                   \
  ELEM(SAO_OFFSET_ABS, 0)                                                      \
  ELEM(SAO_OFFSET_SIGN, 0)                                                     \
  ELEM(END_OF_SLICE_FLAG, 0)                                                   \
  ELEM(SPLIT_CODING_UNIT_FLAG, 3)                                              \
  ELEM(CU_TRANSQUANT_BYPASS_FLAG, 1)                                           \
  ELEM(SKIP_FLAG, 3)                                                           \
  ELEM(CU_QP_DELTA, 3)                                                         \
  ELEM(PRED_MODE_FLAG, 1)                                                      \
  ELEM(PART_MODE, 4)                                                           \
  ELEM(PCM_FLAG, 0)                                                            \
  ELEM(PREV_INTRA_LUMA_PRED_FLAG, 1)                                           \
  ELEM(MPM_IDX, 0)                                                             \
  ELEM(REM_INTRA_LUMA_PRED_MODE, 0)                                            \
  ELEM(INTRA_CHROMA_PRED_MODE, 2)                                              \
  ELEM(MERGE_FLAG, 1)                                                          \
  ELEM(MERGE_IDX, 1)                                                           \
  ELEM(INTER_PRED_IDC, 5)                                                      \
  ELEM(REF_IDX_L0, 2)                                                          \
  ELEM(REF_IDX_L1, 2)                                                          \
  ELEM(ABS_MVD_GREATER0_FLAG, 2)                                               \
  ELEM(ABS_MVD_GREATER1_FLAG, 2)                                               \
  ELEM(ABS_MVD_MINUS2, 0)                                                      \
  ELEM(MVD_SIGN_FLAG, 0)                                                       \
  ELEM(MVP_LX_FLAG, 1)                                                         \
  ELEM(NO_RESIDUAL_DATA_FLAG, 1)                                               \
  ELEM(SPLIT_TRANSFORM_FLAG, 3)                                                \
  ELEM(CBF_LUMA, 2)                                                            \
  ELEM(CBF_CB_CR, 5)                                                           \
  ELEM(TRANSFORM_SKIP_FLAG, 2)                                                 \
  ELEM(EXPLICIT_RDPCM_FLAG, 2)                                                 \
  ELEM(EXPLICIT_RDPCM_DIR_FLAG, 2)                                             \
  ELEM(LAST_SIGNIFICANT_COEFF_X_PREFIX, 18)                                    \
  ELEM(LAST_SIGNIFICANT_COEFF_Y_PREFIX, 18)                                    \
  ELEM(LAST_SIGNIFICANT_COEFF_X_SUFFIX, 0)                                     \
  ELEM(LAST_SIGNIFICANT_COEFF_Y_SUFFIX, 0)                                     \
  ELEM(SIGNIFICANT_COEFF_GROUP_FLAG, 4)                                        \
  ELEM(SIGNIFICANT_COEFF_FLAG, 44)                                             \
  ELEM(COEFF_ABS_LEVEL_GREATER1_FLAG, 24)                                      \
  ELEM(COEFF_ABS_LEVEL_GREATER2_FLAG, 6)                                       \
  ELEM(COEFF_ABS_LEVEL_REMAINING, 0)                                           \
  ELEM(COEFF_SIGN_FLAG, 0)                                                     \
  ELEM(LOG2_RES_SCALE_ABS, 8)                                                  \
  ELEM(RES_SCALE_SIGN_FLAG, 2)                                                 \
  ELEM(CU_CHROMA_QP_OFFSET_FLAG, 1)                                            \
  ELEM(CU_CHROMA_QP_OFFSET_IDX, 1)

#endif /* end of include guard: H264CABAC_HPP_YF2ZLNUA */
