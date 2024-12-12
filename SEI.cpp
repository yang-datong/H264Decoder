#include "SEI.hpp"
#include "BitStream.hpp"
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>

void SEI::sei_message() {
  payloadType = 0;

  uint8_t last_payload_type_byte = 0;
  while ((last_payload_type_byte = _bs->readUn(8)) == 0xFF)
    payloadType += 255;
  payloadType += last_payload_type_byte;

  payloadSize = 0;
  uint8_t last_payload_size_byte = 0;
  while ((last_payload_size_byte = _bs->readUn(8)) == 0xFF)
    payloadSize += 255;
  payloadSize += last_payload_size_byte;

  sei_payload();
}

void SEI::buffering_period() {
  int NalHrdBpPresentFlag = 0, VclHrdBpPresentFlag = 0;

  if (sps->nal_hrd_parameters_present_flag == 1) NalHrdBpPresentFlag = 1;
  if (sps->vcl_hrd_parameters_present_flag == 1) VclHrdBpPresentFlag = 1;

  /*int seq_parameter_set_id =*/_bs->readUE();
  if (NalHrdBpPresentFlag) {
    initial_cpb_removal_delay = new uint32_t[sps->cpb_cnt_minus1 + 1]{0};
    initial_cpb_removal_delay_offset = new uint32_t[sps->cpb_cnt_minus1 + 1]{0};
    for (int SchedSelIdx = 0; SchedSelIdx <= (int)sps->cpb_cnt_minus1;
         SchedSelIdx++) {
      initial_cpb_removal_delay[SchedSelIdx] =
          _bs->readUn(log2(sps->MaxFrameNum));
      initial_cpb_removal_delay_offset[SchedSelIdx] =
          _bs->readUn(log2(sps->MaxFrameNum));
    }
  }
  if (VclHrdBpPresentFlag) {
    for (int SchedSelIdx = 0; SchedSelIdx <= (int)sps->cpb_cnt_minus1;
         SchedSelIdx++) {
      initial_cpb_removal_delay[SchedSelIdx] =
          _bs->readUn(log2(sps->MaxFrameNum));
      initial_cpb_removal_delay_offset[SchedSelIdx] =
          _bs->readUn(log2(sps->MaxFrameNum));
    }
  }
}

void SEI::pic_timing() {
  /* 变量 CpbDpbDelaysPresentFlag 的推导如下： 
   * – 如果以下任一条件成立，则 CpbDpbDelaysPresentFlag 的值应设置为等于 1： 
   * – nal_hrd_parameters_present_flag 存在于比特流中且等于 1， 
   * – vcl_hrd_parameters_present_flag 存在于比特流中且为等于1， 
   * – 图像定时SEI消息中的比特流中存在CPB和DPB输出延迟的需要由应用程序通过本建议书|中未指定的某些方式来确定国际标准。  
   * — 否则，CpbDpbDelaysPresentFlag 的值应设置为 0。 */
  bool CpbDpbDelaysPresentFlag = false;
  if (sps->nal_hrd_parameters_present_flag)
    CpbDpbDelaysPresentFlag = true;
  else if (sps->vcl_hrd_parameters_present_flag)
    CpbDpbDelaysPresentFlag = true;

  if (CpbDpbDelaysPresentFlag) {
    cpb_removal_delay = _bs->readUn(log2(sps->MaxFrameNum));
    std::cout << "\t当前图片从 Coded Picture Buffer (CPB) "
                 "移除的延迟时间，以时钟计数为单位:"
              << cpb_removal_delay << std::endl;
    dpb_output_delay = _bs->readUn(log2(sps->MaxFrameNum));
    std::cout << "\t当前图片从 Decoded Picture Buffer (DPB) "
                 "输出的延迟时间，以时钟计数为单位:"
              << dpb_output_delay << std::endl;
  }

  /* clock_timestamp_flag[i]等于1指示存在并且紧随其后的多个时钟时间戳语法元素。 
   * clock_timestamp_flag[i]等于0指示相关联的时钟时间戳语法元素不存在。
   * 当 NumClockTS 大于 1 并且对于多个 i 值，clock_timestamp_flag[ i ] 等于 1 时，clockTimestamp 的值不会随着 i 值的增加而减少。 */

  /* NumClockTS 由表 D-1 中指定的 pic_struct 确定。图片的时钟时间戳信息最多有 NumClockTS 组，由每组的clock_timestamp_flag[ i ]指定。时钟时间戳信息集适用于通过 pic_struct 与图像关联的字段或帧。 */
  const uint8_t NumClockTS[16] = {1, 1, 1, 2, 2, 3, 3, 2,
                                  3, 0, 0, 0, 0, 0, 0, 0};
  bool clock_timestamp_flag[16] = {false};

  if (sps->pic_struct_present_flag) {
    pic_struct = _bs->readUn(4);
    if (pic_struct > 15) return;
    std::string pic_struct_str;
    switch (pic_struct) {
    case 0:
      pic_struct_str = "(progressive) frame";
      break;
    case 1:
      pic_struct_str = "top field";
      break;
    case 2:
      pic_struct_str = "bottom field";
      break;
    case 3:
      pic_struct_str = "top field, bottom field, in that order";
      break;
    case 4:
      pic_struct_str = "bottom field, top field, in that order";
      break;
    case 5:
      pic_struct_str =
          "top field, bottom field, top field repeated, in that order";
      break;
    case 6:
      pic_struct_str =
          "bottom field, top field, bottom field repeated, in that order";
      break;
    case 7:
      pic_struct_str = "frame doubling";
      break;
    case 8:
      pic_struct_str = "frame tripling";
      break;
    case 9:
      pic_struct_str =
          "Top field paired with previous bottom field in output order";
      break;
    case 10:
      pic_struct_str =
          "Bottom field paired with previous top field in output order";
      break;
    case 11:
      pic_struct_str =
          "Top field paired with next bottom field in output order";
      break;
    case 12:
      pic_struct_str =
          "Bottom field paired with next top field in output order";
      break;
    }
    std::cout << "\t当前图片的结构类型:" << pic_struct_str << std::endl;

    for (int i = 0; i < NumClockTS[pic_struct]; i++) {
      clock_timestamp_flag[i] = _bs->readU1();
      if (clock_timestamp_flag[i]) {
        ct_type = _bs->readUn(2);
        std::cout << "\t当前图片的时钟类型:" << ct_type << std::endl;
        nuit_field_based_flag = _bs->readU1();
        std::cout << "\t当前图片是否基于场:" << nuit_field_based_flag
                  << std::endl;
        counting_type = _bs->readUn(5);
        std::cout << "\t时间戳的计数方法，取值范围：0 到 7:" << counting_type
                  << std::endl;
        full_timestamp_flag = _bs->readU1();
        std::cout << "\t是否提供完整的时间戳信息，如果为真，则提供完整的时间戳"
                     "（包括秒、分钟和小时）:"
                  << full_timestamp_flag << std::endl;
        discontinuity_flag = _bs->readU1();
        std::cout << "\t时间戳是否存在不连续性:" << discontinuity_flag
                  << std::endl;
        cnt_dropped_flag = _bs->readU1();
        std::cout << "\t指示是否存在丢帧现象:" << cnt_dropped_flag << std::endl;
        n_frames = _bs->readUn(8);
        std::cout << "\t当前时间戳周期内的帧数:" << n_frames << std::endl;
        if (full_timestamp_flag) {
          seconds_value = _bs->readUn(6) /* 0..59 */;
          minutes_value = _bs->readUn(6) /* 0..59 */;
          hours_value = _bs->readUn(5) /* 0..23 */;
          std::cout << "\t当前时间戳:" << hours_value << ":" << minutes_value
                    << ":" << seconds_value << std::endl;
        } else {
          seconds_flag = _bs->readU1();
          if (seconds_flag) {
            seconds_value = _bs->readUn(6) /* range 0..59 */;
            minutes_flag = _bs->readU1();
            if (minutes_flag) {
              minutes_value = _bs->readUn(6) /* 0..59 */;
              hours_flag = _bs->readU1();
              if (hours_flag) hours_value = _bs->readUn(5) /* 0..23 */;
            }
          }
        }
        if (sps->time_offset_length > 0) {
          time_offset = _bs->readUn(sps->time_offset_length);
          std::cout << "\t时间偏移量，用于调整时间戳:" << time_offset
                    << std::endl;
        }
      }
    }
  }
}

void SEI::user_data_unregistered() {
  /*uint32_t uuid_iso_iec_11578 = */ _bs->readUn(128);
  std::string _text;
  for (int i = 0x10; i < payloadSize; i++) {
    char user_data_payload_byte = _bs->readUn(8);
    _text += user_data_payload_byte;
  }
  if (!_text.empty()) std::cout << "\tuser_data:" << _text << std::endl;
}

void SEI::time_code() {
  BitStream &bs = *_bs;
  int num_clock_ts = bs.readUn(2);
  int *clock_timestamp_flag = new int[num_clock_ts]{0};
  int *units_field_based_flag = new int[num_clock_ts]{0};
  int *counting_type = new int[num_clock_ts]{0};
  int *full_timestamp_flag = new int[num_clock_ts]{0};
  int *discontinuity_flag = new int[num_clock_ts]{0};
  int *cnt_dropped_flag = new int[num_clock_ts]{0};
  int *n_frames = new int[num_clock_ts]{0};
  int *seconds_value = new int[num_clock_ts]{0};
  int *minutes_value = new int[num_clock_ts]{0};
  int *hours_value = new int[num_clock_ts]{0};
  int *seconds_flag = new int[num_clock_ts]{0};
  int *minutes_flag = new int[num_clock_ts]{0};
  int *hours_flag = new int[num_clock_ts]{0};
  int *time_offset_length = new int[num_clock_ts]{0};
  /* TODO YangJing dele mem <24-12-02 17:02:34> */
  for (int i = 0; i < num_clock_ts; i++) {
    clock_timestamp_flag[i] = bs.readUn(1);
    if (clock_timestamp_flag[i]) {
      units_field_based_flag[i] = bs.readUn(1);
      counting_type[i] = bs.readUn(5);
      full_timestamp_flag[i] = bs.readUn(1);
      discontinuity_flag[i] = bs.readUn(1);
      cnt_dropped_flag[i] = bs.readUn(1);
      n_frames[i] = bs.readUn(9);
      if (full_timestamp_flag[i]) {
        seconds_value[i] /* 0..59 */ = bs.readUn(6);
        minutes_value[i] /* 0..59 */ = bs.readUn(6);
        hours_value[i] /* 0..23 */ = bs.readUn(5);
      } else {
        seconds_flag[i] = bs.readUn(1);
        if (seconds_flag[i]) {
          seconds_value[i] /* 0..59 */ = bs.readUn(6);
          minutes_flag[i] = bs.readUn(1);
          if (minutes_flag[i]) {
            minutes_value[i] /* 0..59 */ = bs.readUn(6);
            hours_flag[i] = bs.readUn(1);
            if (hours_flag[i]) hours_value[i] /* 0..23 */ = bs.readUn(5);
          }
        }
      }
      time_offset_length[i] = bs.readUn(5);
      //if (time_offset_length[i] > 0) time_offset_value[i] i(v)
    }
  }
}

void SEI::sei_payload() {
  /* TODO YangJing sei_payload <24-09-11 17:15:28> */
  if (payloadType == 0)
    buffering_period();
  else if (payloadType == 1)
    pic_timing();
  //  else if (payloadType == 2)
  //    pan_scan_rect(payloadSize);
  //  else if (payloadType == 3)
  //    filler_payload(payloadSize);
  //  else if (payloadType == 4)
  //    user_data_registered_itu_t_t35(payloadSize);
  else if (payloadType == 5)
    user_data_unregistered();
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
  else if (payloadType == 136)
    time_code();
  else
    std::cerr << "Unrecognized type:" << payloadType << std::endl; 

  //reserved_sei_message();
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
  _bs->rbsp_trailing_bits();
  return 0;
}
