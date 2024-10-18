
#ifndef __H264_RESIDUAL_BLOCK_CAVLC_H__
#define __H264_RESIDUAL_BLOCK_CAVLC_H__

/*
 * T-REC-H.264-201704-S!!PDF-E.pdf
 * Page 60/82/812
 * 7.3.5.3.2 Residual block CAVLC syntax
 */
#include "BitStream.hpp"
#include "PictureBase.hpp"
#include <cstdint>
class Cavlc {
 public:
  int32_t coeff_token;
  int32_t trailing_ones_sign_flag;
  int32_t levelVal[16];
  int32_t level_prefix;
  int32_t level_suffix;
  int32_t total_zeros;
  int32_t run_before;
  int32_t runVal[16];

 private:
  PictureBase *_picture = nullptr;
  BitStream *_bs = nullptr;

 public:
  Cavlc(PictureBase *picture, BitStream *bs) : _picture(picture), _bs(bs){};

  int residual_block_cavlc(int32_t *coeffLevel, int32_t startIdx,
                           int32_t endIdx, int32_t maxNumCoeff,
                           MB_RESIDUAL_LEVEL mb_residual_level,
                           int32_t MbPartPredMode, int32_t BlkIdx,
                           int32_t &TotalCoeff);
  int process_nC(MB_RESIDUAL_LEVEL mb_residual_level, int32_t MbPartPredMode,
                 int32_t BlkIdx, int32_t &nC);
  int getMbAddrN(int32_t xN, int32_t yN, int32_t maxW, int32_t maxH,
                 MB_ADDR_TYPE &mbAddrN);
  int process_coeff_token(MB_RESIDUAL_LEVEL mb_residual_level,
                          int32_t MbPartPredMode, int32_t BlkIdx,
                          uint16_t coeff_token, int32_t &coeff_token_bit_length,
                          int32_t &TrailingOnes, int32_t &TotalCoeff);
  int process_total_zeros(int32_t maxNumCoeff, int32_t tzVlcIndex,
                          int32_t &total_zeros);
  int process_run_before(int32_t zerosLeft, int32_t &run_before);
};

#endif
