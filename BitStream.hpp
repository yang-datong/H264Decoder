#ifndef BITSTREAM_HPP_AUHM38NB
#define BITSTREAM_HPP_AUHM38NB

#include <stdint.h>

class BitStream {
 public:
  BitStream(uint8_t *buf, int size);
  ~BitStream();

  /* 读取1 bit */
  bool readU1();

  /* 读取n bit */
  uint32_t readUn(uint32_t num);
  // long readU(long num);

  /* 读取无符号指数哥伦布编码 */
  uint32_t readUE();

  /* 读取有符号指数哥伦布编码 */
  uint32_t readSE();

  /* 用于计算是否以及字节对齐，比如_bitsLeft % 8 == 0 */
  // int getBitsLeft() { return _bitsLeft; }
  bool endOfBit();

 private:
  // buffer length
  int _size = 0;
  // curent byte
  uint8_t *_p = nullptr;
  // curent byte in the bit
  int _bitsLeft = 8;
};

#endif /* end of include guard: BITSTREAM_HPP_AUHM38NB */
