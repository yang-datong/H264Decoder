#include "SliceHeader.hpp"
#include "BitStream.hpp"
#include "SPS.hpp"
#include "Type.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>

SliceHeader::~SliceHeader() {
  FREE(mapUnitToSliceGroupMap);
  FREE(MbToSliceGroupMap);
}

int SliceHeader::st_ref_pic_set(BitStream *bs, int32_t stRpsIdx) {
  if (stRpsIdx != 0) inter_ref_pic_set_prediction_flag = bs->readUn(1);
  if (inter_ref_pic_set_prediction_flag) {
    if (stRpsIdx == m_sps->num_short_term_ref_pic_sets)
      delta_idx_minus1 = bs->readUE();
    delta_rps_sign = bs->readUn(1);
    abs_delta_rps_minus1 = bs->readUE();
    RefRpsIdx = stRpsIdx - (delta_idx_minus1 + 1);
    deltaRps = (1 - 2 * delta_rps_sign) * (abs_delta_rps_minus1 + 1);
    for (int32_t j = 0; j <= NumDeltaPocs[RefRpsIdx]; j++) {
      used_by_curr_pic_flag[j] = bs->readUn(1);
      if (!used_by_curr_pic_flag[j]) use_delta_flag[j] = bs->readUn(1);
    }
  } else {

    //used_by_curr_pic_s1_flag
    num_negative_pics = bs->readUE();
    NumNegativePics[stRpsIdx] = num_negative_pics;
    num_positive_pics = bs->readUE();
    NumPositivePics[stRpsIdx] = num_positive_pics;
    NumDeltaPocs[stRpsIdx] =
        NumNegativePics[stRpsIdx] + NumPositivePics[stRpsIdx]; //(7-71)

    for (int32_t i = 0; i < num_negative_pics; i++) {
      delta_poc_s0_minus1[i] = bs->readUE();
      UsedByCurrPicS0[stRpsIdx][i] = used_by_curr_pic_s0_flag[i] =
          bs->readUn(1);
      if (i == 0) {
        DeltaPocS0[stRpsIdx][i] = -(delta_poc_s0_minus1[i] + 1);
      } else {
        DeltaPocS0[stRpsIdx][i] =
            DeltaPocS0[stRpsIdx][i - 1] - (delta_poc_s0_minus1[i] + 1);
      }
    }
    for (int32_t i = 0; i < num_positive_pics; i++) {
      delta_poc_s1_minus1[i] = bs->readUE();
      UsedByCurrPicS1[stRpsIdx][i] = used_by_curr_pic_s1_flag[i] =
          bs->readUn(1);
      if (i == 0) {
        DeltaPocS1[stRpsIdx][i] = delta_poc_s1_minus1[i] + 1;
      } else {
        DeltaPocS1[stRpsIdx][i] =
            DeltaPocS1[stRpsIdx][i - 1] + (delta_poc_s1_minus1[i] + 1);
      }
    }
  }

  // use_delta_flag
  int i = 0, j = 0;
  for (j = NumPositivePics[RefRpsIdx] - 1; j >= 0; j--) {
    int dPoc = DeltaPocS1[RefRpsIdx][j] + deltaRps;
    if (dPoc < 0 && use_delta_flag[NumNegativePics[RefRpsIdx] + j]) {
      DeltaPocS0[stRpsIdx][i] = dPoc;
      UsedByCurrPicS0[stRpsIdx][i++] =
          used_by_curr_pic_flag[NumNegativePics[RefRpsIdx] + j];
    }
  }
  if (deltaRps < 0 && use_delta_flag[NumDeltaPocs[RefRpsIdx]]) {
    DeltaPocS0[stRpsIdx][i] = deltaRps;
    UsedByCurrPicS0[stRpsIdx][i++] =
        used_by_curr_pic_flag[NumDeltaPocs[RefRpsIdx]];
  }
  for (j = 0; j < NumNegativePics[RefRpsIdx]; j++) {
    int dPoc = DeltaPocS0[RefRpsIdx][j] + deltaRps;
    if (dPoc < 0 && use_delta_flag[j]) {
      DeltaPocS0[stRpsIdx][i] = dPoc;
      UsedByCurrPicS0[stRpsIdx][i++] = used_by_curr_pic_flag[j];
    }
  }
  NumNegativePics[stRpsIdx] = i;
  i = 0;
  for (j = NumNegativePics[RefRpsIdx] - 1; j >= 0; j--) {
    int dPoc = DeltaPocS0[RefRpsIdx][j] + deltaRps;
    if (dPoc > 0 && use_delta_flag[j]) {
      DeltaPocS1[stRpsIdx][i] = dPoc;
      UsedByCurrPicS1[stRpsIdx][i++] = used_by_curr_pic_flag[j];
    }
  }
  if (deltaRps > 0 && use_delta_flag[NumDeltaPocs[RefRpsIdx]]) {
    DeltaPocS1[stRpsIdx][i] = deltaRps;
    UsedByCurrPicS1[stRpsIdx][i++] =
        used_by_curr_pic_flag[NumDeltaPocs[RefRpsIdx]];
  }
  for (j = 0; j < NumPositivePics[RefRpsIdx]; j++) {
    int dPoc = DeltaPocS1[RefRpsIdx][j] + deltaRps;
    if (dPoc > 0 && use_delta_flag[NumNegativePics[RefRpsIdx] + j]) {
      DeltaPocS1[stRpsIdx][i] = dPoc;
      UsedByCurrPicS1[stRpsIdx][i++] =
          used_by_curr_pic_flag[NumNegativePics[RefRpsIdx] + j];
    }
  }
  NumPositivePics[stRpsIdx] = i;

  return 0;
}

#define IS_IRAP(nal_unit_type)                                                 \
  (nal_unit_type >= HEVC_NAL_BLA_W_LP &&                                       \
   nal_unit_type <= HEVC_NAL_RSV_IRAP_VCL23)
#define IS_IDR(nal_unit_type)                                                  \
  (nal_unit_type == HEVC_NAL_IDR_W_RADL || nal_unit_type == HEVC_NAL_IDR_N_LP)
/* Slice header syntax -> 51 page */
int SliceHeader::slice_segment_header(BitStream &bitStream, GOP &gop) {
  _bs = &bitStream;

  first_slice_segment_in_pic_flag = _bs->readU1();
  cout << "\tSlice中第一个宏块的索引:" << first_slice_segment_in_pic_flag
       << endl;

  if (IS_IRAP(nal_unit_type)) no_output_of_prior_pics_flag = _bs->readU1();

  slice_pic_parameter_set_id = _bs->readUE();
  m_pps = &gop.m_ppss[slice_pic_parameter_set_id];
  m_sps = &gop.m_spss[m_pps->seq_parameter_set_id];

  dependent_slice_segment_flag = false;
  if (!first_slice_segment_in_pic_flag) {
    if (m_pps->dependent_slice_segments_enabled_flag)
      dependent_slice_segment_flag = _bs->readU1();
    slice_segment_address = _bs->readUn(CEIL(LOG2(m_sps->PicSizeInCtbsY)));
  }
  if (dependent_slice_segment_flag == false) {
    SliceAddrRs = slice_segment_address;
  } else {
    /* TODO YangJing SliceData? <24-10-21 13:52:37> */
    //SliceAddrRs = CtbAddrTsToRs[CtbAddrRsToTs[slice_segment_address] - 1];
  }

  CuQpDeltaVal = 0;
  if (!dependent_slice_segment_flag) {
    for (int32_t i = 0; i < m_pps->num_extra_slice_header_bits; i++)
      _bs->readUn(1);
    slice_type = _bs->readUE();
    //if (IS_IRAP(nal_unit_type) && nuh_layer_id == 0 && m_pps->pps_curr_pic_ref_enabled_flag == 0) {
    //slice_type = HEVC_SLICE_I;
    //}
    switch (slice_type) {
    case HEVC_SLICE_B:
      cout << "\tB Slice" << endl;
      break;
    case HEVC_SLICE_P:
      cout << "\tP Slice" << endl;
      break;
    case HEVC_SLICE_I:
      cout << "\tI Slice" << endl;
      break;
    default:
      cerr << "An error occurred SliceType:" << slice_type << " on "
           << __FUNCTION__ << "():" << __LINE__ << endl;
      return -1;
    }
    if (m_pps->output_flag_present_flag) pic_output_flag = _bs->readUn(1);
    // color_plane_id 值 0、1 和 2 分别对应于 Y、Cb 和 Cr 平面
    if (m_sps->separate_colour_plane_flag == 1)
      colour_plane_id = _bs->readUn(2);

    if (IS_IDR(nal_unit_type) == false) {
      slice_pic_order_cnt_lsb = _bs->readUn(m_sps->log2_max_pic_order_cnt_lsb);
      short_term_ref_pic_set_sps_flag = _bs->readUn(1);
      if (!short_term_ref_pic_set_sps_flag)
        st_ref_pic_set(_bs, m_sps->num_short_term_ref_pic_sets);
      else if (m_sps->num_short_term_ref_pic_sets > 1)
        short_term_ref_pic_set_idx =
            _bs->readUn(LOG2(m_sps->num_short_term_ref_pic_sets));
      if (m_sps->long_term_ref_pics_present_flag) {
        if (m_sps->num_long_term_ref_pics_sps > 0)
          num_long_term_sps = _bs->readUE();
        num_long_term_pics = _bs->readUE();
        for (int32_t i = 0; i < num_long_term_sps + num_long_term_pics; i++) {
          if (i < num_long_term_sps) {
            if (m_sps->num_long_term_ref_pics_sps > 1) {
              lt_idx_sps[i] =
                  _bs->readUn(LOG2(m_sps->num_long_term_ref_pics_sps));
            }
          } else {
            poc_lsb_lt[i] = _bs->readUn(m_sps->log2_max_pic_order_cnt_lsb);
            // used_by_curr_pic_lt_flag[i]等于0指定当前图片的长期RPS中的第i个条目不被当前图片参考
            used_by_curr_pic_lt_flag[i] = _bs->readUn(1);
            // NOTE: Add
            if (i < num_long_term_sps) {
              PocLsbLt[i] = m_sps->lt_ref_pic_poc_lsb_sps[lt_idx_sps[i]];
              UsedByCurrPicLt[i] =
                  m_sps->used_by_curr_pic_lt_sps_flag[lt_idx_sps[i]];
            } else {
              PocLsbLt[i] = poc_lsb_lt[lt_idx_sps[i]];
              UsedByCurrPicLt[i] = used_by_curr_pic_lt_flag[lt_idx_sps[i]];
            }
          }
          /* TODO YangJing (Rec. ITU-T H.265 (V10) (07/2024) 97)When there is more than one value in setOfPrevPocVals for which the value modulo MaxPicOrderCntLsb is equal to PocLsbLt[ i ], delta_poc_msb_present_flag[ i ] shall be equal to 1. <24-10-23 08:32:28> */

          delta_poc_msb_present_flag[i] = _bs->readUn(1);
          if (delta_poc_msb_present_flag[i])
            delta_poc_msb_cycle_lt[i] = _bs->readUE();

          if (i == 0 || i == num_long_term_sps)
            DeltaPocMsbCycleLt[i] = delta_poc_msb_cycle_lt[i];
          else
            DeltaPocMsbCycleLt[i] =
                delta_poc_msb_cycle_lt[i] + DeltaPocMsbCycleLt[i - 1];
        }
      }
      if (m_sps->sps_temporal_mvp_enabled_flag)
        slice_temporal_mvp_enabled_flag = _bs->readUn(1);
    }
    if (m_sps->sample_adaptive_offset_enabled_flag) {
      slice_sao_luma_flag = _bs->readUn(1);
      if (m_sps->ChromaArrayType != 0) slice_sao_chroma_flag = _bs->readUn(1);
    }

    if (slice_type == HEVC_SLICE_P || slice_type == HEVC_SLICE_B) {
      num_ref_idx_active_override_flag = _bs->readUn(1);
      if (num_ref_idx_active_override_flag) {
        num_ref_idx_l0_active_minus1 = _bs->readUE();
        if (slice_type == HEVC_SLICE_B)
          num_ref_idx_l1_active_minus1 = _bs->readUE();
      }
      int32_t nb_refs = NumPicTotalCurr;
      if (m_pps->lists_modification_present_flag && nb_refs > 1)
        ref_pic_lists_modification(_bs, nb_refs);
      if (slice_type == HEVC_SLICE_B) mvd_l1_zero_flag = _bs->readUn(1);
      if (m_pps->cabac_init_present_flag) cabac_init_flag = _bs->readUn(1);
      if (slice_temporal_mvp_enabled_flag) {
        if (slice_type == HEVC_SLICE_B)
          collocated_from_l0_flag = _bs->readUn(1);
        if ((collocated_from_l0_flag && num_ref_idx_l0_active_minus1 > 0) ||
            (!collocated_from_l0_flag && num_ref_idx_l1_active_minus1 > 0))
          collocated_ref_idx = _bs->readUE();
      }
      if ((weighted_pred_flag && slice_type == HEVC_SLICE_P) ||
          (weighted_bipred_flag && slice_type == HEVC_SLICE_B))
        pred_weight_table();
      five_minus_max_num_merge_cand = _bs->readUE();
      if (motion_vector_resolution_control_idc == 2)
        use_integer_mv_flag = _bs->readUn(1);
    }
    slice_qp_delta = _bs->readSE();
    if (m_pps->pps_slice_chroma_qp_offsets_present_flag) {
      slice_cb_qp_offset = _bs->readSE();
      slice_cr_qp_offset = _bs->readSE();
    }
    if (m_pps->pps_slice_act_qp_offsets_present_flag) {
      slice_act_y_qp_offset = _bs->readSE();
      slice_act_cb_qp_offset = _bs->readSE();
      slice_act_cr_qp_offset = _bs->readSE();
    }
    if (m_pps->chroma_qp_offset_list_enabled_flag)
      cu_chroma_qp_offset_enabled_flag = _bs->readUn(1);
    if (m_pps->deblocking_filter_override_enabled_flag)
      deblocking_filter_override_flag = _bs->readUn(1);
    if (deblocking_filter_override_flag) {
      slice_deblocking_filter_disabled_flag = _bs->readUn(1);
      if (!slice_deblocking_filter_disabled_flag) {
        slice_beta_offset_div2 = _bs->readSE();
        slice_tc_offset_div2 = _bs->readSE();
      }
    }
    if (m_pps->pps_loop_filter_across_slices_enabled_flag &&
        (slice_sao_luma_flag || slice_sao_chroma_flag ||
         !slice_deblocking_filter_disabled_flag))
      slice_loop_filter_across_slices_enabled_flag = _bs->readUn(1);
  }
  if (m_pps->tiles_enabled_flag || m_pps->entropy_coding_sync_enabled_flag) {
    num_entry_point_offsets = _bs->readUE();
    if (num_entry_point_offsets > 0) {
      offset_len_minus1 = _bs->readUE();
      for (int32_t i = 0; i < num_entry_point_offsets; i++)
        entry_point_offset_minus1[i] = _bs->readUn(offset_len_minus1 + 1);
    }
  }
  if (m_pps->slice_segment_header_extension_present_flag) {
    slice_segment_header_extension_length = _bs->readUE();
    for (int32_t i = 0; i < slice_segment_header_extension_length; i++)
      slice_segment_header_extension_data_byte[i] = _bs->readUn(8);
  }
  _bs->byte_alignment();

  slice_ctb_addr_rs = slice_segment_address;
  /* SliceHeader 同时初始化？ 然后需要共享数据？ */
  //s->HEVClc->first_qp_group = !dependent_slice_segment_flag;
  //if (!m_pps->cu_qp_delta_enabled_flag) s->HEVClc->qp_y = s->sh.slice_qp;
  //slice_initialized = 1;
  //s->HEVClc->tu.cu_qp_offset_cb = 0;
  //s->HEVClc->tu.cu_qp_offset_cr = 0;

  //-- Append
  //96 Rec. ITU-T H.265 (V10) (07/2024)
  if (short_term_ref_pic_set_sps_flag == 1) {
    CurrRpsIdx = short_term_ref_pic_set_idx;
  } else {
    CurrRpsIdx = m_sps->num_short_term_ref_pic_sets;
  }

  int MaxNumMergeCand = 5 - five_minus_max_num_merge_cand;
  SliceQpY = 26 + m_pps->init_qp + slice_qp_delta;

  /* TODO YangJing    firstByte,lastByte (7-55,7-56) -> 100 Rec. ITU-T H.265 (V10) (07/2024) <24-10-23 08:41:38> */

  //7.4.7.2 Reference picture list modification semantics

  NumPicTotalCurr = 0;
  for (int32_t i = 0; i < NumNegativePics[CurrRpsIdx]; i++)
    if (UsedByCurrPicS0[CurrRpsIdx][i]) NumPicTotalCurr++;
  for (int32_t i = 0; i < NumPositivePics[CurrRpsIdx]; i++)
    if (UsedByCurrPicS1[CurrRpsIdx][i]) NumPicTotalCurr++;
  for (int32_t i = 0; i < num_long_term_sps + num_long_term_pics; i++)
    if (UsedByCurrPicLt[i]) NumPicTotalCurr++;
  if (m_pps->pps_curr_pic_ref_enabled_flag) NumPicTotalCurr++;

  /* TODO YangJing 7.4.7.3 Weighted prediction parameters semantics -> Rec. ITU-T H.265 (V10) (07/2024) 101 <24-10-23 08:44:47> */

  return 0;
}

void SliceHeader::ref_pic_list_mvc_modification() { /* specified in Annex H */ }

/* 调整解码器的参考图片列表，根据当前视频帧的特定需求调整参考帧的使用顺序或选择哪些帧作为参考 */
void SliceHeader::ref_pic_lists_modification(BitStream *bs, int32_t nb_refs) {
  ref_pic_list_modification_flag_l0 = bs->readUn(1);
  if (ref_pic_list_modification_flag_l0)
    for (int32_t i = 0; i <= num_ref_idx_l0_active_minus1; i++)
      list_entry_lx[0][i] = bs->readUn(nb_refs);
  if (slice_type == HEVC_SLICE_B) {
    ref_pic_list_modification_flag_l1 = bs->readUn(1);
    if (ref_pic_list_modification_flag_l1)
      for (int32_t i = 0; i <= num_ref_idx_l1_active_minus1; i++)
        list_entry_lx[1][i] = bs->readUn(nb_refs);
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
