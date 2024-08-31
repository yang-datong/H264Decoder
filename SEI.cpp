#include "SEI.hpp"
#include "BitStream.hpp"
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>

void SEI::sei_message() {
  payloadType = 0;
  while (_bs->readUn(8) == 0xFF) {
    _bs->readUn(8);
    payloadType += 255;
  }
  uint8_t last_payload_type_byte = _bs->readUn(8);
  payloadType += last_payload_type_byte;

  payloadSize = 0;
  while (_bs->readUn(8) == 0xFF) {
    _bs->readUn(8);
    payloadSize += 255;
  }
  uint8_t last_payload_size_byte = _bs->readUn(8);
  payloadSize += last_payload_size_byte;
  sei_payload();
}

void SEI::buffering_period() {
  int NalHrdBpPresentFlag = 0, VclHrdBpPresentFlag = 0;

  if (sps->nal_hrd_parameters_present_flag == 1)
    NalHrdBpPresentFlag = 1;
  if (sps->vcl_hrd_parameters_present_flag == 1)
    VclHrdBpPresentFlag = 1;

  /*int seq_parameter_set_id =*/ _bs->readUE();
  if (NalHrdBpPresentFlag) {
    initial_cpb_removal_delay = new uint32_t[sps->cpb_cnt_minus1 + 1]{0};
    initial_cpb_removal_delay_offset = new uint32_t[sps->cpb_cnt_minus1 + 1]{0};
    for (int SchedSelIdx = 0; SchedSelIdx <= sps->cpb_cnt_minus1;
         SchedSelIdx++) {
      initial_cpb_removal_delay[SchedSelIdx] =
          _bs->readUn(log2(sps->MaxFrameNum));
      initial_cpb_removal_delay_offset[SchedSelIdx] =
          _bs->readUn(log2(sps->MaxFrameNum));
    }
  }
  if (VclHrdBpPresentFlag) {
    for (int SchedSelIdx = 0; SchedSelIdx <= sps->cpb_cnt_minus1;
         SchedSelIdx++) {
      initial_cpb_removal_delay[SchedSelIdx] =
          _bs->readUn(log2(sps->MaxFrameNum));
      initial_cpb_removal_delay_offset[SchedSelIdx] =
          _bs->readUn(log2(sps->MaxFrameNum));
    }
  }
}

void SEI::reserved_sei_message() {
  /* NOTE:针对x264的编码器 */
  _bs->readUn(0x70);
  uint8_t c;
  while ((c = _bs->readUn(8)) && c != 0)
    _text += static_cast<char>(c);
}

void SEI::sei_payload() {
  if (payloadType == 0)
    buffering_period();
  // else if (payloadType == 1)
  // pic_timing(payloadSize);
  //  else if (payloadType == 2)
  //    pan_scan_rect(payloadSize);
  //  else if (payloadType == 3)
  //    filler_payload(payloadSize);
  //  else if (payloadType == 4)
  //    user_data_registered_itu_t_t35(payloadSize);
  //  else if (payloadType == 5)
  //    user_data_unregistered(payloadSize);
  //  else if (payloadType == 6)
  //    recovery_point(payloadSize);
  //  else if (payloadType == 7)
  //    dec_ref_pic_marking_repetition(payloadSize);
  //  else if (payloadType == 8)
  //    spare_pic(payloadSize);
  //  else if (payloadType == 9)
  //    scene_info(payloadSize);
  //  else if (payloadType == 10)
  //    sub_seq_info(payloadSize);
  //  else if (payloadType == 11)
  //    sub_seq_layer_characteristics(payloadSize);
  //  else if (payloadType == 12)
  //    sub_seq_characteristics(payloadSize);
  //  else if (payloadType == 13)
  //    full_frame_freeze(payloadSize);
  //  else if (payloadType == 14)
  //    full_frame_freeze_release(payloadSize);
  //  else if (payloadType == 15)
  //    full_frame_snapshot(payloadSize);
  //  else if (payloadType == 16)
  //    progressive_refinement_segment_start(payloadSize);
  //  else if (payloadType == 17)
  //    progressive_refinement_segment_end(payloadSize);
  //  else if (payloadType == 18)
  //    motion_constrained_slice_group_set(payloadSize);
  else
    reserved_sei_message();

  if (payloadType == 255)
    reserved_sei_message();

  if (!_bs->byte_aligned()) {
     _bs->readU1();
    while (!_bs->byte_aligned())
       _bs->readU1();
  }
}

int SEI::extractParameters(SPS &sps) {
  /* 初始化bit处理器，填充sei的数据 */
  _bs = new BitStream(_buf, _len);
  this->sps = &sps;

  do
    sei_message();
  while (_bs->more_rbsp_data());

  if (!_text.empty())
    std::cout << "\t" << _text << std::endl;

  _bs->rbsp_trailing_bits();
  return 0;
}
