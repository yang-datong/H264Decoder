#include "./BitStream.hpp"
#include <cstdint>
#include <stdio.h>

BitStream::BitStream(uint8_t *buf, int size) {
  _size = size;
  _p = buf;
  _bitsLeft = 8;
}

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
  r = readUn(zero_count + 1); // read zero_count + 1 bits
  r = r + (1 << zero_count) - 1; /* 防止负溢出，如果r = 0,再去-1就会负溢出 */
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
