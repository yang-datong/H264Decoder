#ifndef BITSTREAM_HPP_AUHM38NB
#define BITSTREAM_HPP_AUHM38NB

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
};

#endif /* end of include guard: BITSTREAM_HPP_AUHM38NB */
