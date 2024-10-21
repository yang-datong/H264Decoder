#ifndef VPS_HPP_QLZP3M2R
#define VPS_HPP_QLZP3M2R

#include "BitStream.hpp"
#include "Common.hpp"
#include <cstdint>

class VPS {
 public:
  int extractParameters(BitStream &bitStream);

 public:
  // VPS的唯一标识符多个视频参数集
  int32_t vps_video_parameter_set_id = 0;
  // 指示基础层是否为内部层，即是否完全自包含
  int32_t vps_base_layer_internal_flag = 0;
  // 指示基础层是否可用于解码
  int32_t vps_base_layer_available_flag = 0;
  // 表示编码视频中使用的最大层数
  int32_t vps_max_layers = 0;
  // 指示每个层最多有多少个子层，减1表示
  int32_t vps_max_sub_layers = 0;
  // 标志位，用于指示是否所有的VCL（视频编码层）NAL单元都具有相同或增加的时间层ID
  int32_t vps_temporal_id_nesting_flag = 0;
  // 保留字段，通常用于特定于格式的扩展
  int32_t vps_reserved_0xffff_16bits = 0;
  // 标志位，指示是否为每个子层都提供了排序信息
  int32_t vps_sub_layer_ordering_info_present_flag = 0;

  // 数组，对每个子层定义解码图像缓冲的最大数量减1
  int32_t vps_max_dec_pic_buffering[32] = {0};
  // 数组，对每个子层定义重排序图片的最大数量
  int32_t vps_max_num_reorder_pics[32] = {0};
  // 数组，定义了每个子层的最大延迟增加值加1
  int32_t vps_max_latency_increase[32] = {0};

  // 最大的层ID，用于指示在该VPS定义的层中可以使用的最大层标识符
  int32_t vps_max_layer_id = 0;
  // 表示层集合的数量减1，用于定义不同层集的组合
  int32_t vps_num_layer_sets = 0;
  // 二维数组，指示在每个层集合中哪些层被包括
  int32_t layer_id_included_flag[32][32] = {0};

  // 标志位，指示是否在VPS中存在定时信息
  int32_t vps_timing_info_present_flag = 0;
  // 定义时间尺度和时间单位，用于计算视频的时间长度
  int32_t vps_num_units_in_tick = 0;
  // 标志位，指示画面输出顺序是否与时间信息成比例
  int32_t vps_time_scale = 0;
  // 定义两个连续POC（解码顺序号）间的时间单位数减1
  int32_t vps_poc_proportional_to_timing_flag = 0;
  // 指示HRD（超时率解码）参数集的数量
  int32_t vps_num_ticks_poc_diff_one = 0;
  // 为每个HRD参数集定义使用的层集索引
  int32_t vps_num_hrd_parameters = 0;
  // 每个HRD参数集是否包含共同参数集的标志
  int32_t hrd_layer_set_idx[32] = {0};
  // 标志位，指示是否有额外的扩展数据
  int32_t cprms_present_flag[32] = {0};

  // 扩展数据是否存在的标志
  int32_t vps_extension_flag = 0;
  int32_t vps_extension_data_flag = 0;
};

#endif /* end of include guard: VPS_HPP_QLZP3M2R */
