#ifndef NALU_HPP_YDI8RPRP
#define NALU_HPP_YDI8RPRP

#include "BitStream.hpp"
#include "Cabac.hpp"
#include "Common.hpp"
#include "EBSP.hpp"
#include "PictureBase.hpp"
#include "RBSP.hpp"
#include "Type.hpp"
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

  char forbidden_zero_bit = 0;
  char nal_ref_idc = 0;
  char nal_unit_type = 0;

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
  uint32_t chroma_format_idc = 0;
  bool separate_colour_plane_flag = 0;
  uint32_t bit_depth_luma_minus8 = 0;
  uint32_t bit_depth_chroma_minus8 = 0;
  bool frame_mbs_only_flag = 0;
  uint32_t pic_order_cnt_type = 0;
  bool delta_pic_order_always_zero_flag = 0;
  uint32_t ChromaArrayType = 0;
  bool mb_adaptive_frame_field_flag = 0;
  uint32_t PicWidthInMbs = 0;
  uint32_t PicHeightInMapUnits = 0;
  uint32_t PicSizeInMapUnits = 0;
  uint32_t frameHeightInMbs = 0;
  bool qpprime_y_zero_transform_bypass_flag = 0;
  bool seq_scaling_matrix_present_flag = 0;
  bool seq_scaling_list_present_flag[12] = {false};
  uint32_t BitDepthY = 0;
  uint32_t QpBitDepthY = 0;
  uint32_t BitDepthUV = 0;
  uint32_t QpBitDepthUV = 0;
  uint32_t MbWidthC = 0;          //色度宏块宽度
  uint32_t MbHeightC = 0;         //色度宏块高度
  int32_t pcm_sample_chroma[256]; // 3 u(v)
  uint32_t log2_max_pic_order_cnt_lsb_minus4 = 0;
  int32_t offset_for_non_ref_pic = 0;
  int32_t offset_for_top_to_bottom_field = 0;
  uint32_t num_ref_frames_in_pic_order_cnt_cycle = 0;
  int Chroma_Format = 0;
  int SubWidthC = 0;
  int SubHeightC = 0;
  uint32_t pic_parametter_set_id = 0;
  uint8_t colour_plane_id = 0;
  uint32_t frame_num = 0; // u(v)
  bool bottom_field_flag = 0;
  uint32_t idr_pic_id = 0;
  uint32_t pic_order_cnt_lsb = 0;
  int32_t delta_pic_order_cnt_bottom = 0;
  int32_t delta_pic_order_cnt[2] = {0};
  uint32_t redundant_pic_cnt = 0;
  bool direct_spatial_mv_pred_flag = 0;
  bool num_ref_idx_active_override_flag = 0;
  uint32_t cabac_init_idc = 0;
  int32_t slice_qp_delta = 0;
  bool sp_for_switch_flag = 0;
  uint32_t disable_deblocking_filter_idc = 0;

  int SliceQPY = 0;
  int QPY_prev = 0;
  int PicHeightInSamplesL = 0;
  int PicHeightInSamplesC = 0;
  int MaxPicNum = 0;
  int CurrPicNum = 0;
  int QSY = 0;
  int FilterOffsetA = 0;
  int FilterOffsetB = 0;

  /* PPS 参数 */
  bool more_rbsp_data();
  void rbsp_trailing_bits();
  bool bottom_field_pic_order_in_frame_present_flag = 0;
  bool redundant_pic_cnt_present_flag = 0;
  bool weighted_pred_flag = 0;
  uint32_t weighted_bipred_idc = 0;
  bool entropy_coding_mode_flag = 0;
  bool deblocking_filter_control_present_flag = 0;
  uint32_t num_slice_groups_minus1 = 0;
  uint32_t slice_group_map_type = 0;
  uint32_t *run_length_minus1 = 0;
  uint32_t *top_left = 0;
  uint32_t *bottom_right = 0;
  bool slice_group_change_direction_flag = 0;
  uint32_t slice_group_change_rate_minus1 = 0;
  uint32_t *slice_group_id = 0;
  bool pic_scaling_matrix_present_flag = 0;

  uint32_t ScalingList4x4[6][16];
  uint32_t ScalingList8x8[6][64];
  uint32_t UseDefaultScalingMatrix4x4Flag[6] = {0};
  uint32_t UseDefaultScalingMatrix8x8Flag[6] = {0};

  uint32_t *pic_scaling_list_present_flag = 0;
  int32_t pic_init_qp_minus26 = 0;
  int32_t pic_init_qs_minus26 = 0;

  uint32_t pic_parameter_set_id = 0;
  uint32_t seq_parameter_set_id = 0;
  uint32_t pic_size_in_map_units_minus1 = 0;
  uint32_t num_ref_idx_l0_default_active_minus1 = 0;
  uint32_t num_ref_idx_l1_default_active_minus1 = 0;
  int32_t chroma_qp_index_offset = 0;
  bool constrained_intra_pred_flag = 0;
  bool transform_8x8_mode_flag = 0;
  uint32_t maxPICScalingList = 0;
  int32_t second_chroma_qp_index_offset = 0;

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
  int m_is_malloc_mem_self = 0;
  uint32_t MaxFrameNum = 0;
  uint32_t maxPicOrderCntLsb = 0;
  bool field_pic_flag = 0;
  uint32_t slice_type = 0;
  uint32_t num_ref_idx_l0_active_minus1 = 0;
  uint32_t num_ref_idx_l1_active_minus1 = 0;
  bool IdrPicFlag = 0;
  uint32_t first_mb_in_slice = 0;
  bool MbaffFrameFlag = 0;
  int32_t *mapUnitToSliceGroupMap = nullptr;
  int32_t *MbToSliceGroupMap = nullptr;
  uint32_t slice_group_change_cycle = 0;
  int MapUnitsInSliceGroup0 = 0;

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
  int32_t slice_qs_delta = 0;
  int32_t slice_alpha_c0_offset_div2 = 0;
  int32_t slice_beta_offset_div2 = 0;

  /* IDR */
  int NextMbAddress(int n);
  int32_t PicHeightInMbs = 0;
  int32_t PicSizeInMbs = 0;
  int macroblock_layer(BitStream &bs);
};

#endif /* end of include guard: NALU_HPP_YDI8RPRP */
