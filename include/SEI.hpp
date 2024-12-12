#ifndef SEI_HPP_TGZDOMB3
#define SEI_HPP_TGZDOMB3

#include "Common.hpp"
#include "SPS.hpp"

class SEI {
 public:
  uint8_t *_buf = nullptr;
  int _len = 0;

 private:
  long payloadType = 0;
  long payloadSize = 0;
  SPS *sps;

 public:
  int extractParameters(SPS &sps);

 private:
  BitStream *_bs = nullptr;

  uint32_t *initial_cpb_removal_delay = nullptr;
  uint32_t *initial_cpb_removal_delay_offset = nullptr;

  void sei_message();
  void sei_payload();
  void buffering_period();
  void reserved_sei_message();

  //----------------------- SEI::pic_timing ------------------------------
  //当前图片从 Coded Picture Buffer (CPB) 移除的延迟时间，以时钟计数为单位
  uint32_t cpb_removal_delay = 0;
  //当前图片从 Decoded Picture Buffer (DPB) 输出的延迟时间，以时钟计数为单位
  uint32_t dpb_output_delay = 0;
  //当前图片的结构类型，例如帧、场或帧场组合
  uint32_t pic_struct = 0;
  //当前图片的时钟类型
  uint32_t ct_type = 0;
  //当前图片是否基于场
  bool nuit_field_based_flag = 0;
  //时间戳的计数方法，取值范围：0 到 7
  uint32_t counting_type = 0;
  //是否提供完整的时间戳信息，如果为真，则提供完整的时间戳（包括秒、分钟和小时）。
  bool full_timestamp_flag = 0;
  //时间戳是否存在不连续性
  bool discontinuity_flag = 0;
  //指示是否存在丢帧现象
  bool cnt_dropped_flag = 0;
  //当前时间戳周期内的帧数
  uint32_t n_frames = 0;
  //当前时间戳的秒、分、时数部分
  uint32_t seconds_value = 0;
  uint32_t minutes_value = 0;
  uint32_t hours_value = 0;

  bool seconds_flag = 0;
  bool minutes_flag = 0;
  bool hours_flag = 0;
  //时间偏移量，用于调整时间戳
  uint32_t time_offset = 0;
  void pic_timing();

  void user_data_unregistered();
  void time_code();
};

#endif /* end of include guard: SEI_HPP_TGZDOMB3 */
