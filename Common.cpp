#include "Common.hpp"
#include "CH264Golomb.hpp"
#include <assert.h>

int InverseRasterScan(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e) {
  int ret = 0;

  if (e == 0) {
    ret = (a % (d / b)) * b;
  } else // if (e == 1)
  {
    ret = (a / (d / b)) * c;
  }
  return ret;
}

int h264_log2(int32_t value) {
  assert(value > 0);
  int log2 = 0;
  while (value) {
    value >>= 1;
    log2++;
  }
  return log2;
}

void *my_malloc(size_t size) {
  void *ptr = malloc(size);
  // printf("%s: size=%ld; ptr=%p;\n", __FUNCTION__, size, ptr);
  return ptr;
}

void my_free(void *ptr) {
  // printf("%s: ptr=%p;\n", __FUNCTION__, ptr);
  return free(ptr);
}

int32_t h264_power2(int32_t value) {
  int32_t power2 = 1;
  for (int32_t i = 0; i < value; ++i) {
    power2 *= 2;
  }
  return power2;
  //    return 1 << value;
}

/*
 * T-REC-H.264-201704-S!!PDF-E.pdf
 * Page 44/66/812
 * 7.3.2.1.1.1 Scaling list syntax
 */
void scaling_list(BitStream &bs, uint32_t *scalingList,
                  uint32_t sizeOfScalingList,
                  uint32_t &useDefaultScalingMatrixFlag) {
  int32_t lastScale = 8;
  int32_t nextScale = 8;
  CH264Golomb gb;

  for (int j = 0; j < sizeOfScalingList; j++) {
    if (nextScale != 0) {
      int delta_scale = gb.get_se_golomb(bs); // delta_scale 0 | 1 se(v)
      nextScale = (lastScale + delta_scale + 256) % 256;
      useDefaultScalingMatrixFlag = (j == 0 && nextScale == 0);
    }
    // FIXE: What meaning 'When useDefaultScalingMatrixFlag is derived to be
    // equal to 1, the scaling list shall be inferred to be equal to the default
    // scaling list as specified in Table 7-2.'
    scalingList[j] = (nextScale == 0) ? lastScale : nextScale;
    lastScale = scalingList[j];
  }
}
