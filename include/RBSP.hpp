#ifndef RBSP_HPP_XCS546BD
#define RBSP_HPP_XCS546BD

#include <iostream>

class RBSP {
 public:
  RBSP();
  ~RBSP();
  uint8_t *_buf = nullptr;
  int _len = 0;
};
#endif /* end of include guard: RBSP_HPP_XCS546BD */
