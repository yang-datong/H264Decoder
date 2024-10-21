#include "VPS.hpp"
#include "BitStream.hpp"

int VPS::extractParameters(BitStream &bs) {
  vps_video_parameter_set_id = bs.readUn(4);
  cout << "\tVPS ID:" << vps_video_parameter_set_id << endl;
  vps_base_layer_internal_flag = bs.readUn(1);
  cout << "\t基础层是否为内部层(是否完全自包含):"
       << vps_base_layer_internal_flag << endl;
  vps_base_layer_available_flag = bs.readUn(1);
  cout << "\t基础层是否可用于解码:" << vps_base_layer_available_flag << endl;
  vps_max_layers = bs.readUn(6) + 1;
  cout << "\t表示编码视频中使用的最大层数:" << vps_max_layers << endl;
  vps_max_sub_layers = bs.readUn(3) + 1;
  cout << "\t每个层最多有多少个子层:" << vps_max_sub_layers << endl;
  vps_temporal_id_nesting_flag = bs.readUn(1);
  cout << "\t是否所有的VCL（视频编码层）NAL单元都具有相同或增加的时间层ID:"
       << vps_temporal_id_nesting_flag << endl;
  vps_reserved_0xffff_16bits = bs.readUn(16);
  profile_tier_level(bs, 1, vps_max_sub_layers - 1);
  vps_sub_layer_ordering_info_present_flag = bs.readUn(1);
  cout << "\t是否为每个子层都提供了排序信息:"
       << vps_sub_layer_ordering_info_present_flag << endl;

  int32_t n =
      vps_sub_layer_ordering_info_present_flag ? 0 : vps_max_sub_layers - 1;
  for (int32_t i = n; i <= vps_max_sub_layers - 1; i++) {
    vps_max_dec_pic_buffering[i] = bs.readUE() + 1;
    cout << "\t对每个子层定义解码图像缓冲的最大数量:"
         << vps_max_dec_pic_buffering[i] << endl;
    vps_max_num_reorder_pics[i] = bs.readUE();
    cout << "\t数组，对每个子层定义重排序图片的最大数量:"
         << vps_max_num_reorder_pics[i] << endl;
    vps_max_latency_increase[i] = bs.readUE() - 1;
    cout << "\t数组，定义了每个子层的最大延迟增加值:"
         << vps_max_latency_increase[i] << endl;
  }

  vps_max_layer_id = bs.readUn(6);
  cout << "\t可使用的最大的层ID:" << vps_max_layer_id << endl;
  vps_num_layer_sets = bs.readUE() + 1;
  cout << "\t表示层集合的数量，用于定义不同层集的组合:" << vps_num_layer_sets
       << endl;

  layer_id_included_flag[32][32] = {0};
  for (int32_t i = 1; i <= vps_num_layer_sets - 1; i++)
    for (int32_t j = 0; j <= vps_max_layer_id; j++)
      layer_id_included_flag[i][j] = bs.readUn(1);
  cout << "\t在每个层集合中哪些层被包括:" << layer_id_included_flag[0][0]
       << endl;

  vps_timing_info_present_flag = bs.readUn(1);
  cout << "\t是否在VPS中存在定时信息:" << vps_timing_info_present_flag << endl;
  if (vps_timing_info_present_flag) {
    vps_num_units_in_tick = bs.readUn(32);
    cout << "\t时间单位:" << vps_num_units_in_tick << endl;
    vps_time_scale = bs.readUn(32);
    cout << "\t时间尺度:" << vps_time_scale << endl;
    vps_poc_proportional_to_timing_flag = bs.readUn(1);
    cout << "\t标志位，画面输出顺序是否与时间信息成比例:"
         << vps_poc_proportional_to_timing_flag << endl;
    if (vps_poc_proportional_to_timing_flag) {
      vps_num_ticks_poc_diff_one = bs.readUE() + 1;
      cout << "\t定义两个连续POC（解码顺序号）间的时间单位数:"
           << vps_num_ticks_poc_diff_one << endl;
    }
    vps_num_hrd_parameters = bs.readUE();
    cout << "\tHRD（超时率解码）参数集的数量:" << vps_num_hrd_parameters
         << endl;

    hrd_layer_set_idx[32] = {0};
    cprms_present_flag[32] = {0};
    for (int32_t i = 0; i < vps_num_hrd_parameters; i++) {
      hrd_layer_set_idx[i] = bs.readUE();
      cout << "\t为每个HRD参数集定义使用的层集索引:" << hrd_layer_set_idx[i]
           << endl;
      if (i > 0) {
        cprms_present_flag[i] = bs.readUn(1);
        cout << "\t每个HRD参数集是否包含共同参数集的标志:" << cprms_present_flag
             << endl;
      }
      hrd_parameters(bs, cprms_present_flag[i], vps_max_sub_layers - 1);
    }
  }
  vps_extension_flag = bs.readUn(1);
  cout << "\t是否有额外的扩展数据:" << vps_extension_flag << endl;
  if (vps_extension_flag) {
    while (!bs.byte_aligned())
      vps_extension_alignment_bit_equal_to_one = bs.readUn(1);
    vps_extension();
    vps_extension2_flag = bs.readUn(1);
    if (vps_extension2_flag)
      while (bs.more_rbsp_data())
        vps_extension_data_flag = bs.readUn(1);
  }

  bs.rbsp_trailing_bits();

  return 0;
}

int VPS::vps_extension() {
  //  int i, j;
  //  if (vps_max_layers - 1 > 0 && vps_base_layer_internal_flag)
  //    profile_tier_level(0, vps_max_sub_layers - 1);
  //  splitting_flag = bs.readUn(1);
  //  for (i = 0, NumScalabilityTypes = 0; i < 16; i++) {
  //    scalability_mask_flag[i] = bs.readUn(1);
  //    NumScalabilityTypes += scalability_mask_flag[i]
  //  }
  //  for (j = 0; j < (NumScalabilityTypes - splitting_flag); j++)
  //    dimension_id_len_minus1[j] = bs.readUn(3);
  //  vps_nuh_layer_id_present_flag = bs.readUn(1);
  //  for (i = 1; i <= MaxLayersMinus1; i++) {
  //    if (vps_nuh_layer_id_present_flag) layer_id_in_nuh[i] = bs.readUn(6);
  //    if (!splitting_flag)
  //      for (j = 0; j < NumScalabilityTypes; j++)
  //        dimension_id[i][j] u(v)
  //  }
  //  view_id_len = bs.readUn(4);
  //  if (view_id_len > 0)
  //    for (i = 0; i < NumViews; i++)
  //      view_id_val[i] u(v) for (i = 1; i <= MaxLayersMinus1;
  //                               i++) for (j = 0; j < i; j++)
  //          direct_dependency_flag[i][j] = bs.readUn(1);
  //  if (NumIndependentLayers > 1) num_add_layer_sets = bs.readUE();
  //  for (i = 0; i < num_add_layer_sets; i++)
  //    for (j = 1; j < NumIndependentLayers; j++)
  //      highest_layer_idx_plus1[i][j] u(v)
  //          vps_sub_layers_max_minus1_present_flag = bs.readUn(1);
  //  if (vps_sub_layers_max_minus1_present_flag)
  //    for (i = 0; i <= MaxLayersMinus1; i++)
  //      sub_layers_vps_max_minus1[i] = bs.readUn(3);
  //  max_tid_ref_present_flag = bs.readUn(1);
  //  if (max_tid_ref_present_flag)
  //    for (i = 0; i < MaxLayersMinus1; i++)
  //      for (j = i + 1; j <= MaxLayersMinus1; j++)
  //        if (direct_dependency_flag[j][i])
  //          max_tid_il_ref_pics_plus1[i][j] = bs.readUn(3);
  //  default_ref_layers_active_flag = bs.readUn(1);
  //  vps_num_profile_tier_level_minus1 = bs.readUE();
  //  for (i = vps_base_layer_internal_flag ? 2 : 1;
  //       i <= vps_num_profile_tier_level_minus1; i++) {
  //    vps_profile_present_flag[i] = bs.readUn(1);
  //    profile_tier_level(vps_profile_present_flag[i], vps_max_sub_layers_minus1)
  //  }
  //  if (NumLayerSets > 1) {
  //    num_add_olss = bs.readUE();
  //    default_output_layer_idc = bs.readUn(2);
  //  }
  //  NumOutputLayerSets =
  //      num_add_olss + NumLayerSets for (i = 1; i < NumOutputLayerSets; i++) {
  //    if (NumLayerSets > 2 && i >= NumLayerSets)
  //      layer_set_idx_for_ols_minus1[i] u(
  //          v) if (i > vps_num_layer_sets_minus1 ||
  //                 defaultOutputLayerIdc ==
  //                     2) for (j = 0; j < NumLayersInIdList[OlsIdxToLsIdx[i]];
  //                             j++) output_layer_flag[i][j] = bs.readUn(1);
  //    for (j = 0; j < NumLayersInIdList[OlsIdxToLsIdx[i]]; j++)
  //      if (NecessaryLayerFlag[i][j] && vps_num_profile_tier_level_minus1 > 0)
  //        profile_tier_level_idx[i][j] u(
  //            v) if (NumOutputLayersInOutputLayerSet[i] == 1 &&
  //                   NumDirectRefLayers[OlsHighestOutputLayerId[i]] > 0)
  //            alt_output_layer_flag[i] = bs.readUn(1);
  //  }
  //  vps_num_rep_formats_minus1 = bs.readUE();
  //  for (i = 0; i <= vps_num_rep_formats_minus1; i++)
  //    rep_format() if (vps_num_rep_formats_minus1 > 0)
  //        rep_format_idx_present_flag = bs.readUn(1);
  //  if (rep_format_idx_present_flag)
  //    for (i = vps_base_layer_internal_flag ? 1 : 0; i <= MaxLayersMinus1; i++)
  //      vps_rep_format_idx[i] u(v) max_one_active_ref_layer_flag = bs.readUn(1);
  //  vps_poc_lsb_aligned_flag = bs.readUn(1);
  //  for (i = 1; i <= MaxLayersMinus1; i++)
  //    if (NumDirectRefLayers[layer_id_in_nuh[i]] == 0)
  //      poc_lsb_not_present_flag[i] = bs.readUn(1);
  //  dpb_size() direct_dep_type_len_minus2 = bs.readUE();
  //  direct_dependency_all_layers_flag = bs.readUn(1);
  //  if (direct_dependency_all_layers_flag)
  //    direct_dependency_all_layers_type u(v) else {
  //      for (i = vps_base_layer_internal_flag ? 1 : 2; i <= MaxLayersMinus1; i++)
  //        for (j = vps_base_layer_internal_flag ? 0 : 1; j < i; j++)
  //          if (direct_dependency_flag[i][j]) direct_dependency_type[i][j] u(v)
  //    }
  //  vps_non_vui_extension_length = bs.readUE();
  //  for (i = 1; i <= vps_non_vui_extension_length; i++)
  //    vps_non_vui_extension_data_byte = bs.readUn(8);
  //  vps_vui_present_flag = bs.readUn(1);
  //  if (vps_vui_present_flag) {
  //    while (!byte_aligned())
  //      vps_vui_alignment_bit_equal_to_one = bs.readUn(1);
  //    vps_vui()
  //  }
  return 0;
}

int VPS::sub_layer_hrd_parameters(BitStream &bs, int subLayerId) {

  int bit_rate_value_minus1[32] = {0};
  int cpb_size_value_minus1[32] = {0};
  int cpb_size_du_value_minus1[32] = {0};
  int bit_rate_du_value_minus1[32] = {0};
  int cbr_flag[32] = {0};

  int i = 0;
  for (i = 0; i < cpb_cnt_minus1[subLayerId] + 1; i++) {
    bit_rate_value_minus1[i] = bs.readUE();
    cpb_size_value_minus1[i] = bs.readUE();
    if (sub_pic_hrd_params_present_flag) {
      cpb_size_du_value_minus1[i] = bs.readUE();
      bit_rate_du_value_minus1[i] = bs.readUE();
    }
    cbr_flag[i] = bs.readUn(1);
  }
  return 0;
}

int VPS::hrd_parameters(BitStream &bs, int commonInfPresentFlag,
                        int maxNumSubLayersMinus1) {
  if (commonInfPresentFlag) {
    nal_hrd_parameters_present_flag = bs.readUn(1);
    vcl_hrd_parameters_present_flag = bs.readUn(1);
    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
      sub_pic_hrd_params_present_flag = bs.readUn(1);
      if (sub_pic_hrd_params_present_flag) {
        tick_divisor_minus2 = bs.readUn(8);
        du_cpb_removal_delay_increment_length_minus1 = bs.readUn(5);
        sub_pic_cpb_params_in_pic_timing_sei_flag = bs.readUn(1);
        dpb_output_delay_du_length_minus1 = bs.readUn(5);
      }
      bit_rate_scale = bs.readUn(4);
      cpb_size_scale = bs.readUn(4);
      if (sub_pic_hrd_params_present_flag) cpb_size_du_scale = bs.readUn(4);
      initial_cpb_removal_delay_length_minus1 = bs.readUn(5);
      au_cpb_removal_delay_length_minus1 = bs.readUn(5);
      dpb_output_delay_length_minus1 = bs.readUn(5);
    }
  }
  for (int i = 0; i <= maxNumSubLayersMinus1; i++) {
    fixed_pic_rate_general_flag[i] = bs.readUn(1);
    if (!fixed_pic_rate_general_flag[i])
      fixed_pic_rate_within_cvs_flag[i] = bs.readUn(1);
    if (fixed_pic_rate_within_cvs_flag[i])
      elemental_duration_in_tc_minus1[i] = bs.readUE();
    else
      low_delay_hrd_flag[i] = bs.readUn(1);
    if (!low_delay_hrd_flag[i]) cpb_cnt_minus1[i] = bs.readUE();
    if (nal_hrd_parameters_present_flag) sub_layer_hrd_parameters(bs, i);
    if (vcl_hrd_parameters_present_flag) sub_layer_hrd_parameters(bs, i);
  }

  return 0;
}
