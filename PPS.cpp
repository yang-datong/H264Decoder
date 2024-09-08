#include "PPS.hpp"
#include <iostream>

int PPS::extractParameters(uint32_t chroma_format_idc) {
  /* 初始化bit处理器，填充pps的数据 */
  BitStream bs(_buf, _len);

  pic_parameter_set_id = bs.readUE();
  seq_parameter_set_id = bs.readUE();
  std::cout << "\tPPS ID:" << pic_parameter_set_id
            << ",SPS ID:" << seq_parameter_set_id << std::endl;

  entropy_coding_mode_flag = bs.readU1();

  if (entropy_coding_mode_flag == 0)
    std::cout << "\t熵编码模式:CAVLC(上下文可变长度编码)" << std::endl;
  else
    std::cout << "\t熵编码模式:CABAC(上下文自适应二进制算术编码)" << std::endl;

  bottom_field_pic_order_in_frame_present_flag = bs.readU1();
  std::cout << "\t存在场序信息:" << bottom_field_pic_order_in_frame_present_flag
            << std::endl;
  num_slice_groups_minus1 = bs.readUE();
  if (num_slice_groups_minus1 > 0) {
    slice_group_map_type = bs.readUE();
    if (slice_group_map_type == 0) {
      std::cout << "\tslice_group_map_type:逐行切片组映射" << std::endl;
      run_length_minus1 = new uint32_t[num_slice_groups_minus1 + 1];
      for (int iGroup = 0; iGroup <= (int)num_slice_groups_minus1; iGroup++)
        run_length_minus1[iGroup] = bs.readUE();
    } else if (slice_group_map_type == 1) {
      std::cout << "\tslice_group_map_type:逐列切片组映射" << std::endl;
    } else if (slice_group_map_type == 2) {
      std::cout << "\tslice_group_map_type:逐块切片组映射" << std::endl;
      top_left = new uint32_t[num_slice_groups_minus1];
      bottom_right = new uint32_t[num_slice_groups_minus1];
      for (int iGroup = 0; iGroup < (int)num_slice_groups_minus1; iGroup++) {
        top_left[iGroup] = bs.readUE();
        bottom_right[iGroup] = bs.readUE();
      }
    } else if (slice_group_map_type == 3 || slice_group_map_type == 4 ||
               slice_group_map_type == 5) {
      std::cout << "\tslice_group_map_type:逐(帧/字段/两行)切片组映射"
                << std::endl;
      slice_group_change_direction_flag = bs.readU1();
      slice_group_change_rate_minus1 = bs.readUE();
    } else if (slice_group_map_type == 6) {
      std::cout << "\tslice_group_map_type:逐两列切片组映射" << std::endl;
      pic_size_in_map_units_minus1 = bs.readUE();
      std::cout << "\tpic_size_in_map_units_minus1:"
                << pic_size_in_map_units_minus1 << std::endl;
      slice_group_id = new uint32_t[pic_size_in_map_units_minus1 + 1];
      for (int i = 0; i <= (int)pic_size_in_map_units_minus1; i++)
        slice_group_id[i] = bs.readUE();
    }
  }

  num_ref_idx_l0_default_active_minus1 = bs.readUE();
  num_ref_idx_l1_default_active_minus1 = bs.readUE();
  weighted_pred_flag = bs.readU1();
  std::cout << "\t存在加权预测:" << weighted_bipred_idc << std::endl;
  weighted_bipred_idc = bs.readUn(2);
  std::cout << "\t加权双向预测类型:" << weighted_bipred_idc << std::endl;
  pic_init_qp_minus26 = bs.readSE();
  std::cout << "\t图像的初始量化参数:" << pic_init_qp_minus26 + 26 << std::endl;
  pic_init_qs_minus26 = bs.readSE();
  std::cout << "\t图像的初始 QP 步长:" << pic_init_qs_minus26 + 26 << std::endl;
  chroma_qp_index_offset = bs.readSE();
  std::cout << "\t色度量化参数索引偏移:" << chroma_qp_index_offset << std::endl;
  deblocking_filter_control_present_flag = bs.readU1();
  std::cout << "\t存在去块滤波器控制信息:"
            << deblocking_filter_control_present_flag << std::endl;
  constrained_intra_pred_flag = bs.readU1();
  std::cout << "\t存在约束的帧内预测:" << constrained_intra_pred_flag
            << std::endl;
  redundant_pic_cnt_present_flag = bs.readU1();
  std::cout << "\t存在冗余图像计数:" << redundant_pic_cnt_present_flag
            << std::endl;
  if (bs.more_rbsp_data()) {
    transform_8x8_mode_flag = bs.readU1();
    std::cout << "\t存在 8x8 变换模式:" << transform_8x8_mode_flag << std::endl;
    pic_scaling_matrix_present_flag = bs.readU1();
    std::cout << "\t存在图像缩放矩阵:" << pic_scaling_matrix_present_flag
              << std::endl;
    if (pic_scaling_matrix_present_flag) {
      maxPICScalingList =
          6 + ((chroma_format_idc != 3) ? 2 : 6) * transform_8x8_mode_flag;
      pic_scaling_list_present_flag = new uint32_t[maxPICScalingList]{0};
      for (int i = 0; i < (int)maxPICScalingList; i++) {
        pic_scaling_list_present_flag[i] = bs.readU1();
        if (pic_scaling_list_present_flag[i]) {
          if (i < 6) {
            scaling_list(bs, ScalingList4x4[i], 16,
                         UseDefaultScalingMatrix4x4Flag[i]);
          } else
            scaling_list(bs, ScalingList8x8[i - 6], 64,
                         UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
      }
      second_chroma_qp_index_offset = bs.readSE();
    }
  }
  bs.rbsp_trailing_bits();
  return 0;
}
