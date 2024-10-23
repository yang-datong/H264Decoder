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
  // 指示基础层是否可用于解码：
  // 0：表示基础层不可用。解码器不能使用基础层来解码视频流。
  // 1：表示基础层可用。解码器可以使用基础层进行解码，这是一个有效的层，可以提供基本的视频内容
  int32_t vps_base_layer_available_flag = 0;
  // 表示编码视频中使用的最大层数，如vps_max_layers = 3，意味着视频流中可以有最多三层（如三个空间层），每一层可能对应不同的分辨率。解码器可以根据网络条件决定解码哪个层的信息，例如只解码480p版本以节省带宽，或者在条件允许时解码1080p版本。
  int32_t vps_max_layers = 0;
  // 可以使用的最大临时层的数量。这个值通常介于1到7之间，其中1表示没有额外的临时层（时间分层）
  int32_t vps_max_sub_layers = 0;
  // 标志位，用于指示是否所有的VCL（视频编码层）NAL单元都具有相同或增加的时间层ID
  int32_t vps_temporal_id_nesting_flag = 0;
  // 保留字段，通常用于特定于格式的扩展
  int32_t vps_reserved_0xffff_16bits = 0;
  // 标志位，指示是否为每个子层都提供了排序信息
  int32_t vps_sub_layer_ordering_info_present_flag = 0;

  // 在解码过程中需要为每个临时层（temporal layer）保留的最大解码图片缓冲区的数量。这个参数对于确保视频流的平滑播放和管理内存资源非常关键。
  int32_t vps_max_dec_pic_buffering[8] = {0};
  // 对每个子层定义重排序图片的最大数量
  int32_t vps_max_num_reorder_pics[8] = {0};
  // 在给定的临时层（temporal layer）中，相对于理想无延迟解码的情况下，允许的最大延迟增加量。用于控制和管理视频播放的延迟。
  int32_t vps_max_latency_increase[8] = {0};

  int VpsMaxLatencyPictures[8] = {0};

  // 最大的层ID，用于指示在该VPS定义的层中可以使用的最大层标识符
  int32_t vps_max_layer_id = 0;
  // 表示层集合的数量减1，用于定义不同层集的组合
  int32_t vps_num_layer_sets = 0;
  // 指示在每个层集合中哪些层被包括
  int32_t layer_id_included_flag[32][32] = {0};
  int LayerSetLayerIdList[32][32] = {0};
  int NumLayersInIdList[32] = {0};

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
  int32_t vps_extension_alignment_bit_equal_to_one = 0;
  int32_t vps_extension2_flag = 0;
  int32_t vps_extension_data_flag = 0;
  int vps_extension();
  int32_t layer_id_in_nuh[32] = {0};

  int sub_layer_hrd_parameters(BitStream &bs, int subLayerId);
  int nal_hrd_parameters_present_flag = 0;
  int vcl_hrd_parameters_present_flag = 0;
  int sub_pic_hrd_params_present_flag = 0;
  int tick_divisor_minus2 = 0;
  int du_cpb_removal_delay_increment_length_minus1 = 0;
  int sub_pic_cpb_params_in_pic_timing_sei_flag = 0;
  int dpb_output_delay_du_length_minus1 = 0;
  int bit_rate_scale = 0;
  int cpb_size_scale = 0;
  int cpb_size_du_scale = 0;
  int initial_cpb_removal_delay_length_minus1 = 0;
  int au_cpb_removal_delay_length_minus1 = 0;
  int dpb_output_delay_length_minus1 = 0;
  int fixed_pic_rate_general_flag[32] = {0};
  int fixed_pic_rate_within_cvs_flag[32] = {0};
  int elemental_duration_in_tc_minus1[32] = {0};
  int low_delay_hrd_flag[32] = {0};
  int cpb_cnt_minus1[32] = {0};
  int hrd_parameters(BitStream &bs, int commonInfPresentFlag,
                     int maxNumSubLayersMinus1);
};

#endif /* end of include guard: VPS_HPP_QLZP3M2R */
