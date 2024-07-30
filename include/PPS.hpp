#ifndef PPS_CPP_F6QSULFM
#define PPS_CPP_F6QSULFM

#include "RBSP.hpp"
#include "SPS.hpp"

class PPS : public RBSP {
 public:
  // PPS();
  //~PPS();
  uint8_t *_buf = nullptr;
  int _len = 0;
  SPS sps;

 public:
  int extractParameters();
  bool more_rbsp_data(BitStream &bs);
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

  uint32_t ScalingList4x4[6][16] = {{0}};
  uint32_t ScalingList8x8[6][64] = {{0}};
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
};

#endif /* end of include guard: PPS_CPP_F6QSULFM */
