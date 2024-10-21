#ifndef H264CABAC_HPP_YF2ZLNUA
#define H264CABAC_HPP_YF2ZLNUA

#include "BitStream.hpp"
#include "Type.hpp"
#include <stdint.h>
#include <stdlib.h>

class PictureBase;
class Cabac {
 private:
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
};

static const int8_t num_bins_in_se[] = {
     1, // sao_merge_flag
     1, // sao_type_idx
     0, // sao_eo_class
     0, // sao_band_position
     0, // sao_offset_abs
     0, // sao_offset_sign
     0, // end_of_slice_flag
     3, // split_coding_unit_flag
     1, // cu_transquant_bypass_flag
     3, // skip_flag
     3, // cu_qp_delta
     1, // pred_mode
     4, // part_mode
     0, // pcm_flag
     1, // prev_intra_luma_pred_mode
     0, // mpm_idx
     0, // rem_intra_luma_pred_mode
     2, // intra_chroma_pred_mode
     1, // merge_flag
     1, // merge_idx
     5, // inter_pred_idc
     2, // ref_idx_l0
     2, // ref_idx_l1
     2, // abs_mvd_greater0_flag
     2, // abs_mvd_greater1_flag
     0, // abs_mvd_minus2
     0, // mvd_sign_flag
     1, // mvp_lx_flag
     1, // no_residual_data_flag
     3, // split_transform_flag
     2, // cbf_luma
     5, // cbf_cb, cbf_cr
     2, // transform_skip_flag[][]
     2, // explicit_rdpcm_flag[][]
     2, // explicit_rdpcm_dir_flag[][]
    18, // last_significant_coeff_x_prefix
    18, // last_significant_coeff_y_prefix
     0, // last_significant_coeff_x_suffix
     0, // last_significant_coeff_y_suffix
     4, // significant_coeff_group_flag
    44, // significant_coeff_flag
    24, // coeff_abs_level_greater1_flag
     6, // coeff_abs_level_greater2_flag
     0, // coeff_abs_level_remaining
     0, // coeff_sign_flag
     8, // log2_res_scale_abs
     2, // res_scale_sign_flag
     1, // cu_chroma_qp_offset_flag
     1, // cu_chroma_qp_offset_idx
};
static const int elem_offset[sizeof(num_bins_in_se)] = {
    0, // sao_merge_flag
    1, // sao_type_idx
    2, // sao_eo_class
    2, // sao_band_position
    2, // sao_offset_abs
    2, // sao_offset_sign
    2, // end_of_slice_flag
    2, // split_coding_unit_flag
    5, // cu_transquant_bypass_flag
    6, // skip_flag
    9, // cu_qp_delta
    12, // pred_mode
    13, // part_mode
    17, // pcm_flag
    17, // prev_intra_luma_pred_mode
    18, // mpm_idx
    18, // rem_intra_luma_pred_mode
    18, // intra_chroma_pred_mode
    20, // merge_flag
    21, // merge_idx
    22, // inter_pred_idc
    27, // ref_idx_l0
    29, // ref_idx_l1
    31, // abs_mvd_greater0_flag
    33, // abs_mvd_greater1_flag
    35, // abs_mvd_minus2
    35, // mvd_sign_flag
    35, // mvp_lx_flag
    36, // no_residual_data_flag
    37, // split_transform_flag
    40, // cbf_luma
    42, // cbf_cb, cbf_cr
    47, // transform_skip_flag[][]
    49, // explicit_rdpcm_flag[][]
    51, // explicit_rdpcm_dir_flag[][]
    53, // last_significant_coeff_x_prefix
    71, // last_significant_coeff_y_prefix
    89, // last_significant_coeff_x_suffix
    89, // last_significant_coeff_y_suffix
    89, // significant_coeff_group_flag
    93, // significant_coeff_flag
    137, // coeff_abs_level_greater1_flag
    161, // coeff_abs_level_greater2_flag
    167, // coeff_abs_level_remaining
    167, // coeff_sign_flag
    167, // log2_res_scale_abs
    175, // res_scale_sign_flag
    177, // cu_chroma_qp_offset_flag
    178, // cu_chroma_qp_offset_idx
};

#endif /* end of include guard: H264CABAC_HPP_YF2ZLNUA */
