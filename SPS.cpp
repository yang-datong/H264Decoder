#include "SPS.hpp"
#include <cstdint>
#include <iostream>
#include <ostream>

/* TODO YangJing 这个函数后续好好看一下 <24-09-13 10:16:29> */
void SPS::vui_parameters(BitStream &bitStream) {
  std::cout << "\tVUI -> {" << std::endl;
  aspect_ratio_info_present_flag = bitStream.readU1();
  if (aspect_ratio_info_present_flag) {
    aspect_ratio_idc = bitStream.readUn(8);
    std::cout << "\t\t宽高比标识符，视频的宽高比类型:" << aspect_ratio_idc
              << std::endl;
    if (aspect_ratio_idc == Extended_SAR) {
      sar_width = bitStream.readUn(16);
      std::cout << "\t\t表示样本的宽度（SAR，样本宽高比）:" << sar_width
                << std::endl;
      sar_height = bitStream.readUn(16);
      std::cout << "\t\t表示样本的高度（SAR，样本宽高比）:" << sar_height
                << std::endl;
    }
  }
  overscan_info_present_flag = bitStream.readU1();
  if (overscan_info_present_flag) {
    overscan_appropriate_flag = bitStream.readU1();
    std::cout << "\t\t视频适合超扫描显示:" << overscan_appropriate_flag
              << std::endl;
  }

  video_signal_type_present_flag = bitStream.readU1();
  if (video_signal_type_present_flag) {
    video_format = bitStream.readUn(3);
    std::cout << "\t\t视频格式标识符，视频的类型（如未压缩、压缩等）:"
              << (int)video_format << std::endl;
    video_full_range_flag = bitStream.readU1();
    std::cout << "\t\t视频使用全范围色彩（0-255）或限范围色彩（16-235）:"
              << video_full_range_flag << std::endl;
    colour_description_present_flag = bitStream.readU1();
    if (colour_description_present_flag) {
      colour_primaries = bitStream.readUn(8);
      std::cout << "\t\t颜色原色的类型（如BT.709、BT.601等）:"
                << (int)colour_primaries << std::endl;
      transfer_characteristics = bitStream.readUn(8);
      std::cout << "\t\t传输特性（如线性、伽马等）:"
                << (int)transfer_characteristics << std::endl;
      matrix_coefficients = bitStream.readUn(8);
      std::cout << "\t\t矩阵系数，用于颜色空间转换:" << (int)matrix_coefficients
                << std::endl;
    }
  }

  chroma_loc_info_present_flag = bitStream.readU1();
  if (chroma_loc_info_present_flag) {
    chroma_sample_loc_type_top_field = bitStream.readSE();
    chroma_sample_loc_type_bottom_field = bitStream.readSE();
    std::cout << "\t\t顶场色度样本位置类型:" << chroma_sample_loc_type_top_field
              << ",底场色度样本位置类型:" << chroma_sample_loc_type_bottom_field
              << std::endl;
  }

  timing_info_present_flag = bitStream.readU1();
  if (timing_info_present_flag) {
    num_units_in_tick = bitStream.readUn(32);
    time_scale = bitStream.readUn(32);
    std::cout << "\t\t每个时钟周期的单位数:" << num_units_in_tick
              << ",每秒的单位数(时间尺度):" << time_scale << std::endl;
    fixed_frame_rate_flag = bitStream.readU1();
    std::cout << "\t\t使用固定帧率:" << fixed_frame_rate_flag << std::endl;
  }

  nal_hrd_parameters_present_flag = bitStream.readU1();
  vcl_hrd_parameters_present_flag = bitStream.readU1();
  if (nal_hrd_parameters_present_flag) hrd_parameters(bitStream);
  if (vcl_hrd_parameters_present_flag) hrd_parameters(bitStream);
  std::cout << "\t\t存在NAL HRD（网络提取率控制）参数:"
            << nal_hrd_parameters_present_flag
            << ",存在VCL HRD参数:" << vcl_hrd_parameters_present_flag
            << std::endl;

  if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
    low_delay_hrd_flag = bitStream.readU1();
    std::cout << "\t\t使用低延迟HRD:" << low_delay_hrd_flag << std::endl;
  }

  pic_struct_present_flag = bitStream.readU1();
  std::cout << "\t\t存在图像结构信息:" << pic_struct_present_flag << std::endl;
  bitstream_restriction_flag = bitStream.readU1();
  std::cout << "\t\t存在比特流限制:" << bitstream_restriction_flag << std::endl;

  if (bitstream_restriction_flag) {
    motion_vectors_over_pic_boundaries_flag = bitStream.readU1();
    std::cout << "\t\t允许运动矢量跨越图像边界:"
              << motion_vectors_over_pic_boundaries_flag << std::endl;
    max_bytes_per_pic_denom = bitStream.readUE();
    std::cout << "\t\t每帧最大字节数的分母:" << max_bytes_per_pic_denom
              << std::endl;
    max_bits_per_mb_denom = bitStream.readUE();
    std::cout << "\t\t每个宏块最大比特数的分母:" << max_bits_per_mb_denom
              << std::endl;
    log2_max_mv_length_horizontal = bitStream.readUE();
    log2_max_mv_length_vertical = bitStream.readUE();
    std::cout << "\t\t水平运动矢量的最大长度的对数值:"
              << log2_max_mv_length_horizontal
              << ",垂直运动矢量的最大长度的对数值:"
              << log2_max_mv_length_vertical << std::endl;
    max_num_reorder_frames = bitStream.readUE();
    std::cout << "\t\t最大重排序帧数:" << max_num_reorder_frames << std::endl;
    max_dec_frame_buffering = bitStream.readUE();
    std::cout << "\t\t最大解码帧缓冲区大小:" << max_dec_frame_buffering
              << std::endl;
  }

  if (max_num_reorder_frames == -1) {
    if ((profile_idc == 44 || profile_idc == 86 || profile_idc == 100 ||
         profile_idc == 110 || profile_idc == 122 || profile_idc == 244) &&
        constraint_set3_flag)
      max_num_reorder_frames = 0;
    else {
      int32_t MaxDpbFrames = 0;
      for (int i = 0; i < 19; ++i) {
        if (level_idc == LevelNumber_MaxDpbMbs[i][0])
          MaxDpbFrames = MIN(LevelNumber_MaxDpbMbs[i][1] /
                                 (PicWidthInMbs * FrameHeightInMbs),
                             16);
        break;
      }
      max_num_reorder_frames = MaxDpbFrames;
    }
  }

  int32_t fps = 0;
  if (vui_parameters_present_flag && timing_info_present_flag)
    fps = time_scale / num_units_in_tick / 2;
  std::cout << "\t\tfps:" << fps << std::endl;
  std::cout << "\t }" << std::endl;
}

void SPS::hrd_parameters(BitStream &bitStream) {
  cpb_cnt_minus1 = bitStream.readUE();
  bit_rate_scale = bitStream.readUn(8);
  cpb_size_scale = bitStream.readUn(8);

  bit_rate_value_minus1 = new uint32_t[cpb_cnt_minus1];
  cpb_size_value_minus1 = new uint32_t[cpb_cnt_minus1];
  cbr_flag = new bool[cpb_cnt_minus1];

  for (int SchedSelIdx = 0; SchedSelIdx <= (int)cpb_cnt_minus1; SchedSelIdx++) {
    bit_rate_value_minus1[SchedSelIdx] = bitStream.readUE();
    cpb_size_value_minus1[SchedSelIdx] = bitStream.readUE();
    cbr_flag[SchedSelIdx] = bitStream.readU1();
  }
  initial_cpb_removal_delay_length_minus1 = bitStream.readUn(5);
  cpb_removal_delay_length_minus1 = bitStream.readUn(5);
  dpb_output_delay_length_minus1 = bitStream.readUn(5);
  time_offset_length = bitStream.readUn(5);
}

int SPS::extractParameters(BitStream &bs) {
  /* 读取profile_idc等等(4 bytes) */
  profile_idc = bs.readUn(8); // 0x64
  constraint_set0_flag = bs.readUn(1);
  constraint_set1_flag = bs.readUn(1);
  constraint_set2_flag = bs.readUn(1);
  constraint_set3_flag = bs.readUn(1);
  constraint_set4_flag = bs.readUn(1);
  constraint_set5_flag = bs.readUn(1);
  reserved_zero_2bits = bs.readUn(2);
  level_idc = bs.readUn(8); // 0
  seq_parameter_set_id = bs.readUE();
  std::cout << "\tSPS ID:" << seq_parameter_set_id << std::endl;
  std::cout << "\tlevel_idc:" << (int)level_idc << std::endl;
  // 通过gdb断点到这里然后 "p /t {ssp._buf[1],profile_idc}"即可判断是否读取正确

  switch (profile_idc) {
  case 66:
    std::cout << "\tprofile_idc:Baseline" << std::endl;
    break;
  case 77:
    std::cout << "\tprofile_idc:Main" << std::endl;
    break;
  case 100:
    std::cout << "\tprofile_idc:High" << std::endl;
    break;
  default:
    break;
  }

  //constraint_set5_flag is specified as follows -> page 74
  if ((profile_idc == 77 || profile_idc == 88 || profile_idc == 100) &&
      constraint_set5_flag)
    std::cout << "\t当前含有B Slice" << std::endl;

  if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 ||
      profile_idc == 244 || profile_idc == 44 || profile_idc == 83 ||
      profile_idc == 86 || profile_idc == 118 || profile_idc == 128 ||
      profile_idc == 138 || profile_idc == 139 || profile_idc == 134 ||
      profile_idc == 135) {
    chroma_format_idc = bs.readUE();
    switch (chroma_format_idc) {
    case 0:
      std::cout << "\tchroma_format_idc:YUV400" << std::endl;
      break;
    case 1:
      std::cout << "\tchroma_format_idc:YUV420" << std::endl;
      break;
    case 2:
      std::cout << "\tchroma_format_idc:YUV422" << std::endl;
      break;
    case 3:
      std::cout << "\tchroma_format_idc:YUB444" << std::endl;
      separate_colour_plane_flag = bs.readU1();
      break;
    }

    bit_depth_luma_minus8 = bs.readUE();
    bit_depth_chroma_minus8 = bs.readUE();

    qpprime_y_zero_transform_bypass_flag = bs.readU1();
    seq_scaling_matrix_present_flag = bs.readU1();
    std::cout << "\t编码器是否提供量化矩阵:" << seq_scaling_matrix_present_flag
              << std::endl;

    if (seq_scaling_matrix_present_flag) {
      /* 读取编码器提供的特定量化矩阵 */
      for (int i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++) {
        seq_scaling_list_present_flag[i] = bs.readU1();
        if (seq_scaling_list_present_flag[i]) {
          if (i <= 5)
            scaling_list(bs, ScalingList4x4[i], 16,
                         UseDefaultScalingMatrix4x4Flag[i]);
          else
            scaling_list(bs, ScalingList8x8[i - 6], 64,
                         UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
      }
    }
  }

  /* 确定色度数组类型,YUV400,YUV420,YUV422,YUV444... 74 page */
  ChromaArrayType = (separate_colour_plane_flag) ? 0 : chroma_format_idc;
  /* 7.4.2.1.1 Sequence parameter set data semantics -> (7-3) */
  BitDepthY = bit_depth_luma_minus8 + 8;
  QpBdOffsetY = bit_depth_luma_minus8 * 6;
  BitDepthC = bit_depth_chroma_minus8 + 8;
  QpBdOffsetC = bit_depth_chroma_minus8 * 6;
  std::cout << "\t亮度分量位深:" << BitDepthY << ",色度分量位深:" << BitDepthC
            << std::endl;
  std::cout << "\t亮度分量Qp:" << QpBdOffsetY << ",色度分量Qp:" << QpBdOffsetC
            << std::endl;

  // 色度分量的采样宽度  (7-7)
  RawMbBits = 256 * BitDepthY + 2 * MbWidthC * MbHeightC * BitDepthC;

  log2_max_frame_num_minus4 = bs.readUE();
  pic_order_cnt_type = bs.readUE();

  int32_t *offset_for_ref_frame = nullptr;
  if (pic_order_cnt_type == 0)
    log2_max_pic_order_cnt_lsb_minus4 = bs.readUE();
  else if (pic_order_cnt_type == 1) {
    delta_pic_order_always_zero_flag = bs.readU1();
    offset_for_non_ref_pic = bs.readSE();
    offset_for_top_to_bottom_field = bs.readSE();
    num_ref_frames_in_pic_order_cnt_cycle = bs.readUE();
    if (num_ref_frames_in_pic_order_cnt_cycle != 0)
      offset_for_ref_frame = new int32_t[num_ref_frames_in_pic_order_cnt_cycle];

    for (int i = 0; i < (int)num_ref_frames_in_pic_order_cnt_cycle; i++)
      offset_for_ref_frame[i] = bs.readSE();
  }

  /* 计算最大帧号和最大图像顺序计数 LSB (7-10,7-11)*/
  MaxFrameNum = pow(log2_max_frame_num_minus4 + 4, 2);
  MaxPicOrderCntLsb = pow(log2_max_pic_order_cnt_lsb_minus4 + 4, 2);

  /* 当 max_num_ref_frames 等于 0 时，slice_type 应等于 I或SI Slice -> page 87 */
  max_num_ref_frames = bs.readUE();
  std::cout << "\t解码器需要支持的最大参考帧数:" << max_num_ref_frames
            << std::endl;
  gaps_in_frame_num_value_allowed_flag = bs.readU1();
  pic_width_in_mbs_minus1 = bs.readUE();
  pic_height_in_map_units_minus1 = bs.readUE();

  frame_mbs_only_flag = bs.readU1();
  if (!frame_mbs_only_flag) {
    std::cout << "\t当前存在场编码:" << !frame_mbs_only_flag << std::endl;
    mb_adaptive_frame_field_flag = bs.readU1();
    std::cout << "\t是否使用基于宏块的自适应帧/场编码:"
              << mb_adaptive_frame_field_flag << std::endl;
  } else
    std::cout << "\t仅有帧编码:" << frame_mbs_only_flag << std::endl;

  direct_8x8_inference_flag = bs.readU1();
  frame_cropping_flag = bs.readU1();
  if (frame_cropping_flag) {
    frame_crop_left_offset = bs.readUE();
    std::cout << "\t";
    std::cout << "帧裁剪左偏移量:" << frame_crop_left_offset;
    frame_crop_right_offset = bs.readUE();
    std::cout << ",帧裁剪右偏移量:" << frame_crop_left_offset;
    frame_crop_top_offset = bs.readUE();
    std::cout << ",帧裁剪顶偏移量:" << frame_crop_left_offset;
    frame_crop_bottom_offset = bs.readUE();
    std::cout << ",帧裁剪底偏移量:" << frame_crop_left_offset;
    std::cout << std::endl;
  }

  vui_parameters_present_flag = bs.readU1();
  std::cout << "\t存在视频用户界面(VUI)参数:" << vui_parameters_present_flag
            << std::endl;

  if (vui_parameters_present_flag) vui_parameters(bs);

  // 宏块单位的图像宽度 = pic_width_in_mbs_minus1 + 1 (7-13)
  PicWidthInMbs = pic_width_in_mbs_minus1 + 1;
  // 宏块单位的图像高度 = pic_height_in_map_units_minus1 + 1
  PicHeightInMapUnits = pic_height_in_map_units_minus1 + 1;
  // 宏块单位的图像大小 = 宽 * 高
  PicSizeInMapUnits = PicWidthInMbs * PicHeightInMapUnits;

  //(7-18)
  FrameHeightInMbs = (2 - frame_mbs_only_flag) * PicHeightInMapUnits;

  /* 6.2 Source, decoded, and output picture formats */
  derived_SubWidthC_and_SubHeightC();

  /* 计算采样宽度和比特深度 */
  picWidthInSamplesL = PicWidthInMbs * 16;
  // 亮度分量的采样宽度，等于宏块宽度乘以 16
  picWidthInSamplesC = PicWidthInMbs * MbWidthC;
  std::cout << "\tCodec width:" << picWidthInSamplesL
            << ", Codec height:" << PicHeightInMapUnits * 16 << std::endl;
  return 0;
}

// 6.2 Source, decoded, and output picture formats
int SPS::derived_SubWidthC_and_SubHeightC() {
  int32_t _chroma_format_idc = chroma_format_idc;
  //Table 6-1 – SubWidthC, and SubHeightC values derived from chroma_format_idc and separate_colour_plane_flag
  /* 均无子宽高 */
  if (chroma_format_idc == 0 || separate_colour_plane_flag)
    MbWidthC = MbHeightC = 0;
  else {
    if (chroma_format_idc == 3 && separate_colour_plane_flag)
      /* YUV444 且每个分量都单独编码 */
      _chroma_format_idc = 4;
    //不单独编码分量，YUV420,YUV422,YUV444的情况
    Chroma_Format = chroma_format_idcs[_chroma_format_idc].Chroma_Format;
    SubWidthC = chroma_format_idcs[_chroma_format_idc].SubWidthC;
    SubHeightC = chroma_format_idcs[_chroma_format_idc].SubHeightC;
    MbWidthC = 16 / SubWidthC;   //(6-1)
    MbHeightC = 16 / SubHeightC; //(6-2)
  }
  return 0;
}

// 7.3.2.1.2 Sequence parameter set extension RBSP syntax
int SPS::seq_parameter_set_extension_rbsp() {
  /* TODO YangJing  <24-09-08 23:19:32> */
  return 0;
}

// 7.3.2.1.3 Subset sequence parameter set RBSP syntax
int subset_seq_parameter_set_rbsp() {
  /* TODO YangJing  <24-09-08 23:20:38> */
  return 0;
}
