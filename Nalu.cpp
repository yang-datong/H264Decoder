#include "Nalu.hpp"

#define Extended_SAR 255

Nalu::Nalu() {}

Nalu::~Nalu() {
  if (_buffer != nullptr) {
    free(_buffer);
    _buffer = nullptr;
  }
}

int Nalu::setBuffer(uint8_t *buf, int len) {
  if (_buffer != nullptr) {
    free(_buffer);
    _buffer = nullptr;
  }
  uint8_t *tmpBuf = (uint8_t *)malloc(len);
  memcpy(tmpBuf, buf, len);
  _buffer = tmpBuf;
  _len = len;
  return 0;
}

int Nalu::parseEBSP(EBSP &ebsp) {
  ebsp._len = _len - _startCodeLenth;
  uint8_t *ebspBuffer = new uint8_t[ebsp._len];
  memcpy(ebspBuffer, _buffer + _startCodeLenth, ebsp._len);
  ebsp._buf = ebspBuffer;
  return 0;
}

/* 注意，这里解析出来的RBSP是不包括RBSP head的一个字节的 */
int Nalu::parseRBSP(EBSP &ebsp, RBSP &rbsp) {
  parseHeader(ebsp); // RBSP的头也是EBSP的头

  bool NumBytesInRBSP = 0;
  bool nalUnitHeaderBytes = 1; // nalUnitHeaderBytes的默认head大小为1字节

  if (nal_unit_type == 14 || nal_unit_type == 20 || nal_unit_type == 21) {
    bool svc_extension_flag = 0, avc_3d_extension_flag = 0;

    if (nal_unit_type != 21)
      svc_extension_flag = ebsp._buf[1] >> 7;
    else
      avc_3d_extension_flag = ebsp._buf[1] >> 7;

    if (svc_extension_flag)
      // nal_unit_header_svc_extension();
      nalUnitHeaderBytes += 3;
    else if (avc_3d_extension_flag)
      // nal_unit_header_3davc_extension()
      /* specified in Annex J */
      nalUnitHeaderBytes += 2;
    else
      // nal_unit_header_mvc_extension()
      /* specified in Annex H */
      nalUnitHeaderBytes += 3;
  }
  if (nalUnitHeaderBytes != 1) {
    std::cerr << "\033[31m未实现NAL head 为多字节的情况~\033[0m" << std::endl;
    return -1;
  }

  uint8_t *rbspBuffer = new uint8_t[ebsp._len - 1]; // 去掉RBSP head (1 byte)
  int index = 0;
  rbspBuffer[index++] = ebsp._buf[1]; // 从RBSP body开始
  rbspBuffer[index++] = ebsp._buf[2];
  rbsp._len = ebsp._len - 1; // 不包括RBSP head
  for (int i = 3; i < ebsp._len - 1; i++) {
    if (ebsp._buf[i] == 3 && ebsp._buf[i - 1] == 0 && ebsp._buf[i - 2] == 0) {
      if (ebsp._buf[i + 1] == 0 || ebsp._buf[i + 1] == 1 ||
          ebsp._buf[i + 1] == 2 || ebsp._buf[i + 1] == 3)
        // 满足0030, 0031, 0032, 0033的特征，故一定是防竞争字节序
        rbsp._len--;
    } else
      rbspBuffer[index++] = ebsp._buf[i];
    // 如果不是防竞争字节序就依次放入到rbspbuff
  }
  rbsp._buf = rbspBuffer;
  return 0;
}

int Nalu::parseHeader(EBSP &ebsp) {
  uint8_t firstByte = ebsp._buf[0];
  nal_unit_type = firstByte & 0b00011111;
  /* 取低5bit，即0-4 bytes */
  nal_ref_idc = (firstByte & 0b01100000) >> 5;
  /* 取5-6 bytes */
  forbidden_zero_bit = firstByte >> 7;
  /* 取最高位，即7 byte */
  return 0;
}

int Nalu::extractSSPparameters(RBSP &ssp) {
  /* 初始化bit处理器，填充ssp的数据 */
  BitStream bitStream(ssp._buf, ssp._len);

  /* 读取profile_idc等等(4 bytes) */
  uint8_t profile_idc = bitStream.readUn(8); // 0x64
  uint8_t constraint_set0_5_flag = bitStream.readUn(6);
  uint8_t reserved_zero_2bits = bitStream.readUn(2);
  uint8_t level_idc = bitStream.readUn(8); // 0
  uint32_t seq_parameter_set_id = bitStream.readUE();
  // 通过gdb断点到这里然后 "p /t {ssp._buf[1],profile_idc}"即可判断是否读取正确

  switch (profile_idc) {
  case 66:
    std::cout << "profile_idc: Baseline" << std::endl;
    break;
  case 77:
    std::cout << "profile_idc: Main" << std::endl;
    break;
  case 100:
    std::cout << "profile_idc: High" << std::endl;
    break;
  default:
    std::cout << "TODO  <24-03-22 02:39:03, YangJing> " << std::endl;
    break;
  }

  if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 ||
      profile_idc == 244 || profile_idc == 44 || profile_idc == 83 ||
      profile_idc == 86 || profile_idc == 118 || profile_idc == 128 ||
      profile_idc == 138 || profile_idc == 139 || profile_idc == 134 ||
      profile_idc == 135) {
    uint32_t chroma_format_idc = bitStream.readUE();
    switch (chroma_format_idc) {
    case 0:
      // std::cout << "单色" << std::endl;
      break;
    case 1:
      // std::cout << "YUV420" << std::endl;
      break;
    case 2:
      // std::cout << "YUV422" << std::endl;
      break;
    case 3:
      std::cout << "YUB443" << std::endl;
      bool separate_colour_plane_flag = bitStream.readU1();
      std::cout << "separate_colour_plane_flag:" << separate_colour_plane_flag
                << std::endl;
      break;
    }
    uint32_t bit_depth_luma_minus8 = bitStream.readUE();
    uint32_t bit_depth_chroma_minus8 = bitStream.readUE();
    bool qpprime_y_zero_transform_bypass_flag = bitStream.readU1();
    bool seq_scaling_matrix_present_flag = bitStream.readU1();

    uint32_t ScalingList4x4[6][16];
    uint32_t ScalingList8x8[6][64];

    uint32_t UseDefaultScalingMatrix4x4Flag[6];
    uint32_t UseDefaultScalingMatrix8x8Flag[6];

    if (seq_scaling_matrix_present_flag) {
      bool seq_scaling_list_present_flag[12] = {false};
      for (int i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++) {
        seq_scaling_list_present_flag[i] = bitStream.readU1();
        if (seq_scaling_list_present_flag[i]) {
          if (i < 6)
            scaling_list(bitStream, ScalingList4x4[i], 16,
                         UseDefaultScalingMatrix4x4Flag[i]);
          else
            scaling_list(bitStream, ScalingList8x8[i - 6], 16,
                         UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
      }
    }
  }

  uint32_t log2_max_frame_num_minus4 = bitStream.readUE();
  uint32_t pic_order_cnt_type = bitStream.readUE();

  int32_t *offset_for_ref_frame = nullptr;

  if (pic_order_cnt_type == 0) {
    uint32_t log2_max_pic_order_cnt_lsb_minus4 = bitStream.readUE();
  } else if (pic_order_cnt_type == 1) {
    bool delta_pic_order_always_zero_flag = bitStream.readU1();
    int32_t offset_for_non_ref_pic = bitStream.readSE();
    int32_t offset_for_top_to_bottom_field = bitStream.readSE();
    uint32_t num_ref_frames_in_pic_order_cnt_cycle = bitStream.readUE();
    if (num_ref_frames_in_pic_order_cnt_cycle != 0)
      offset_for_ref_frame = new int32_t[num_ref_frames_in_pic_order_cnt_cycle];
    /* TODO YangJing [offset_for_ref_frame -> delete] <24-04-04 01:24:42> */

    for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
      offset_for_ref_frame[i] = bitStream.readSE();
  }

  uint32_t max_num_ref_frames = bitStream.readUE();
  bool gaps_in_frame_num_value_allowed_flag = bitStream.readU1();
  uint32_t pic_width_in_mbs_minus1 =
      bitStream.readUE(); // 宏块（MB）单位的图像宽度减 1
  uint32_t pic_height_in_map_units_minus1 = bitStream.readUE();

  int width = (pic_width_in_mbs_minus1 + 1) * 16;
  int height = (pic_height_in_map_units_minus1 + 1) * 16;
  printf("width:%d, height=%d\n", width, height);
  /* TODO YangJing 有问题 <24-04-04 19:38:39> */

  bool frame_mbs_only_flag = bitStream.readU1();
  if (!frame_mbs_only_flag)
    bool mb_adaptive_frame_field_flag = bitStream.readU1();
  bool direct_8x8_inference_flag = bitStream.readU1();
  bool frame_cropping_flag = bitStream.readU1();
  if (frame_cropping_flag) {
    uint32_t frame_crop_left_offset = bitStream.readUE();
    uint32_t frame_crop_right_offset = bitStream.readUE();
    uint32_t frame_crop_top_offset = bitStream.readUE();
    uint32_t frame_crop_bottom_offset = bitStream.readUE();
  }
  bool vui_parameters_present_flag = bitStream.readU1();
  if (vui_parameters_present_flag)
    vui_parameters(bitStream);

  std::cout << "end:" << __FUNCTION__ << std::endl;

  return 0;
}

int Nalu::extractIDRparameters(RBSP &idr) {
  /* 初始化bit处理器，填充ssp的数据 */
  BitStream bitStream(idr._buf + 1, idr._len - 1);
  uint32_t first_mb_in_slice = bitStream.readUE();
  uint32_t slice_type = bitStream.readUE();
  uint32_t pic_parametter_set_id = bitStream.readUE();
  std::cout << "pic_parametter_set_id:" << pic_parametter_set_id << std::endl;
  int index = 0;
  switch (slice_type % 5) {
  case 0:
    std::cout << "P Slice" << std::endl;
    break;
  case 1:
    std::cout << "B Slice" << std::endl;
    break;
  case 2:
    std::cout << "I Slice" << std::endl;
    break;
  case 3:
    std::cout << "SP Slice" << std::endl;
    break;
  case 4:
    std::cout << "SI Slice" << std::endl;
    break;
  }

  return 0;
}

void Nalu::scaling_list(BitStream &bitStream, uint32_t *scalingList,
                        uint32_t sizeOfScalingList,
                        uint32_t &useDefaultScalingMatrixFlag) {
  int lastScale = 8;
  int nextScale = 8;
  for (int j = 0; j < sizeOfScalingList; j++) {
    if (nextScale != 0) {
      bool delta_scale = bitStream.readU1();
      int nextScale = (lastScale + delta_scale + 256) % 256;
      useDefaultScalingMatrixFlag = (j == 0 && nextScale == 0);
    }
    scalingList[j] = (nextScale == 0) ? lastScale : nextScale;
    lastScale = scalingList[j];
  }
}

void Nalu::vui_parameters(BitStream &bitStream) {
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

void Nalu::hrd_parameters(BitStream &bitStream) {
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
