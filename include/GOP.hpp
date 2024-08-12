#ifndef GOP_HPP_PUYEPJDM
#define GOP_HPP_PUYEPJDM

#include "Nalu.hpp"
#define GOP_SIZE 16

class GOP {
 public:
  GOP();
  ~GOP();
  /* TODO YangJing 这里一个Nalu就是一个Slice是有问题的，后续改 <24-08-12
   * 11:19:49> */
  Nalu *nalu;

  int gop_fill_size = 0;
};

#endif /* end of include guard: GOP_HPP_PUYEPJDM */
