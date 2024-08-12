#ifndef EBSP_HPP_6XPPLZUK
#define EBSP_HPP_6XPPLZUK

#include <cstdint>
class EBSP {
 public:
  EBSP();
  ~EBSP();
  uint8_t *_buf = nullptr;
  int _len = 0;
};
#endif /* end of include guard: EBSP_HPP_6XPPLZUK */
