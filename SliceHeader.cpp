#include "SliceHeader.hpp"
#include "Nalu.hpp"
#include "Type.hpp"
#include <cstdint>
#include <cstring>

int SliceHeader::set_scaling_lists_values() {
  int ret = 0;

  //--------------------------------
  static int32_t Flat_4x4_16[16] = {16, 16, 16, 16, 16, 16, 16, 16,
                                    16, 16, 16, 16, 16, 16, 16, 16};
  static int32_t Flat_8x8_16[64] = {
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  };

  //--------------------------------
  // Table 7-3 – Specification of default scaling lists Default_4x4_Intra and
  // Default_4x4_Inter
  static int32_t Default_4x4_Intra[16] = {6,  13, 13, 20, 20, 20, 28, 28,
                                          28, 28, 32, 32, 32, 37, 37, 42};
  static int32_t Default_4x4_Inter[16] = {10, 14, 14, 20, 20, 20, 24, 24,
                                          24, 24, 27, 27, 27, 30, 30, 34};

  // Table 7-4 – Specification of default scaling lists Default_8x8_Intra and
  // Default_8x8_Inter
  static int32_t Default_8x8_Intra[64] = {
      6,  10, 10, 13, 11, 13, 16, 16, 16, 16, 18, 18, 18, 18, 18, 23,
      23, 23, 23, 23, 23, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27,
      27, 27, 27, 27, 29, 29, 29, 29, 29, 29, 29, 31, 31, 31, 31, 31,
      31, 33, 33, 33, 33, 33, 36, 36, 36, 36, 38, 38, 38, 40, 40, 42,
  };

  static int32_t Default_8x8_Inter[64] = {
      9,  13, 13, 15, 13, 15, 17, 17, 17, 17, 19, 19, 19, 19, 19, 21,
      21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 24, 24, 24, 24,
      24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27, 27,
      27, 28, 28, 28, 28, 28, 30, 30, 30, 30, 32, 32, 32, 33, 33, 35,
  };

  //--------------------------------
  int32_t i = 0;
  int32_t scaling_list_size = (m_sps->chroma_format_idc != 3) ? 8 : 12;

  if (m_sps->seq_scaling_matrix_present_flag == 0 &&
      m_pps->pic_scaling_matrix_present_flag == 0) {
    // 如果编码器未给出缩放矩阵值，则缩放矩阵值全部默认为16
    for (i = 0; i < scaling_list_size; i++) {
      if (i < 6) {
        memcpy(ScalingList4x4[i], Flat_4x4_16, sizeof(int32_t) * 16);
      } else // if (i >= 6)
      {
        memcpy(ScalingList8x8[i - 6], Flat_8x8_16, sizeof(int32_t) * 64);
      }
    }
  } else {
    if (m_sps->seq_scaling_matrix_present_flag == 1) {
      for (i = 0; i < scaling_list_size; i++) {
        if (i < 6) {
          if (m_sps->seq_scaling_list_present_flag[i] ==
              0) // 参照 Table 7-2 Scaling list fall-back rule A
          {
            if (i == 0) {
              memcpy(ScalingList4x4[i], Default_4x4_Intra,
                     sizeof(int32_t) * 16);
            } else if (i == 3) {
              memcpy(ScalingList4x4[i], Default_4x4_Inter,
                     sizeof(int32_t) * 16);
            } else {
              memcpy(ScalingList4x4[i], ScalingList4x4[i - 1],
                     sizeof(int32_t) * 16);
            }
          } else {
            if (m_pps->UseDefaultScalingMatrix4x4Flag[i] == 1) {
              if (i < 3) {
                memcpy(ScalingList4x4[i], Default_4x4_Intra,
                       sizeof(int32_t) * 16);
              } else // if (i >= 3)
              {
                memcpy(ScalingList4x4[i], Default_4x4_Inter,
                       sizeof(int32_t) * 16);
              }
            } else {
              memcpy(ScalingList4x4[i], ScalingList4x4[i],
                     sizeof(int32_t) *
                         16); // 采用编码器传送过来的量化系数的缩放值
            }
          }
        } else // if (i >= 6)
        {
          if (m_sps->seq_scaling_list_present_flag[i] ==
              0) // 参照 Table 7-2 Scaling list fall-back rule A
          {
            if (i == 6) {
              memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                     sizeof(int32_t) * 64);
            } else if (i == 7) {
              memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                     sizeof(int32_t) * 64);
            } else {
              memcpy(ScalingList8x8[i - 6], ScalingList8x8[i - 8],
                     sizeof(int32_t) * 64);
            }
          } else {
            if (m_pps->UseDefaultScalingMatrix8x8Flag[i - 6] == 1) {
              if (i == 6 || i == 8 || i == 10) {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                       sizeof(int32_t) * 64);
              } else {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                       sizeof(int32_t) * 64);
              }
            } else {
              memcpy(ScalingList8x8[i - 6], ScalingList8x8[i - 6],
                     sizeof(int32_t) *
                         64); // 采用编码器传送过来的量化系数的缩放值
            }
          }
        }
      }
    }

    // 注意：此处不是"else if"，意即面的值，可能会覆盖之前到的值
    if (m_pps->pic_scaling_matrix_present_flag == 1) {
      for (i = 0; i < scaling_list_size; i++) {
        if (i < 6) {
          if (m_pps->pic_scaling_list_present_flag[i] ==
              0) // 参照 Table 7-2 Scaling list fall-back rule B
          {
            if (i == 0) {
              if (m_sps->seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList4x4[i], Default_4x4_Intra,
                       sizeof(int32_t) * 16);
              }
            } else if (i == 3) {
              if (m_sps->seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList4x4[i], Default_4x4_Inter,
                       sizeof(int32_t) * 16);
              }
            } else {
              memcpy(ScalingList4x4[i], ScalingList4x4[i - 1],
                     sizeof(int32_t) * 16);
            }
          } else {
            if (m_pps->UseDefaultScalingMatrix4x4Flag[i] == 1) {
              if (i < 3) {
                memcpy(ScalingList4x4[i], Default_4x4_Intra,
                       sizeof(int32_t) * 16);
              } else // if (i >= 3)
              {
                memcpy(ScalingList4x4[i], Default_4x4_Inter,
                       sizeof(int32_t) * 16);
              }
            } else {
              memcpy(ScalingList4x4[i], ScalingList4x4[i],
                     sizeof(int32_t) *
                         16); // 采用编码器传送过来的量化系数的缩放值
            }
          }
        } else // if (i >= 6)
        {
          if (m_pps->pic_scaling_list_present_flag[i] ==
              0) // 参照 Table 7-2 Scaling list fall-back rule B
          {
            if (i == 6) {
              if (m_sps->seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                       sizeof(int32_t) * 64);
              }
            } else if (i == 7) {
              if (m_sps->seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                       sizeof(int32_t) * 64);
              }
            } else {
              memcpy(ScalingList8x8[i - 6], ScalingList8x8[i - 8],
                     sizeof(int32_t) * 64);
            }
          } else {
            if (m_pps->UseDefaultScalingMatrix8x8Flag[i - 6] == 1) {
              if (i == 6 || i == 8 || i == 10) {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                       sizeof(int32_t) * 64);
              } else {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                       sizeof(int32_t) * 64);
              }
            } else {
              memcpy(ScalingList8x8[i - 6], ScalingList8x8[i - 6],
                     sizeof(int32_t) *
                         64); // 采用编码器传送过来的量化系数的缩放值
            }
          }
        }
      }
    }
  }

  return ret;
}

/* Slice header syntax -> 51 page */
int SliceHeader::parseSliceHeader(BitStream &bitStream, GOP &gop) {
  first_mb_in_slice = bitStream.readUE();
  /* TODO YangJing 好像现在还不用%5，如果真的有变分辨率的Slice 呢？ <24-09-08 23:30:50> */
  slice_type = bitStream.readUE() % 5;
  switch (slice_type % 5) {
  case SLICE_P:
    std::cout << "\tP Slice" << std::endl;
    break;
  case SLICE_B:
    std::cout << "\tB Slice" << std::endl;
    break;
  case SLICE_I:
    std::cout << "\tI Slice" << std::endl;
    break;
  case SLICE_SP:
    std::cout << "\tSP Slice" << std::endl;
    break;
  case SLICE_SI:
    std::cout << "\tSI Slice" << std::endl;
    break;
  }
  std::cout << "\tSlice中第一个宏块的索引:" << first_mb_in_slice << std::endl;

  /* 更新GOP中当前Slice使用的SPS、PPS ID */
  gop.curr_pps_id = pic_parameter_set_id = bitStream.readUE();
  gop.curr_sps_id = gop.m_ppss[gop.curr_pps_id].seq_parameter_set_id;
  m_sps = &gop.m_spss[gop.curr_sps_id];
  m_pps = &gop.m_ppss[gop.curr_pps_id];
  std::cout << "\tPPS ID:" << pic_parameter_set_id << std::endl;

  if (m_sps->separate_colour_plane_flag) {
    colour_plane_id = bitStream.readUn(2);
    std::cout << "\t颜色平面ID:" << colour_plane_id << std::endl;
  }

  /* 如果当前图片是IDR图片，frame_num应等于0。 */
  frame_num = bitStream.readUn(log2(m_sps->MaxFrameNum));
  std::cout << "\t当前帧的编号:" << frame_num << std::endl;
  if (!m_sps->frame_mbs_only_flag) {
    field_pic_flag = bitStream.readU1();
    std::cout << "\t场图像标志:" << field_pic_flag << std::endl;
    if (field_pic_flag) {
      bottom_field_flag = bitStream.readU1();
      std::cout << "\t底场标志:" << bottom_field_flag << std::endl;
    }
  }
  //7.4.1 NAL unit semantics (7-1)
  IdrPicFlag = ((nal_unit_type == 5) ? 1 : 0);
  if (IdrPicFlag) {
    idr_pic_id = bitStream.readUE();
    std::cout << "\tIDR图像ID:" << idr_pic_id << std::endl;
  }

  if (m_sps->pic_order_cnt_type == 0) {
    pic_order_cnt_lsb =
        bitStream.readUn(m_sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    std::cout << "\t图像顺序计数LSB:" << pic_order_cnt_lsb << std::endl;
    if (m_pps->bottom_field_pic_order_in_frame_present_flag &&
        !field_pic_flag) {
      delta_pic_order_cnt_bottom = bitStream.readSE();
      std::cout << "\t底场的图像顺序计数增量:" << delta_pic_order_cnt_bottom
                << std::endl;
    }
  }

  if (m_sps->pic_order_cnt_type == 1 &&
      !m_sps->delta_pic_order_always_zero_flag) {
    std::cout << "\t存在B帧和P帧混合的情况:" << m_sps->pic_order_cnt_type
              << std::endl;
    delta_pic_order_cnt[0] = bitStream.readSE();
    std::cout << "\t图像顺序计数增量1:" << delta_pic_order_cnt[0] << std::endl;
    if (m_pps->bottom_field_pic_order_in_frame_present_flag && !field_pic_flag)
      delta_pic_order_cnt[1] = bitStream.readSE();
    std::cout << "\t图像顺序计数增量2:" << delta_pic_order_cnt[1] << std::endl;
  }

  if (m_sps->pic_order_cnt_type == 2)
    std::cout << "\t是否存在B帧:0" << std::endl;

  if (m_pps->redundant_pic_cnt_present_flag) {
    redundant_pic_cnt = bitStream.readUE();
    std::cout << "\t冗余图像计数:" << redundant_pic_cnt << std::endl;
  }
  if (slice_type % 5 == SLICE_B) {
    direct_spatial_mv_pred_flag = bitStream.readU1();
    std::cout << "\t直接空间运动矢量预测标志:" << direct_spatial_mv_pred_flag
              << std::endl;
  }
  if (slice_type % 5 == SLICE_P || slice_type % 5 == SLICE_SP ||
      slice_type % 5 == SLICE_B) {
    num_ref_idx_active_override_flag = bitStream.readU1();
    std::cout << "\t覆盖活动参考帧数标志:" << num_ref_idx_active_override_flag
              << std::endl;
    num_ref_idx_l0_active_minus1 = m_pps->num_ref_idx_l0_default_active_minus1;
    if (num_ref_idx_active_override_flag) {
      num_ref_idx_l0_active_minus1 = bitStream.readUE();
      if (slice_type % 5 == SLICE_B) {
        num_ref_idx_l1_active_minus1 = bitStream.readUE();
        std::cout << "\t参考帧列表0的活动参考帧数减1:"
                  << num_ref_idx_l0_active_minus1
                  << ",参考帧列表1的活动参考帧数减1:"
                  << num_ref_idx_l1_active_minus1 << std::endl;
      }
    }
  }
  if (nal_unit_type == 20 || nal_unit_type == 21)
    ref_pic_list_mvc_modification(bitStream); /* specified in Annex H */
  else
    ref_pic_list_modification(bitStream);
  if ((m_pps->weighted_pred_flag &&
       (slice_type % 5 == SLICE_P || slice_type % 5 == SLICE_SP)) ||
      (m_pps->weighted_bipred_idc == 1 && slice_type % 5 == SLICE_B))
    pred_weight_table(bitStream);
  if (nal_ref_idc != 0) dec_ref_pic_marking(bitStream);
  if (m_pps->entropy_coding_mode_flag && slice_type % 5 != SLICE_I &&
      slice_type % 5 != SLICE_SI) {
    cabac_init_idc = bitStream.readUE();
    std::cout << "\tCABAC初始化索引:" << cabac_init_idc << std::endl;
  }

  slice_qp_delta = bitStream.readSE();
  if (slice_type % 5 == SLICE_SP || slice_type % 5 == SLICE_SI) {
    if (slice_type % 5 == SLICE_SP) {
      sp_for_switch_flag = bitStream.readU1();
      std::cout << "\tSP切换标志:" << sp_for_switch_flag << std::endl;
    }
    slice_qs_delta = bitStream.readSE();
    std::cout << "\tSlice的量化参数调整值:" << slice_qp_delta
              << ",Slice的量化步长调整值:" << slice_qs_delta << std::endl;
  }
  if (m_pps->deblocking_filter_control_present_flag) {
    disable_deblocking_filter_idc = bitStream.readUE();
    std::cout << "\t禁用去块效应滤波器标志:" << disable_deblocking_filter_idc
              << std::endl;
    if (disable_deblocking_filter_idc != 1) {
      slice_alpha_c0_offset_div2 = bitStream.readSE();
      slice_beta_offset_div2 = bitStream.readSE();
      std::cout << "\t去块效应滤波器的Alpha偏移值:"
                << slice_alpha_c0_offset_div2
                << ",去块效应滤波器的Beta偏移值:" << slice_beta_offset_div2
                << std::endl;
    }
  }
  if (m_pps->num_slice_groups_minus1 > 0 && m_pps->slice_group_map_type >= 3 &&
      m_pps->slice_group_map_type <= 5)
    slice_group_change_cycle = bitStream.readUE();

  //----------- 下面都是一些额外信息，比如还原偏移，或者事先计算一些值，后面方便用 ------------
  int SliceGroupChangeRate = m_pps->slice_group_change_rate_minus1 + 1;
  if (m_pps->num_slice_groups_minus1 > 0 && m_pps->slice_group_map_type >= 3 &&
      m_pps->slice_group_map_type <= 5) {
    int32_t temp = m_sps->PicSizeInMapUnits / SliceGroupChangeRate + 1;
    int32_t v = h264_log2(temp);
    // Ceil( Log2( PicSizeInMapUnits ÷ SliceGroupChangeRate + 1 ) );
    slice_group_change_cycle = bitStream.readUn(v); // 2 u(v)
  }
  if (slice_group_change_cycle != 0)
    std::cout << "\tSlice组改变周期:" << slice_group_change_cycle << std::endl;

  SliceQPY = 26 + m_pps->pic_init_qp_minus26 + slice_qp_delta;
  std::cout << "\tSlice的量化参数:" << SliceQPY << std::endl;
  /* 对于首个Slice而言前一个Slice的量化参数应该初始化为当前量化参数，而不是0 */
  QPY_prev = SliceQPY;

  MbaffFrameFlag = (m_sps->mb_adaptive_frame_field_flag && !field_pic_flag);
  std::cout << "\t宏块自适应帧场标志:" << MbaffFrameFlag << std::endl;
  PicHeightInMbs = m_sps->FrameHeightInMbs / (1 + field_pic_flag);
  PicHeightInSamplesL = PicHeightInMbs * 16;
  PicHeightInSamplesC = PicHeightInMbs * m_sps->MbHeightC;
  std::cout << "\t图像高度（宏块数）:" << PicHeightInMbs
            << ",图像高度（亮度样本）:" << PicHeightInSamplesL
            << ",图像高度（色度样本）:" << PicHeightInSamplesC << std::endl;
  PicSizeInMbs = m_sps->PicWidthInMbs * PicHeightInMbs;
  std::cout << "\t图像大小（宏块数）:" << PicSizeInMbs << std::endl;
  MaxPicNum =
      (field_pic_flag == 0) ? m_sps->MaxFrameNum : (2 * m_sps->MaxFrameNum);
  CurrPicNum = (field_pic_flag == 0) ? frame_num : (2 * frame_num + 1);
  std::cout << "\t最大图像编号:" << MaxPicNum << ",当前图像编号:" << CurrPicNum
            << std::endl;
  MapUnitsInSliceGroup0 = MIN(slice_group_change_cycle * SliceGroupChangeRate,
                              m_sps->PicSizeInMapUnits);
  std::cout << "\tSlice组0中的映射单元数:" << MapUnitsInSliceGroup0
            << std::endl;
  QSY = 26 + m_pps->pic_init_qs_minus26 + slice_qs_delta;
  std::cout << "\tSlice的量化参数（色度）:" << QSY << std::endl;
  FilterOffsetA = slice_alpha_c0_offset_div2 << 1;
  FilterOffsetB = slice_beta_offset_div2 << 1;
  std::cout << "\t去块效应滤波器的A偏移值:" << FilterOffsetA
            << ",去块效应滤波器的B偏移值:" << FilterOffsetB << std::endl;

  if (!mapUnitToSliceGroupMap) {
    mapUnitToSliceGroupMap = new int32_t[m_sps->PicSizeInMapUnits]{0};
    std::cout << "\t映射单元到Slice组的映射表(size):"
              << m_sps->PicSizeInMapUnits << std::endl;
  }

  if (!MbToSliceGroupMap) {
    MbToSliceGroupMap = new int32_t[PicSizeInMbs]{0};
    std::cout << "\t宏块到Slice组的映射表:" << PicSizeInMbs << std::endl;
  }

  set_scaling_lists_values();
  m_is_malloc_mem_self = 1;
  return 0;
}

void SliceHeader::ref_pic_list_mvc_modification(BitStream &bitStream) {
  /* TODO YangJing 函数未实现（好像不用实现？） <24-08-16 00:08:32> */
}

void SliceHeader::ref_pic_list_modification(BitStream &bitStream) {
  if (slice_type % 5 != SLICE_I && slice_type % 5 != SLICE_SI) {
    ref_pic_list_modification_flag_l0 = bitStream.readU1();
    if (ref_pic_list_modification_flag_l0) {
      int i = 0;
      do {
        modification_of_pic_nums_idc[0][i] = bitStream.readUE();
        if (modification_of_pic_nums_idc[0][i] == 0 ||
            modification_of_pic_nums_idc[0][i] == 1)
          abs_diff_pic_num_minus1[0][i] = bitStream.readUE();
        else if (modification_of_pic_nums_idc[0][i] == 2)
          long_term_pic_num[0][i] = bitStream.readUE();
        i++;
      } while (modification_of_pic_nums_idc[0][i - 1] != 3);
      ref_pic_list_modification_count_l0 = i;
    }
  }

  if (slice_type % 5 == SLICE_B) {
    ref_pic_list_modification_flag_l1 = bitStream.readU1();
    if (ref_pic_list_modification_flag_l1) {
      int i = 0;
      do {
        modification_of_pic_nums_idc[1][i] = bitStream.readUE();
        if (modification_of_pic_nums_idc[1][i] == 0 ||
            modification_of_pic_nums_idc[1][i] == 1)
          abs_diff_pic_num_minus1[1][i] = bitStream.readUE();
        else if (modification_of_pic_nums_idc[1][i] == 2)
          long_term_pic_num[1][i] = bitStream.readUE();
        i++;
      } while (modification_of_pic_nums_idc[1][i - 1] != 3);
      ref_pic_list_modification_count_l1 = i;
    }
  }
}

void SliceHeader::pred_weight_table(BitStream &bitStream) {
  luma_log2_weight_denom = bitStream.readUE();
  if (m_sps->ChromaArrayType != 0) {
    chroma_log2_weight_denom = bitStream.readUE();
    std::cout << "\t亮度权重的对数基数:" << luma_log2_weight_denom
              << ",色度权重的对数基数:" << chroma_log2_weight_denom
              << std::endl;
  }

  for (int i = 0; i <= (int)num_ref_idx_l0_active_minus1; i++) {
    luma_weight_l0[i] = 1 << luma_log2_weight_denom;
    luma_offset_l0[i] = 0;
    bool luma_weight_l0_flag = bitStream.readU1();
    if (luma_weight_l0_flag) {
      luma_weight_l0[i] = bitStream.readSE();
      luma_offset_l0[i] = bitStream.readSE();
    }
    if (m_sps->ChromaArrayType != 0) {
      chroma_weight_l0[i][0] = 1 << chroma_log2_weight_denom;
      chroma_weight_l0[i][1] = 1 << chroma_log2_weight_denom;
      chroma_offset_l0[i][0] = 0;
      chroma_offset_l0[i][1] = 0;
      bool chroma_weight_l0_flag = bitStream.readU1();
      if (chroma_weight_l0_flag) {
        for (int j = 0; j < 2; j++) {
          chroma_weight_l0[i][j] = bitStream.readSE();
          chroma_offset_l0[i][j] = bitStream.readSE();
        }
      }
    }
  }
  //for (int i = 0; i <= num_ref_idx_l0_active_minus1; ++i) {
  //std::cout << "\t参考帧列表0的亮度权重:" << luma_weight_l0[i] << std::endl;
  //std::cout << "\t参考帧列表0的亮度权重:" << luma_offset_l0[i] << std::endl;
  //std::cout << "\t参考帧列表0的色度权重:" << chroma_weight_l0[i][0]
  //<< std::endl;
  //std::cout << "\t参考帧列表0的色度权重:" << chroma_weight_l0[i][1]
  //<< std::endl;
  //std::cout << "\t参考帧列表0的色度偏移:" << chroma_offset_l0[i][0]
  //<< std::endl;
  //std::cout << "\t参考帧列表0的色度偏移:" << chroma_offset_l0[i][1]
  //<< std::endl;
  //}

  if (slice_type % 5 == SLICE_B) {
    for (int i = 0; i <= (int)num_ref_idx_l1_active_minus1; i++) {
      luma_weight_l1[i] = 1 << luma_log2_weight_denom;
      luma_offset_l1[i] = 0;
      bool luma_weight_l1_flag = bitStream.readU1();
      if (luma_weight_l1_flag) {
        luma_weight_l1[i] = bitStream.readSE();
        luma_offset_l1[i] = bitStream.readSE();
      }

      if (m_sps->ChromaArrayType != 0) {
        chroma_weight_l1[i][0] = 1 << chroma_log2_weight_denom;
        chroma_weight_l1[i][1] = 1 << chroma_log2_weight_denom;
        chroma_offset_l1[i][0] = 0;
        chroma_offset_l1[i][1] = 0;
        bool chroma_weight_l1_flag = bitStream.readU1();
        if (chroma_weight_l1_flag) {
          for (int j = 0; j < 2; j++) {
            chroma_weight_l1[i][j] = bitStream.readSE();
            chroma_offset_l1[i][j] = bitStream.readSE();
          }
        }
      }
    }
    for (int i = 0; i <= (int)num_ref_idx_l1_active_minus1; ++i) {
      std::cout << "\t参考帧列表1的亮度权重:" << luma_weight_l1[i] << std::endl;
      std::cout << "\t参考帧列表1的亮度偏移:" << luma_offset_l1[i] << std::endl;
      std::cout << "\t参考帧列表1的色度权重:" << chroma_weight_l1[i][0]
                << std::endl;
      std::cout << "\t参考帧列表1的色度权重:" << chroma_weight_l1[i][1]
                << std::endl;
      std::cout << "\t参考帧列表1的色度偏移:" << chroma_offset_l1[i][0]
                << std::endl;
      std::cout << "\t参考帧列表1的色度偏移:" << chroma_offset_l1[i][1]
                << std::endl;
    }
  }
}

void SliceHeader::dec_ref_pic_marking(BitStream &bs) {
  if (IdrPicFlag) {
    no_output_of_prior_pics_flag = bs.readU1();
    long_term_reference_flag = bs.readU1();
  } else {
    adaptive_ref_pic_marking_mode_flag = bs.readU1();
    if (adaptive_ref_pic_marking_mode_flag) {
      uint32_t i = 0;
      do {
        if (i > 31) {
          std::cerr << "An error occurred on " << __FUNCTION__
                    << "():" << __LINE__ << std::endl;
          break;
        }
        m_dec_ref_pic_marking[i].memory_management_control_operation =
            bs.readUE();
        if (m_dec_ref_pic_marking[i].memory_management_control_operation == 1 ||
            m_dec_ref_pic_marking[i].memory_management_control_operation == 3) {
          m_dec_ref_pic_marking[i].difference_of_pic_nums_minus1 = bs.readUE();
        }
        if (m_dec_ref_pic_marking[i].memory_management_control_operation == 2) {
          m_dec_ref_pic_marking[i].long_term_pic_num_2 = bs.readUE();
        }
        if (m_dec_ref_pic_marking[i].memory_management_control_operation == 3 ||
            m_dec_ref_pic_marking[i].memory_management_control_operation == 6) {
          m_dec_ref_pic_marking[i].long_term_frame_idx = bs.readUE();
        }
        if (m_dec_ref_pic_marking[i].memory_management_control_operation == 4) {
          m_dec_ref_pic_marking[i].max_long_term_frame_idx_plus1 = bs.readUE();
        }
        i++;
      } while (
          m_dec_ref_pic_marking[i - 1].memory_management_control_operation);
      dec_ref_pic_marking_count = i;
    }
  }
}
