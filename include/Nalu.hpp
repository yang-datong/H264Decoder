#ifndef NALU_HPP_YDI8RPRP
#define NALU_HPP_YDI8RPRP

#include "Common.hpp"
#include "EBSP.hpp"
#include "Frame.hpp"
#include "GOP.hpp"
#include "PPS.hpp"
#include "RBSP.hpp"
#include "SEI.hpp"
#include "SPS.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

// 用于存放264中每一个单个Nalu数据
class Nalu {
 public:
  Nalu();
  /* 拷贝构造函数 */
  // Nalu(const Nalu &nalu);
  ~Nalu();

  int _startCodeLenth = 0;
  uint8_t *_buffer = nullptr;
  int _len = 0;

  int setBuffer(uint8_t *buf, int len);
  // 用于给外界传输buf进来

 public:
  int parseEBSP(EBSP &ebsp);
  int parseRBSP(EBSP &ebsp, RBSP &rbsp);

  char forbidden_zero_bit = 0;
  char nal_ref_idc = 0;
  char nal_unit_type = 0;

  int extractSPSparameters(RBSP &rbsp, SPS &sps);
  int extractPPSparameters(RBSP &rbsp, PPS &pps, uint32_t chroma_format_idc);
  int extractSEIparameters(RBSP &rbsp, SEI &sei);
  int extractSliceparameters(BitStream &bitStream, GOP &gop, Frame &frame);
  int extractIDRparameters(BitStream &bitStream, GOP &gop, Frame &frame);

  int GetNaluType();

 private:
  int parseNALHeader(EBSP &rbsp);
};

#endif /* end of include guard: NALU_HPP_YDI8RPRP */
