#include "Nalu.hpp"
#include "BitStream.hpp"
#include "H264SPS.hpp"
#include "MacroBlock.hpp"
#include "PictureBase.hpp"
#include "RBSP.hpp"
#include <cmath>
#include <cstdint>

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
int Nalu::extractSPSparameters(RBSP &rbsp) {
  sps._buf = rbsp._buf;
  sps._len = rbsp._len;

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
    sps.chroma_format_idc = bitStream.readUE();
    switch (sps.chroma_format_idc) {
    case 0:
      std::cout << "\tsps.chroma_format_idc:单色" << std::endl;
      break;
    case 1:
      std::cout << "\tsps.chroma_format_idc:YUV420" << std::endl;
      break;
    case 2:
      std::cout << "\tsps.chroma_format_idc:YUV422" << std::endl;
      break;
    case 3:
      std::cout << "\tsps.chroma_format_idc:YUB444" << std::endl;
      sps.separate_colour_plane_flag = bitStream.readU1();
      break;
    }
    sps.bit_depth_luma_minus8 = bitStream.readUE();
    sps.bit_depth_chroma_minus8 = bitStream.readUE();
    sps.qpprime_y_zero_transform_bypass_flag = bitStream.readU1();
    sps.seq_scaling_matrix_present_flag = bitStream.readU1();

    if (sps.seq_scaling_matrix_present_flag) {
      for (int i = 0; i < ((sps.chroma_format_idc != 3) ? 8 : 12); i++) {
        sps.seq_scaling_list_present_flag[i] = bitStream.readU1();
        if (sps.seq_scaling_list_present_flag[i]) {
          if (i < 6)
            scaling_list(bitStream, sps.ScalingList4x4[i], 16,
                         sps.UseDefaultScalingMatrix4x4Flag[i]);
          else
            scaling_list(bitStream, sps.ScalingList8x8[i - 6], 64,
                         sps.UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
      }
    }
  }

  uint32_t log2_max_frame_num_minus4 = bitStream.readUE();
  sps.pic_order_cnt_type = bitStream.readUE();

  int32_t *offset_for_ref_frame = nullptr;

  if (sps.pic_order_cnt_type == 0) {
    sps.log2_max_pic_order_cnt_lsb_minus4 = bitStream.readUE();
  } else if (sps.pic_order_cnt_type == 1) {
    sps.delta_pic_order_always_zero_flag = bitStream.readU1();
    sps.offset_for_non_ref_pic = bitStream.readSE();
    sps.offset_for_top_to_bottom_field = bitStream.readSE();
    sps.num_ref_frames_in_pic_order_cnt_cycle = bitStream.readUE();
    if (sps.num_ref_frames_in_pic_order_cnt_cycle != 0)
      offset_for_ref_frame =
          new int32_t[sps.num_ref_frames_in_pic_order_cnt_cycle];
    /* TODO YangJing [offset_for_ref_frame -> delete] <24-04-04 01:24:42> */

    for (int i = 0; i < sps.num_ref_frames_in_pic_order_cnt_cycle; i++)
      offset_for_ref_frame[i] = bitStream.readSE();
  }

  sps.max_num_ref_frames = bitStream.readUE();
  sps.gaps_in_frame_num_value_allowed_flag = bitStream.readU1();
  sps.pic_width_in_mbs_minus1 = bitStream.readUE();
  sps.pic_height_in_map_units_minus1 = bitStream.readUE();

  sps.frame_mbs_only_flag = bitStream.readU1();
  if (!sps.frame_mbs_only_flag)
    sps.mb_adaptive_frame_field_flag = bitStream.readU1();
  sps.direct_8x8_inference_flag = bitStream.readU1();
  sps.frame_cropping_flag = bitStream.readU1();
  if (sps.frame_cropping_flag) {
    sps.frame_crop_left_offset = bitStream.readUE();
    sps.frame_crop_right_offset = bitStream.readUE();
    sps.frame_crop_top_offset = bitStream.readUE();
    sps.frame_crop_bottom_offset = bitStream.readUE();
  }
  bool vui_parameters_present_flag = bitStream.readU1();
  if (vui_parameters_present_flag)
    sps.vui_parameters(bitStream);

  /* 计算宏块大小以及图像宽、高 */
  sps.PicWidthInMbs = sps.pic_width_in_mbs_minus1 + 1;
  // 宏块单位的图像宽度 = pic_width_in_mbs_minus1 + 1
  sps.PicHeightInMapUnits = sps.pic_height_in_map_units_minus1 + 1;
  // 宏块单位的图像高度 = pic_height_in_map_units_minus1 + 1
  sps.PicSizeInMapUnits = sps.PicWidthInMbs * sps.PicHeightInMapUnits;
  // 宏块单位的图像大小 = 宽 * 高
  sps.frameHeightInMbs =
      (2 - sps.frame_mbs_only_flag) * sps.PicHeightInMapUnits;
  // 指示图像是否仅包含帧（而不是场）。
  // frame_mbs_only_flag 为1，则图像仅包含帧，并且帧高度等于图像高度。
  // frame_mbs_only_flag 为0，则图像包含场，并且帧高度等于图像高度的一半。

  //----------- 下面都是一些需要进行额外计算的（文档都有需要自己找）------------
  int width = (sps.pic_width_in_mbs_minus1 + 1) * 16;
  int height = (sps.pic_height_in_map_units_minus1 + 1) * 16;
  printf("\tprediction width:%d, prediction height:%d\n", width, height);

  /* 获取帧率 */
  /* TODO YangJing  <24-04-05 00:22:50> */

  /* 获取B帧是否配置 */
  /* TODO YangJing  <24-04-05 00:30:03> */

  /* 确定色度数组类型 74 page */
  if (sps.separate_colour_plane_flag == 0)
    sps.ChromaArrayType = sps.chroma_format_idc;
  else
    sps.ChromaArrayType = 0;

  /* 计算位深度 */
  sps.BitDepthY = sps.bit_depth_luma_minus8 + 8;
  // 亮度分量的位深度
  sps.QpBdOffsetY = sps.bit_depth_luma_minus8 * 6;
  // 亮度分量的量化参数步长偏移
  sps.BitDepthC = sps.bit_depth_chroma_minus8 + 8;
  // 色度分量的位深度
  sps.QpBdOffsetC = sps.bit_depth_chroma_minus8 * 6;
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
  if (sps.chroma_format_idc == 0 || sps.separate_colour_plane_flag == 1) {
    // 色度子采样宽度和高度均为 0。
    sps.MbWidthC = 0;
    sps.MbHeightC = 0;
  } else {
    int32_t index = sps.chroma_format_idc;
    if (sps.chroma_format_idc == 3 && sps.separate_colour_plane_flag == 1) {
      index = 4;
    }
    sps.Chroma_Format = g_chroma_format_idcs[index].Chroma_Format;
    sps.SubWidthC = g_chroma_format_idcs[index].SubWidthC;
    sps.SubHeightC = g_chroma_format_idcs[index].SubHeightC;
    //  根据 sps.chroma_format_idc
    //  查找色度格式、色度子采样宽度和色度子采样高度。

    sps.MbWidthC = 16 / sps.SubWidthC;
    sps.MbHeightC = 16 / sps.SubHeightC;
  }

  /* 计算采样宽度和比特深度 */
  uint32_t picWidthInSamplesL = sps.PicWidthInMbs * 16;
  // 亮度分量的采样宽度，等于宏块宽度乘以 16
  uint32_t picWidthInSamplesC = sps.PicWidthInMbs * sps.MbWidthC;
  // 色度分量的采样宽度，等于宏块宽度乘以 MbWidthC。
  uint32_t RawMbBits =
      256 * sps.BitDepthY + 2 * sps.MbWidthC * sps.MbHeightC * sps.BitDepthY;

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
  sps.MaxFrameNum = std::pow(log2_max_frame_num_minus4 + 4, 2);
  sps.maxPicOrderCntLsb =
      std::pow(sps.log2_max_pic_order_cnt_lsb_minus4 + 4, 2);

  /* 计算预期图像顺序计数周期增量 */
  if (sps.pic_order_cnt_type == 1) {
    int expectedDeltaPerPicOrderCntCycle = 0;
    for (int i = 0; i < sps.num_ref_frames_in_pic_order_cnt_cycle; i++) {
      expectedDeltaPerPicOrderCntCycle += offset_for_ref_frame[i];
    }
  }

  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -47页 */
int Nalu::extractPPSparameters(RBSP &rbsp) {
  pps._buf = rbsp._buf;
  pps._len = rbsp._len;

  /* 初始化bit处理器，填充pps的数据 */
  BitStream bitStream(pps._buf, pps._len);

  pps.pic_parameter_set_id = bitStream.readUE();
  std::cout << "\tpic_parameter_set_id:" << pps.pic_parameter_set_id
            << std::endl;
  pps.seq_parameter_set_id = bitStream.readUE();
  std::cout << "\tseq_parameter_set_id:" << pps.seq_parameter_set_id
            << std::endl;

  pps.entropy_coding_mode_flag = bitStream.readUE();
  if (pps.entropy_coding_mode_flag == 0)
    std::cout << "\tentropy_coding_mode_flag:CAVLC" << std::endl;
  else if (pps.entropy_coding_mode_flag == 1)
    std::cout << "\tentropy_coding_mode_flag:CABAC" << std::endl;
  else
    std::cout << "\tentropy_coding_mode_flag:?????" << std::endl;

  pps.bottom_field_pic_order_in_frame_present_flag = bitStream.readUE();
  pps.num_slice_groups_minus1 = bitStream.readUE();
  if (pps.num_slice_groups_minus1 > 0) {
    pps.slice_group_map_type = bitStream.readUE();
    std::cout << "\tslice_group_map_type:" << pps.slice_group_map_type
              << std::endl;
    if (pps.slice_group_map_type == 0) {
      pps.run_length_minus1 = new uint32_t[pps.num_slice_groups_minus1 + 1];
      for (int iGroup = 0; iGroup <= pps.num_slice_groups_minus1; iGroup++)
        pps.run_length_minus1[iGroup] = bitStream.readUE();
    } else if (pps.slice_group_map_type == 2) {
      pps.top_left = new uint32_t[pps.num_slice_groups_minus1];
      pps.bottom_right = new uint32_t[pps.num_slice_groups_minus1];
      for (int iGroup = 0; iGroup < pps.num_slice_groups_minus1; iGroup++) {
        pps.top_left[iGroup] = bitStream.readUE();
        pps.bottom_right[iGroup] = bitStream.readUE();
      }
    } else if (pps.slice_group_map_type == 3 || pps.slice_group_map_type == 4 ||
               pps.slice_group_map_type == 5) {
      pps.slice_group_change_direction_flag = bitStream.readU1();
      pps.slice_group_change_rate_minus1 = bitStream.readUE();
    } else if (pps.slice_group_map_type == 6) {
      pps.pic_size_in_map_units_minus1 = bitStream.readUE();
      std::cout << "\tpic_size_in_map_units_minus1:"
                << pps.pic_size_in_map_units_minus1 << std::endl;
      pps.slice_group_id = new uint32_t[pps.pic_size_in_map_units_minus1 + 1];
      for (int i = 0; i <= pps.pic_size_in_map_units_minus1; i++)
        pps.slice_group_id[i] = bitStream.readUE();
    }
  }

  pps.num_ref_idx_l0_default_active_minus1 = bitStream.readUE();
  pps.num_ref_idx_l1_default_active_minus1 = bitStream.readUE();
  pps.weighted_pred_flag = bitStream.readU1();
  pps.weighted_bipred_idc = bitStream.readUn(2);
  pps.pic_init_qp_minus26 = bitStream.readSE();
  std::cout << "\tpic_init_qp:" << pps.pic_init_qp_minus26 + 26 << std::endl;
  pps.pic_init_qs_minus26 = bitStream.readSE();
  std::cout << "\tpic_init_qs:" << pps.pic_init_qs_minus26 + 26 << std::endl;
  pps.chroma_qp_index_offset = bitStream.readSE();
  pps.deblocking_filter_control_present_flag = bitStream.readU1();
  pps.constrained_intra_pred_flag = bitStream.readU1();
  pps.redundant_pic_cnt_present_flag = bitStream.readU1();
  if (pps.more_rbsp_data()) {
    pps.transform_8x8_mode_flag = bitStream.readU1();
    pps.pic_scaling_matrix_present_flag = bitStream.readU1();
    if (pps.pic_scaling_matrix_present_flag) {
      pps.maxPICScalingList = 6 + ((sps.chroma_format_idc != 3) ? 2 : 6) *
                                      pps.transform_8x8_mode_flag;
      pps.pic_scaling_list_present_flag =
          new uint32_t[pps.maxPICScalingList]{0};
      for (int i = 0; i < pps.maxPICScalingList; i++) {
        pps.pic_scaling_list_present_flag[i] = bitStream.readU1();
        if (pps.pic_scaling_list_present_flag[i]) {
          if (i < 6) {
            scaling_list(bitStream, pps.ScalingList4x4[i], 16,
                         pps.UseDefaultScalingMatrix4x4Flag[i]);
          } else
            scaling_list(bitStream, pps.ScalingList8x8[i - 6], 64,
                         pps.UseDefaultScalingMatrix8x8Flag[i - 6]);
        }
      }
    }
    pps.second_chroma_qp_index_offset = bitStream.readSE();
  }
  pps.rbsp_trailing_bits();

  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -48页 */
int Nalu::extractSEIparameters(RBSP &rbsp) {
  sei._buf = rbsp._buf;
  sei._len = rbsp._len;
  /* 初始化bit处理器，填充sei的数据 */
  BitStream bitStream(sei._buf, sei._len);
  do {
    sei.sei_message(bitStream);
  } while (pps.more_rbsp_data());
  return 0;
}

int Nalu::extractSliceparameters(RBSP &rbsp) {
  /* 初始化bit处理器，填充slice的数据 */
  BitStream bitStream(rbsp._buf, rbsp._len);
  slice_header.m_sps = sps;
  slice_header.m_pps = pps;
  slice_header.m_idr = idr;
  slice_header.parseSliceHeader(bitStream, rbsp, this);
  return 0;
}

int Nalu::extractIDRparameters(RBSP &rbsp) {
  /* 初始化bit处理器，填充idr的数据 */
  BitStream bitStream(rbsp._buf, rbsp._len);
  slice_header.m_sps = sps;
  slice_header.m_pps = pps;
  slice_header.m_idr = idr;
  slice_header.parseSliceHeader(bitStream, rbsp, this);
  return 0;
}

int Nalu::decode(RBSP &rbsp) {
  /* 初始化bit处理器，填充sps的数据 */
  BitStream bitStream(rbsp._buf, rbsp._len);

  PictureBase picture;

  picture.m_picture_coded_type = H264_PICTURE_CODED_TYPE_FRAME;
  picture.m_parent = this;
  /* TODO YangJing 这里先放置一下，这里是在处理前一帧的数据 <24-07-30 10:39:24>
   */
  // memcpy(picture.m_dpb, dpb, sizeof(Picture *) * size_pdb);
  m_current_picture_ptr = &m_picture_frame;
  m_picture_frame.init(slice_header);

  slice_body.slice_header = this->slice_header;
  slice_body.m_sps = this->sps;
  slice_body.m_pps = this->pps;
  slice_body.m_idr = this->idr;
  slice_body.parseSliceData(bitStream, rbsp, picture);
  return 0;
}
