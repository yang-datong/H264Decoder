#ifndef NALU_HPP_YDI8RPRP
#define NALU_HPP_YDI8RPRP

#include "BitStream.hpp"
#include "EBSP.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// 用于存放264中每一个单个Nalu数据
class Nalu {
 public:
  Nalu();
  /* 拷贝构造函数 */
  Nalu(const Nalu &nalu);
  ~Nalu();

  int _startCodeLenth = 0;
  uint8_t *_buffer = nullptr;
  int _len = 0;

  int setBuffer(uint8_t *buf, int len);
  // 用于给外界传输buf进来

 public:
  int parseEBSP(EBSP &ebsp);
  int parseRBSP(EBSP &ebsp, RBSP &rbsp);

  char forbidden_zero_bit;
  char nal_ref_idc;
  char nal_unit_type;

  int extractSPSparameters(RBSP &sps);
  int extractPPSparameters(RBSP &pps);
  int extractSEIparameters(RBSP &sei);
  int extractSliceparameters(RBSP &rbsp);
  int extractIDRparameters(RBSP &idr);

  int GetNaluType();

 private:
  int parseNALHeader(EBSP &rbsp);
  void scaling_list(BitStream &bitStream, uint32_t *scalingList,
                    uint32_t sizeOfScalingList,
                    uint32_t &useDefaultScalingMatrixFlag);

  void vui_parameters(BitStream &bitStream);

  void hrd_parameters(BitStream &bitStream);

  /* SPS 参数 */
  uint32_t chroma_format_idc;
  bool separate_colour_plane_flag;
  uint32_t bit_depth_luma_minus8;
  uint32_t bit_depth_chroma_minus8;

  /* PPS 参数 */
  bool more_rbsp_data();
  void rbsp_trailing_bits();

  /* SEI */
  void sei_message(BitStream &bitStream);
  void sei_payload(BitStream &bitStream, long payloadType, long payloadSize);
  bool byte_aligned(BitStream &bitStream);

  /* Slice */
  int parseSliceHeader(BitStream bitStream, RBSP &rbsp);

  /* IDR */
};

#endif /* end of include guard: NALU_HPP_YDI8RPRP */
