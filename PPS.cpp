#include "PPS.hpp"
#include "BitStream.hpp"
#include <iostream>
#include <ostream>

int PPS::extractParameters() {
  /* 初始化bit处理器，填充pps的数据 */
  BitStream bitStream(_buf, _len);

  pic_parameter_set_id = bitStream.readUE();
  std::cout << "\tpic_parameter_set_id:" << pic_parameter_set_id << std::endl;
  seq_parameter_set_id = bitStream.readUE();
  std::cout << "\tseq_parameter_set_id:" << seq_parameter_set_id << std::endl;

  entropy_coding_mode_flag = bitStream.readU1();
  if (entropy_coding_mode_flag == 0)
    std::cout << "\tentropy_coding_mode_flag:CAVLC" << std::endl;
  else if (entropy_coding_mode_flag == 1)
    std::cout << "\tentropy_coding_mode_flag:CABAC" << std::endl;
  else
    std::cout << "\tentropy_coding_mode_flag:?????" << std::endl;

  bottom_field_pic_order_in_frame_present_flag = bitStream.readU1();
  num_slice_groups_minus1 = bitStream.readUE();
  if (num_slice_groups_minus1 > 0) {
    slice_group_map_type = bitStream.readUE();
    std::cout << "\tslice_group_map_type:" << slice_group_map_type << std::endl;
    if (slice_group_map_type == 0) {
      run_length_minus1 = new uint32_t[num_slice_groups_minus1 + 1];
      for (int iGroup = 0; iGroup <= num_slice_groups_minus1; iGroup++)
        run_length_minus1[iGroup] = bitStream.readUE();
    } else if (slice_group_map_type == 2) {
      top_left = new uint32_t[num_slice_groups_minus1];
      bottom_right = new uint32_t[num_slice_groups_minus1];
      for (int iGroup = 0; iGroup < num_slice_groups_minus1; iGroup++) {
        top_left[iGroup] = bitStream.readUE();
        bottom_right[iGroup] = bitStream.readUE();
      }
    } else if (slice_group_map_type == 3 || slice_group_map_type == 4 ||
               slice_group_map_type == 5) {
      slice_group_change_direction_flag = bitStream.readU1();
      slice_group_change_rate_minus1 = bitStream.readUE();
    } else if (slice_group_map_type == 6) {
      pic_size_in_map_units_minus1 = bitStream.readUE();
      std::cout << "\tpic_size_in_map_units_minus1:"
                << pic_size_in_map_units_minus1 << std::endl;
      slice_group_id = new uint32_t[pic_size_in_map_units_minus1 + 1];
      for (int i = 0; i <= pic_size_in_map_units_minus1; i++)
        slice_group_id[i] = bitStream.readUE();
    }
  }

  num_ref_idx_l0_default_active_minus1 = bitStream.readUE();
  num_ref_idx_l1_default_active_minus1 = bitStream.readUE();
  weighted_pred_flag = bitStream.readU1();
  weighted_bipred_idc = bitStream.readUn(2);
  pic_init_qp_minus26 = bitStream.readSE();
  std::cout << "\tpic_init_qp:" << pic_init_qp_minus26 + 26 << std::endl;
  pic_init_qs_minus26 = bitStream.readSE();
  std::cout << "\tpic_init_qs:" << pic_init_qs_minus26 + 26 << std::endl;
  chroma_qp_index_offset = bitStream.readSE();
  deblocking_filter_control_present_flag = bitStream.readU1();
  constrained_intra_pred_flag = bitStream.readU1();
  redundant_pic_cnt_present_flag = bitStream.readU1();
  if (bitStream.more_rbsp_data()) {
    transform_8x8_mode_flag = bitStream.readU1();
    pic_scaling_matrix_present_flag = bitStream.readU1();
    if (pic_scaling_matrix_present_flag) {
      maxPICScalingList =
          6 + ((sps.chroma_format_idc != 3) ? 2 : 6) * transform_8x8_mode_flag;
      pic_scaling_list_present_flag = new uint32_t[maxPICScalingList]{0};
      for (int i = 0; i < maxPICScalingList; i++) {
        pic_scaling_list_present_flag[i] = bitStream.readU1();
        if (pic_scaling_list_present_flag[i]) {
          if (i < 6) {
            scaling_list(bitStream, ScalingList4x4[i], 16,
                         UseDefaultScalingMatrix4x4Flag[i]);
          } else
            scaling_list(bitStream, ScalingList8x8[i - 6], 64,
                         UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
      }
      second_chroma_qp_index_offset = bitStream.readSE();
    }
  }
  bitStream.rbsp_trailing_bits();
  return 0;
}
