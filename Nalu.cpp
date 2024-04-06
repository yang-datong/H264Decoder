#include "Nalu.hpp"
#include "BitStream.hpp"

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
  parseNALHeader(ebsp); // RBSP的头也是EBSP的头

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

int Nalu::parseNALHeader(EBSP &ebsp) {
  uint8_t firstByte = ebsp._buf[0];
  nal_unit_type = firstByte & 0b00011111;
  /* 取低5bit，即0-4 bytes */
  nal_ref_idc = (firstByte & 0b01100000) >> 5;
  /* 取5-6 bytes */
  forbidden_zero_bit = firstByte >> 7;
  /* 取最高位，即7 byte */
  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -43页 */
int Nalu::extractSPSparameters(RBSP &sps) {
  /* 初始化bit处理器，填充sps的数据 */
  BitStream bitStream(sps._buf, sps._len);

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
            scaling_list(bitStream, ScalingList8x8[i - 6], 64,
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
  uint32_t pic_width_in_mbs_minus1 = bitStream.readUE();
  uint32_t pic_height_in_map_units_minus1 = bitStream.readUE();

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

  /* 计算宏块大小以及图像宽、高 */
  uint32_t PicWidthInMbs = pic_width_in_mbs_minus1 + 1;
  // 宏块单位的图像宽度 = pic_width_in_mbs_minus1 + 1
  uint32_t PicHeightInMapUnits = pic_height_in_map_units_minus1 + 1;
  // 宏块单位的图像高度 = pic_height_in_map_units_minus1 + 1
  uint32_t PicSizeInMapUnits = PicWidthInMbs * PicHeightInMapUnits;
  // 宏块单位的图像大小 = 宽 * 高
  uint32_t frameHeightInMbs = (2 - frame_mbs_only_flag) * PicHeightInMapUnits;
  // 指示图像是否仅包含帧（而不是场）。
  // frame_mbs_only_flag 为1，则图像仅包含帧，并且帧高度等于图像高度。
  // frame_mbs_only_flag 为0，则图像包含场，并且帧高度等于图像高度的一半。

  int width = (pic_width_in_mbs_minus1 + 1) * 16;
  int height = (pic_height_in_map_units_minus1 + 1) * 16;
  printf("\tprediction width:%d, prediction height:%d\n", width, height);

  /* 获取帧率 */
  /* TODO YangJing  <24-04-05 00:22:50> */

  /* 获取B帧是否配置 */
  /* TODO YangJing  <24-04-05 00:30:03> */

  /* 确定色度数组类型 */
  /* TODO YangJing  <24-04-05 00:34:16> */

  /* 计算位深度 */
  uint32_t bitDepthY = bit_depth_luma_minus8 + 8;
  // 亮度分量的位深度
  uint32_t qpBitDepthY = bit_depth_luma_minus8 * 6;
  // 亮度分量的量化参数步长偏移
  uint32_t bitDepthUV = bit_depth_chroma_minus8 + 8;
  // 色度分量的位深度
  uint32_t qpBitDepthUV = bit_depth_chroma_minus8 * 6;
  // 色度分量的量化参数步长偏移

  /* 计算色度子采样参数 */
  /* TODO YangJing  <24-04-05 00:44:15> */
  //  uint32_t MbWidthC; //色度宏块宽度
  //  uint32_t MbHeightC; //色度宏块高度
  //  if (chroma_format_idc == 0 || separate_colour_plane_flag == 1) {
  //    // 色度子采样宽度和高度均为 0。
  //    MbWidthC = 0;
  //    MbHeightC = 0;
  //  } else {
  //    int32_t index = chroma_format_idc;
  //    if (chroma_format_idc == 3 && separate_colour_plane_flag == 1) {
  //      index = 4;
  //    }
  //    Chroma_Format = g_chroma_format_idcs[index].Chroma_Format;
  //    SubWidthC = g_chroma_format_idcs[index].SubWidthC;
  //    SubHeightC = g_chroma_format_idcs[index].SubHeightC;
  //    // 根据 chroma_format_idc 查找色度格式、色度子采样宽度和色度子采样高度。
  //
  //    MbWidthC = 16 / SubWidthC;
  //    MbHeightC = 16 / SubHeightC;
  //  }

  /* 计算采样宽度和比特深度 */
  //  uint32_t picWidthInSamplesL = PicWidthInMbs * 16;
  //  //亮度分量的采样宽度，等于宏块宽度乘以 16
  //  uint32_t picWidthInSamplesC = PicWidthInMbs * MbWidthC;
  //  //色度分量的采样宽度，等于宏块宽度乘以 MbWidthC。
  //  RawMbBits = 256 * BitDepthY + 2 * MbWidthC * MbHeightC * BitDepthC;

  /* 计算最大帧号和最大图像顺序计数 LSB */
  //  maxFrameNum = h264_power2(log2_max_frame_num_minus4 + 4);
  //  maxPicOrderCntLsb = h264_power2(log2_max_pic_order_cnt_lsb_minus4 + 4);

  /* 计算预期图像顺序计数周期增量 */
  //  if (pic_order_cnt_type == 1) {
  //    expectedDeltaPerPicOrderCntCycle = 0;
  //    for (i = 0; i < (int32_t)num_ref_frames_in_pic_order_cnt_cycle; i++) {
  //      expectedDeltaPerPicOrderCntCycle += offset_for_ref_frame[i];
  //    }
  //  }

  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -47页 */
int Nalu::extractPPSparameters(RBSP &pps) {
  /* 初始化bit处理器，填充pps的数据 */
  BitStream bitStream(pps._buf, pps._len);

  uint32_t pic_parameter_set_id = bitStream.readUE();
  std::cout << "\tpic_parameter_set_id:" << pic_parameter_set_id << std::endl;
  uint32_t seq_parameter_set_id = bitStream.readUE();
  std::cout << "\tseq_parameter_set_id:" << seq_parameter_set_id << std::endl;

  bool entropy_coding_mode_flag = bitStream.readUE();
  if (entropy_coding_mode_flag == 0)
    std::cout << "\tentropy_coding_mode_flag:CAVLC" << std::endl;
  else if (entropy_coding_mode_flag == 1)
    std::cout << "\tentropy_coding_mode_flag:CABAC" << std::endl;
  else
    std::cout << "\tentropy_coding_mode_flag:?????" << std::endl;

  bool bottom_field_pic_order_in_frame_present_flag = bitStream.readUE();
  uint32_t num_slice_groups_minus1 = bitStream.readUE();
  if (num_slice_groups_minus1 > 0) {
    uint32_t slice_group_map_type = bitStream.readUE();
    std::cout << "\tslice_group_map_type:" << slice_group_map_type << std::endl;
    if (slice_group_map_type == 0) {
      uint32_t run_length_minus1[num_slice_groups_minus1 + 1];
      for (int iGroup = 0; iGroup <= num_slice_groups_minus1; iGroup++)
        run_length_minus1[iGroup] = bitStream.readUE();
    } else if (slice_group_map_type == 2) {
      uint32_t top_left[num_slice_groups_minus1];
      uint32_t bottom_right[num_slice_groups_minus1];
      for (int iGroup = 0; iGroup < num_slice_groups_minus1; iGroup++) {
        top_left[iGroup] = bitStream.readUE();
        bottom_right[iGroup] = bitStream.readUE();
      }
    } else if (slice_group_map_type == 3 || slice_group_map_type == 4 ||
               slice_group_map_type == 5) {
      bool slice_group_change_direction_flag = bitStream.readU1();
      uint32_t slice_group_change_rate_minus1 = bitStream.readUE();
    } else if (slice_group_map_type == 6) {
      uint32_t pic_size_in_map_units_minus1 = bitStream.readUE();
      std::cout << "\tpic_size_in_map_units_minus1:"
                << pic_size_in_map_units_minus1 << std::endl;
      uint32_t slice_group_id[pic_size_in_map_units_minus1 + 1];
      for (int i = 0; i <= pic_size_in_map_units_minus1; i++)
        slice_group_id[i] = bitStream.readUE();
    }
  }

  uint32_t num_ref_idx_l0_default_active_minus1 = bitStream.readUE();
  uint32_t num_ref_idx_l1_default_active_minus1 = bitStream.readUE();
  bool weighted_pred_flag = bitStream.readU1();
  uint32_t weighted_bipred_idc = bitStream.readUn(2);
  int32_t pic_init_qp_minus26 = bitStream.readSE();
  std::cout << "\tpic_init_qp:" << pic_init_qp_minus26 + 26 << std::endl;
  int32_t pic_init_qs_minus26 = bitStream.readSE();
  std::cout << "\tpic_init_qs:" << pic_init_qs_minus26 + 26 << std::endl;
  int32_t chroma_qp_index_offset = bitStream.readSE();
  bool deblocking_filter_control_present_flag = bitStream.readU1();
  bool constrained_intra_pred_flag = bitStream.readU1();
  bool redundant_pic_cnt_present_flag = bitStream.readU1();
  if (more_rbsp_data()) {
    bool transform_8x8_mode_flag = bitStream.readU1();
    bool pic_scaling_matrix_present_flag = bitStream.readU1();
    if (pic_scaling_matrix_present_flag) {
      uint32_t maxPICScalingList =
          6 + ((chroma_format_idc != 3) ? 2 : 6) * transform_8x8_mode_flag;
      uint32_t pic_scaling_list_present_flag[maxPICScalingList];
      for (int i = 0; i < maxPICScalingList; i++) {
        pic_scaling_list_present_flag[i] = bitStream.readU1();
        if (pic_scaling_list_present_flag[i]) {
          uint32_t ScalingList4x4[6][16];
          uint32_t ScalingList8x8[6][64];
          uint32_t UseDefaultScalingMatrix4x4Flag[6];
          uint32_t UseDefaultScalingMatrix8x8Flag[6];
          if (i < 6) {
            scaling_list(bitStream, ScalingList4x4[i], 16,
                         UseDefaultScalingMatrix4x4Flag[i]);
          } else
            scaling_list(bitStream, ScalingList8x8[i - 6], 64,
                         UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
      }
    }
    int32_t second_chroma_qp_index_offset = bitStream.readSE();
  }
  rbsp_trailing_bits();

  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -48页 */
int Nalu::extractSEIparameters(RBSP &sei) {
  /* 初始化bit处理器，填充sei的数据 */
  BitStream bitStream(sei._buf, sei._len);
  do {
    sei_message(bitStream);
  } while (more_rbsp_data());
  return 0;
}

void Nalu::sei_message(BitStream &bitStream) {
  long payloadType = 0;
  while (bitStream.readUn(8) == 0xFF) {
    int8_t ff_byte = bitStream.readUn(8);
    payloadType += 255;
  }
  uint8_t last_payload_type_byte = bitStream.readUn(8);
  payloadType += last_payload_type_byte;

  long payloadSize = 0;
  while (bitStream.readUn(8) == 0xFF) {
    int8_t ff_byte = bitStream.readUn(8);
    payloadSize += 255;
  }
  uint8_t last_payload_size_byte = bitStream.readUn(8);
  payloadSize += last_payload_size_byte;
  sei_payload(bitStream, payloadType, payloadSize);
  std::cout << "\tpayloadType:" << payloadType << std::endl;
  std::cout << "\tpayloadSize:" << payloadSize << std::endl;
}

void Nalu::sei_payload(BitStream &bitStream, long payloadType,
                       long payloadSize) {
  /* TODO YangJing 忽略了一些if elseif, 见T-REC-H.264-202108-I!!PDF-E.pdf
   * 331-332页 <24-04-05 02:19:55> */
  if (!byte_aligned(bitStream)) {
    int8_t bit_equal_to_one = bitStream.readU1();
    while (!byte_aligned(bitStream))
      int8_t bit_equal_to_zero = bitStream.readU1();
  }
}

bool Nalu::byte_aligned(BitStream &bitStream) {
  /*
   * 1. If the current position in the bitstream is on a byte boundary, i.e.,
   * the next bit in the bitstream is the first bit in a byte, the return value
   * of byte_aligned( ) is equal to TRUE.
   * 2. Otherwise, the return value of byte_aligned( ) is equal to FALSE.
   */
  return bitStream.endOfBit();
}

int Nalu::extractSliceparameters(RBSP &rbsp) {
  /* 初始化bit处理器，填充idr的数据 */
  BitStream bitStream(rbsp._buf, rbsp._len);
  parseSliceHeader(bitStream, rbsp);

  return 0;
}

int Nalu::extractIDRparameters(RBSP &idr) {
  /* 初始化bit处理器，填充idr的数据 */
  BitStream bitStream(idr._buf, idr._len);
  parseSliceHeader(bitStream, idr);

  return 0;
}

int Nalu::parseSliceHeader(BitStream bitStream, RBSP &rbsp) {
  uint32_t first_mb_in_slice = bitStream.readUE();
  uint32_t slice_type = bitStream.readUE();
  uint32_t pic_parametter_set_id = bitStream.readUE();
  if (separate_colour_plane_flag == 1)
    uint8_t colour_plane_id = bitStream.readUn(2);
  frame_num
      /* TODO YangJing 不懂u(v) 是什么 <24-04-07 00:24:54> */
      if (!frame_mbs_only_flag) {
    field_pic_flag if (field_pic_flag) bottom_field_flag
  }
  if (IdrPicFlag)
    idr_pic_id if (pic_order_cnt_type = = 0) {
      pic_order_cnt_lsb if (bottom_field_pic_order_in_frame_present_flag &&
                            !field_pic_flag) delta_pic_order_cnt_bottom
    }

  int index = 0;
  switch (slice_type % 5) {
  case 0:
    std::cout << "\tP Slice" << std::endl;
    break;
  case 1:
    std::cout << "\tB Slice" << std::endl;
    break;
  case 2:
    std::cout << "\tI Slice" << std::endl;
    break;
  case 3:
    std::cout << "\tSP Slice" << std::endl;
    break;
  case 4:
    std::cout << "\tSI Slice" << std::endl;
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

bool Nalu::more_rbsp_data() { return false; }

void Nalu::rbsp_trailing_bits() {}
