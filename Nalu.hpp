#ifndef NALU_HPP_YDI8RPRP
#define NALU_HPP_YDI8RPRP

#include "BitStream.hpp"
#include "EBSP.hpp"
#include "RBSP.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

enum H264_SLICE_TYPE { SLICE_P = 0, SLICE_B, SLICE_I, SLICE_SP, SLICE_SI };

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

  /* 开始解码图像 */
  int decode(RBSP &rbsp);

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
  bool frame_mbs_only_flag;
  uint32_t pic_order_cnt_type;
  bool delta_pic_order_always_zero_flag;
  uint32_t ChromaArrayType;
  bool mb_adaptive_frame_field_flag;

  /* PPS 参数 */
  bool more_rbsp_data();
  void rbsp_trailing_bits();
  bool bottom_field_pic_order_in_frame_present_flag;
  bool redundant_pic_cnt_present_flag;
  bool weighted_pred_flag;
  uint32_t weighted_bipred_idc;
  bool entropy_coding_mode_flag;
  bool deblocking_filter_control_present_flag;
  uint32_t num_slice_groups_minus1;
  uint32_t slice_group_map_type;

  /* SEI */
  void sei_message(BitStream &bitStream);
  void sei_payload(BitStream &bitStream, long payloadType, long payloadSize);
  bool byte_aligned(BitStream &bitStream);

  /* Slice */
  int parseSliceHeader(BitStream &bitStream, RBSP &rbsp);
  int parseSliceData(BitStream &bitStream, RBSP &rbsp);
  void ref_pic_list_mvc_modification(BitStream &bitStream);
  void ref_pic_list_modification(BitStream &bitStream);
  void pred_weight_table(BitStream &bitStream);
  void dec_ref_pic_marking(BitStream &bitStream);
  uint32_t maxFrameNum;
  bool field_pic_flag;
  uint32_t slice_type;
  uint32_t num_ref_idx_l0_active_minus1;
  uint32_t num_ref_idx_l1_active_minus1;
  bool IdrPicFlag;
  uint32_t first_mb_in_slice;
  bool MbaffFrameFlag;

  /* IDR */
};

#endif /* end of include guard: NALU_HPP_YDI8RPRP */
