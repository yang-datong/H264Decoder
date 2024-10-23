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

uint32_t BitStream::readME(int32_t ChromaArrayType,
                           H264_MB_PART_PRED_MODE MbPartPredMode) {
  int32_t coded_block_pattern = 0;
  int32_t codeNum = readUE();
  const maping_exp_golomb_t maping_exp_golomb_arrays1[] = {
      {0, 47, 0},   {1, 31, 16},  {2, 15, 1},   {3, 0, 2},    {4, 23, 4},
      {5, 27, 8},   {6, 29, 32},  {7, 30, 3},   {8, 7, 5},    {9, 11, 10},
      {10, 13, 12}, {11, 14, 15}, {12, 39, 47}, {13, 43, 7},  {14, 45, 11},
      {15, 46, 13}, {16, 16, 14}, {17, 3, 6},   {18, 5, 9},   {19, 10, 31},
      {20, 12, 35}, {21, 19, 37}, {22, 21, 42}, {23, 26, 44}, {24, 28, 33},
      {25, 35, 34}, {26, 37, 36}, {27, 42, 40}, {28, 44, 39}, {29, 1, 43},
      {30, 2, 45},  {31, 4, 46},  {32, 8, 17},  {33, 17, 18}, {34, 18, 20},
      {35, 20, 24}, {36, 24, 19}, {37, 6, 21},  {38, 9, 26},  {39, 22, 28},
      {40, 25, 23}, {41, 32, 27}, {42, 33, 29}, {43, 34, 30}, {44, 36, 22},
      {45, 40, 25}, {46, 38, 38}, {47, 41, 41},
  };

  const maping_exp_golomb_t maping_exp_golomb_arrays2[] = {
      {0, 15, 0},  {1, 0, 1},   {2, 7, 2},  {3, 11, 4},
      {4, 13, 8},  {5, 14, 3},  {6, 3, 5},  {7, 5, 10},
      {8, 10, 12}, {9, 12, 15}, {10, 1, 7}, {11, 2, 11},
      {12, 4, 13}, {13, 8, 14}, {14, 6, 6}, {15, 9, 9},
  };

  if (ChromaArrayType == 1 || ChromaArrayType == 2) {
    RET(codeNum < 0 || codeNum > 47);
    if (MbPartPredMode == Intra_4x4 || MbPartPredMode == Intra_8x8)
      coded_block_pattern = maping_exp_golomb_arrays1[codeNum]
                                .coded_block_pattern_of_Intra_4x4_or_Intra_8x8;
    else
      coded_block_pattern =
          maping_exp_golomb_arrays1[codeNum].coded_block_pattern_of_Inter;
  } else { //0,3
    RET(codeNum < 0 || codeNum > 15);
    if (MbPartPredMode == Intra_4x4 || MbPartPredMode == Intra_8x8)
      coded_block_pattern = maping_exp_golomb_arrays2[codeNum]
                                .coded_block_pattern_of_Intra_4x4_or_Intra_8x8;
    else
      coded_block_pattern =
          maping_exp_golomb_arrays2[codeNum].coded_block_pattern_of_Inter;
  }

  return coded_block_pattern;
}

uint32_t BitStream::readTE(int32_t r) {
  int32_t codeNum = 0;
  if (r <= 0)
    return 0;
  else if (r == 1) {
    int b = readU1();
    codeNum = !b;
  } else
    codeNum = readUE();
  return codeNum;
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
  // 从后往前找，直到找到第一个非0值字节位置为止
  while (p1 > getP() && *p1 == 0)
    p1--;

  if (p1 > getP())
    return 1; // 说明当前位置m_p后面还有码流数据
  else {
    // 在单个字节的8个比特位中，从后往前找，找到rbsp_stop_one_bit位置
    int flag = 0, i = 0;
    for (i = 0; i < 8; i++)
      if ((((*(getP())) >> i) & 0x01) == 1) {
        flag = 1;
        break;
      }

    if (flag == 1 && (i + 1) < getBitsLeft()) return 1;
  }

  return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
/* rbsp_trailing_bits( ) 语法结构出现在 SODB 之后，如下所示：
 * 1. 最终 RBSP 字节的第一个（最高有效、最左边）位包含 SODB 的其余位（如果有）。
 * 2. 下一位由等于 1 的单个位组成（即 rbsp_stop_one_bit）。
 * 3. 当 rbsp_stop_one_bit 不是字节对齐字节的最后一位时，出现一个或多个零值位（即 rbsp_alignment_zero_bit 的实例）以导致字节对齐。*/
//page -> 67
int BitStream::rbsp_trailing_bits() {
  if (getP() >= getEndBuf()) return 0;
  int32_t rbsp_stop_one_bit = readU1(); // /* equal to 1 */ All f(1)
  while (!byte_aligned())
    int32_t rbsp_alignment_zero_bit = readU1();
  return 0;
}
#pragma GCC diagnostic pop

int BitStream::byte_alignment() {
  int alignment_bit_equal_to_one = readU1();
  while (!byte_aligned())
    readU1();
  return 0;
}
