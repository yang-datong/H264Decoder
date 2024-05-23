#include "Nalu.hpp"
#include "BitStream.hpp"
#include "PictureBase.hpp"
#include "RBSP.hpp"
#include <cmath>
#include <cstdint>

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
    qpprime_y_zero_transform_bypass_flag = bitStream.readU1();
    seq_scaling_matrix_present_flag = bitStream.readU1();

    uint32_t ScalingList4x4[6][16];
    uint32_t ScalingList8x8[6][64];

    uint32_t UseDefaultScalingMatrix4x4Flag[6];
    uint32_t UseDefaultScalingMatrix8x8Flag[6];

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
    uint32_t log2_max_pic_order_cnt_lsb_minus4 = bitStream.readUE();
  } else if (pic_order_cnt_type == 1) {
    delta_pic_order_always_zero_flag = bitStream.readU1();
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

  frame_mbs_only_flag = bitStream.readU1();
  if (!frame_mbs_only_flag)
    mb_adaptive_frame_field_flag = bitStream.readU1();
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
  bitDepthY = bit_depth_luma_minus8 + 8;
  // 亮度分量的位深度
  qpBitDepthY = bit_depth_luma_minus8 * 6;
  // 亮度分量的量化参数步长偏移
  bitDepthUV = bit_depth_chroma_minus8 + 8;
  // 色度分量的位深度
  qpBitDepthUV = bit_depth_chroma_minus8 * 6;
  // 色度分量的量化参数步长偏移

  /* 计算色度子采样参数 */
  /* TODO YangJing  <24-04-05 00:44:15> */
  uint32_t MbWidthC;  //色度宏块宽度
  uint32_t MbHeightC; //色度宏块高度
  if (chroma_format_idc == 0 || separate_colour_plane_flag == 1) {
    // 色度子采样宽度和高度均为 0。
    MbWidthC = 0;
    MbHeightC = 0;
  } else {
    int32_t index = chroma_format_idc;
    if (chroma_format_idc == 3 && separate_colour_plane_flag == 1) {
      index = 4;
    }
    int Chroma_Format = g_chroma_format_idcs[index].Chroma_Format;
    int SubWidthC = g_chroma_format_idcs[index].SubWidthC;
    int SubHeightC = g_chroma_format_idcs[index].SubHeightC;
    // 根据 chroma_format_idc 查找色度格式、色度子采样宽度和色度子采样高度。

    MbWidthC = 16 / SubWidthC;
    MbHeightC = 16 / SubHeightC;
  }

  /* 计算采样宽度和比特深度 */
  //  uint32_t picWidthInSamplesL = PicWidthInMbs * 16;
  //  //亮度分量的采样宽度，等于宏块宽度乘以 16
  //  uint32_t picWidthInSamplesC = PicWidthInMbs * MbWidthC;
  //  //色度分量的采样宽度，等于宏块宽度乘以 MbWidthC。
  //  RawMbBits = 256 * BitDepthY + 2 * MbWidthC * MbHeightC * BitDepthC;

  /* 计算最大帧号和最大图像顺序计数 LSB  in 77 page*/
  /*
   *log2_max_frame_num_minus4 specifies the value of the variable MaxFrameNum
that is used in frame_num related derivations as follows:
$$
    MaxFrameNum = 2^{( log2_max_frame_num_minus4 + 4 )}
$$
The value of log2_max_frame_num_minus4 shall be in the range of 0 to
12,inclusive.
   * */
  maxFrameNum = std::pow(log2_max_frame_num_minus4 + 4, 2);
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

  entropy_coding_mode_flag = bitStream.readUE();
  if (entropy_coding_mode_flag == 0)
    std::cout << "\tentropy_coding_mode_flag:CAVLC" << std::endl;
  else if (entropy_coding_mode_flag == 1)
    std::cout << "\tentropy_coding_mode_flag:CABAC" << std::endl;
  else
    std::cout << "\tentropy_coding_mode_flag:?????" << std::endl;

  bottom_field_pic_order_in_frame_present_flag = bitStream.readUE();
  num_slice_groups_minus1 = bitStream.readUE();
  if (num_slice_groups_minus1 > 0) {
    slice_group_map_type = bitStream.readUE();
    std::cout << "\tslice_group_map_type:" << slice_group_map_type << std::endl;
    if (slice_group_map_type == 0) {
      run_length_minus1 = new uint32_t[num_slice_groups_minus1 + 1];
      for (int iGroup = 0; iGroup <= num_slice_groups_minus1; iGroup++)
        run_length_minus1[iGroup] = bitStream.readUE();
    } else if (slice_group_map_type == 2) {
      top_left = new uint32_t[num_slice_groups_minus1];
      bottom_right = new uint32_t[num_slice_groups_minus1];
      for (int iGroup = 0; iGroup < num_slice_groups_minus1; iGroup++) {
        top_left[iGroup] = bitStream.readUE();
        bottom_right[iGroup] = bitStream.readUE();
      }
    } else if (slice_group_map_type == 3 || slice_group_map_type == 4 ||
               slice_group_map_type == 5) {
      slice_group_change_direction_flag = bitStream.readU1();
      slice_group_change_rate_minus1 = bitStream.readUE();
    } else if (slice_group_map_type == 6) {
      uint32_t pic_size_in_map_units_minus1 = bitStream.readUE();
      std::cout << "\tpic_size_in_map_units_minus1:"
                << pic_size_in_map_units_minus1 << std::endl;
      slice_group_id = new uint32_t[pic_size_in_map_units_minus1 + 1];
      for (int i = 0; i <= pic_size_in_map_units_minus1; i++)
        slice_group_id[i] = bitStream.readUE();
    }
  }

  uint32_t num_ref_idx_l0_default_active_minus1 = bitStream.readUE();
  uint32_t num_ref_idx_l1_default_active_minus1 = bitStream.readUE();
  weighted_pred_flag = bitStream.readU1();
  weighted_bipred_idc = bitStream.readUn(2);
  int32_t pic_init_qp_minus26 = bitStream.readSE();
  std::cout << "\tpic_init_qp:" << pic_init_qp_minus26 + 26 << std::endl;
  int32_t pic_init_qs_minus26 = bitStream.readSE();
  std::cout << "\tpic_init_qs:" << pic_init_qs_minus26 + 26 << std::endl;
  int32_t chroma_qp_index_offset = bitStream.readSE();
  deblocking_filter_control_present_flag = bitStream.readU1();
  bool constrained_intra_pred_flag = bitStream.readU1();
  redundant_pic_cnt_present_flag = bitStream.readU1();
  if (more_rbsp_data()) {
    bool transform_8x8_mode_flag = bitStream.readU1();
    pic_scaling_matrix_present_flag = bitStream.readU1();
    if (pic_scaling_matrix_present_flag) {
      uint32_t maxPICScalingList =
          6 + ((chroma_format_idc != 3) ? 2 : 6) * transform_8x8_mode_flag;
      pic_scaling_list_present_flag = new uint32_t[maxPICScalingList]{0};
      for (int i = 0; i < maxPICScalingList; i++) {
        pic_scaling_list_present_flag[i] = bitStream.readU1();
        if (pic_scaling_list_present_flag[i]) {
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
  // parseSliceData(bitStream, idr);

  return 0;
}

/* Slice header syntax -> 51 page */
int Nalu::parseSliceHeader(BitStream &bitStream, RBSP &rbsp) {
  first_mb_in_slice = bitStream.readUE();
  uint32_t slice_type = bitStream.readUE();
  uint32_t pic_parametter_set_id = bitStream.readUE();
  if (separate_colour_plane_flag == 1)
    uint8_t colour_plane_id = bitStream.readUn(2);

  uint32_t frame_num = bitStream.readUn(std::log2(maxFrameNum)); // u(v)
  if (!frame_mbs_only_flag) {
    field_pic_flag = bitStream.readU1();
    if (field_pic_flag)
      bool bottom_field_flag = bitStream.readU1();
  }
  IdrPicFlag = ((nal_unit_type == 5) ? 1 : 0);
  if (IdrPicFlag)
    uint32_t idr_pic_id = bitStream.readUE();
  if (pic_order_cnt_type == 0) {
    uint32_t pic_order_cnt_lsb = bitStream.readUn(std::log2(maxFrameNum));
    if (bottom_field_pic_order_in_frame_present_flag && !field_pic_flag)
      int32_t delta_pic_order_cnt_bottom = bitStream.readSE();
  }

  int32_t delta_pic_order_cnt[2] = {0};
  if (pic_order_cnt_type == 1 && !delta_pic_order_always_zero_flag) {
    delta_pic_order_cnt[0] = bitStream.readSE();
    if (bottom_field_pic_order_in_frame_present_flag && !field_pic_flag)
      delta_pic_order_cnt[1] = bitStream.readSE();
  }

  if (redundant_pic_cnt_present_flag)
    uint32_t redundant_pic_cnt = bitStream.readUE();
  if (slice_type % 5 == SLICE_B)
    bool direct_spatial_mv_pred_flag = bitStream.readU1();
  if (slice_type % 5 == SLICE_P || slice_type % 5 == SLICE_SP ||
      slice_type % 5 == SLICE_B) {
    bool num_ref_idx_active_override_flag = bitStream.readU1();
    if (num_ref_idx_active_override_flag) {
      num_ref_idx_l0_active_minus1 = bitStream.readUE();
      if (slice_type % 5 == SLICE_B)
        num_ref_idx_l1_active_minus1 = bitStream.readUE();
    }
  }
  if (nal_unit_type == 20 || nal_unit_type == 21)
    ref_pic_list_mvc_modification(bitStream); /* specified in Annex H */
  else
    ref_pic_list_modification(bitStream);
  if ((weighted_pred_flag &&
       (slice_type % 5 == SLICE_P || slice_type % 5 == SLICE_SP)) ||
      (weighted_bipred_idc == 1 && slice_type % 5 == SLICE_B))
    pred_weight_table(bitStream);
  if (nal_ref_idc != 0)
    dec_ref_pic_marking(bitStream);
  if (entropy_coding_mode_flag && slice_type % 5 != SLICE_I &&
      slice_type % 5 != SLICE_SI)
    uint32_t cabac_init_idc = bitStream.readUE();
  int32_t slice_qp_delta = bitStream.readSE();
  if (slice_type % 5 == SLICE_SP || slice_type % 5 == SLICE_SI) {
    if (slice_type % 5 == SLICE_SP)
      bool sp_for_switch_flag = bitStream.readU1();
    int32_t slice_qs_delta = bitStream.readSE();
  }
  if (deblocking_filter_control_present_flag) {
    uint32_t disable_deblocking_filter_idc = bitStream.readUE();
    if (disable_deblocking_filter_idc != 1) {
      int32_t slice_alpha_c0_offset_div2 = bitStream.readSE();
      int32_t slice_beta_offset_div2 = bitStream.readSE();
    }
  }
  if (num_slice_groups_minus1 > 0 && slice_group_map_type >= 3 &&
      slice_group_map_type <= 5)
    slice_group_change_cycle = bitStream.readUE();

  switch (slice_type % 5) {
  case SLICE_P:
    std::cout << "\tP Slice" << std::endl;
    break;
  case SLICE_B:
    std::cout << "\tB Slice" << std::endl;
    break;
  case SLICE_I:
    std::cout << "\tI Slice" << std::endl;
    break;
  case SLICE_SP:
    std::cout << "\tSP Slice" << std::endl;
    break;
  case SLICE_SI:
    std::cout << "\tSI Slice" << std::endl;
    break;
  }

  //----------- 下面都是一些需要进行额外计算的（文档都有需要自己找）------------
  int SliceGroupChangeRate = slice_group_change_rate_minus1 + 1;
  MbaffFrameFlag = (mb_adaptive_frame_field_flag && !field_pic_flag);
  PicHeightInMbs = frameHeightInMbs / (1 + field_pic_flag);
  PicSizeInMbs = PicWidthInMbs * PicHeightInMbs;
  MapUnitsInSliceGroup0 = std::min(
      slice_group_change_cycle * SliceGroupChangeRate, PicSizeInMapUnits);

  if (!mapUnitToSliceGroupMap) {
    mapUnitToSliceGroupMap = new int32_t[PicSizeInMapUnits]{0};
  }

  if (!MbToSliceGroupMap) {
    MbToSliceGroupMap = new int32_t[PicSizeInMbs]{0};
  }

  setMapUnitToSliceGroupMap();

  setMbToSliceGroupMap();

  set_scaling_lists_values();

  // m_is_malloc_mem_self = 1;
  return 0;
}

int Nalu::setMapUnitToSliceGroupMap() {
  int32_t i = 0;
  int32_t j = 0;
  int32_t k = 0;
  int32_t x = 0;
  int32_t y = 0;
  int32_t iGroup = 0;

  if (num_slice_groups_minus1 == 0) {
    for (i = 0; i < PicSizeInMapUnits; i++) {
      mapUnitToSliceGroupMap[i] = 0;
    }
    return 0;
  }

  if (slice_group_map_type == 0) // 8.2.2.1 Specification for interleaved slice
                                 // group map type 交叉型 slice组映射类型的描述
  {
    i = 0;
    do {
      for (iGroup = 0;
           iGroup <= num_slice_groups_minus1 && i < PicSizeInMapUnits;
           i += run_length_minus1[iGroup++] + 1) {
        for (j = 0; j <= run_length_minus1[iGroup] && i + j < PicSizeInMapUnits;
             j++) {
          mapUnitToSliceGroupMap[i + j] = iGroup;
        }
      }
    } while (i < PicSizeInMapUnits);
  } else if (slice_group_map_type ==
             1) // 8.2.2.2 Specification for dispersed slice group map type
                // 分散型 slice 组映射类型的描述
  {
    for (i = 0; i < PicSizeInMapUnits; i++) {
      mapUnitToSliceGroupMap[i] =
          ((i % PicWidthInMbs) +
           (((i / PicWidthInMbs) * (num_slice_groups_minus1 + 1)) / 2)) %
          (num_slice_groups_minus1 + 1);
    }
  } else if (slice_group_map_type ==
             2) // 8.2.2.3 Specification for foreground with left-over slice
                // group map type 前景加剩余型 slice 组映射类型的描述
  {
    for (i = 0; i < PicSizeInMapUnits; i++) {
      mapUnitToSliceGroupMap[i] = num_slice_groups_minus1;
    }
    for (iGroup = num_slice_groups_minus1 - 1; iGroup >= 0; iGroup--) {
      int32_t yTopLeft = top_left[iGroup] / PicWidthInMbs;
      int32_t xTopLeft = top_left[iGroup] % PicWidthInMbs;
      int32_t yBottomRight = bottom_right[iGroup] / PicWidthInMbs;
      int32_t xBottomRight = bottom_right[iGroup] % PicWidthInMbs;
      for (y = yTopLeft; y <= yBottomRight; y++) {
        for (x = xTopLeft; x <= xBottomRight; x++) {
          mapUnitToSliceGroupMap[y * PicWidthInMbs + x] = iGroup;
        }
      }
    }
  } else if (slice_group_map_type == 3) {
    // 8.2.2.4 Specification for box-out slice group map types
    // 外旋盒子型 slice 组映射类型的描述
    for (i = 0; i < PicSizeInMapUnits; i++) {
      mapUnitToSliceGroupMap[i] = 1;
    }
    x = (PicWidthInMbs - slice_group_change_direction_flag) / 2;
    y = (PicHeightInMapUnits - slice_group_change_direction_flag) / 2;

    int32_t leftBound = x;
    int32_t topBound = y;
    int32_t rightBound = x;
    int32_t bottomBound = y;
    int32_t xDir = slice_group_change_direction_flag - 1;
    int32_t yDir = slice_group_change_direction_flag;
    int32_t mapUnitVacant = 0;

    for (k = 0; k < MapUnitsInSliceGroup0; k += mapUnitVacant) {
      mapUnitVacant = (mapUnitToSliceGroupMap[y * PicWidthInMbs + x] == 1);
      if (mapUnitVacant) {
        mapUnitToSliceGroupMap[y * PicWidthInMbs + x] = 0;
      }
      if (xDir == -1 && x == leftBound) {
        leftBound = std::max(leftBound - 1, 0);
        x = leftBound;
        xDir = 0;
        yDir = 2 * slice_group_change_direction_flag - 1;
      } else if (xDir == 1 && x == rightBound) {
        rightBound = MIN(rightBound + 1, PicWidthInMbs - 1);
        x = rightBound;
        xDir = 0;
        yDir = 1 - 2 * slice_group_change_direction_flag;
      } else if (yDir == -1 && y == topBound) {
        topBound = MAX(topBound - 1, 0);
        y = topBound;
        xDir = 1 - 2 * slice_group_change_direction_flag;
        yDir = 0;
      } else if (yDir == 1 && y == bottomBound) {
        bottomBound = MIN(bottomBound + 1, PicHeightInMapUnits - 1);
        y = bottomBound;
        xDir = 2 * slice_group_change_direction_flag - 1;
        yDir = 0;
      } else {
        (x, y) = (x + xDir, y + yDir);
      }
    }
  } else if (slice_group_map_type ==
             4) // 8.2.2.5 Specification for raster scan slice group map types
                // 栅格扫描型 slice 组映射类型的描述
  {
    int32_t sizeOfUpperLeftGroup = 0;
    if (num_slice_groups_minus1 == 1) {
      sizeOfUpperLeftGroup = (slice_group_change_direction_flag
                                  ? (PicSizeInMapUnits - MapUnitsInSliceGroup0)
                                  : MapUnitsInSliceGroup0);
    }

    for (i = 0; i < PicSizeInMapUnits; i++) {
      if (i < sizeOfUpperLeftGroup) {
        mapUnitToSliceGroupMap[i] = slice_group_change_direction_flag;
      } else {
        mapUnitToSliceGroupMap[i] = 1 - slice_group_change_direction_flag;
      }
    }
  } else if (slice_group_map_type ==
             5) // 8.2.2.6 Specification for wipe slice group map types 擦除型
                // slice 组映射类型的描述
  {
    int32_t sizeOfUpperLeftGroup = 0;
    if (num_slice_groups_minus1 == 1) {
      sizeOfUpperLeftGroup = (slice_group_change_direction_flag
                                  ? (PicSizeInMapUnits - MapUnitsInSliceGroup0)
                                  : MapUnitsInSliceGroup0);
    }

    k = 0;
    for (j = 0; j < PicWidthInMbs; j++) {
      for (i = 0; i < PicHeightInMapUnits; i++) {
        if (k++ < sizeOfUpperLeftGroup) {
          mapUnitToSliceGroupMap[i * PicWidthInMbs + j] =
              slice_group_change_direction_flag;
        } else {
          mapUnitToSliceGroupMap[i * PicWidthInMbs + j] =
              1 - slice_group_change_direction_flag;
        }
      }
    }
  } else if (slice_group_map_type ==
             6) // 8.2.2.7 Specification for explicit slice group map type
                // 显式型 slice 组映射类型的描述
  {
    for (i = 0; i < PicSizeInMapUnits; i++) {
      mapUnitToSliceGroupMap[i] = slice_group_id[i];
    }
  } else {
    printf("slice_group_map_type=%d, must be in [0..6];\n",
           slice_group_map_type);
    return -1;
  }

  return 0;
}

int Nalu::setMbToSliceGroupMap() {
  for (int i = 0; i < PicSizeInMbs; i++) {
    if (frame_mbs_only_flag == 1 || field_pic_flag == 1) {
      MbToSliceGroupMap[i] = mapUnitToSliceGroupMap[i];
    } else if (MbaffFrameFlag == 1) {
      MbToSliceGroupMap[i] = mapUnitToSliceGroupMap[i / 2];
    } else // if (frame_mbs_only_flag == 0 &&
           // mb_adaptive_frame_field_flag == 0 && field_pic_flag == 0)
    {
      MbToSliceGroupMap[i] =
          mapUnitToSliceGroupMap[(i / (2 * PicWidthInMbs)) * PicWidthInMbs +
                                 (i % PicWidthInMbs)];
    }
  }

  return 0;
}

int Nalu::set_scaling_lists_values() {
  int ret = 0;

  //--------------------------------
  static int32_t Flat_4x4_16[16] = {16, 16, 16, 16, 16, 16, 16, 16,
                                    16, 16, 16, 16, 16, 16, 16, 16};
  static int32_t Flat_8x8_16[64] = {
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
  };

  //--------------------------------
  // Table 7-3 – Specification of default scaling lists Default_4x4_Intra and
  // Default_4x4_Inter
  static int32_t Default_4x4_Intra[16] = {6,  13, 13, 20, 20, 20, 28, 28,
                                          28, 28, 32, 32, 32, 37, 37, 42};
  static int32_t Default_4x4_Inter[16] = {10, 14, 14, 20, 20, 20, 24, 24,
                                          24, 24, 27, 27, 27, 30, 30, 34};

  // Table 7-4 – Specification of default scaling lists Default_8x8_Intra and
  // Default_8x8_Inter
  static int32_t Default_8x8_Intra[64] = {
      6,  10, 10, 13, 11, 13, 16, 16, 16, 16, 18, 18, 18, 18, 18, 23,
      23, 23, 23, 23, 23, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27,
      27, 27, 27, 27, 29, 29, 29, 29, 29, 29, 29, 31, 31, 31, 31, 31,
      31, 33, 33, 33, 33, 33, 36, 36, 36, 36, 38, 38, 38, 40, 40, 42,
  };

  static int32_t Default_8x8_Inter[64] = {
      9,  13, 13, 15, 13, 15, 17, 17, 17, 17, 19, 19, 19, 19, 19, 21,
      21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 24, 24, 24, 24,
      24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27, 27,
      27, 28, 28, 28, 28, 28, 30, 30, 30, 30, 32, 32, 32, 33, 33, 35,
  };

  //--------------------------------
  int32_t i = 0;
  int32_t scaling_list_size = (chroma_format_idc != 3) ? 8 : 12;

  if (seq_scaling_matrix_present_flag == 0 &&
      pic_scaling_matrix_present_flag == 0) {
    // 如果编码器未给出缩放矩阵值，则缩放矩阵值全部默认为16
    for (i = 0; i < scaling_list_size; i++) {
      if (i < 6) {
        memcpy(ScalingList4x4[i], Flat_4x4_16, sizeof(int32_t) * 16);
      } else // if (i >= 6)
      {
        memcpy(ScalingList8x8[i - 6], Flat_8x8_16, sizeof(int32_t) * 64);
      }
    }
  } else {
    if (seq_scaling_matrix_present_flag == 1) {
      for (i = 0; i < scaling_list_size; i++) {
        if (i < 6) {
          if (seq_scaling_list_present_flag[i] ==
              0) // 参照 Table 7-2 Scaling list fall-back rule A
          {
            if (i == 0) {
              memcpy(ScalingList4x4[i], Default_4x4_Intra,
                     sizeof(int32_t) * 16);
            } else if (i == 3) {
              memcpy(ScalingList4x4[i], Default_4x4_Inter,
                     sizeof(int32_t) * 16);
            } else {
              memcpy(ScalingList4x4[i], ScalingList4x4[i - 1],
                     sizeof(int32_t) * 16);
            }
          } else {
            if (UseDefaultScalingMatrix4x4Flag[i] == 1) {
              if (i < 3) {
                memcpy(ScalingList4x4[i], Default_4x4_Intra,
                       sizeof(int32_t) * 16);
              } else // if (i >= 3)
              {
                memcpy(ScalingList4x4[i], Default_4x4_Inter,
                       sizeof(int32_t) * 16);
              }
            } else {
              memcpy(ScalingList4x4[i], ScalingList4x4[i],
                     sizeof(int32_t) *
                         16); // 采用编码器传送过来的量化系数的缩放值
            }
          }
        } else // if (i >= 6)
        {
          if (seq_scaling_list_present_flag[i] ==
              0) // 参照 Table 7-2 Scaling list fall-back rule A
          {
            if (i == 6) {
              memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                     sizeof(int32_t) * 64);
            } else if (i == 7) {
              memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                     sizeof(int32_t) * 64);
            } else {
              memcpy(ScalingList8x8[i - 6], ScalingList8x8[i - 8],
                     sizeof(int32_t) * 64);
            }
          } else {
            if (UseDefaultScalingMatrix8x8Flag[i - 6] == 1) {
              if (i == 6 || i == 8 || i == 10) {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                       sizeof(int32_t) * 64);
              } else {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                       sizeof(int32_t) * 64);
              }
            } else {
              memcpy(ScalingList8x8[i - 6], ScalingList8x8[i - 6],
                     sizeof(int32_t) *
                         64); // 采用编码器传送过来的量化系数的缩放值
            }
          }
        }
      }
    }

    // 注意：此处不是"else if"，意即面的值，可能会覆盖之前到的值
    if (pic_scaling_matrix_present_flag == 1) {
      for (i = 0; i < scaling_list_size; i++) {
        if (i < 6) {
          if (pic_scaling_list_present_flag[i] ==
              0) // 参照 Table 7-2 Scaling list fall-back rule B
          {
            if (i == 0) {
              if (seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList4x4[i], Default_4x4_Intra,
                       sizeof(int32_t) * 16);
              }
            } else if (i == 3) {
              if (seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList4x4[i], Default_4x4_Inter,
                       sizeof(int32_t) * 16);
              }
            } else {
              memcpy(ScalingList4x4[i], ScalingList4x4[i - 1],
                     sizeof(int32_t) * 16);
            }
          } else {
            if (UseDefaultScalingMatrix4x4Flag[i] == 1) {
              if (i < 3) {
                memcpy(ScalingList4x4[i], Default_4x4_Intra,
                       sizeof(int32_t) * 16);
              } else // if (i >= 3)
              {
                memcpy(ScalingList4x4[i], Default_4x4_Inter,
                       sizeof(int32_t) * 16);
              }
            } else {
              memcpy(ScalingList4x4[i], ScalingList4x4[i],
                     sizeof(int32_t) *
                         16); // 采用编码器传送过来的量化系数的缩放值
            }
          }
        } else // if (i >= 6)
        {
          if (pic_scaling_list_present_flag[i] ==
              0) // 参照 Table 7-2 Scaling list fall-back rule B
          {
            if (i == 6) {
              if (seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                       sizeof(int32_t) * 64);
              }
            } else if (i == 7) {
              if (seq_scaling_matrix_present_flag == 0) {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                       sizeof(int32_t) * 64);
              }
            } else {
              memcpy(ScalingList8x8[i - 6], ScalingList8x8[i - 8],
                     sizeof(int32_t) * 64);
            }
          } else {
            if (UseDefaultScalingMatrix8x8Flag[i - 6] == 1) {
              if (i == 6 || i == 8 || i == 10) {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Intra,
                       sizeof(int32_t) * 64);
              } else {
                memcpy(ScalingList8x8[i - 6], Default_8x8_Inter,
                       sizeof(int32_t) * 64);
              }
            } else {
              memcpy(ScalingList8x8[i - 6], ScalingList8x8[i - 6],
                     sizeof(int32_t) *
                         64); // 采用编码器传送过来的量化系数的缩放值
            }
          }
        }
      }
    }
  }

  return ret;
}

void Nalu::ref_pic_list_mvc_modification(BitStream &bitStream) {}

void Nalu::ref_pic_list_modification(BitStream &bitStream) {
  uint32_t modification_of_pic_nums_idc;
  if (slice_type % 5 != SLICE_I && slice_type % 5 != SLICE_SI) {
    bool ref_pic_list_modification_flag_l0 = bitStream.readU1();
    if (ref_pic_list_modification_flag_l0) {
      do {
        modification_of_pic_nums_idc = bitStream.readUE();
        if (modification_of_pic_nums_idc == 0 ||
            modification_of_pic_nums_idc == 1)
          uint32_t abs_diff_pic_num_minus1 = bitStream.readUE();
        else if (modification_of_pic_nums_idc == 2)
          uint32_t long_term_pic_num = bitStream.readUE();
      } while (modification_of_pic_nums_idc != 3);
    }
  }
  if (slice_type % 5 == SLICE_B) {
    bool ref_pic_list_modification_flag_l1 = bitStream.readU1();
    if (ref_pic_list_modification_flag_l1)
      do {
        modification_of_pic_nums_idc = bitStream.readUE();
        if (modification_of_pic_nums_idc == 0 ||
            modification_of_pic_nums_idc == 1)
          uint32_t abs_diff_pic_num_minus1 = bitStream.readUE();
        else if (modification_of_pic_nums_idc == 2)
          uint32_t long_term_pic_num = bitStream.readUE();
      } while (modification_of_pic_nums_idc != 3);
  }
}

void Nalu::pred_weight_table(BitStream &bitStream) {
  uint32_t luma_log2_weight_denom = bitStream.readUE();
  if (ChromaArrayType != 0) {
    uint32_t chroma_log2_weight_denom = bitStream.readUE();
  }

  int32_t luma_weight_l0[num_ref_idx_l0_active_minus1 + 1];
  int32_t luma_offset_l0[num_ref_idx_l0_active_minus1 + 1];

  int32_t chroma_weight_l0[num_ref_idx_l0_active_minus1 + 1][2];
  int32_t chroma_offset_l0[num_ref_idx_l0_active_minus1 + 1][2];

  for (int i = 0; i <= num_ref_idx_l0_active_minus1; i++) {
    bool luma_weight_l0_flag = bitStream.readU1();
    if (luma_weight_l0_flag) {
      luma_weight_l0[i] = bitStream.readSE();
      luma_offset_l0[i] = bitStream.readSE();
    }
    if (ChromaArrayType != 0) {
      bool chroma_weight_l0_flag = bitStream.readU1();
      if (chroma_weight_l0_flag) {
        for (int j = 0; j < 2; j++) {
          chroma_weight_l0[i][j] = bitStream.readSE();
          chroma_offset_l0[i][j] = bitStream.readSE();
        }
      }
    }
  }

  if (slice_type % 5 == SLICE_B) {
    int32_t luma_weight_l1[num_ref_idx_l1_active_minus1 + 1];
    int32_t luma_offset_l1[num_ref_idx_l1_active_minus1 + 1];

    int32_t chroma_weight_l1[num_ref_idx_l1_active_minus1 + 1][2];
    int32_t chroma_offset_l1[num_ref_idx_l1_active_minus1 + 1][2];

    for (int i = 0; i <= num_ref_idx_l1_active_minus1; i++) {
      bool luma_weight_l1_flag = bitStream.readU1();
      if (luma_weight_l1_flag) {
        luma_weight_l1[i] = bitStream.readSE();
        luma_offset_l1[i] = bitStream.readSE();
      }

      if (ChromaArrayType != 0) {
        bool chroma_weight_l1_flag = bitStream.readU1();
        if (chroma_weight_l1_flag) {
          for (int j = 0; j < 2; j++) {
            chroma_weight_l1[i][j] = bitStream.readSE();
            chroma_offset_l1[i][j] = bitStream.readSE();
          }
        }
      }
    }
  }
}

void Nalu::dec_ref_pic_marking(BitStream &bitStream) {
  if (IdrPicFlag) {
    bool no_output_of_prior_pics_flag = bitStream.readU1();
    bool long_term_reference_flag = bitStream.readU1();
  } else {
    bool adaptive_ref_pic_marking_mode_flag = bitStream.readU1();

    uint32_t memory_management_control_operation;
    if (adaptive_ref_pic_marking_mode_flag) {
      do {
        memory_management_control_operation = bitStream.readUE();
        if (memory_management_control_operation == 1 ||
            memory_management_control_operation == 3)
          uint32_t difference_of_pic_nums_minus1 = bitStream.readUE();
        if (memory_management_control_operation == 2)
          uint32_t long_term_pic_num = bitStream.readUE();
        if (memory_management_control_operation == 3 ||
            memory_management_control_operation == 6)
          uint32_t long_term_frame_idx = bitStream.readUE();
        if (memory_management_control_operation == 4)
          uint32_t max_long_term_frame_idx_plus1 = bitStream.readUE();
      } while (memory_management_control_operation != 0);
    }
  }
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

void Nalu::rbsp_trailing_bits() { /* TODO YangJing  <24-04-07 21:55:36> */
}

int Nalu::decode(RBSP &rbsp) {
  /* 初始化bit处理器，填充sps的数据 */
  BitStream bitStream(rbsp._buf, rbsp._len);
  parseSliceData(bitStream, rbsp);
  return 0;
}

/* Rec. ITU-T H.264 (08/2021) 56 */
int Nalu::parseSliceData(BitStream &bitStream, RBSP &rbsp) {
  if (entropy_coding_mode_flag) {
    while (!byte_aligned(bitStream))
      cabac_alignment_one_bit = bitStream.readU1();
  }
  CurrMbAddr = first_mb_in_slice * (1 + MbaffFrameFlag);
  picture.CurrMbAddr = CurrMbAddr;

  do {
    if (slice_type != SLICE_I && slice_type != SLICE_SI)
      if (!entropy_coding_mode_flag) {
        uint32_t mb_skip_run = bitStream.readUE();
        prevMbSkipped = (mb_skip_run > 0);
        for (int i = 0; i < mb_skip_run; i++)
          CurrMbAddr = NextMbAddress(CurrMbAddr);
        if (mb_skip_run > 0)
          moreDataFlag = more_rbsp_data();
      } else {
        set_mb_skip_flag(mb_skip_flag, picture, bitStream);
        moreDataFlag = !mb_skip_flag;
      }
    if (moreDataFlag) {
      if (MbaffFrameFlag &&
          (CurrMbAddr % 2 == 0 || (CurrMbAddr % 2 == 1 && prevMbSkipped)))
        mb_field_decoding_flag;
      macroblock_layer(bitStream);
    }
    if (!entropy_coding_mode_flag)
      moreDataFlag = more_rbsp_data();
    else {
      if (slice_type != SLICE_I && slice_type != SLICE_SI)
        prevMbSkipped = mb_skip_flag;
      if (MbaffFrameFlag && CurrMbAddr % 2 == 0)
        moreDataFlag = 1;
      else {
        cabac.CABAC_decode_end_of_slice_flag(picture,
                                             end_of_slice_flag); // 2 ae(v)
        moreDataFlag = !end_of_slice_flag;
      }
    }
    CurrMbAddr = NextMbAddress(CurrMbAddr);
  } while (moreDataFlag);
  return 0;
}

int Nalu::set_mb_skip_flag(int32_t &mb_skip_flag, PictureBase &picture,
                           BitStream &bs) {
  picture.mb_x = (CurrMbAddr % (picture.PicWidthInMbs * (1 + MbaffFrameFlag))) /
                 (1 + MbaffFrameFlag);
  picture.mb_y =
      (CurrMbAddr / (picture.PicWidthInMbs * (1 + MbaffFrameFlag)) *
       (1 + MbaffFrameFlag)) +
      ((CurrMbAddr % (picture.PicWidthInMbs * (1 + MbaffFrameFlag))) %
       (1 + MbaffFrameFlag));
  picture.CurrMbAddr = CurrMbAddr;

  // picture.m_mbs[picture.CurrMbAddr].MbaffFrameFlag =
  // MbaffFrameFlag;
  // //因为解码mb_skip_flag需要事先知道MbaffFrameFlag的值
  picture.m_mbs[picture.CurrMbAddr].slice_number =
      slice_number; // 因为解码mb_skip_flag需要事先知道slice_id的值

  if (MbaffFrameFlag) {
    if (CurrMbAddr % 2 == 0) // 顶场宏块
    {
      if (picture.mb_x == 0 &&
          picture.mb_y >=
              2) // 注意：此处在T-REC-H.264-201704-S!!PDF-E.pdf文档中，并没有明确写出来，所以这是一个坑
      {
        // When MbaffFrameFlag is equal to 1 and mb_field_decoding_flag is
        // not present for both the top and the bottom macroblock of a
        // macroblock pair
        if (picture.mb_x > 0 &&
            picture.m_mbs[CurrMbAddr - 2].slice_number ==
                slice_number) // the left of the current macroblock pair
                              // in the same slice
        {
          mb_field_decoding_flag =
              picture.m_mbs[CurrMbAddr - 2].mb_field_decoding_flag;
        } else if (picture.mb_y > 0 &&
                   picture.m_mbs[CurrMbAddr - 2 * picture.PicWidthInMbs]
                           .slice_number ==
                       slice_number) // above the current macroblock pair
                                     // in the same slice
        {
          mb_field_decoding_flag =
              picture.m_mbs[CurrMbAddr - 2 * picture.PicWidthInMbs]
                  .mb_field_decoding_flag;
        } else {
          mb_field_decoding_flag = 0; // is inferred to be equal to 0
        }
      }
    }

    picture.m_mbs[picture.CurrMbAddr].mb_field_decoding_flag =
        mb_field_decoding_flag; // 因为解码mb_skip_flag需要事先知道mb_field_decoding_flag的值
  }

  //-------------解码mb_skip_flag-----------------------
  if (MbaffFrameFlag && CurrMbAddr % 2 == 1 &&
      prevMbSkipped) // 如果是bottom field macroblock
  {
    mb_skip_flag = mb_skip_flag_next_mb;
  } else {
    cabac.CABAC_decode_mb_skip_flag(picture, bs, CurrMbAddr,
                                    mb_skip_flag); // 2 ae(v)
  }

  //------------------------------------
  if (mb_skip_flag ==
      1) // 表示本宏块没有残差数据，相应的像素值只需要利用之前已经解码的I/P帧来预测获得
  {
    picture.mb_cnt++;

    if (MbaffFrameFlag) {
      if (CurrMbAddr % 2 == 0) // 只需要处理top field macroblock
      {
        picture.m_mbs[picture.CurrMbAddr].mb_skip_flag =
            mb_skip_flag; // 因为解码mb_skip_flag_next_mb需要事先知道前面顶场宏块的mb_skip_flag值
        picture.m_mbs[picture.CurrMbAddr + 1].slice_number =
            slice_number; // 因为解码mb_skip_flag需要事先知道slice_id的值
        picture.m_mbs[picture.CurrMbAddr + 1].mb_field_decoding_flag =
            mb_field_decoding_flag; // 特别注意：底场宏块和顶场宏块的mb_field_decoding_flag值是相同的

        cabac.CABAC_decode_mb_skip_flag(
            picture, bs, CurrMbAddr + 1,
            mb_skip_flag_next_mb); // 2 ae(v) 先读取底场宏块的mb_skip_flag

        if (mb_skip_flag_next_mb == 0) // 如果底场宏块mb_skip_flag=0
        {
          cabac.CABAC_decode_mb_field_decoding_flag(
              picture, bs,
              mb_field_decoding_flag); // 2 u(1) | ae(v)
                                       // 再读取底场宏块的mb_field_decoding_flag

          is_need_skip_read_mb_field_decoding_flag = true;
        } else // if (mb_skip_flag_next_mb == 1)
        {
          // When MbaffFrameFlag is equal to 1 and mb_field_decoding_flag
          // is not present for both the top and the bottom macroblock of
          // a macroblock pair
          if (picture.mb_x > 0 &&
              picture.m_mbs[CurrMbAddr - 2].slice_number ==
                  slice_number) // the left of the current macroblock pair
                                // in the same slice
          {
            mb_field_decoding_flag =
                picture.m_mbs[CurrMbAddr - 2].mb_field_decoding_flag;
          } else if (picture.mb_y > 0 &&
                     picture.m_mbs[CurrMbAddr - 2 * picture.PicWidthInMbs]
                             .slice_number ==
                         slice_number) // above the current macroblock
                                       // pair in the same slice
          {
            mb_field_decoding_flag =
                picture.m_mbs[CurrMbAddr - 2 * picture.PicWidthInMbs]
                    .mb_field_decoding_flag;
          } else {
            mb_field_decoding_flag = 0; // is inferred to be equal to 0
          }
        }
      }
    }

    //-----------------------------------------------------------------
    //    picture.m_mbs[picture.CurrMbAddr].macroblock_layer_mb_skip(picture,
    //    cabac); // 2 | 3 | 4

    // The inter prediction process for P and B macroblocks is specified
    // in clause 8.4 with inter prediction samples being the output.
    picture.Inter_prediction_process(); // 帧间预测
  }
  return 0;
}

int Nalu::NextMbAddress(int n) {
  int i = n + 1;
  while (i < PicSizeInMbs && MbToSliceGroupMap[i] != MbToSliceGroupMap[n])
    i++;
  return i;
}

int Nalu::macroblock_layer(BitStream &bs) {
  int is_ae = entropy_coding_mode_flag; // ae(v)表示CABAC编码
  int mb_type = 0;

  if (is_ae) // ae(v) 表示CABAC编码
  {
    cabac.CABAC_decode_mb_type(picture, bs, mb_type); // 2 ue(v) | ae(v)
  } else                                              // ue(v) 表示CAVLC编码
  {
    mb_type = bs.readUE();
  }
  const int I_PCM = 25;
  if (mb_type == I_PCM) {
    while (!byte_aligned(bs))
      uint8_t pcm_alignment_zero_bit = bs.readU1();
    int32_t pcm_sample_luma[256]; // 3 u(v)
    for (int i = 0; i < 256; i++) {
      int32_t v = bitDepthY;
      pcm_sample_luma[i] = bs.readUn(v); // 3 u(v)
    }
    for (int i = 0; i < 2 * MbWidthC * MbHeightC; i++)
      pcm_sample_chroma[i];
  } else {
    noSubMbPartSizeLessThan8x8Flag = 1;
    if (mb_type != I_NxN && MbPartPredMode(mb_type, 0) != Intra_16x16 &&
            NumMbPart(mb_type) = = 4) {
      sub_mb_pred(mb_type)

          for (mbPartIdx = 0; mbPartIdx < 4;
               mbPartIdx++) if (sub_mb_type[mbPartIdx] != B_Direct_8x8) {
        if (NumSubMbPart(sub_mb_type[mbPartIdx]) > 1)
          noSubMbPartSizeLessThan8x8Flag = 0;
      }
      else if (!direct_8x8_inference_flag) noSubMbPartSizeLessThan8x8Flag = 0;
    } else {
      if (transform_8x8_mode_flag &&mb_type = = I_NxN)
        transform_size_8x8_flag;
      mb_pred(mb_type);
    }
    if (MbPartPredMode(mb_type, 0) != Intra_16x16) {
      coded_block_pattern;
      if (CodedBlockPatternLuma > 0 && transform_8x8_mode_flag &&
          mb_type != I_NxN && noSubMbPartSizeLessThan8x8Flag &&
          (mb_type != B_Direct_16x16 | | direct_8x8_inference_flag))
        transform_size_8x8_flag;
    }
    if (CodedBlockPatternLuma > 0 | | CodedBlockPatternChroma > 0 | |
            MbPartPredMode(mb_type, 0) = = Intra_16x16) {
      // mb_qp_delta;
      residual(0, 15);
    }
  }
}
