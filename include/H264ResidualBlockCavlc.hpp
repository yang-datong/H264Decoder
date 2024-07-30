
#ifndef __H264_RESIDUAL_BLOCK_CAVLC_H__
#define __H264_RESIDUAL_BLOCK_CAVLC_H__

/*
 * T-REC-H.264-201704-S!!PDF-E.pdf
 * Page 60/82/812
 * 7.3.5.3.2 Residual block CAVLC syntax
 */
#include "PictureBase.hpp"
#include <cstdint>
class CH264ResidualBlockCavlc {
 public:
  int32_t coeff_token;             // 3 | 4 ce(v)
  int32_t trailing_ones_sign_flag; // 3 | 4 u(1)
  int32_t levelVal[16];
  int32_t level_prefix; // 3 | 4 ce(v)
  int32_t level_suffix; // 3 | 4 u(v)
  int32_t total_zeros;  // 3 | 4 ce(v)
  int32_t run_before;   // 3 | 4 ce(v)
  int32_t runVal[16];

 public:
  CH264ResidualBlockCavlc();
  ~CH264ResidualBlockCavlc();

  int printInfo();

  int residual_block_cavlc(PictureBase &picture, BitStream &bs,
                           int32_t *coeffLevel, int32_t startIdx,
                           int32_t endIdx, int32_t maxNumCoeff,
                           MB_RESIDUAL_LEVEL mb_residual_level,
                           int32_t MbPartPredMode, int32_t BlkIdx,
                           int32_t &TotalCoeff);
  int get_nC(PictureBase &picture, MB_RESIDUAL_LEVEL mb_residual_level,
             int32_t MbPartPredMode, int32_t BlkIdx, int32_t &nC);
  int getMbAddrN(int32_t xN, int32_t yN, int32_t maxW, int32_t maxH,
                 MB_ADDR_TYPE &mbAddrN);
  int coeff_token_table(int32_t nC, uint16_t coeff_token,
                        int32_t &coeff_token_bit_length, int32_t &TrailingOnes,
                        int32_t &TotalCoeff);
  int get_total_zeros(BitStream &bs, int32_t maxNumCoeff, int32_t tzVlcIndex,
                      int32_t &total_zeros);
  int get_run_before(BitStream &bs, int32_t zerosLeft, int32_t &run_before);
};

#endif