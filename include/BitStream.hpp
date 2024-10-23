#ifndef BITSTREAM_HPP_AUHM38NB
#define BITSTREAM_HPP_AUHM38NB

#include "Common.hpp"
#include <cstdint>
#include <math.h>
#include <stdint.h>

#define ARCH_32_BIT_COUNT 4
#define ARCH_64_BIT_COUNT 8

class BitStream {
 public:
  BitStream(uint8_t *buf, int size)
      : _size(size), _p(buf), _endBuf(&buf[_size - 1]) {}

  /* 读取1 bit */
  bool readU1();

  /* 读取n bit(消耗bitStream) */
  uint32_t readUn(uint32_t num);
  /* 获取n bit(不消耗bitStream) */
  uint32_t getUn(uint32_t num);

  /* 读取无符号指数哥伦布编码 */
  uint32_t readUE();

  /* 读取有符号指数哥伦布编码 */
  uint32_t readSE();

  uint32_t readME(int32_t ChromaArrayType,
                  H264_MB_PART_PRED_MODE MbPartPredMode);

  uint32_t readTE(int32_t r);

  bool endOfBit();

  bool byte_aligned();

  bool isEndOf();

 private:
  // buffer length
  int _size = 0;
  // curent byte
  uint8_t *_p = nullptr;

  uint8_t *_endBuf = 0;
  // curent byte in the bit
  int _bitsLeft = ARCH_64_BIT_COUNT;

 public:
  uint8_t *getP() { return _p; }
  uint8_t *getEndBuf() { return _endBuf; }
  int getBitsLeft() { return _bitsLeft; }
  bool more_rbsp_data();
  int rbsp_trailing_bits();
  int byte_alignment();
};

struct maping_exp_golomb_t {
  int32_t code_num;
  int32_t coded_block_pattern_of_Intra_4x4_or_Intra_8x8;
  int32_t coded_block_pattern_of_Inter;
};

#endif /* end of include guard: BITSTREAM_HPP_AUHM38NB */
