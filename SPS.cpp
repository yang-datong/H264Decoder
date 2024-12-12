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
  /* TODO YangJing 要移动到其他地方去,暂时放在这里(还有问题) <24-12-02 17:17:35> */
  int MaxFPS = 0;
  //if ((field_seq_flag == 1 || frame_field_info_present_flag == 1) &&
  //(1 <= pic_struct <= 6 || 9 < pic_struct < 12)) {
  //MaxFPS =
  //ceil(vui_time_scale / ((1 + 1) * vui_num_units_in_tick));
  //}else{
  MaxFPS = ceil(vui_time_scale / ((1 + 0) * vui_num_units_in_tick));
  //}
  cout << "\t\tfps:" << MaxFPS << endl;
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
  m_vps = &vpss[sps_video_parameter_set_id];
  cout << "\tVPS ID:" << sps_video_parameter_set_id << endl;

  //NOTE: 与VPS中的vps_max_sub_layers区别是
  sps_max_sub_layers = bs->readUn(3) + 1;
  cout << "\tSPS适用的最大子层级数:" << sps_max_sub_layers << endl;

  sps_temporal_id_nesting_flag = bs->readUn(1);
  cout << "\t是否允许在所有子层中嵌套时间ID:" << sps_temporal_id_nesting_flag
       << endl;

  // 解析视频的Profile、Tier和Level信息
  profile_tier_level(*bs, 1, sps_max_sub_layers - 1);

  sps_seq_parameter_set_id = bs->readUE();
  cout << "\tSPS ID:" << sps_seq_parameter_set_id << endl;

  chroma_format_idc = bs->readUE();
  if (chroma_format_idc == 0)
    cout << "\tchroma_format_idc:YUV400" << endl;
  else if (chroma_format_idc == 1)
    cout << "\tchroma_format_idc:YUV420" << endl;
  else if (chroma_format_idc == 2)
    cout << "\tchroma_format_idc:YUV422" << endl;
  else if (chroma_format_idc == 3) {
    cout << "\tchroma_format_idc:YUB444" << endl;
    separate_colour_plane_flag = bs->readU1();
    cout << "\t亮度和色度样本是否被分别处理:" << separate_colour_plane_flag
         << endl;
  }

  pic_width_in_luma_samples = bs->readUE();
  pic_height_in_luma_samples = bs->readUE();
  width = pic_width_in_luma_samples;
  height = pic_height_in_luma_samples;
  cout << "\t图像大小，单位为亮度样本:" << pic_width_in_luma_samples << "x"
       << pic_height_in_luma_samples << endl;

  conformance_window_flag = bs->readU1();
  cout << "\t是否裁剪图像边缘:" << conformance_window_flag << endl;
  if (conformance_window_flag) {
    conf_win_left_offset = bs->readUE();
    conf_win_right_offset = bs->readUE();
    conf_win_top_offset = bs->readUE();
    conf_win_bottom_offset = bs->readUE();
  }

  BitDepthY = bit_depth_luma = bs->readUE() + 8;
  QpBdOffsetY = 6 * (bit_depth_luma - 8);
  BitDepthC = bit_depth_chroma = bs->readUE() + 8;
  QpBdOffsetC = 6 * (bit_depth_chroma - 8);
  cout << "\t分别表示亮、色度的位深:" << bit_depth_luma << ","
       << bit_depth_chroma << endl;

  log2_max_pic_order_cnt_lsb = bs->readUE() + 4;
  MaxPicOrderCntLsb = LOG2(log2_max_pic_order_cnt_lsb);
  cout << "\t最大图片顺序计数（POC）的LSB（最低有效位）的对数值:"
       << log2_max_pic_order_cnt_lsb << endl;

  sps_sub_layer_ordering_info_present_flag = bs->readU1();
  cout << "\t是否为每个子层提供单独的顺序信息:"
       << sps_sub_layer_ordering_info_present_flag << endl;

  // 解析层次和图像块大小
  int32_t n = sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers;
  for (int32_t i = n; i < sps_max_sub_layers; i++) {
    sps_max_dec_pic_buffering[i] = bs->readUE() + 1;
    sps_max_num_reorder_pics[i] = bs->readUE();
    sps_max_latency_increase[i] = bs->readUE() - 1;
    cout << "\t分别定义解码缓冲需求、重排序需求和最大允许的延迟增加:"
         << sps_max_dec_pic_buffering[i] << "," << sps_max_num_reorder_pics[i]
         << "," << sps_max_latency_increase[i] << endl;
    if (sps_max_latency_increase[i] != 0) {
      SpsMaxLatencyPictures[i] =
          sps_max_num_reorder_pics[i] + sps_max_latency_increase[i];
    }
  }

  // 亮度编码块的最小和最大尺寸
  MinCbLog2SizeY = log2_min_luma_coding_block_size = bs->readUE() + 3;
  min_cb_width = width >> log2_min_luma_coding_block_size;
  cout << "\t编码的块大小:" << log2_min_luma_coding_block_size << endl;

  int log2_min_pu_size = log2_min_luma_coding_block_size - 1;

  min_cb_width = width >> log2_min_luma_coding_block_size;
  min_cb_height = height >> log2_min_luma_coding_block_size;
  min_tb_width = width >> log2_min_luma_transform_block_size;
  min_tb_height = height >> log2_min_luma_transform_block_size;
  min_pu_width = width >> log2_min_pu_size;
  min_pu_height = height >> log2_min_pu_size;
  tb_mask = (1 << (CtbLog2SizeY - log2_min_luma_transform_block_size)) - 1;
  qp_bd_offset = 6 * (bit_depth_luma - 8);

  log2_diff_max_min_luma_coding_block_size = bs->readUE();
  cout << "\t编码变换块大小:" << log2_diff_max_min_luma_coding_block_size
       << endl;

  CtbLog2SizeY = MinCbLog2SizeY + log2_diff_max_min_luma_coding_block_size;
  MinCbSizeY = 1 << MinCbLog2SizeY;
  CtbSizeY = 1 << CtbLog2SizeY;

  PicWidthInMinCbsY = pic_width_in_luma_samples / MinCbSizeY;
  PicWidthInCtbsY = CEIL(pic_width_in_luma_samples / CtbSizeY);
  PicHeightInMinCbsY = pic_height_in_luma_samples / MinCbSizeY;
  PicHeightInCtbsY = CEIL(pic_height_in_luma_samples / CtbSizeY);
  PicSizeInMinCbsY = PicWidthInMinCbsY * PicHeightInMinCbsY;
  PicSizeInCtbsY = PicWidthInCtbsY * PicHeightInCtbsY;
  PicSizeInSamplesY = pic_width_in_luma_samples * pic_height_in_luma_samples;

  if (chroma_format_idc == 1 && separate_colour_plane_flag == 0)
    SubWidthC = SubHeightC = 2;
  else if (chroma_format_idc == 2 && separate_colour_plane_flag == 0)
    SubWidthC = 2;
  else
    SubWidthC = SubHeightC = 1;

  //7.4.3.10 RBSP slice segment trailing bits semantics
  RawMinCuBits = MinCbSizeY * MinCbSizeY *
                 (BitDepthY + 2 * BitDepthC / (SubWidthC * SubHeightC));

  PicWidthInSamplesC = pic_width_in_luma_samples / SubWidthC;
  PicHeightInSamplesC = pic_height_in_luma_samples / SubHeightC;

  ctb_width = (width + (1 << CtbLog2SizeY) - 1) >> CtbLog2SizeY;
  ctb_height = (height + (1 << CtbLog2SizeY) - 1) >> CtbLog2SizeY;
  ctb_size = ctb_width * ctb_height;

  if (chroma_format_idc != 0 && separate_colour_plane_flag != 1) {
    CtbWidthC = CtbSizeY / SubWidthC;
    CtbHeightC = CtbSizeY / SubHeightC;
  }

  log2_min_luma_transform_block_size = bs->readUE() + 2;
  log2_diff_max_min_luma_transform_block_size = bs->readUE();

  // 编码变换的层次深度
  max_transform_hierarchy_depth_inter = bs->readUE();
  max_transform_hierarchy_depth_intra = bs->readUE();

  // 是否启用缩放列表
  scaling_list_enabled_flag = bs->readUn(1);
  cout << "\t是否使用量化缩放列表:" << scaling_list_enabled_flag << endl;
  if (scaling_list_enabled_flag) {
    sps_scaling_list_data_present_flag = bs->readUn(1);
    cout << "\t是否在SPS中携带缩放列表数据:"
         << sps_scaling_list_data_present_flag << endl;
    if (sps_scaling_list_data_present_flag) scaling_list_data();
  }

  amp_enabled_flag = bs->readUn(1);
  cout << "\t异构模式分割（AMP）的启用标志:" << amp_enabled_flag << endl;

  // 改进去块效应
  sample_adaptive_offset_enabled_flag = bs->readUn(1);
  cout << "\t样本自适应偏移（SAO）的启用标志:"
       << sample_adaptive_offset_enabled_flag << endl;

  pcm_enabled_flag = bs->readUn(1);
  cout << "\tPCM（脉冲编码调制）模式:" << pcm_enabled_flag << endl;
  if (pcm_enabled_flag) {
    PcmBitDepthY = pcm_sample_bit_depth_luma = bs->readUn(4) + 1;
    PcmBitDepthC = pcm_sample_bit_depth_chroma = bs->readUn(4) + 1;
    cout << "\t定义PCM编码的亮度和色度位深:" << pcm_sample_bit_depth_luma << ","
         << pcm_sample_bit_depth_chroma << endl;
    log2_min_pcm_luma_coding_block_size = bs->readUE() + 3;
    log2_diff_max_min_pcm_luma_coding_block_size = bs->readUE();
    pcm_loop_filter_disabled_flag = bs->readUn(1);
    cout << "\t指示是否禁用循环滤波器:" << pcm_loop_filter_disabled_flag
         << endl;
  }

  num_short_term_ref_pic_sets = bs->readUE();
  cout << "\t短期参考帧的数量:" << num_short_term_ref_pic_sets << endl;
  for (int32_t i = 0; i < num_short_term_ref_pic_sets; i++) {
    std::cout << "hi~" << std::endl;
    return 0;
    //st_ref_pic_set(i);
  }

  long_term_ref_pics_present_flag = bs->readUn(1);
  if (long_term_ref_pics_present_flag) {
    num_long_term_ref_pics_sps = bs->readUE();
    cout << "\t长期参考图像的数量:" << num_long_term_ref_pics_sps << endl;
    for (int32_t i = 0; i < num_long_term_ref_pics_sps; i++) {
      lt_ref_pic_poc_lsb_sps[i] = bs->readUn(log2_max_pic_order_cnt_lsb);
      used_by_curr_pic_lt_sps_flag[i] = bs->readUn(1);
    }
  }

  // 一种用于帧间预测的技术，它允许从时间上相邻的参考帧中推导出当前帧的运动矢量
  sps_temporal_mvp_enabled_flag = bs->readUn(1);
  cout << "\t是否启用时间运动矢量预测(TMVP):" << sps_temporal_mvp_enabled_flag
       << endl;

  // 强帧内平滑是一种用于帧内预测的技术，主要用于处理大块区域的平滑过渡。它通过对帧内预测块的边界进行平滑处理，减少块效应（block artifacts）
  strong_intra_smoothing_enabled_flag = bs->readUn(1);
  cout << "\t示是否启用强帧内平滑:" << strong_intra_smoothing_enabled_flag
       << endl;

  //VUI
  vui_parameters_present_flag = bs->readUn(1);
  if (vui_parameters_present_flag) vui_parameters();

  //各种SPS扩展的启用标志
  sps_extension_present_flag = bs->readUn(1);
  if (sps_extension_present_flag) {
    sps_range_extension_flag = bs->readUn(1);
    if (sps_range_extension_flag) sps_range_extension();
    sps_multilayer_extension_flag = bs->readUn(1);
    //if (sps_multilayer_extension_flag) sps_multilayer_extension();
    sps_3d_extension_flag = bs->readUn(1);
    //if (sps_3d_extension_flag) sps_3d_extension();
    sps_scc_extension_flag = bs->readUn(1);
    if (sps_scc_extension_flag) sps_scc_extension();
    sps_extension_4bits = bs->readUn(4);
    if (sps_extension_4bits)
      while (bs->more_rbsp_data())
        int sps_extension_data_flag = bs->readUn(1);
  }

  bs->rbsp_trailing_bits();

  return 0;
}

// 7.4.5 Scaling list data semantics
int SPS::scaling_list_data() {
  int scaling_list_pred_mode_flag[32][32] = {{0}};
  int scaling_list_pred_matrix_id_delta[32][32] = {{0}};
  int ScalingList[32][32][32] = {{{0}}};
  int scaling_list_dc_coef_minus8[32][32] = {{0}};

  for (int sizeId = 0; sizeId < 4; sizeId++)
    for (int matrixId = 0; matrixId < 6; matrixId += (sizeId == 3) ? 3 : 1) {
      scaling_list_pred_mode_flag[sizeId][matrixId] = bs->readUn(1);
      if (!scaling_list_pred_mode_flag[sizeId][matrixId]) {
        scaling_list_pred_matrix_id_delta[sizeId][matrixId] = bs->readUE();
        // If scaling_list_pred_matrix_id_delta[ sizeId ][ matrixId ] is equal to 0, the scaling list is inferred from the default scaling list ScalingList[ sizeId ][ matrixId ][ i ] as specified in Table 7-5 and Table 7-6 for i = 0..Min( 63, ( 1 << ( 4 + ( sizeId << 1 ) ) ) − 1 )
        if (scaling_list_pred_matrix_id_delta[sizeId][matrixId] == 0) {

        } else {
          int efMatrixId =
              matrixId - scaling_list_pred_matrix_id_delta[sizeId][matrixId] *
                             (sizeId == 3 ? 3 : 1);
          for (int i = 0; i < MIN(63, (1 << (4 + (sizeId << 1))) - 1); ++i)
            ScalingList[sizeId][matrixId][i] = ScalingList[sizeId][matrixId][i];
        }
      } else {
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
  {
    int CoeffMinY =
        -(1 << (extended_precision_processing_flag ? MAX(15, BitDepthY + 6)
                                                   : 15));
    int CoeffMinC =
        -(1 << (extended_precision_processing_flag ? MAX(15, BitDepthC + 6)
                                                   : 15));
    int CoeffMaxY =
        (1 << (extended_precision_processing_flag ? MAX(15, BitDepthY + 6)
                                                  : 15)) -
        1;
    int CoeffMaxC =
        (1 << (extended_precision_processing_flag ? MAX(15, BitDepthC + 6)
                                                  : 15)) -
        1;
  }
  int intra_smoothing_disabled_flag = bs->readUn(1);
  int high_precision_offsets_enabled_flag = bs->readUn(1);
  {
    int WpOffsetBdShiftY =
        high_precision_offsets_enabled_flag ? 0 : (BitDepthY - 8);
    int WpOffsetBdShiftC =
        high_precision_offsets_enabled_flag ? 0 : (BitDepthC - 8);
    int WpOffsetHalfRangeY =
        1 << (high_precision_offsets_enabled_flag ? (BitDepthY - 1) : 7);
    int WpOffsetHalfRangeC =
        1 << (high_precision_offsets_enabled_flag ? (BitDepthC - 1) : 7);
  }
  int persistent_rice_adaptation_enabled_flag = bs->readUn(1);
  int cabac_bypass_alignment_enabled_flag = bs->readUn(1);
  return 0;
}

int SPS::sps_scc_extension() {
  int sps_curr_pic_ref_enabled_flag = bs->readUn(1);
  int palette_mode_enabled_flag = bs->readUn(1);

  if (palette_mode_enabled_flag) {
    int palette_max_size = bs->readUE();
    int delta_palette_max_predictor_size = bs->readUE();
    sps_palette_predictor_initializers_present_flag = bs->readUn(1);
    if (sps_palette_predictor_initializers_present_flag) {
      sps_num_palette_predictor_initializers_minus1 = bs->readUE();
      int numComps = (chroma_format_idc == 0) ? 1 : 3;
      for (int comp = 0; comp < numComps; comp++) {
        int bit_depth = comp == 0 ? bit_depth_luma : bit_depth_chroma;
        for (int i = 0; i <= sps_num_palette_predictor_initializers_minus1; i++)
          sps_palette_predictor_initializer[comp][i] = bs->readUn(bit_depth);
      }
    }
  }
  int motion_vector_resolution_control_idc = bs->readUn(2);
  int intra_boundary_filtering_disabled_flag = bs->readUn(1);
}
