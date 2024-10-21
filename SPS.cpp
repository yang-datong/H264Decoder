#include "SPS.hpp"
#include "BitStream.hpp"
#include <cstdint>
#include <iostream>
#include <ostream>

void SPS::vui_parameters() {
  cout << "\tVUI -> {" << endl;
  aspect_ratio_info_present_flag = bs->readU1();
  if (aspect_ratio_info_present_flag) {
    aspect_ratio_idc = bs->readUn(8);
    cout << "\t\t宽高比标识符，视频的宽高比类型:" << aspect_ratio_idc << endl;
    if (aspect_ratio_idc == Extended_SAR) {
      sar_width = bs->readUn(16);
      cout << "\t\t表示样本的宽度（SAR，样本宽高比）:" << sar_width << endl;
      sar_height = bs->readUn(16);
      cout << "\t\t表示样本的高度（SAR，样本宽高比）:" << sar_height << endl;
    }
  }
  overscan_info_present_flag = bs->readU1();
  if (overscan_info_present_flag) {
    overscan_appropriate_flag = bs->readU1();
    cout << "\t\t视频适合超扫描显示:" << overscan_appropriate_flag << endl;
  }

  video_signal_type_present_flag = bs->readU1();
  if (video_signal_type_present_flag) {
    video_format = bs->readUn(3);
    cout << "\t\t视频格式标识符，视频的类型（如未压缩、压缩等）:"
         << (int)video_format << endl;
    video_full_range_flag = bs->readU1();
    cout << "\t\t视频使用全范围色彩（0-255）或限范围色彩（16-235）:"
         << video_full_range_flag << endl;
    colour_description_present_flag = bs->readU1();
    if (colour_description_present_flag) {
      colour_primaries = bs->readUn(8);
      cout << "\t\t颜色原色的类型（如BT.709、BT.601等）:"
           << (int)colour_primaries << endl;
      transfer_characteristics = bs->readUn(8);
      cout << "\t\t传输特性（如线性、伽马等）:" << (int)transfer_characteristics
           << endl;
      matrix_coefficients = bs->readUn(8);
      cout << "\t\t矩阵系数，用于颜色空间转换:" << (int)matrix_coefficients
           << endl;
    }
  }

  chroma_loc_info_present_flag = bs->readU1();
  if (chroma_loc_info_present_flag) {
    chroma_sample_loc_type_top_field = bs->readSE();
    chroma_sample_loc_type_bottom_field = bs->readSE();
    cout << "\t\t顶场色度样本位置类型:" << chroma_sample_loc_type_top_field
         << ",底场色度样本位置类型:" << chroma_sample_loc_type_bottom_field
         << endl;
  }

  // --- H.264

  /*
  timing_info_present_flag = bitStream.readU1();
  if (timing_info_present_flag) {
    num_units_in_tick = bitStream.readUn(32);
    time_scale = bitStream.readUn(32);
    cout << "\t\t每个时钟周期的单位数:" << num_units_in_tick
         << ",每秒的单位数(时间尺度):" << time_scale << endl;
    fixed_frame_rate_flag = bitStream.readU1();
    cout << "\t\t使用固定帧率:" << fixed_frame_rate_flag << endl;
  }

  nal_hrd_parameters_present_flag = bitStream.readU1();
  vcl_hrd_parameters_present_flag = bitStream.readU1();
  if (nal_hrd_parameters_present_flag) hrd_parameters(bitStream);
  if (vcl_hrd_parameters_present_flag) hrd_parameters(bitStream);
  cout << "\t\t存在NAL HRD（网络提取率控制）参数:"
       << nal_hrd_parameters_present_flag
       << ",存在VCL HRD参数:" << vcl_hrd_parameters_present_flag << endl;

  if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
    low_delay_hrd_flag = bitStream.readU1();
    cout << "\t\t使用低延迟HRD:" << low_delay_hrd_flag << endl;
  }

  pic_struct_present_flag = bitStream.readU1();
  cout << "\t\t存在图像结构信息:" << pic_struct_present_flag << endl;
  bitstream_restriction_flag = bitStream.readU1();
  cout << "\t\t存在比特流限制:" << bitstream_restriction_flag << endl;

  if (bitstream_restriction_flag) {
    motion_vectors_over_pic_boundaries_flag = bitStream.readU1();
    cout << "\t\t允许运动矢量跨越图像边界:"
         << motion_vectors_over_pic_boundaries_flag << endl;
    max_bytes_per_pic_denom = bitStream.readUE();
    cout << "\t\t每帧最大字节数的分母:" << max_bytes_per_pic_denom << endl;
    max_bits_per_mb_denom = bitStream.readUE();
    cout << "\t\t每个宏块最大比特数的分母:" << max_bits_per_mb_denom << endl;
    log2_max_mv_length_horizontal = bitStream.readUE();
    log2_max_mv_length_vertical = bitStream.readUE();
    cout << "\t\t水平运动矢量的最大长度的对数值:"
         << log2_max_mv_length_horizontal
         << ",垂直运动矢量的最大长度的对数值:" << log2_max_mv_length_vertical
         << endl;
    max_num_reorder_frames = bitStream.readUE();
    cout << "\t\t最大重排序帧数:" << max_num_reorder_frames << endl;
    max_dec_frame_buffering = bitStream.readUE();
    cout << "\t\t最大解码帧缓冲区大小:" << max_dec_frame_buffering << endl;
  }
  */

  // --- H.265
  neutral_chroma_indication_flag = bs->readUn(1);
  field_seq_flag = bs->readUn(1);
  frame_field_info_present_flag = bs->readUn(1);
  default_display_window_flag = bs->readUn(1);
  if (default_display_window_flag) {
    def_disp_win_left_offset = bs->readUE();
    def_disp_win_right_offset = bs->readUE();
    def_disp_win_top_offset = bs->readUE();
    def_disp_win_bottom_offset = bs->readUE();
  }
  vui_timing_info_present_flag = bs->readUn(1);
  if (vui_timing_info_present_flag) {
    vui_num_units_in_tick = bs->readUn(32);
    vui_time_scale = bs->readUn(32);
    vui_poc_proportional_to_timing_flag = bs->readUn(1);
    if (vui_poc_proportional_to_timing_flag)
      vui_num_ticks_poc_diff_one_minus1 = bs->readUE();
    vui_hrd_parameters_present_flag = bs->readUn(1);
    if (vui_hrd_parameters_present_flag) {
      std::cout << "hi~" << std::endl;
      //hrd_parameters(bitStream,1, sps_max_sub_layers_minus1);
    }
  }
  bitstream_restriction_flag = bs->readUn(1);
  if (bitstream_restriction_flag) {
    tiles_fixed_structure_flag = bs->readUn(1);
    motion_vectors_over_pic_boundaries_flag = bs->readUn(1);
    restricted_ref_pic_lists_flag = bs->readUn(1);
    min_spatial_segmentation_idc = bs->readUE();
    max_bytes_per_pic_denom = bs->readUE();
    max_bits_per_min_cu_denom = bs->readUE();
    log2_max_mv_length_horizontal = bs->readUE();
    log2_max_mv_length_vertical = bs->readUE();
  }

  //int32_t fps = 0;
  //if (vui_parameters_present_flag && timing_info_present_flag)
  //fps = time_scale / num_units_in_tick / 2;
  //cout << "\t\tfps:" << fps << endl;
  cout << "\t }" << endl;
}

void SPS::hrd_parameters() {
  cpb_cnt_minus1 = bs->readUE();
  bit_rate_scale = bs->readUn(8);
  cpb_size_scale = bs->readUn(8);

  bit_rate_value_minus1 = new uint32_t[cpb_cnt_minus1];
  cpb_size_value_minus1 = new uint32_t[cpb_cnt_minus1];
  cbr_flag = new bool[cpb_cnt_minus1];

  for (int SchedSelIdx = 0; SchedSelIdx <= (int)cpb_cnt_minus1; SchedSelIdx++) {
    bit_rate_value_minus1[SchedSelIdx] = bs->readUE();
    cpb_size_value_minus1[SchedSelIdx] = bs->readUE();
    cbr_flag[SchedSelIdx] = bs->readU1();
  }
  initial_cpb_removal_delay_length_minus1 = bs->readUn(5);
  cpb_removal_delay_length_minus1 = bs->readUn(5);
  dpb_output_delay_length_minus1 = bs->readUn(5);
  time_offset_length = bs->readUn(5);
}

int SPS::extractParameters(BitStream &bitStream, VPS vpss[MAX_SPS_COUNT]) {
  this->bs = &bitStream;
  sps_video_parameter_set_id = bs->readUn(4);
  cout << "\tVPS ID:" << sps_video_parameter_set_id << endl;
  m_vps = &vpss[sps_video_parameter_set_id];
  sps_max_sub_layers_minus1 = bs->readUn(3);
  cout << "\t表示SPS适用的最大子层级数减1:" << sps_max_sub_layers_minus1
       << endl;
  sps_temporal_id_nesting_flag = bs->readUn(1);
  cout << "\t指示在SPS的有效范围内，所有的NAL单元是否遵循时间ID嵌套的规则:"
       << sps_temporal_id_nesting_flag << endl;
  profile_tier_level(*bs, 1, sps_max_sub_layers_minus1);

  sps_seq_parameter_set_id = bs->readUE();
  cout << "\tSPS ID:" << sps_seq_parameter_set_id << endl;
  chroma_format_idc = bs->readUE();
  switch (chroma_format_idc) {
  case 0:
    cout << "\tchroma_format_idc:YUV400" << endl;
    break;
  case 1:
    cout << "\tchroma_format_idc:YUV420" << endl;
    break;
  case 2:
    cout << "\tchroma_format_idc:YUV422" << endl;
    break;
  case 3:
    cout << "\tchroma_format_idc:YUB444" << endl;
    separate_colour_plane_flag = bs->readU1();
    cout << "\t若为1，则亮度和色度样本被分别处理和编码:"
         << separate_colour_plane_flag << endl;
    break;
  }
  pic_width_in_luma_samples = bs->readUE();
  pic_height_in_luma_samples = bs->readUE();
  width = pic_width_in_luma_samples;
  height = pic_height_in_luma_samples;

  cout << "\t图像大小，单位为亮度样本:" << pic_width_in_luma_samples << "x"
       << pic_height_in_luma_samples << endl;
  conformance_window_flag = bs->readU1();
  cout << "\t表示是否裁剪图像边缘以符合显示要求:" << conformance_window_flag
       << endl;
  if (conformance_window_flag) {
    conf_win_left_offset = bs->readUE();
    conf_win_right_offset = bs->readUE();
    conf_win_top_offset = bs->readUE();
    conf_win_bottom_offset = bs->readUE();
  }

  bit_depth_luma_minus8 = bs->readUE();
  cout << "\t分别表示亮度的位深减8:" << bit_depth_luma_minus8 << endl;
  bit_depth_chroma_minus8 = bs->readUE();
  cout << "\t分别表示色度的位深减8:" << bit_depth_chroma_minus8 << endl;
  log2_max_pic_order_cnt_lsb_minus4 = bs->readUE();
  cout << "\t表示解码顺序计数器的最大二进制位数减4:"
       << log2_max_pic_order_cnt_lsb_minus4 << endl;
  sps_sub_layer_ordering_info_present_flag = bs->readU1();
  cout << "\t表示SPS是否为每个子层指定解码和输出缓冲需求:"
       << sps_sub_layer_ordering_info_present_flag << endl;

  int32_t n =
      sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1;
  for (int32_t i = n; i <= sps_max_sub_layers_minus1; i++) {
    sps_max_dec_pic_buffering_minus1[i] = bs->readUE();
    sps_max_num_reorder_pics[i] = bs->readUE();
    sps_max_latency_increase_plus1[i] = bs->readUE();
    cout << "\t分别定义解码缓冲需求、重排序需求和最大允许的延迟增加:"
         << sps_max_dec_pic_buffering_minus1[i] << ","
         << sps_max_num_reorder_pics[i] << ","
         << sps_max_latency_increase_plus1[i] << endl;
  }
  log2_min_luma_coding_block_size_minus3 = bs->readUE();
  cout << "\t编码的块大小:" << log2_min_luma_coding_block_size_minus3 << endl;
  log2_diff_max_min_luma_coding_block_size = bs->readUE();
  cout << "\t编码变换块大小:" << log2_diff_max_min_luma_coding_block_size
       << endl;

  MinCbLog2SizeY = log2_min_luma_coding_block_size_minus3 + 3;
  CtbLog2SizeY = MinCbLog2SizeY + log2_diff_max_min_luma_coding_block_size;
  CtbSizeY = 1 << CtbLog2SizeY;
  PicWidthInCtbsY = CEIL(pic_width_in_luma_samples / CtbSizeY);
  PicHeightInCtbsY = CEIL(pic_height_in_luma_samples / CtbSizeY);
  PicSizeInCtbsY = PicWidthInCtbsY * PicHeightInCtbsY;

  ctb_width = (width + (1 << CtbLog2SizeY) - 1) >> CtbLog2SizeY;
  ctb_height = (height + (1 << CtbLog2SizeY) - 1) >> CtbLog2SizeY;
  ctb_size = ctb_width * ctb_height;

  log2_min_luma_transform_block_size_minus2 = bs->readUE();
  log2_diff_max_min_luma_transform_block_size = bs->readUE();
  max_transform_hierarchy_depth_inter = bs->readUE();
  max_transform_hierarchy_depth_intra = bs->readUE();
  cout << "\t编码变换的层次深度:" << max_transform_hierarchy_depth_intra
       << endl;
  scaling_list_enabled_flag = bs->readUn(1);
  if (scaling_list_enabled_flag) {
    sps_scaling_list_data_present_flag = bs->readUn(1);
    cout << "\t指示是否使用量化缩放列表和是否在SPS中携带缩放列表数据:"
         << scaling_list_enabled_flag << ","
         << sps_scaling_list_data_present_flag << endl;
    if (sps_scaling_list_data_present_flag) scaling_list_data();
  }
  amp_enabled_flag = bs->readUn(1);
  cout << "\t异构模式分割（AMP）的启用标志:" << amp_enabled_flag << endl;
  sample_adaptive_offset_enabled_flag = bs->readUn(1);
  cout << "\t样本自适应偏移（SAO）的启用标志，用于改进去块效应:"
       << sample_adaptive_offset_enabled_flag << endl;
  pcm_enabled_flag = bs->readUn(1);
  cout << "\tPCM（脉冲编码调制）的启用标志，用于无损编码块:" << pcm_enabled_flag
       << endl;
  if (pcm_enabled_flag) {
    pcm_sample_bit_depth_luma_minus1 = bs->readUn(4);
    pcm_sample_bit_depth_chroma_minus1 = bs->readUn(4);
    cout << "\t定义PCM编码的亮度和色度位深减1:"
         << pcm_sample_bit_depth_luma_minus1 << ","
         << pcm_sample_bit_depth_chroma_minus1 << endl;
    log2_min_pcm_luma_coding_block_size_minus3 = bs->readUE();
    log2_diff_max_min_pcm_luma_coding_block_size = bs->readUE();
    pcm_loop_filter_disabled_flag = bs->readUn(1);
    cout << "\t指示是否禁用循环滤波器:" << pcm_loop_filter_disabled_flag
         << endl;
  }
  num_short_term_ref_pic_sets = bs->readUE();
  cout << "\t短期参考图片集的数量:" << num_short_term_ref_pic_sets << endl;
  for (int32_t i = 0; i < num_short_term_ref_pic_sets; i++) {
    std::cout << "hi~" << std::endl;
    return 0;
    //st_ref_pic_set(i);
  }
  long_term_ref_pics_present_flag = bs->readUn(1);
  cout << "\t指示是否使用长期参考图像:" << long_term_ref_pics_present_flag
       << endl;
  if (long_term_ref_pics_present_flag) {
    num_long_term_ref_pics_sps = bs->readUE();
    cout << "\t长期参考图像的数量:" << num_long_term_ref_pics_sps << endl;
    for (int32_t i = 0; i < num_long_term_ref_pics_sps; i++) {
      lt_ref_pic_poc_lsb_sps[i] =
          bs->readUn(log2_max_pic_order_cnt_lsb_minus4 + 4);
      used_by_curr_pic_lt_sps_flag[i] = bs->readUn(1);
      cout << "\t分别定义长期参考图像的POC LSB和其使用状态:"
           << lt_ref_pic_poc_lsb_sps << "," << used_by_curr_pic_lt_sps_flag
           << endl;
    }
  }
  sps_temporal_mvp_enabled_flag = bs->readUn(1);
  cout << "\t时间多视点预测的启用标志:" << sps_temporal_mvp_enabled_flag
       << endl;
  strong_intra_smoothing_enabled_flag = bs->readUn(1);
  cout << "\t强内部平滑的启用标志:" << strong_intra_smoothing_enabled_flag
       << endl;

  vui_parameters_present_flag = bs->readUn(1);
  cout << "\t指示视频可用性信息（VUI）是否存在:" << vui_parameters_present_flag
       << endl;
  if (vui_parameters_present_flag) vui_parameters();
  sps_extension_present_flag = bs->readUn(1);
  cout << "\t各种SPS扩展的启用标志:" << sps_extension_present_flag << endl;
  if (sps_extension_present_flag) {
    sps_range_extension_flag = bs->readUn(1);
    sps_multilayer_extension_flag = bs->readUn(1);
    sps_3d_extension_flag = bs->readUn(1);
    sps_scc_extension_flag = bs->readUn(1);
    sps_extension_4bits = bs->readUn(4);
  }
  if (sps_range_extension_flag) sps_range_extension();
  if (sps_multilayer_extension_flag) sps_multilayer_extension();
  if (sps_3d_extension_flag) sps_3d_extension();
  if (sps_scc_extension_flag) sps_scc_extension();
  if (sps_extension_4bits)
    while (bs->more_rbsp_data())
      int32_t sps_extension_data_flag = bs->readUn(1);
  cout << "\tSPS扩展和扩展数据的存在标志:" << sps_extension_4bits << ","
       << sps_extension_data_flag << endl;
  bs->rbsp_trailing_bits();

  return 0;
}

int SPS::scaling_list_data() {
  int scaling_list_pred_mode_flag[32][32] = {{0}};
  int scaling_list_pred_matrix_id_delta[32][32] = {{0}};
  int ScalingList[32][32][32] = {{{0}}};
  int scaling_list_dc_coef_minus8[32][32] = {{0}};

  for (int sizeId = 0; sizeId < 4; sizeId++)
    for (int matrixId = 0; matrixId < 6; matrixId += (sizeId == 3) ? 3 : 1) {
      scaling_list_pred_mode_flag[sizeId][matrixId] = bs->readUn(1);
      if (!scaling_list_pred_mode_flag[sizeId][matrixId])
        scaling_list_pred_matrix_id_delta[sizeId][matrixId] = bs->readUE();
      else {
        int nextCoef = 8;
        int coefNum = MIN(64, (1 << (4 + (sizeId << 1))));
        if (sizeId > 1) {
          scaling_list_dc_coef_minus8[sizeId - 2][matrixId] = bs->readSE();
          nextCoef = scaling_list_dc_coef_minus8[sizeId - 2][matrixId] + 8;
        }
        for (int i = 0; i < coefNum; i++) {
          int scaling_list_delta_coef = bs->readSE();
          nextCoef = (nextCoef + scaling_list_delta_coef + 256) % 256;
          ScalingList[sizeId][matrixId][i] = nextCoef;
        }
      }
    }
  return 0;
}

int SPS::sps_range_extension() {
  int transform_skip_rotation_enabled_flag = bs->readUn(1);
  int transform_skip_context_enabled_flag = bs->readUn(1);
  int implicit_rdpcm_enabled_flag = bs->readUn(1);
  int explicit_rdpcm_enabled_flag = bs->readUn(1);
  int extended_precision_processing_flag = bs->readUn(1);
  int intra_smoothing_disabled_flag = bs->readUn(1);
  int high_precision_offsets_enabled_flag = bs->readUn(1);
  int persistent_rice_adaptation_enabled_flag = bs->readUn(1);
  int cabac_bypass_alignment_enabled_flag = bs->readUn(1);
  return 0;
}
int SPS::sps_multilayer_extension() {
  int inter_view_mv_vert_constraint_flag = bs->readU1();
  return 0;
}
int SPS::sps_3d_extension() {
  int cp_precision = 0;
  int num_cp[32] = {0};
  int cp_in_slice_segment_header_flag[32] = {0};
  int cp_ref_voi[32][32] = {0};
  int vps_cp_scale[32][32] = {0};
  int vps_cp_off[32][32] = {0};
  int vps_cp_inv_scale_plus_scale[32][32] = {0};
  int vps_cp_inv_off_plus_off[32][32] = {0};

  int NumViews = 1;
  //  for (int i = 0; i <= m_vps->vps_max_layers - 1; i++) {
  //    int lId = m_vps->layer_id_in_nuh[i];
  //    for (int smIdx = 0, j = 0; smIdx < 16; smIdx++) {
  //      if (scalability_mask_flag[smIdx])
  //        ScalabilityId[i][smIdx] = dimension_id[i][j++];
  //      else
  //        ScalabilityId[i][smIdx] = 0
  //    }
  //    DepthLayerFlag[lId] = ScalabilityId[i][0];
  //    ViewOrderIdx[lId] = ScalabilityId[i][1];
  //    DependencyId[lId] = ScalabilityId[i][2](F - 3);
  //    AuxId[lId] = ScalabilityId[i][3];
  //    if (i > 0) {
  //      newViewFlag = 1;
  //      for (int j = 0; j < i; j++)
  //        if (ViewOrderIdx[lId] == ViewOrderIdx[layer_id_in_nuh[j]])
  //          newViewFlag = 0;
  //      NumViews += newViewFlag;
  //    }
  //  }

  //  cp_precision = bs->readUE();
  //  for (int n = 1; n < NumViews; n++) {
  //    int i = ViewOIdxList[n];
  //    num_cp[i] = bs->readUn(6);
  //    if (num_cp[i] > 0) {
  //      cp_in_slice_segment_header_flag[i] = bs->readUn(1);
  //      for (int m = 0; m < num_cp[i]; m++) {
  //        cp_ref_voi[i][m] = bs->readUE();
  //        if (!cp_in_slice_segment_header_flag[i]) {
  //          int j = cp_ref_voi[i][m];
  //          vps_cp_scale[i][j] = bs->readSE();
  //          vps_cp_off[i][j] = bs->readSE();
  //          vps_cp_inv_scale_plus_scale[i][j] = bs->readSE();
  //          vps_cp_inv_off_plus_off[i][j] = bs->readSE();
  //        }
  //      }
  //    }
  //  }
  return 0;
}
int SPS::sps_scc_extension() {}
