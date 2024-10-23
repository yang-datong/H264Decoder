#ifndef NALU_HPP_YDI8RPRP
#define NALU_HPP_YDI8RPRP

#include "Common.hpp"
#include "Frame.hpp"
#include "GOP.hpp"
#include "PPS.hpp"
#include "SEI.hpp"
#include "SPS.hpp"
#include "VPS.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

// 用于存放264中每一个单个Nalu数据
class Nalu {
 public:
  /* 拷贝构造函数 */
  // Nalu(const Nalu &nalu);
  ~Nalu();

  int startCodeLenth = 0;
  uint8_t *buffer = nullptr;
  int len = 0;
  int setBuffer(uint8_t *buf, int len);

 public:
  class EBSP {
   public:
    ~EBSP();
    uint8_t *buf = nullptr;
    int len = 0;
  };
  class RBSP {
   public:
    ~RBSP();
    uint8_t *buf = nullptr;
    int len = 0;
  };

 private:
  [[deprecated]]
  int parseNALHeader(EBSP &rbsp);
  int parseNALHeader(EBSP &rbsp, BitStream *bs);

 public:
  uint8_t forbidden_zero_bit = 0;
  /* Nal单元的重要性，越大则说明该Nal越重要（不可随意丢弃），取值为[0-3],不为0则表示为参考帧，而参考帧又分为短期参考帧和长期参考帧（这对于后面解码是非常重要的） */
  uint8_t nal_ref_idc = 0;
  uint8_t nal_unit_type = 0;
  uint8_t nuh_layer_id = 0;
  uint8_t nuh_temporal_id_plus1 = 0;
  uint8_t TemporalId = 0;

 public:
  int parseEBSP(EBSP &ebsp);
  int parseRBSP(EBSP &ebsp, RBSP &rbsp);

  /* Non-VCL */
  int extractVPSparameters(RBSP &rbsp, VPS vpss[MAX_SPS_COUNT],
                           uint32_t &curr_vps_id);
  int extractSPSparameters(RBSP &rbsp, SPS spss[MAX_SPS_COUNT],
                           uint32_t &curr_sps_id, VPS vpss[MAX_SPS_COUNT]);
  int extractPPSparameters(RBSP &rbsp, PPS ppss[MAX_PPS_COUNT],
                           uint32_t &curr_pps_id, uint32_t chroma_format_idc,
                           SPS spss[MAX_SPS_COUNT]);
  int extractSEIparameters(RBSP &rbsp, SEI &sei, SPS &sps);

  /* VCL */
  int extractSliceparameters(BitStream &bitStream, GOP &gop, Frame &frame);
  int extractIDRparameters(BitStream &bitStream, GOP &gop, Frame &frame);
};

#endif /* end of include guard: NALU_HPP_YDI8RPRP */
