#include "SPS.hpp"
#include "Type.hpp"
#include <iostream>
#include <ostream>

SPS::SPS() {
  if (pic_order_cnt_type == 1) {
    ExpectedDeltaPerPicOrderCntCycle = 0;
    for (int i = 0; i < (int32_t)num_ref_frames_in_pic_order_cnt_cycle; i++) {
      ExpectedDeltaPerPicOrderCntCycle += offset_for_ref_frame[i];
    }
  }
}

SPS::~SPS() {}

void SPS::vui_parameters(BitStream &bitStream) {
  bool aspect_ratio_info_present_flag = bitStream.readU1();
  if (aspect_ratio_info_present_flag) {
    uint8_t aspect_ratio_idc = bitStream.readUn(8);
    if (aspect_ratio_idc == Extended_SAR) {
      uint16_t sar_width = bitStream.readUn(16);
      uint16_t sar_height = bitStream.readUn(16);
    }
  }
  bool overscan_info_present_flag = bitStream.readU1();
  if (overscan_info_present_flag)
    bool overscan_appropriate_flag = bitStream.readU1();

  bool video_signal_type_present_flag = bitStream.readU1();

  if (video_signal_type_present_flag) {
    uint8_t video_format = bitStream.readUn(3);
    bool video_full_range_flag = bitStream.readU1();
    bool colour_description_present_flag = bitStream.readU1();
    if (colour_description_present_flag) {
      uint8_t colour_primaries = bitStream.readUn(8);
      uint8_t transfer_characteristics = bitStream.readUn(8);
      uint8_t matrix_coefficients = bitStream.readUn(8);
    }
  }

  bool chroma_loc_info_present_flag = bitStream.readU1();
  if (chroma_loc_info_present_flag) {
    int32_t chroma_sample_loc_type_top_field = bitStream.readSE();
    int32_t chroma_sample_loc_type_bottom_field = bitStream.readSE();
  }
  bool timing_info_present_flag = bitStream.readU1();

  if (timing_info_present_flag) {
    uint32_t num_units_in_tick = bitStream.readUn(32);
    uint32_t time_scale = bitStream.readUn(32);
    bool fixed_frame_rate_flag = bitStream.readU1();
  }
  bool nal_hrd_parameters_present_flag = bitStream.readU1();
  if (nal_hrd_parameters_present_flag)
    hrd_parameters(bitStream);

  bool vcl_hrd_parameters_present_flag = bitStream.readU1();
  if (vcl_hrd_parameters_present_flag)
    hrd_parameters(bitStream);

  if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag)
    bool low_delay_hrd_flag = bitStream.readU1();

  bool pic_struct_present_flag = bitStream.readU1();
  bool bitstream_restriction_flag = bitStream.readU1();

  if (bitstream_restriction_flag) {
    bool motion_vectors_over_pic_boundaries_flag = bitStream.readU1();
    uint32_t max_bytes_per_pic_denom = bitStream.readUE();
    uint32_t max_bits_per_mb_denom = bitStream.readUE();
    uint32_t log2_max_mv_length_horizontal = bitStream.readUE();
    uint32_t log2_max_mv_length_vertical = bitStream.readUE();
    uint32_t max_num_reorder_frames = bitStream.readUE();
    uint32_t max_dec_frame_buffering = bitStream.readUE();
  }
}

void SPS::hrd_parameters(BitStream &bitStream) {
  uint32_t cpb_cnt_minus1 = bitStream.readUE();
  uint8_t bit_rate_scale = bitStream.readUn(8);
  uint8_t cpb_size_scale = bitStream.readUn(8);

  uint32_t *bit_rate_value_minus1 = new uint32_t[cpb_cnt_minus1];
  uint32_t *cpb_size_value_minus1 = new uint32_t[cpb_cnt_minus1];
  bool *cbr_flag = new bool[cpb_cnt_minus1];

  for (int SchedSelIdx = 0; SchedSelIdx <= cpb_cnt_minus1; SchedSelIdx++) {
    bit_rate_value_minus1[SchedSelIdx] = bitStream.readUE();
    cpb_size_value_minus1[SchedSelIdx] = bitStream.readUE();
    cbr_flag[SchedSelIdx] = bitStream.readU1();
  }
  uint8_t initial_cpb_removal_delay_length_minus1 = bitStream.readUn(5);
  uint8_t cpb_removal_delay_length_minus1 = bitStream.readUn(5);
  uint8_t dpb_output_delay_length_minus1 = bitStream.readUn(5);
  uint8_t time_offset_length = bitStream.readUn(5);
}

int SPS::extractParameters() {
  /* 初始化bit处理器，填充sps的数据 */
  BitStream bitStream(_buf, _len);

  /* 读取profile_idc等等(4 bytes) */
  uint8_t profile_idc = bitStream.readUn(8); // 0x64
  uint8_t constraint_set0_5_flag = bitStream.readUn(6);
  uint8_t reserved_zero_2bits = bitStream.readUn(2);
  uint8_t level_idc = bitStream.readUn(8); // 0
  uint32_t seq_parameter_set_id = bitStream.readUE();
  std::cout << "\tseq_parameter_set_id:" << seq_parameter_set_id << std::endl;
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

  if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 ||
      profile_idc == 244 || profile_idc == 44 || profile_idc == 83 ||
      profile_idc == 86 || profile_idc == 118 || profile_idc == 128 ||
      profile_idc == 138 || profile_idc == 139 || profile_idc == 134 ||
      profile_idc == 135) {
    chroma_format_idc = bitStream.readUE();
    switch (chroma_format_idc) {
    case 0:
      std::cout << "\tchroma_format_idc:单色" << std::endl;
      break;
    case 1:
      std::cout << "\tchroma_format_idc:YUV420" << std::endl;
      break;
    case 2:
      std::cout << "\tchroma_format_idc:YUV422" << std::endl;
      break;
    case 3:
      std::cout << "\tchroma_format_idc:YUB444" << std::endl;
      separate_colour_plane_flag = bitStream.readU1();
      break;
    }
    bit_depth_luma_minus8 = bitStream.readUE();
    bit_depth_chroma_minus8 = bitStream.readUE();
    qpprime_y_zero_transform_bypass_flag = bitStream.readU1();
    seq_scaling_matrix_present_flag = bitStream.readU1();

    if (seq_scaling_matrix_present_flag) {
      for (int i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++) {
        seq_scaling_list_present_flag[i] = bitStream.readU1();
        if (seq_scaling_list_present_flag[i]) {
          if (i < 6)
            scaling_list(bitStream, ScalingList4x4[i], 16,
                         UseDefaultScalingMatrix4x4Flag[i]);
          else
            scaling_list(bitStream, ScalingList8x8[i - 6], 64,
                         UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
      }
    }
  }

  uint32_t log2_max_frame_num_minus4 = bitStream.readUE();
  pic_order_cnt_type = bitStream.readUE();

  int32_t *offset_for_ref_frame = nullptr;

  if (pic_order_cnt_type == 0) {
    log2_max_pic_order_cnt_lsb_minus4 = bitStream.readUE();
  } else if (pic_order_cnt_type == 1) {
    delta_pic_order_always_zero_flag = bitStream.readU1();
    offset_for_non_ref_pic = bitStream.readSE();
    offset_for_top_to_bottom_field = bitStream.readSE();
    num_ref_frames_in_pic_order_cnt_cycle = bitStream.readUE();
    if (num_ref_frames_in_pic_order_cnt_cycle != 0)
      offset_for_ref_frame = new int32_t[num_ref_frames_in_pic_order_cnt_cycle];
    /* TODO YangJing [offset_for_ref_frame -> delete] <24-04-04 01:24:42> */

    for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
      offset_for_ref_frame[i] = bitStream.readSE();
  }

  max_num_ref_frames = bitStream.readUE();
  gaps_in_frame_num_value_allowed_flag = bitStream.readU1();
  pic_width_in_mbs_minus1 = bitStream.readUE();
  pic_height_in_map_units_minus1 = bitStream.readUE();

  frame_mbs_only_flag = bitStream.readU1();
  if (!frame_mbs_only_flag)
    mb_adaptive_frame_field_flag = bitStream.readU1();
  direct_8x8_inference_flag = bitStream.readU1();
  frame_cropping_flag = bitStream.readU1();
  if (frame_cropping_flag) {
    frame_crop_left_offset = bitStream.readUE();
    frame_crop_right_offset = bitStream.readUE();
    frame_crop_top_offset = bitStream.readUE();
    frame_crop_bottom_offset = bitStream.readUE();
  }
  vui_parameters_present_flag = bitStream.readU1();
  if (vui_parameters_present_flag)
    vui_parameters(bitStream);

  /* 计算宏块大小以及图像宽、高 */
  PicWidthInMbs = pic_width_in_mbs_minus1 + 1;
  // 宏块单位的图像宽度 = pic_width_in_mbs_minus1 + 1
  PicHeightInMapUnits = pic_height_in_map_units_minus1 + 1;
  // 宏块单位的图像高度 = pic_height_in_map_units_minus1 + 1
  PicSizeInMapUnits = PicWidthInMbs * PicHeightInMapUnits;
  // 宏块单位的图像大小 = 宽 * 高
  frameHeightInMbs = (2 - frame_mbs_only_flag) * PicHeightInMapUnits;
  // 指示图像是否仅包含帧（而不是场）。
  // frame_mbs_only_flag 为1，则图像仅包含帧，并且帧高度等于图像高度。
  // frame_mbs_only_flag 为0，则图像包含场，并且帧高度等于图像高度的一半。

  //----------- 下面都是一些需要进行额外计算的（文档都有需要自己找）------------
  int width = (pic_width_in_mbs_minus1 + 1) * 16;
  int height = (pic_height_in_map_units_minus1 + 1) * 16;
  printf("\tprediction width:%d, prediction height:%d\n", width, height);
  /* TODO YangJing 这里的高为什么是1088？ <24-07-30 20:18:36> */

  /* 获取帧率 */
  /* TODO YangJing  <24-04-05 00:22:50> */

  /* 获取B帧是否配置 */
  /* TODO YangJing  <24-04-05 00:30:03> */

  /* 确定色度数组类型 74 page */
  if (separate_colour_plane_flag == 0)
    ChromaArrayType = chroma_format_idc;
  else
    ChromaArrayType = 0;

  /* 计算位深度 */
  BitDepthY = bit_depth_luma_minus8 + 8;
  // 亮度分量的位深度
  QpBdOffsetY = bit_depth_luma_minus8 * 6;
  // 亮度分量的量化参数步长偏移
  BitDepthC = bit_depth_chroma_minus8 + 8;
  // 色度分量的位深度
  QpBdOffsetC = bit_depth_chroma_minus8 * 6;
  // 色度分量的量化参数步长偏移
  //

  CHROMA_FORMAT_IDC_T g_chroma_format_idcs[5] = {
      {0, 0, MONOCHROME, NA, NA},
      {1, 0, CHROMA_FORMAT_IDC_420, 2, 2},
      {2, 0, CHROMA_FORMAT_IDC_422, 2, 1},
      {3, 0, CHROMA_FORMAT_IDC_444, 1, 1},
      {3, 1, CHROMA_FORMAT_IDC_444, NA, NA},
  };

  /* 计算色度子采样参数 */
  if (chroma_format_idc == 0 || separate_colour_plane_flag == 1) {
    // 色度子采样宽度和高度均为 0。
    MbWidthC = 0;
    MbHeightC = 0;
  } else {
    int32_t index = chroma_format_idc;
    if (chroma_format_idc == 3 && separate_colour_plane_flag == 1) {
      index = 4;
    }
    Chroma_Format = g_chroma_format_idcs[index].Chroma_Format;
    SubWidthC = g_chroma_format_idcs[index].SubWidthC;
    SubHeightC = g_chroma_format_idcs[index].SubHeightC;
    //  根据 chroma_format_idc
    //  查找色度格式、色度子采样宽度和色度子采样高度。

    MbWidthC = 16 / SubWidthC;
    MbHeightC = 16 / SubHeightC;
  }

  /* 计算采样宽度和比特深度 */
  picWidthInSamplesL = PicWidthInMbs * 16;
  // 亮度分量的采样宽度，等于宏块宽度乘以 16
  picWidthInSamplesC = PicWidthInMbs * MbWidthC;
  // 色度分量的采样宽度，等于宏块宽度乘以 MbWidthC。
  RawMbBits = 256 * BitDepthY + 2 * MbWidthC * MbHeightC * BitDepthY;

  /* 计算最大帧号和最大图像顺序计数 LSB  in 77 page*/
  /*
   *log2_max_frame_num_minus4 specifies the value of the variable
slice.MaxFrameNum that is used in frame_num related derivations as follows:
$$
    slice.MaxFrameNum = 2^{( log2_max_frame_num_minus4 + 4 )}
$$
The value of log2_max_frame_num_minus4 shall be in the range of 0 to
12,inclusive.
   * */
  MaxFrameNum = std::pow(log2_max_frame_num_minus4 + 4, 2);
  MaxPicOrderCntLsb = std::pow(log2_max_pic_order_cnt_lsb_minus4 + 4, 2);

  /* 计算预期图像顺序计数周期增量 */
  if (pic_order_cnt_type == 1) {
    int expectedDeltaPerPicOrderCntCycle = 0;
    for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
      expectedDeltaPerPicOrderCntCycle += offset_for_ref_frame[i];
    }
  }

  return 0;
}
