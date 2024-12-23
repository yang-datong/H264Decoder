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

#endif /* end of include guard: H264CABAC_HPP_YF2ZLNUA */
