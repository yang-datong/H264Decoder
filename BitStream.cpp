#include "./BitStream.hpp"
#include <cstdint>

/* TODO YangJing
 * 这里有问题，不应该是[index]的方式，这里一个index就是8bits，如果是比特流应该不是这样
 * <24-07-30 20:16:44> */
BitStream::BitStream(uint8_t *buf, int size)
    : _size(size), _p(buf), _endBuf(&buf[_size - 1]) {}

BitStream::~BitStream() {}

bool BitStream::readU1() {
  _bitsLeft--;
  bool b = (_p[0] >> _bitsLeft) & 1;
  /* 取最高位 */
  if (_bitsLeft == 0) {
    _p++;
    _bitsLeft = 8;
  }
  return b;
}

uint32_t BitStream::readUn(uint32_t num) {
  uint32_t n = 0;
  for (int i = 0; i < (int)num; i++) {
    n = (n << 1) | readU1();
  }
  return n;
}

uint32_t BitStream::readUE() {
  uint32_t r = 0;
  uint32_t zero_count = 0; // How many 0 bits
  while ((readU1() == 0) && zero_count < 32) {
    zero_count++;
  }
  r = readUn(zero_count);
  /* read zero_count + 1 bits，
   * 因为上面while循环中以及读取了一个非0字节，故这里不需要
   * 对zero_count + 1 */
  r += (1 << zero_count);
  r--;

  /* 上述的步骤可以考虑 0b00101001 (1 byte)
   * 1. 得到zero_count = 2
   * 2. 二进制数据：01
   * 3. 给第一位+1：01 + (1 << 2) = 01 + 100 = 101
   * 4. 给最低位-1：101 - 1 = 100 = 4
   */
  return r;
}

uint32_t BitStream::readSE() {
  int32_t r = readUE();
  r++;
  bool sign = r & 1; // fetch the min{endpos} bit
  r >>= 1;
  if (sign)
    r *= -1; // 去绝对值
  return r;
}

bool BitStream::endOfBit() { return _bitsLeft % 8 == 0; }

bool BitStream::byte_aligned() {
  /*
   * 1. If the current position in the bitstream is on a byte boundary, i.e.,
   * the next bit in the bitstream is the first bit in a byte, the return value
   * of byte_aligned( ) is equal to TRUE.
   * 2. Otherwise, the return value of byte_aligned( ) is equal to FALSE.
   */
  return endOfBit();
}

bool BitStream::isEndOf() { return ((*_p == *_endBuf) && _bitsLeft == 0); }
