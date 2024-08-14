#include "SliceHeader.hpp"
#include "Nalu.hpp"
#include "Type.hpp"
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
  int32_t scaling_list_size = (m_sps.chroma_format_idc != 3) ? 8 : 12;

  if (m_sps.seq_scaling_matrix_present_flag == 0 &&
      m_pps.pic_scaling_matrix_present_flag == 0) {
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
    if (m_sps.seq_scaling_matrix_present_flag == 1) {
      for (i = 0; i < scaling_list_size; i++) {
        if (i < 6) {
          if (m_sps.seq_scaling_list_present_flag[i] ==
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
            if (m_pps.UseDefaultScalingMatrix4x4Flag[i] == 1) {
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
          if (m_sps.seq_scaling_list_present_flag[i] ==
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
            if (m_pps.UseDefaultScalingMatrix8x8Flag[i - 6] == 1) {
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
    if (m_pps.pic_scaling_matrix_present_flag == 1) {
      for (i = 0; i < scaling_list_size; i++) {
        if (i < 6) {
          if (m_pps.pic_scaling_list_present_flag[i] ==
              0) // 参照 Table 7-2 Scaling list fall-back rule B
          {
            if (i == 0) {
              if (m_sps.seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList4x4[i], Default_4x4_Intra,
                       sizeof(int32_t) * 16);
              }
            } else if (i == 3) {
              if (m_sps.seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList4x4[i], Default_4x4_Inter,
                       sizeof(int32_t) * 16);
              }
            } else {
              memcpy(ScalingList4x4[i], ScalingList4x4[i - 1],
                     sizeof(int32_t) * 16);
            }
          } else {
            if (m_pps.UseDefaultScalingMatrix4x4Flag[i] == 1) {
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
          if (m_pps.pic_scaling_list_present_flag[i] ==
              0) // 参照 Table 7-2 Scaling list fall-back rule B
          {
            if (i == 6) {
              if (m_sps.seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                       sizeof(int32_t) * 64);
              }
            } else if (i == 7) {
              if (m_sps.seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                       sizeof(int32_t) * 64);
              }
            } else {
              memcpy(ScalingList8x8[i - 6], ScalingList8x8[i - 8],
                     sizeof(int32_t) * 64);
            }
          } else {
            if (m_pps.UseDefaultScalingMatrix8x8Flag[i - 6] == 1) {
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

int SliceHeader::setMapUnitToSliceGroupMap() {
  int32_t i = 0;
  int32_t j = 0;
  int32_t k = 0;
  int32_t x = 0;
  int32_t y = 0;
  int32_t iGroup = 0;

  if (m_pps.num_slice_groups_minus1 == 0) {
    for (i = 0; i < m_sps.PicSizeInMapUnits; i++) {
      mapUnitToSliceGroupMap[i] = 0;
    }
    return 0;
  }

  if (m_pps.slice_group_map_type ==
      0) // 8.2.2.1 Specification for interleaved slice
         // group map type 交叉型 slice组映射类型的描述
  {
    i = 0;
    do {
      for (iGroup = 0; iGroup <= m_pps.num_slice_groups_minus1 &&
                       i < m_sps.PicSizeInMapUnits;
           i += m_pps.run_length_minus1[iGroup++] + 1) {
        for (j = 0; j <= m_pps.run_length_minus1[iGroup] &&
                    i + j < m_sps.PicSizeInMapUnits;
             j++) {
          mapUnitToSliceGroupMap[i + j] = iGroup;
        }
      }
    } while (i < m_sps.PicSizeInMapUnits);
  } else if (m_pps.slice_group_map_type ==
             1) // 8.2.2.2 Specification for dispersed slice group map type
                // 分散型 slice 组映射类型的描述
  {
    for (i = 0; i < m_sps.PicSizeInMapUnits; i++) {
      mapUnitToSliceGroupMap[i] =
          ((i % m_sps.PicWidthInMbs) +
           (((i / m_sps.PicWidthInMbs) * (m_pps.num_slice_groups_minus1 + 1)) /
            2)) %
          (m_pps.num_slice_groups_minus1 + 1);
    }
  } else if (m_pps.slice_group_map_type ==
             2) // 8.2.2.3 Specification for foreground with left-over slice
                // group map type 前景加剩余型 slice 组映射类型的描述
  {
    for (i = 0; i < m_sps.PicSizeInMapUnits; i++) {
      mapUnitToSliceGroupMap[i] = m_pps.num_slice_groups_minus1;
    }
    for (iGroup = m_pps.num_slice_groups_minus1 - 1; iGroup >= 0; iGroup--) {
      int32_t yTopLeft = m_pps.top_left[iGroup] / m_sps.PicWidthInMbs;
      int32_t xTopLeft = m_pps.top_left[iGroup] % m_sps.PicWidthInMbs;
      int32_t yBottomRight = m_pps.bottom_right[iGroup] / m_sps.PicWidthInMbs;
      int32_t xBottomRight = m_pps.bottom_right[iGroup] % m_sps.PicWidthInMbs;
      for (y = yTopLeft; y <= yBottomRight; y++) {
        for (x = xTopLeft; x <= xBottomRight; x++) {
          mapUnitToSliceGroupMap[y * m_sps.PicWidthInMbs + x] = iGroup;
        }
      }
    }
  } else if (m_pps.slice_group_map_type == 3) {
    // 8.2.2.4 Specification for box-out slice group map types
    // 外旋盒子型 slice 组映射类型的描述
    for (i = 0; i < m_sps.PicSizeInMapUnits; i++) {
      mapUnitToSliceGroupMap[i] = 1;
    }
    x = (m_sps.PicWidthInMbs - m_pps.slice_group_change_direction_flag) / 2;
    y = (m_sps.PicHeightInMapUnits - m_pps.slice_group_change_direction_flag) /
        2;

    int32_t leftBound = x;
    int32_t topBound = y;
    int32_t rightBound = x;
    int32_t bottomBound = y;
    int32_t xDir = m_pps.slice_group_change_direction_flag - 1;
    int32_t yDir = m_pps.slice_group_change_direction_flag;
    int32_t mapUnitVacant = 0;

    for (k = 0; k < MapUnitsInSliceGroup0; k += mapUnitVacant) {
      mapUnitVacant =
          (mapUnitToSliceGroupMap[y * m_sps.PicWidthInMbs + x] == 1);
      if (mapUnitVacant) {
        mapUnitToSliceGroupMap[y * m_sps.PicWidthInMbs + x] = 0;
      }
      if (xDir == -1 && x == leftBound) {
        leftBound = std::fmax(leftBound - 1, 0);
        x = leftBound;
        xDir = 0;
        yDir = 2 * m_pps.slice_group_change_direction_flag - 1;
      } else if (xDir == 1 && x == rightBound) {
        rightBound = MIN(rightBound + 1, m_sps.PicWidthInMbs - 1);
        x = rightBound;
        xDir = 0;
        yDir = 1 - 2 * m_pps.slice_group_change_direction_flag;
      } else if (yDir == -1 && y == topBound) {
        topBound = MAX(topBound - 1, 0);
        y = topBound;
        xDir = 1 - 2 * m_pps.slice_group_change_direction_flag;
        yDir = 0;
      } else if (yDir == 1 && y == bottomBound) {
        bottomBound = MIN(bottomBound + 1, m_sps.PicHeightInMapUnits - 1);
        y = bottomBound;
        xDir = 2 * m_pps.slice_group_change_direction_flag - 1;
        yDir = 0;
      } else {
        (x, y) = (x + xDir, y + yDir);
      }
    }
  } else if (m_pps.slice_group_map_type ==
             4) // 8.2.2.5 Specification for raster scan slice group map types
                // 栅格扫描型 slice 组映射类型的描述
  {
    int32_t sizeOfUpperLeftGroup = 0;
    if (m_pps.num_slice_groups_minus1 == 1) {
      sizeOfUpperLeftGroup =
          (m_pps.slice_group_change_direction_flag
               ? (m_sps.PicSizeInMapUnits - MapUnitsInSliceGroup0)
               : MapUnitsInSliceGroup0);
    }

    for (i = 0; i < m_sps.PicSizeInMapUnits; i++) {
      if (i < sizeOfUpperLeftGroup) {
        mapUnitToSliceGroupMap[i] = m_pps.slice_group_change_direction_flag;
      } else {
        mapUnitToSliceGroupMap[i] = 1 - m_pps.slice_group_change_direction_flag;
      }
    }
  } else if (m_pps.slice_group_map_type ==
             5) // 8.2.2.6 Specification for wipe slice group map types 擦除型
                // slice 组映射类型的描述
  {
    int32_t sizeOfUpperLeftGroup = 0;
    if (m_pps.num_slice_groups_minus1 == 1) {
      sizeOfUpperLeftGroup =
          (m_pps.slice_group_change_direction_flag
               ? (m_sps.PicSizeInMapUnits - MapUnitsInSliceGroup0)
               : MapUnitsInSliceGroup0);
    }

    k = 0;
    for (j = 0; j < m_sps.PicWidthInMbs; j++) {
      for (i = 0; i < m_sps.PicHeightInMapUnits; i++) {
        if (k++ < sizeOfUpperLeftGroup) {
          mapUnitToSliceGroupMap[i * m_sps.PicWidthInMbs + j] =
              m_pps.slice_group_change_direction_flag;
        } else {
          mapUnitToSliceGroupMap[i * m_sps.PicWidthInMbs + j] =
              1 - m_pps.slice_group_change_direction_flag;
        }
      }
    }
  } else if (m_pps.slice_group_map_type ==
             6) // 8.2.2.7 Specification for explicit slice group map type
                // 显式型 slice 组映射类型的描述
  {
    for (i = 0; i < m_sps.PicSizeInMapUnits; i++) {
      mapUnitToSliceGroupMap[i] = m_pps.slice_group_id[i];
    }
  } else {
    printf("slice_group_map_type=%d, must be in [0..6];\n",
           m_pps.slice_group_map_type);
    return -1;
  }

  return 0;
}

int SliceHeader::setMbToSliceGroupMap() {
  for (int i = 0; i < m_idr.PicSizeInMbs; i++) {
    if (m_sps.frame_mbs_only_flag == 1 || field_pic_flag == 1) {
      MbToSliceGroupMap[i] = mapUnitToSliceGroupMap[i];
    } else if (MbaffFrameFlag == 1) {
      MbToSliceGroupMap[i] = mapUnitToSliceGroupMap[i / 2];
    } else // if (frame_mbs_only_flag == 0 &&
           // mb_adaptive_frame_field_flag == 0 && slice.field_pic_flag == 0)
    {
      MbToSliceGroupMap[i] =
          mapUnitToSliceGroupMap[(i / (2 * m_sps.PicWidthInMbs)) *
                                     m_sps.PicWidthInMbs +
                                 (i % m_sps.PicWidthInMbs)];
    }
  }

  return 0;
}

/* Slice header syntax -> 51 page */
int SliceHeader::parseSliceHeader(BitStream &bitStream, Nalu *nalu) {
  first_mb_in_slice = bitStream.readUE();
  slice_type = bitStream.readUE() % 5;
  pic_parametter_set_id = bitStream.readUE();
  if (m_sps.separate_colour_plane_flag == 1)
    colour_plane_id = bitStream.readUn(2);

  frame_num = bitStream.readUn(std::log2(m_sps.MaxFrameNum)); // u(v)
  if (!m_sps.frame_mbs_only_flag) {
    field_pic_flag = bitStream.readU1();
    if (field_pic_flag)
      bottom_field_flag = bitStream.readU1();
  }
  IdrPicFlag = ((nalu->nal_unit_type == 5) ? 1 : 0);
  if (IdrPicFlag)
    idr_pic_id = bitStream.readUE();
  if (m_sps.pic_order_cnt_type == 0) {
    pic_order_cnt_lsb =
        bitStream.readUn(m_sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
    if (m_pps.bottom_field_pic_order_in_frame_present_flag && !field_pic_flag)
      delta_pic_order_cnt_bottom = bitStream.readSE();
  }

  if (m_sps.pic_order_cnt_type == 1 &&
      !m_sps.delta_pic_order_always_zero_flag) {
    delta_pic_order_cnt[0] = bitStream.readSE();
    if (m_pps.bottom_field_pic_order_in_frame_present_flag && !field_pic_flag)
      delta_pic_order_cnt[1] = bitStream.readSE();
  }

  if (m_pps.redundant_pic_cnt_present_flag)
    redundant_pic_cnt = bitStream.readUE();
  if (slice_type % 5 == SLICE_B)
    direct_spatial_mv_pred_flag = bitStream.readU1();
  if (slice_type % 5 == SLICE_P || slice_type % 5 == SLICE_SP ||
      slice_type % 5 == SLICE_B) {
    num_ref_idx_active_override_flag = bitStream.readU1();
    if (num_ref_idx_active_override_flag) {
      num_ref_idx_l0_active_minus1 = bitStream.readUE();
      if (slice_type % 5 == SLICE_B)
        num_ref_idx_l1_active_minus1 = bitStream.readUE();
    }
  }
  if (nalu->nal_unit_type == 20 || nalu->nal_unit_type == 21)
    ref_pic_list_mvc_modification(bitStream); /* specified in Annex H */
  else
    ref_pic_list_modification(bitStream);
  if ((m_pps.weighted_pred_flag &&
       (slice_type % 5 == SLICE_P || slice_type % 5 == SLICE_SP)) ||
      (m_pps.weighted_bipred_idc == 1 && slice_type % 5 == SLICE_B))
    pred_weight_table(bitStream);
  if (nalu->nal_ref_idc != 0)
    dec_ref_pic_marking(bitStream);
  if (m_pps.entropy_coding_mode_flag && slice_type % 5 != SLICE_I &&
      slice_type % 5 != SLICE_SI)
    cabac_init_idc = bitStream.readUE();
  slice_qp_delta = bitStream.readSE();
  if (slice_type % 5 == SLICE_SP || slice_type % 5 == SLICE_SI) {
    if (slice_type % 5 == SLICE_SP)
      sp_for_switch_flag = bitStream.readU1();
    slice_qs_delta = bitStream.readSE();
  }
  if (m_pps.deblocking_filter_control_present_flag) {
    disable_deblocking_filter_idc = bitStream.readUE();
    if (disable_deblocking_filter_idc != 1) {
      slice_alpha_c0_offset_div2 = bitStream.readSE();
      slice_beta_offset_div2 = bitStream.readSE();
    }
  }
  if (m_pps.num_slice_groups_minus1 > 0 && m_pps.slice_group_map_type >= 3 &&
      m_pps.slice_group_map_type <= 5)
    slice_group_change_cycle = bitStream.readUE();

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

  //----------- 下面都是一些需要进行额外计算的（文档都有需要自己找）------------
  int SliceGroupChangeRate = m_pps.slice_group_change_rate_minus1 + 1;
  if (m_pps.num_slice_groups_minus1 > 0 && m_pps.slice_group_map_type >= 3 &&
      m_pps.slice_group_map_type <= 5) {
    int32_t temp = m_sps.PicSizeInMapUnits / SliceGroupChangeRate + 1;
    int32_t v = h264_log2(
        temp); // Ceil( Log2( PicSizeInMapUnits ÷ SliceGroupChangeRate + 1 ) );
    slice_group_change_cycle = bitStream.readUn(v); // 2 u(v)
  }

  SliceQPY = 26 + m_pps.pic_init_qp_minus26 + slice_qp_delta;
  QPY_prev = SliceQPY;
  MbaffFrameFlag = (m_sps.mb_adaptive_frame_field_flag && !field_pic_flag);
  PicHeightInMbs = m_sps.frameHeightInMbs / (1 + field_pic_flag);
  PicHeightInSamplesL = PicHeightInMbs * 16;
  PicHeightInSamplesC = PicHeightInMbs * m_sps.MbHeightC;
  PicSizeInMbs = m_sps.PicWidthInMbs * PicHeightInMbs;
  MaxPicNum =
      (field_pic_flag == 0) ? m_sps.MaxFrameNum : (2 * m_sps.MaxFrameNum);
  CurrPicNum = (field_pic_flag == 0) ? frame_num : (2 * frame_num + 1);
  MapUnitsInSliceGroup0 = MIN(slice_group_change_cycle * SliceGroupChangeRate,
                              m_sps.PicSizeInMapUnits);
  QSY = 26 + m_pps.pic_init_qs_minus26 + slice_qs_delta;
  FilterOffsetA = slice_alpha_c0_offset_div2 << 1;
  FilterOffsetB = slice_beta_offset_div2 << 1;

  if (!mapUnitToSliceGroupMap) {
    mapUnitToSliceGroupMap = new int32_t[m_sps.PicSizeInMapUnits]{0};
  }

  if (!MbToSliceGroupMap) {
    MbToSliceGroupMap = new int32_t[PicSizeInMbs]{0};
  }

  setMapUnitToSliceGroupMap();

  setMbToSliceGroupMap();

  set_scaling_lists_values();

  m_is_malloc_mem_self = 1;
  return 0;
}

void SliceHeader::ref_pic_list_mvc_modification(BitStream &bitStream) {}

void SliceHeader::ref_pic_list_modification(BitStream &bitStream) {
  uint32_t modification_of_pic_nums_idc;
  if (slice_type % 5 != SLICE_I && slice_type % 5 != SLICE_SI) {
    ref_pic_list_modification_flag_l0 = bitStream.readU1();
    if (ref_pic_list_modification_flag_l0) {
      do {
        modification_of_pic_nums_idc = bitStream.readUE();
        if (modification_of_pic_nums_idc == 0 ||
            modification_of_pic_nums_idc == 1)
          uint32_t abs_diff_pic_num_minus1 = bitStream.readUE();
        else if (modification_of_pic_nums_idc == 2)
          uint32_t long_term_pic_num = bitStream.readUE();
      } while (modification_of_pic_nums_idc != 3);
    }
  }

  if (slice_type % 5 == SLICE_B) {
    bool ref_pic_list_modification_flag_l1 = bitStream.readU1();
    if (ref_pic_list_modification_flag_l1)
      do {
        modification_of_pic_nums_idc = bitStream.readUE();
        if (modification_of_pic_nums_idc == 0 ||
            modification_of_pic_nums_idc == 1)
          uint32_t abs_diff_pic_num_minus1 = bitStream.readUE();
        else if (modification_of_pic_nums_idc == 2)
          uint32_t long_term_pic_num = bitStream.readUE();
      } while (modification_of_pic_nums_idc != 3);
  }
}

void SliceHeader::pred_weight_table(BitStream &bitStream) {
  luma_log2_weight_denom = bitStream.readUE();
  if (m_sps.ChromaArrayType != 0) {
    chroma_log2_weight_denom = bitStream.readUE();
  }

  for (int i = 0; i <= num_ref_idx_l0_active_minus1; i++) {
    luma_weight_l0[i] = 1 << luma_log2_weight_denom;
    luma_offset_l0[i] = 0;
    bool luma_weight_l0_flag = bitStream.readU1();
    if (luma_weight_l0_flag) {
      luma_weight_l0[i] = bitStream.readSE();
      luma_offset_l0[i] = bitStream.readSE();
    }
    if (m_sps.ChromaArrayType != 0) {
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

  if (slice_type % 5 == SLICE_B) {
    for (int i = 0; i <= num_ref_idx_l1_active_minus1; i++) {
      luma_weight_l1[i] = 1 << luma_log2_weight_denom;
      luma_offset_l1[i] = 0;
      bool luma_weight_l1_flag = bitStream.readU1();
      if (luma_weight_l1_flag) {
        luma_weight_l1[i] = bitStream.readSE();
        luma_offset_l1[i] = bitStream.readSE();
      }

      if (m_sps.ChromaArrayType != 0) {
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
  }
}

void SliceHeader::dec_ref_pic_marking(BitStream &bitStream) {
  if (IdrPicFlag) {
    bool no_output_of_prior_pics_flag = bitStream.readU1();
    bool long_term_reference_flag = bitStream.readU1();
  } else {
    bool adaptive_ref_pic_marking_mode_flag = bitStream.readU1();

    uint32_t memory_management_control_operation;
    if (adaptive_ref_pic_marking_mode_flag) {
      do {
        memory_management_control_operation = bitStream.readUE();
        if (memory_management_control_operation == 1 ||
            memory_management_control_operation == 3)
          uint32_t difference_of_pic_nums_minus1 = bitStream.readUE();
        if (memory_management_control_operation == 2)
          uint32_t long_term_pic_num = bitStream.readUE();
        if (memory_management_control_operation == 3 ||
            memory_management_control_operation == 6)
          uint32_t long_term_frame_idx = bitStream.readUE();
        if (memory_management_control_operation == 4)
          uint32_t max_long_term_frame_idx_plus1 = bitStream.readUE();
      } while (memory_management_control_operation != 0);
    }
  }
}
