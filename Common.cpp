#include "Common.hpp"
#include "BitStream.hpp"
#include <assert.h>

int h264_log2(int32_t value) {
  assert(value > 0);
  int log2 = 0;
  while (value) {
    value >>= 1;
    log2++;
  }
  return log2;
}

int32_t h264_power2(int32_t value) {
  int32_t power2 = 1;
  for (int32_t i = 0; i < value; ++i)
    power2 *= 2;
  return power2;
}

/*
 * T-REC-H.264-201704-S!!PDF-E.pdf
 * Page 44/66/812
 * 7.3.2.1.1.1 Scaling list syntax
 */
void scaling_list(BitStream &bs, uint32_t *scalingList,
                  uint32_t sizeOfScalingList,
                  uint32_t &useDefaultScalingMatrixFlag) {
  int32_t lastScale = 8, nextScale = 8;

  for (int j = 0; j < (int)sizeOfScalingList; j++) {
    if (nextScale != 0) {
      int delta_scale = bs.readSE();
      nextScale = (lastScale + delta_scale + 256) % 256;
      useDefaultScalingMatrixFlag = (j == 0 && nextScale == 0);
    }
    scalingList[j] = (nextScale == 0) ? lastScale : nextScale;
    lastScale = scalingList[j];
  }
}
