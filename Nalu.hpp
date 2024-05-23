#ifndef NALU_HPP_YDI8RPRP
#define NALU_HPP_YDI8RPRP

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#include "BitStream.hpp"
#include "Cabac.hpp"
#include "EBSP.hpp"
#include "PictureBase.hpp"
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
  uint32_t PicWidthInMbs;
  uint32_t PicHeightInMapUnits;
  uint32_t PicSizeInMapUnits;
  uint32_t frameHeightInMbs;
  bool qpprime_y_zero_transform_bypass_flag;
  bool seq_scaling_matrix_present_flag;
  bool seq_scaling_list_present_flag[12] = {false};
  uint32_t bitDepthY;
  uint32_t qpBitDepthY;
  uint32_t bitDepthUV;
  uint32_t qpBitDepthUV;

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
  uint32_t *run_length_minus1;
  uint32_t *top_left;
  uint32_t *bottom_right;
  bool slice_group_change_direction_flag;
  uint32_t slice_group_change_rate_minus1;
  uint32_t *slice_group_id;
  bool pic_scaling_matrix_present_flag;

  uint32_t ScalingList4x4[6][16];
  uint32_t ScalingList8x8[6][64];
  uint32_t UseDefaultScalingMatrix4x4Flag[6];
  uint32_t UseDefaultScalingMatrix8x8Flag[6];

  uint32_t *pic_scaling_list_present_flag;

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
  int32_t *mapUnitToSliceGroupMap;
  int32_t *MbToSliceGroupMap;
  uint32_t slice_group_change_cycle;
  int MapUnitsInSliceGroup0;

  int setMapUnitToSliceGroupMap();
  int setMbToSliceGroupMap();
  int set_scaling_lists_values();
  int set_mb_skip_flag(int32_t &mb_skip_flag, PictureBase &picture,
                       BitStream &bitStream);

  uint32_t cabac_alignment_one_bit = 0;
  uint32_t mb_skip_run = 0;
  int32_t mb_skip_flag = 0;
  int32_t end_of_slice_flag = 0;
  uint32_t mb_field_decoding_flag = 0;
  uint32_t slice_id = 0;
  uint32_t slice_number = -1;
  uint32_t CurrMbAddr = 0;
  uint32_t syntax_element_categories = 0;
  bool moreDataFlag = 1;
  uint32_t prevMbSkipped = 0;
  int32_t mb_skip_flag_next_mb = 0;

  /* IDR */
  int NextMbAddress(int n);
  int32_t PicHeightInMbs;
  int32_t PicSizeInMbs;
  int macroblock_layer(BitStream &bs);

  PictureBase picture;
  Cabac cabac;
};

#endif /* end of include guard: NALU_HPP_YDI8RPRP */
