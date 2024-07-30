#include "SPS.hpp"

SPS::SPS() {
  MaxPicOrderCntLsb = h264_power2(log2_max_pic_order_cnt_lsb_minus4 + 4);
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
