#ifndef H264CABAC_HPP_YF2ZLNUA
#define H264CABAC_HPP_YF2ZLNUA

#include "BitStream.hpp"
#include "Type.hpp"
#include <stdint.h>
#include <stdlib.h>

class PictureBase;
class CH264Cabac {
 private:
  int32_t _codIRange = 0;
  int32_t _codIOffset = 0;

  int32_t _pStateIdxs[1024] = {0};
  int32_t _valMPSs[1024] = {0};

 public:
  /* ============== 9.3.1 Initialization process ============== */
  inline int init_m_n(int32_t ctxIdx, H264_SLICE_TYPE slice_type,
                      int32_t cabac_init_idc, int32_t &m, int32_t &n);
  int init_of_context_variables(H264_SLICE_TYPE slice_type,
                                int32_t cabac_init_idc, int32_t SliceQPY);
  int init_of_decoding_engine(BitStream &bs);

  int derivation_of_ctxIdxInc_for_mb_skip_flag(PictureBase &picture,
                                               const int32_t currMbAddr,
                                               int32_t &ctxIdxInc);
  int Derivation_process_of_ctxIdxInc_for_the_syntax_element_mb_field_decoding_flag(
      PictureBase &picture, int32_t &ctxIdxInc); // 9.3.3.1.1.2
  int Derivation_process_of_ctxIdxInc_for_the_syntax_element_mb_type(
      PictureBase &picture, int32_t ctxIdxOffset,
      int32_t &ctxIdxInc); // 9.3.3.1.1.3
  int Derivation_process_of_ctxIdxInc_for_the_syntax_element_coded_block_pattern(
      PictureBase &picture, int32_t binIdx, int32_t binValues,
      int32_t ctxIdxOffset, int32_t &ctxIdxInc); // 9.3.3.1.1.4
  int Derivation_process_of_ctxIdxInc_for_the_syntax_element_mb_qp_delta(
      PictureBase &picture, int32_t &ctxIdxInc); // 9.3.3.1.1.5
  int Derivation_process_of_ctxIdxInc_for_the_syntax_elements_ref_idx_l0_and_ref_idx_l1(
      PictureBase &picture, int32_t is_ref_idx_10, int32_t mbPartIdx,
      int32_t &ctxIdxInc); // 9.3.3.1.1.6
  int Derivation_process_of_ctxIdxInc_for_the_syntax_elements_mvd_l0_and_mvd_l1(
      PictureBase &picture, int32_t is_mvd_10, int32_t mbPartIdx,
      int32_t subMbPartIdx, int32_t isChroma, int32_t ctxIdxOffset,
      int32_t &ctxIdxInc); // 9.3.3.1.1.7
  int Derivation_process_of_ctxIdxInc_for_the_syntax_element_intra_chroma_pred_mode(
      PictureBase &picture, int32_t &ctxIdxInc); // 9.3.3.1.1.8
  int Derivation_process_of_ctxIdxInc_for_the_syntax_element_coded_block_flag(
      PictureBase &picture, int32_t ctxBlockCat, int32_t BlkIdx, int32_t iCbCr,
      int32_t &ctxIdxInc); // 9.3.3.1.1.9
  int Derivation_process_of_ctxIdxInc_for_the_syntax_element_transform_size_8x8_flag(
      PictureBase &picture, int32_t &ctxIdxInc); // 9.3.3.1.1.10

  //-----------------------------------------
  int DecodeBin(BitStream &bs, int32_t bypassFlag, int32_t ctxIdx,
                int32_t &binVal); // 9.3.3.2
  int DecodeDecision(BitStream &bs, int32_t ctxIdx,
                     int32_t &binVal); // 9.3.3.2.1
  int RenormD(BitStream &bs, int32_t &codIRange,
              int32_t &codIOffset);                    // 9.3.3.2.2
  int DecodeBypass(BitStream &bs, int32_t &binVal);    // 9.3.3.2.3
  int DecodeTerminate(BitStream &bs, int32_t &binVal); // 9.3.3.2.4

  //-----------------------------------------
  int CABAC_decode_mb_type(PictureBase &picture, BitStream &bs,
                           int32_t &synElVal);
  int CABAC_decode_sub_mb_type(PictureBase &picture, BitStream &bs,
                               int32_t &synElVal);
  int CABAC_decode_mb_type_in_I_slices(PictureBase &picture, BitStream &bs,
                                       int32_t ctxIdxOffset, int32_t &synElVal);
  int CABAC_decode_mb_type_in_SI_slices(PictureBase &picture, BitStream &bs,
                                        int32_t &synElVal);
  int CABAC_decode_mb_type_in_P_SP_slices(PictureBase &picture, BitStream &bs,
                                          int32_t &synElVal);
  int CABAC_decode_mb_type_in_B_slices(PictureBase &picture, BitStream &bs,
                                       int32_t &synElVal);
  int CABAC_decode_sub_mb_type_in_P_SP_slices(PictureBase &picture,
                                              BitStream &bs, int32_t &synElVal);
  int CABAC_decode_sub_mb_type_in_B_slices(PictureBase &picture, BitStream &bs,
                                           int32_t &synElVal);

  int decode_mb_skip_flag(PictureBase &picture, BitStream &bs,
                          int32_t _CurrMbAddr, int32_t &synElVal);
  int CABAC_decode_mvd_lX(PictureBase &picture, BitStream &bs, int32_t mvd_flag,
                          int32_t mbPartIdx, int32_t subMbPartIdx,
                          int32_t isChroma, int32_t &synElVal);
  int CABAC_decode_ref_idx_lX(PictureBase &picture, BitStream &bs,
                              int32_t ref_idx_flag, int32_t mbPartIdx,
                              int32_t &synElVal);
  int CABAC_decode_mb_qp_delta(PictureBase &picture, BitStream &bs,
                               int32_t &synElVal);
  int CABAC_decode_intra_chroma_pred_mode(PictureBase &picture, BitStream &bs,
                                          int32_t &synElVal);
  int CABAC_decode_prev_intra4x4_pred_mode_flag_or_prev_intra8x8_pred_mode_flag(
      PictureBase &picture, BitStream &bs, int32_t &synElVal);
  int CABAC_decode_rem_intra4x4_pred_mode_or_rem_intra8x8_pred_mode(
      PictureBase &picture, BitStream &bs, int32_t &synElVal);
  int CABAC_decode_mb_field_decoding_flag(PictureBase &picture, BitStream &bs,
                                          int32_t &synElVal);
  int CABAC_decode_coded_block_pattern(PictureBase &picture, BitStream &bs,
                                       int32_t &synElVal);
  int CABAC_decode_coded_block_flag(PictureBase &picture, BitStream &bs,
                                    MB_RESIDUAL_LEVEL mb_block_level,
                                    int32_t BlkIdx, int32_t iCbCr,
                                    int32_t &synElVal);
  int CABAC_decode_significant_coeff_flag(PictureBase &picture, BitStream &bs,
                                          MB_RESIDUAL_LEVEL mb_block_level,
                                          int32_t levelListIdx,
                                          int32_t last_flag, int32_t &synElVal);
  int CABAC_decode_coeff_abs_level_minus1(PictureBase &picture, BitStream &bs,
                                          MB_RESIDUAL_LEVEL mb_block_level,
                                          int32_t numDecodAbsLevelEq1,
                                          int32_t numDecodAbsLevelGt1,
                                          int32_t &synElVal);
  int CABAC_decode_coeff_sign_flag(PictureBase &picture, BitStream &bs,
                                   int32_t &synElVal);
  int CABAC_decode_end_of_slice_flag(PictureBase &picture, BitStream &bs,
                                     int32_t &synElVal);
  int CABAC_decode_transform_size_8x8_flag(PictureBase &picture, BitStream &bs,
                                           int32_t &synElVal);

  //------------------------------------
  int residual_block_cabac(PictureBase &picture, BitStream &bs,
                           int32_t coeffLevel[], int32_t startIdx,
                           int32_t endIdx, int32_t maxNumCoeff,
                           MB_RESIDUAL_LEVEL mb_block_level, int32_t BlkIdx,
                           int32_t iCbCr, int32_t &TotalCoeff);
};

#endif /* end of include guard: H264CABAC_HPP_YF2ZLNUA */
