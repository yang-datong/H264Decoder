#ifndef SPS_CPP_F6QSULFM
#define SPS_CPP_F6QSULFM

#include "Common.hpp"
#include "RBSP.hpp"

#define H264_MAX_OFFSET_REF_FRAME_COUNT                                        \
  256 // 7.4.2.1.1: num_ref_frames_in_pic_order_cnt_cycle shall be in the range
      // of 0 to 255, inclusive.
class SPS : public RBSP {
 public:
  SPS();
  ~SPS();
  uint8_t *_buf = nullptr;
  int _len = 0;

 public:
  uint32_t MaxFrameNum = 0;
  uint32_t maxPicOrderCntLsb = 0;
  bool direct_8x8_inference_flag = 0;
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
  uint32_t QpBdOffsetY = 0;
  uint32_t BitDepthC = 0;
  uint32_t QpBdOffsetC = 0;
  uint32_t MbWidthC = 0;          // 色度宏块宽度
  uint32_t MbHeightC = 0;         // 色度宏块高度
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

  int32_t MaxPicOrderCntLsb; // MaxPicOrderCntLsb = 2 ^ (
  int32_t ExpectedDeltaPerPicOrderCntCycle;

  int32_t offset_for_ref_frame
      [H264_MAX_OFFSET_REF_FRAME_COUNT]; // offset_for_ref_frame[
                                         // num_ref_frames_in_pic_order_cnt_cycle
                                         // ] 0 se(v)
  uint32_t max_num_ref_frames = 0;
  bool gaps_in_frame_num_value_allowed_flag = 0;
  uint32_t pic_width_in_mbs_minus1 = 0;
  uint32_t pic_height_in_map_units_minus1 = 0;

  bool frame_cropping_flag = 0;
  uint32_t frame_crop_left_offset = 0;
  uint32_t frame_crop_right_offset = 0;
  uint32_t frame_crop_top_offset = 0;
  uint32_t frame_crop_bottom_offset = 0;
  bool vui_parameters_present_flag = 0;

  uint32_t ScalingList4x4[6][16];
  uint32_t ScalingList8x8[6][64];

  uint32_t UseDefaultScalingMatrix4x4Flag[6];
  uint32_t UseDefaultScalingMatrix8x8Flag[6];
};

#endif /* end of include guard: SPS_CPP_F6QSULFM */
