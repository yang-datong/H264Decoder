
#ifndef __H264_VUI_H__
#define __H264_VUI_H__

#include "BitStream.hpp"
#include "H264HrdParameters.hpp"
#include <stdint.h>
#include <stdlib.h>

class CH264VUI {
 public:
  int32_t aspect_ratio_info_present_flag = 0;
  int32_t aspect_ratio_idc = 0;
  int32_t sar_width = 0;
  int32_t sar_height = 0;
  int32_t overscan_info_present_flag = 0;
  int32_t overscan_appropriate_flag = 0;
  int32_t video_signal_type_present_flag = 0;
  int32_t video_format = 0;
  int32_t video_full_range_flag = 0;
  int32_t colour_description_present_flag = 0;
  int32_t colour_primaries = 0;
  int32_t transfer_characteristics = 0;
  int32_t matrix_coefficients = 0;
  int32_t chroma_loc_info_present_flag = 0;
  int32_t chroma_sample_loc_type_top_field = 0;
  int32_t chroma_sample_loc_type_bottom_field = 0;
  int32_t timing_info_present_flag = 0;
  int32_t num_units_in_tick = 0;
  /*  the number of time units of a clock operating at the frequency time_scale
   * Hz that corresponds to one increment (called a clock tick) of a clock tick
   * counter.*/
  int32_t time_scale = 0;
  // the number of time units that pass in one second.
  int32_t fixed_frame_rate_flag = 0;
  int32_t nal_hrd_parameters_present_flag = 0;
  int32_t vcl_hrd_parameters_present_flag = 0;
  int32_t low_delay_hrd_flag = 0;
  int32_t pic_struct_present_flag = 0;
  int32_t bitstream_restriction_flag = 0;
  int32_t motion_vectors_over_pic_boundaries_flag = 0;
  int32_t max_bytes_per_pic_denom = 0;
  int32_t max_bits_per_mb_denom = 0;
  int32_t log2_max_mv_length_horizontal = 0;
  int32_t log2_max_mv_length_vertical = 0;
  int32_t max_num_reorder_frames = 0;
  /*  indicates an upper bound for the number of frames buffers, in the decoded
   picture buffer (DPB). 大于等于2表示含有B帧 */
  int32_t max_dec_frame_buffering = 0;

  CHrdParameters m_hrd_parameter_nal;
  CHrdParameters m_hrd_parameter_vcl;

 public:
  CH264VUI();
  ~CH264VUI();

  int printInfo();

  int vui_parameters(BitStream &bs);
};

#endif
