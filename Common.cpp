#include "Common.hpp"

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
