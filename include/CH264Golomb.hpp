#ifndef CH264GOLOMB_HPP_MIED40NI
#define CH264GOLOMB_HPP_MIED40NI

#include "BitStream.hpp"
#include "Type.hpp"
#include "Common.hpp"

#include <cstdint>
typedef enum {
  CODE_BLOCK_PATTERN_UNKNOWN,
  CODE_BLOCK_PATTERN_INTRA_4x4,
  CODE_BLOCK_PATTERN_INTRA_8x8,
  CODE_BLOCK_PATTERN_INTRA_INTER,
} CODE_BLOCK_PATTERN;

/*
 * T-REC-H.264-201704-S!!PDF-E.pdf
 * Page 208/230/812
 * 9.1 Parsing process for Exp-Golomb codes
 * 哥伦布编码
 */
class CH264Golomb {
 public:
 public:
  CH264Golomb();
  ~CH264Golomb();

  int get_ue_golomb(BitStream &bs);
  int get_se_golomb(BitStream &bs);
  int get_me_golomb(BitStream &bs, int32_t ChromaArrayType,
                    H264_MB_PART_PRED_MODE MbPartPredMode);
  int get_te_golomb(BitStream &bs, int range);
};

#endif /* end of include guard: CH264GOLOMB_HPP_MIED40NI */
