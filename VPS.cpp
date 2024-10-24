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
  cout << "\t最大临时层的数量:" << vps_max_sub_layers << endl;
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
    VpsMaxLatencyPictures[i] =
        vps_max_num_reorder_pics[i] + vps_max_latency_increase[i];
  }

  vps_max_layer_id = bs.readUn(6);
  cout << "\t可使用的最大的层ID:" << vps_max_layer_id << endl;
  vps_num_layer_sets = bs.readUE() + 1;
  cout << "\t表示层集合的数量，用于定义不同层集的组合:" << vps_num_layer_sets
       << endl;

  n = 0;
  layer_id_included_flag[32][32] = {0};
  for (int32_t i = 1; i <= vps_num_layer_sets - 1; i++)
    for (int32_t j = 0; j <= vps_max_layer_id; j++) {
      layer_id_included_flag[i][j] = bs.readUn(1);
      if (layer_id_included_flag[i][j]) LayerSetLayerIdList[i][n++] = j;
      NumLayersInIdList[i] = n;
    }
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
  if (vps_extension_flag)
    while (bs.more_rbsp_data())
      vps_extension_data_flag = bs.readUn(1);

  bs.rbsp_trailing_bits();

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
