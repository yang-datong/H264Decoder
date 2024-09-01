#include "BitStream.hpp"
#include <cstdint>

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
  for (int i = 0; i < (int)num; i++)
    n = (n << 1) | readU1();
  return n;
}

uint32_t BitStream::getUn(uint32_t num) {
  uint32_t bitsLeft = _bitsLeft;
  int size = _size;
  uint8_t *p = _p;
  uint8_t *endBuf = _endBuf;

  uint32_t n = 0;
  for (int i = 0; i < (int)num; i++)
    n = (n << 1) | readU1();

  _bitsLeft = bitsLeft;
  _size = size;
  _p = p;
  _endBuf = endBuf;
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
  if (sign) r *= -1; // 去绝对值
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

bool BitStream::more_rbsp_data() {
  if (isEndOf()) return 0;

  uint8_t *p1 = getEndBuf();
  while (p1 > getP() && *p1 == 0) {
    // 从后往前找，直到找到第一个非0值字节位置为止
    p1--;
  }

  if (p1 > getP())
    return 1; // 说明当前位置m_p后面还有码流数据
  else {
    int flag = 0, i = 0;
    // 在单个字节的8个比特位中，从后往前找，找到rbsp_stop_one_bit位置
    for (i = 0; i < 8; i++) {
      if ((((*(getP())) >> i) & 0x01) == 1) {
        flag = 1;
        break;
      }
    }

    if (flag == 1 && (i + 1) < getBitsLeft()) return 1;
  }

  return 0;
}

int BitStream::rbsp_trailing_bits() {
  if (getP() >= getEndBuf()) return 0;
  /*int32_t rbsp_stop_one_bit =*/readU1(); // /* equal to 1 */ All f(1)
  while (!byte_aligned())
    /*int32_t rbsp_alignment_zero_bit =*/readU1();
  return 0;
}
