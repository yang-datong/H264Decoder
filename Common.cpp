#include "Common.hpp"
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
