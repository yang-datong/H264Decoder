#include "SliceHeader.hpp"
#include "SPS.hpp"
#include "Type.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>

#ifdef DISABLE_COUT
#define cout                                                                   \
  if (false) std::cout
#endif

SliceHeader::~SliceHeader() {
  DELETE(mapUnitToSliceGroupMap);
  DELETE(MbToSliceGroupMap);
}

/* Slice header syntax -> 51 page */
int SliceHeader::parseSliceHeader(BitStream &bitStream, GOP &gop) {
  _bs = &bitStream;

  first_mb_in_slice = _bs->readUE();
  cout << "\tSlice中第一个宏块的索引:" << first_mb_in_slice << endl;

  slice_type = _bs->readUE();
  switch (slice_type) {
  case SLICE_P:
    cout << "\tP Slice" << endl;
    break;
  case SLICE_B:
    cout << "\tB Slice" << endl;
    break;
  case SLICE_I:
    cout << "\tI Slice" << endl;
    break;
  case SLICE_SP:
    cout << "\tSP Slice" << endl;
    break;
  case SLICE_SI:
    cout << "\tSI Slice" << endl;
    break;
  case SLICE_P2:
    cout << "\tP' Slice" << endl;
    break;
  case SLICE_B2:
    cout << "\tB' Slice" << endl;
    break;
  case SLICE_I2:
    cout << "\tI' Slice" << endl;
    break;
  case SLICE_SP2:
    cout << "\tSP' Slice" << endl;
    break;
  case SLICE_SI2:
    cout << "\tSI' Slice" << endl;
    break;
  default:
    cerr << "An error occurred SliceType:" << slice_type << " on "
         << __FUNCTION__ << "():" << __LINE__ << endl;
    return -1;
  }
  slice_type %= 5;

  /* 更新GOP中当前Slice使用的SPS、PPS ID */
  gop.curr_pps_id = pic_parameter_set_id = _bs->readUE();
  gop.curr_sps_id = gop.m_ppss[gop.curr_pps_id].seq_parameter_set_id;
  m_sps = &gop.m_spss[gop.curr_sps_id];
  m_pps = &gop.m_ppss[gop.curr_pps_id];
  cout << "\tPPS ID:" << pic_parameter_set_id << endl;

  if (m_sps->separate_colour_plane_flag) {
    colour_plane_id = _bs->readUn(2);
    cout << "\t颜色平面ID:" << colour_plane_id << endl;
  }

  /* 如果当前图片是IDR图片，frame_num应等于0。 */
  frame_num = _bs->readUn(log2(m_sps->MaxFrameNum));

  cout << "\t当前帧的编号:" << frame_num << endl;
  if (!m_sps->frame_mbs_only_flag) {
    field_pic_flag = _bs->readU1();
    /* NOTE: 对于场编码而言，应进一步判断Table D-1 – Interpretation of pic_struct*/
    cout << "\t场图像标志:" << field_pic_flag << endl;
    if (field_pic_flag) {
      bottom_field_flag = _bs->readU1();
      cout << "\t底场标志:" << bottom_field_flag << endl;
    }
  }
  //7.4.1 NAL unit semantics (7-1)
  IdrPicFlag = ((nal_unit_type == 5) ? 1 : 0);
  if (IdrPicFlag) {
    idr_pic_id = _bs->readUE();
    cout << "\tIDR图像ID:" << idr_pic_id << endl;
  }

  if (m_sps->pic_order_cnt_type == 0) {
    cout << "\t图像顺序计数(POC)方法:使用帧号和帧场号计算";
    pic_order_cnt_lsb =
        _bs->readUn(m_sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    cout << ", 图像顺序计数(POC低位):" << pic_order_cnt_lsb << endl;
    if (m_pps->bottom_field_pic_order_in_frame_present_flag &&
        !field_pic_flag) {
      delta_pic_order_cnt_bottom = _bs->readSE();
      cout << "\t底场的图像顺序计数增量:" << delta_pic_order_cnt_bottom << endl;
    }
  }

  if (m_sps->pic_order_cnt_type == 1 &&
      !m_sps->delta_pic_order_always_zero_flag) {
    cout << "\t图像顺序计数(POC)方法:使用增量计数来计算" << endl;
    delta_pic_order_cnt[0] = _bs->readSE();
    cout << "\t图像顺序计数增量1:" << delta_pic_order_cnt[0] << endl;
    if (m_pps->bottom_field_pic_order_in_frame_present_flag && !field_pic_flag)
      delta_pic_order_cnt[1] = _bs->readSE();
    cout << "\t图像顺序计数增量2:" << delta_pic_order_cnt[1] << endl;
  }

  if (m_sps->pic_order_cnt_type == 2) {
    cout << "\t图像顺序计数(POC)方法:使用帧号(frame_num)来计算" << endl;
    cout << "\t是否存在B帧:0" << endl;
  }

  if (m_pps->redundant_pic_cnt_present_flag) {
    redundant_pic_cnt = _bs->readUE();
    cout << "\t冗余图像计数:" << redundant_pic_cnt << endl;
  }

  /* 对于B Slice的直接预测模式 */
  if (slice_type == SLICE_B) {
    direct_spatial_mv_pred_flag = _bs->readU1();
    cout << "\t直接空间运动矢量预测标志(直接运动预测模式):"
         << direct_spatial_mv_pred_flag << endl;
  }

  /* 动态管理和更新解码器参考帧列表的过程 */
  if (slice_type == SLICE_P || slice_type == SLICE_SP ||
      slice_type == SLICE_B) {
    num_ref_idx_active_override_flag = _bs->readU1();
    cout << "\t覆盖活动参考帧数标志:" << num_ref_idx_active_override_flag
         << endl;

    /* 单个Slice（局部）存在覆盖活动参考帧数标志则使用编码器提供的 参考帧列表活动 参考帧数 */
    if (num_ref_idx_active_override_flag) {
      num_ref_idx_l0_active_minus1 = _bs->readUE();
      if (slice_type == SLICE_B) {
        num_ref_idx_l1_active_minus1 = _bs->readUE();
        cout << "\t参考帧列表0的活动参考帧数:"
             << num_ref_idx_l0_active_minus1 + 1
             << ", 参考帧列表1的活动参考帧数:"
             << num_ref_idx_l1_active_minus1 + 1 << endl;
      }
      /* 编码器未提供则使用PPS（全局）中提供的 */
    } else
      num_ref_idx_l0_active_minus1 =
          m_pps->num_ref_idx_l0_default_active_minus1;
  }

  /* 读取编码器中传递出来的用于调整解码器的参考图片列表的参数或具体值 */
  if (nal_unit_type == 20 || nal_unit_type == 21)
    /* TODO:specified in Annex H */
    ref_pic_list_mvc_modification();
  else
    ref_pic_list_modification();

  /* 读取加权预测中使用的权重因子表，从编码器中
   * 1. P帧满足加权预测
   * 2. B帧满足双向加权预测*/
  if ((m_pps->weighted_pred_flag &&
       (slice_type == SLICE_P || slice_type == SLICE_SP)) ||
      (m_pps->weighted_bipred_idc == 1 && slice_type == SLICE_B))
    pred_weight_table();

  /* 该Slice不会被作为参考帧，被其他帧参考预测 */
  if (nal_ref_idc != 0) dec_ref_pic_marking();

  if (m_pps->entropy_coding_mode_flag && slice_type != SLICE_I &&
      slice_type != SLICE_SI) {
    cabac_init_idc = _bs->readUE();
    cout << "\tCABAC初始化索引:" << cabac_init_idc << endl;
  }

  slice_qp_delta = _bs->readSE();
  if (slice_type == SLICE_SP || slice_type == SLICE_SI) {
    if (slice_type == SLICE_SP) {
      sp_for_switch_flag = _bs->readU1();
      cout << "\tSP切换标志:" << sp_for_switch_flag << endl;
      /* TODO YangJing 未实现 <24-09-15 13:24:02> */
      cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
           << endl;
    }
    /* qs 是专门用于 SP Slice 和 SI Slice 的量化参数。它类似于 qp */
    slice_qs_delta = _bs->readSE();
  }

  if (m_pps->deblocking_filter_control_present_flag) {
    disable_deblocking_filter_idc = _bs->readUE();
    if (disable_deblocking_filter_idc == 0)
      cout << "\t去块效应滤波器:开启" << endl;
    else if (disable_deblocking_filter_idc == 1)
      cout << "\t去块效应滤波器:禁用" << endl;
    else if (disable_deblocking_filter_idc == 2)
      cout << "\t去块效应滤波器:"
              "只应用于slice内部的宏块边缘，不应用于slice边界的宏块边缘"
           << endl;
    else
      direct_spatial_mv_pred_flag = 1;
    if (disable_deblocking_filter_idc != 1) {
      slice_alpha_c0_offset_div2 = _bs->readSE();
      slice_beta_offset_div2 = _bs->readSE();
      cout << "\t去块效应滤波器的Alpha偏移值:" << slice_alpha_c0_offset_div2
           << ", 去块效应滤波器的Beta偏移值:" << slice_beta_offset_div2 << endl;
    }
  }

  /* 当启用了 多切片组，且 Slice Group 的映射类型是 3、4 或 5 时，读取宏块在不同切片组之间的切换周期（slice_group_map_type 3 到 5：这些模式允许更灵活的分割） */
  //NOTE: 具体的FOM机制是在SliceData中实现的
  const int32_t SliceGroupChangeRate =
      m_pps->slice_group_change_rate_minus1 + 1;
  if (m_pps->num_slice_groups_minus1 > 0 && m_pps->slice_group_map_type >= 3 &&
      m_pps->slice_group_map_type <= 5) {
    /* Ceil( Log2( PicSizeInMapUnits ÷ SliceGroupChangeRate + 1 ) ) -> (7-35) -> page 92 */
    int32_t v = log2(m_sps->PicSizeInMapUnits / SliceGroupChangeRate + 1);
    slice_group_change_cycle = _bs->readUn(v);
  }

  //----------- 下面都是一些额外信息，比如还原偏移，或者事先计算一些值，后面方便用 ------------
  MbaffFrameFlag = (m_sps->mb_adaptive_frame_field_flag && !field_pic_flag);
  cout << "\t宏块自适应帧场模式(MBAFF):" << MbaffFrameFlag << endl;

  PicWidthInMbs = m_sps->PicWidthInMbs;
  PicHeightInMbs = m_sps->FrameHeightInMbs / (1 + field_pic_flag);
  PicSizeInMbs = PicWidthInMbs * PicHeightInMbs;
  cout << "\tLuma 水平方向宏块数:" << PicWidthInMbs
       << ", Luma 垂直方向宏块数:" << PicHeightInMbs
       << ", Luma 总宏块数:" << PicSizeInMbs << endl;

  /* TODO YangJing  <24-09-17 16:36:13> */
  //cout << "\tChroma 水平方向宏块数:" << PicWidthInMbs
  //<< ", Chroma 垂直方向宏块数:" << PicHeightInMbs
  //<< ", Chroma 总宏块数:" << PicSizeInMbs << endl;

  /* 计算采样宽、高度和比特深度（Luma） */
  PicWidthInSamplesL = PicWidthInMbs * 16;
  PicHeightInSamplesL = PicHeightInMbs * 16;
  cout << "\tCoded Width(Luma):" << PicWidthInSamplesL
       << ", Coded Height(Luma):" << PicHeightInSamplesL << endl;

  /* 计算采样宽、高度和比特深度（Chrome） */
  PicWidthInSamplesC = PicWidthInMbs * m_sps->MbWidthC;
  PicHeightInSamplesC = PicHeightInMbs * m_sps->MbHeightC;
  cout << "\tCoded Width(Chroma):" << PicWidthInSamplesC
       << ", Coded Height(Chroma):" << PicHeightInSamplesC << endl;

  MaxPicNum = (!field_pic_flag) ? m_sps->MaxFrameNum : (2 * m_sps->MaxFrameNum);

  CurrPicNum = (!field_pic_flag) ? frame_num : (2 * frame_num + 1);
  cout << "\t最大图像编号:" << MaxPicNum << ",当前图像编号:" << CurrPicNum
       << endl;

  /* SliceGroupChangeRate 切片组变化的速率，slice_group_change_cycle 切片组的切换周期，在某个特定时间内，Slice Group 0（通常是第一个切片组）包含的宏块数量 */
  MapUnitsInSliceGroup0 = MIN(slice_group_change_cycle * SliceGroupChangeRate,
                              m_sps->PicSizeInMapUnits);
  cout << "\t首个Slice Group中的宏块数:" << MapUnitsInSliceGroup0 << endl;

  SliceQPY = 26 + m_pps->pic_init_qp_minus26 + slice_qp_delta;
  cout << "\tSlice的量化参数:" << SliceQPY << endl;
  /* 对于首个Slice而言前一个Slice的量化参数应该初始化为当前量化参数，而不是0 */
  QPY_prev = SliceQPY;
  QSY = 26 + m_pps->pic_init_qs_minus26 + slice_qs_delta;
  cout << "\tSP Slice的量化参数:" << QSY << endl;

  FilterOffsetA = slice_alpha_c0_offset_div2 << 1;
  FilterOffsetB = slice_beta_offset_div2 << 1;
  cout << "\t去块效应滤波器的A偏移值:" << FilterOffsetA
       << ",去块效应滤波器的B偏移值:" << FilterOffsetB << endl;

  if (!mapUnitToSliceGroupMap)
    mapUnitToSliceGroupMap = new int32_t[m_sps->PicSizeInMapUnits]{0};

  cout << "\t映射单元到Slice Group的映射表(size):" << m_sps->PicSizeInMapUnits
       << endl;

  if (!MbToSliceGroupMap) MbToSliceGroupMap = new int32_t[PicSizeInMbs]{0};

  set_scaling_lists_values();
  printf_scaling_lists_values();
  //m_is_malloc_mem_self = 1;
  return 0;
}

void SliceHeader::ref_pic_list_mvc_modification() { /* specified in Annex H */ }

/* 调整解码器的参考图片列表，根据当前视频帧的特定需求调整参考帧的使用顺序或选择哪些帧作为参考 */
void SliceHeader::ref_pic_list_modification() {
  /* P帧或B帧，这些帧类型需要参考其他帧来进行编码 */
  if (slice_type != SLICE_I && slice_type != SLICE_SI) {

    ref_pic_list_modification_flag_l0 = _bs->readU1();
    if (ref_pic_list_modification_flag_l0) {
      /* 前参考需要修改 */
      int i = 0;
      do {
        /* 读取操作码，它决定了如何修改参考图片列表 */
        modification_of_pic_nums_idc[0][i] = _bs->readUE();
        if ((modification_of_pic_nums_idc[0][i] & ~1) == 0)
          /* 修改当前列表中的图片编号，abs_diff_pic_num_minus1用来计算实际的图片编号差异 */
          abs_diff_pic_num_minus1[0][i] = _bs->readUE();
        else if (modification_of_pic_nums_idc[0][i] == 2)
          /* 使用长期参考帧，其编号由long_term_pic_num给出 */
          long_term_pic_num[0][i] = _bs->readUE();
        i++;
      } while (modification_of_pic_nums_idc[0][i - 1] != 3);
      /* 遇到操作码3，表示修改结束 */

      /* 修改的个数 For 列表0 */
      ref_pic_list_modification_count_l0 = i;
    }
  }

  /* 同上，对于B帧双向参考而言，需要对后参考进行同样修改 */
  if (slice_type == SLICE_B) {
    ref_pic_list_modification_flag_l1 = _bs->readU1();
    if (ref_pic_list_modification_flag_l1) {
      /* 后参考需要修改 */
      int i = 0;
      do {
        modification_of_pic_nums_idc[1][i] = _bs->readUE();
        if (modification_of_pic_nums_idc[1][i] == 0 ||
            modification_of_pic_nums_idc[1][i] == 1)
          abs_diff_pic_num_minus1[1][i] = _bs->readUE();
        else if (modification_of_pic_nums_idc[1][i] == 2)
          long_term_pic_num[1][i] = _bs->readUE();
        i++;
      } while (modification_of_pic_nums_idc[1][i - 1] != 3);

      /* 修改的个数 For 列表1 */
      ref_pic_list_modification_count_l1 = i;
    }
  }
}

/* 读取编码器传递的加权预测中使用的权重表。权重表包括了每个参考帧的权重因子，这些权重因子会被应用到运动补偿的预测过程中。*/
void SliceHeader::pred_weight_table() {
  cout << "\t加权预测权重因子 -> {" << endl;
  /* Luma */
  luma_log2_weight_denom = _bs->readUE();
  /* Chrome */
  if (m_sps->ChromaArrayType != 0) chroma_log2_weight_denom = _bs->readUE();
  cout << "\t\t亮度权重的对数基数:" << luma_log2_weight_denom
       << ", 色度权重的对数基数:" << chroma_log2_weight_denom << endl;

  for (int i = 0; i <= (int)num_ref_idx_l0_active_minus1; i++) {
    /* 初始化 */
    luma_weight_l0[i] = 1 << luma_log2_weight_denom;

    /* Luma */
    bool luma_weight_l0_flag = _bs->readU1();
    if (luma_weight_l0_flag) {
      luma_weight_l0[i] = _bs->readSE();
      luma_offset_l0[i] = _bs->readSE();
    }

    if (m_sps->ChromaArrayType != 0) {
      /* 初始化 */
      chroma_weight_l0[i][0] = 1 << chroma_log2_weight_denom;
      chroma_weight_l0[i][1] = 1 << chroma_log2_weight_denom;

      /* Cb,Cr*/
      bool chroma_weight_l0_flag = _bs->readU1();
      if (chroma_weight_l0_flag) {
        for (int j = 0; j < 2; j++) {
          chroma_weight_l0[i][j] = _bs->readSE();
          chroma_offset_l0[i][j] = _bs->readSE();
        }
      }
    }
  }

  for (uint32_t i = 0; i <= num_ref_idx_l0_active_minus1; ++i) {
    cout << "\t\t前参考帧列表[" << i << "] -> {"
         << "Luma权重:" << luma_weight_l0[i]
         << ",Luma偏移:" << luma_offset_l0[i]
         << ",Cb权重:" << chroma_weight_l0[i][0]
         << ",Cr权重:" << chroma_weight_l0[i][1]
         << ",Cb偏移:" << chroma_offset_l0[i][0]
         << ",Cr偏移:" << chroma_offset_l0[i][1] << "}" << endl;
  }

  if (slice_type == SLICE_B) {
    for (int i = 0; i <= (int)num_ref_idx_l1_active_minus1; i++) {
      /* 初始化 */
      luma_weight_l1[i] = 1 << luma_log2_weight_denom;

      /* Luma */
      bool luma_weight_l1_flag = _bs->readU1();
      if (luma_weight_l1_flag) {
        luma_weight_l1[i] = _bs->readSE();
        luma_offset_l1[i] = _bs->readSE();
      }

      if (m_sps->ChromaArrayType != 0) {
        /* 初始化 */
        chroma_weight_l1[i][0] = 1 << chroma_log2_weight_denom;
        chroma_weight_l1[i][1] = 1 << chroma_log2_weight_denom;

        /* Cb,Cr*/
        bool chroma_weight_l1_flag = _bs->readU1();
        if (chroma_weight_l1_flag) {
          for (int j = 0; j < 2; j++) {
            chroma_weight_l1[i][j] = _bs->readSE();
            chroma_offset_l1[i][j] = _bs->readSE();
          }
        }
      }
    }

    for (uint32_t i = 0; i <= num_ref_idx_l1_active_minus1; ++i) {
      cout << "\t\t后参考帧列表[" << i << "] -> {"
           << "Luma权重:" << luma_weight_l1[i]
           << ",Luma偏移:" << luma_offset_l1[i]
           << ",Cb权重:" << chroma_weight_l1[i][0]
           << ",Cr权重:" << chroma_weight_l1[i][1]
           << ",Cb偏移:" << chroma_offset_l1[i][0]
           << ",Cr偏移:" << chroma_offset_l1[i][1] << "}" << endl;
    }
  }
  cout << "\t}" << endl;
}

void SliceHeader::dec_ref_pic_marking() {
  if (IdrPicFlag) {
    /* IDR图片，需要重新读取如下字段：
     * 1. 解码器是否应该输出之前的图片
     * 2. 当前图片是否被标记为长期参考图片*/
    no_output_of_prior_pics_flag = _bs->readU1();
    long_term_reference_flag = _bs->readU1();
  } else {
    /* 非IDR帧 */
    adaptive_ref_pic_marking_mode_flag = _bs->readU1();
    if (adaptive_ref_pic_marking_mode_flag) {
      cout << "\t参考帧管理机制:自适应内存控制" << endl;
      /* 自适应参考图片标记模式 */
      uint32_t index = 0;
      do {
        if (index > 31) {
          cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
               << endl;
          break;
        }
        /* 处理多种内存管理控制操作（MMCO），指示如何更新解码器的参考图片列表 */
        int32_t &mmco =
            m_dec_ref_pic_marking[index].memory_management_control_operation;
        mmco = _bs->readUE();

        /* 调整参考图片编号的差异 */
        if (mmco == 1 || mmco == 3)
          m_dec_ref_pic_marking[index].difference_of_pic_nums_minus1 =
              _bs->readUE();
        /* 标记某个图片为长期参考 */
        if (mmco == 2)
          m_dec_ref_pic_marking[index].long_term_pic_num_2 = _bs->readUE();
        /* 设定或更新长期帧索引 */
        if (mmco == 3 || mmco == 6)
          m_dec_ref_pic_marking[index].long_term_frame_idx = _bs->readUE();
        /* 设置最大长期帧索引 */
        if (mmco == 4)
          m_dec_ref_pic_marking[index].max_long_term_frame_idx_plus1 =
              _bs->readUE();

        index++;
      } while (
          m_dec_ref_pic_marking[index - 1].memory_management_control_operation);
      dec_ref_pic_marking_count = index;
    } else
      cout << "\t参考帧管理机制:滑动窗口机制" << endl;
  }
}

int SliceHeader::set_scaling_lists_values() {
  const int32_t scaling_list_size = (m_sps->chroma_format_idc != 3) ? 8 : 12;

  if (m_sps->seq_scaling_matrix_present_flag == 0 &&
      m_pps->pic_scaling_matrix_present_flag == 0) {
    // 如果编码器未给出缩放矩阵值，则缩放矩阵值全部默认为16
    fill_n(&ScalingList4x4[0][0], 6 * 16, 16u);
    fill_n(&ScalingList8x8[0][0], 6 * 64, 16u);
    /* NOTE:这里使用H264的默认矩阵，会有问题，在默认情况下可能编码器就是使用的16量化值 <24-09-15 20:02:42> */
  } else {
    /* PPS中存在缩放矩阵则使用（PPS缩放矩阵优先级更高） */
    if (m_pps->pic_scaling_matrix_present_flag)
      pic_scaling_matrix(scaling_list_size);
    else if (m_sps->seq_scaling_matrix_present_flag)
      /* 反之，使用SPS中默认的存在缩放矩阵 */
      seq_scaling_matrix(scaling_list_size);
  }

  return 0;
}

//Table 7-2 Scaling list fall-back rule A
int SliceHeader::seq_scaling_matrix(int32_t scaling_list_size) {
  for (int32_t i = 0; i < scaling_list_size; i++) {
    /* 4x4 的缩放矩阵 */
    if (i < 6) {
      /* 当前SPS未显式提供，需要使用默认或之前的矩阵 */
      if (m_sps->seq_scaling_list_present_flag[i] == 0) {
        if (i == 0)
          memcpy(ScalingList4x4[i], Default_4x4_Intra,
                 sizeof(Default_4x4_Intra));
        else if (i == 3)
          memcpy(ScalingList4x4[i], Default_4x4_Inter,
                 sizeof(Default_4x4_Inter));
        else
          /* 如果不使用默认矩阵，则复制前一个矩阵的值 */
          memcpy(ScalingList4x4[i], ScalingList4x4[i - 1],
                 sizeof(ScalingList4x4[i]));

        /* 表示当前矩阵已提供，但是否应使用默认的缩放矩阵 */
      } else if (m_pps->UseDefaultScalingMatrix4x4Flag[i]) {
        if (i < 3)
          memcpy(ScalingList4x4[i], Default_4x4_Intra,
                 sizeof(Default_4x4_Intra));
        else
          memcpy(ScalingList4x4[i], Default_4x4_Inter,
                 sizeof(Default_4x4_Inter));
      }

      // 采用SPS中传送过来的量化系数的缩放值
      else
        memcpy(ScalingList4x4[i], m_sps->ScalingList4x4[i],
               sizeof(ScalingList4x4[i]));

      /* 对于8x8矩阵 */
    } else {

      /* 当前SPS未显式提供，需要使用默认或之前的矩阵 */
      if (m_sps->seq_scaling_list_present_flag[i] == 0) {
        if (i == 6)
          memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                 sizeof(Default_8x8_Intra));
        else if (i == 7)
          memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                 sizeof(Default_8x8_Inter));
        else
          /* 如果不使用默认矩阵，则复制前两个矩阵的值 */
          memcpy(ScalingList8x8[i - 6], ScalingList8x8[i - 8],
                 sizeof(ScalingList8x8[i - 6]));

        /* 表示当前矩阵已提供，但是否应使用默认的缩放矩阵 */
      } else if (m_pps->UseDefaultScalingMatrix8x8Flag[i - 6]) {
        if (i == 6 || i == 8 || i == 10)
          memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                 sizeof(Default_8x8_Intra));
        else
          memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                 sizeof(Default_8x8_Inter));
      }

      // 采用SPS中传送过来的量化系数的缩放值
      else
        memcpy(ScalingList8x8[i - 6], m_sps->ScalingList8x8[i - 6],
               sizeof(ScalingList8x8[i - 6]));
    }
  }

  return 0;
}

//Table 7-2 Scaling list fall-back rule A
int SliceHeader::pic_scaling_matrix(int32_t scaling_list_size) {
  for (int32_t i = 0; i < scaling_list_size; i++) {
    /* 4x4 的缩放矩阵 */
    if (i < 6) {
      /* 当前PPS未显式提供，需要使用默认或之前的矩阵 */
      if (m_pps->pic_scaling_list_present_flag[i] == 0) {
        if (i == 0) {
          if (m_sps->seq_scaling_matrix_present_flag == 0)
            memcpy(ScalingList4x4[i], Default_4x4_Intra,
                   sizeof(Default_4x4_Intra));
        } else if (i == 3) {
          if (m_sps->seq_scaling_matrix_present_flag == 0)
            memcpy(ScalingList4x4[i], Default_4x4_Inter,
                   sizeof(Default_4x4_Inter));
        } else
          /* 如果不使用默认矩阵，则复制前一个矩阵的值 */
          memcpy(ScalingList4x4[i], ScalingList4x4[i - 1],
                 sizeof(ScalingList4x4[i]));

        /* 表示当前矩阵已提供，但是否应使用默认的缩放矩阵 */
      } else if (m_pps->UseDefaultScalingMatrix4x4Flag[i]) {
        if (i < 3)
          memcpy(ScalingList4x4[i], Default_4x4_Intra,
                 sizeof(Default_4x4_Intra));
        else
          memcpy(ScalingList4x4[i], Default_4x4_Inter,
                 sizeof(Default_4x4_Inter));

        // 采用PPS中传送过来的量化系数的缩放值
      } else
        memcpy(ScalingList4x4[i], m_pps->ScalingList4x4[i],
               sizeof(ScalingList4x4[i]));

      /* 8x8 的缩放矩阵 */
    } else {

      /* 当前PPS未显式提供，需要使用默认或之前的矩阵 */
      if (m_pps->pic_scaling_list_present_flag[i] == 0) {
        if (i == 6) {
          if (m_sps->seq_scaling_matrix_present_flag == 0)
            memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                   sizeof(Default_8x8_Intra));
        } else if (i == 7) {
          if (m_sps->seq_scaling_matrix_present_flag == 0)
            memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                   sizeof(Default_8x8_Inter));
        } else
          /* 如果不使用默认矩阵，则复制前两个矩阵的值 */
          memcpy(ScalingList8x8[i - 6], ScalingList8x8[i - 8],
                 sizeof(ScalingList8x8[i - 6]));

        /* 表示当前矩阵已提供，但是否应使用默认的缩放矩阵 */
      } else if (m_pps->UseDefaultScalingMatrix8x8Flag[i - 6]) {
        if (i == 6 || i == 8 || i == 10)
          memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                 sizeof(Default_8x8_Intra));
        else
          memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                 sizeof(Default_8x8_Inter));

        // 采用PPS中传送过来的量化系数的缩放值
      } else
        memcpy(ScalingList8x8[i - 6], m_pps->ScalingList8x8[i - 6],
               sizeof(ScalingList8x8[i - 6]));
    }
  }
  return 0;
}

#include <iomanip> //用于格式化输出
void SliceHeader::printf_scaling_lists_values() {
  //ScalingList4x4[6][16]
  uint8_t row = 4, clo = 4;
  cout << "\tScalingList4x4 -> {" << endl;
  for (int index = 0; index < 1; ++index) {
    for (int i = 0; i < row; ++i) {
      cout << "\t\t|";
      for (int j = 0; j < clo; ++j)
        cout << setw(3) << (int)ScalingList4x4[index][row * i + j];
      cout << " |" << endl;
    }
  }
  cout << "\t}" << endl;

  //ScalingList8x8[6][64]
  row = 8, clo = 8;
  cout << "\tScalingList8x8 -> {" << endl;
  for (int index = 0; index < 1; ++index) {
    for (int i = 0; i < row; ++i) {
      cout << "\t\t|";
      for (int j = 0; j < clo; ++j)
        cout << setw(3) << (int)ScalingList8x8[index][row * i + j];
      cout << " |" << endl;
    }
  }
  cout << "\t}" << endl;
}
