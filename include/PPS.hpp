#ifndef PPS_CPP_F6QSULFM
#define PPS_CPP_F6QSULFM

#include "Common.hpp"

#define MAX_PPS_COUNT 256

class PPS {
 public:
  int extractParameters(BitStream &bs, uint32_t chroma_format_idc);

 public:
  /* PPS 的唯一标识符 */
  uint32_t pic_parameter_set_id = 0;
  /* 该PPS对应的SSP标识符 */
  /* TODO：这里的sps id并没有使用到 */
  uint32_t seq_parameter_set_id = 0;
  /* 表示使用的熵编码模式，其中：0: 表示使用 CAVLC, 1: 表示使用 CABAC */
  bool entropy_coding_mode_flag = 0;
  /* 指示是否存在场序信息 */
  bool bottom_field_pic_order_in_frame_present_flag = 0;
  /* 图像大小（以宏块单位表示） */
  uint32_t pic_size_in_map_units_minus1 = 0;
  /* 指定默认激活的 L0 参考索引数量减 1 */
  uint32_t num_ref_idx_l0_default_active_minus1 = 0;
  uint32_t num_ref_idx_l1_default_active_minus1 = 0;
  /* 指定色度量化参数索引偏移 */
  int32_t chroma_qp_index_offset = 0;
  /* 指示是否存在约束的帧内预测 */
  bool constrained_intra_pred_flag = 0;
  /* 指示是否存在 8x8 变换模式 */
  bool transform_8x8_mode_flag = 0;
  /* 指定图像缩放列表的最大数量 */
  uint32_t maxPICScalingList = 0;
  /* 指定第二个色度量化参数索引偏移 */
  int32_t second_chroma_qp_index_offset = 0;
  /* 指示是否存在冗余图像计数 */
  bool redundant_pic_cnt_present_flag = 0;
  /* 指示是否存在加权预测 */
  bool weighted_pred_flag = 0;
  /* 指定加权双向预测类型 */
  uint32_t weighted_bipred_idc = 0;
  /* 指示是否存在去块滤波器控制信息 */
  bool deblocking_filter_control_present_flag = 0;
  /* 表示切片组的数量 */
  uint32_t num_slice_groups_minus1 = 0;
  /* 指定切片组映射类型 */
  uint32_t slice_group_map_type = 0;
  /* 指定每个切片组的运行长度 */
  uint32_t *run_length_minus1 = 0;
  /* 定每个切片组的左上角坐标 */
  uint32_t *top_left = 0;
  /* 指定每个切片组的右下角坐标 */
  uint32_t *bottom_right = 0;
  /* 指示切片组更改方向 */
  bool slice_group_change_direction_flag = 0;
  /* 指定切片组更改速率 */
  uint32_t slice_group_change_rate_minus1 = 0;
  /* 指定每个宏块的切片组 ID */
  uint32_t *slice_group_id = 0;
  /* 指示是否存在图像缩放矩阵 */
  bool pic_scaling_matrix_present_flag = 0;

  /* 缩放矩阵 */
  uint32_t ScalingList4x4[6][16] = {{0}};
  uint32_t ScalingList8x8[6][64] = {{0}};
  uint32_t UseDefaultScalingMatrix4x4Flag[6] = {0};
  uint32_t UseDefaultScalingMatrix8x8Flag[6] = {0};

  /* 图像缩放列表 */
  uint32_t *pic_scaling_list_present_flag = 0;
  /* 指定图像的初始量化参数减26 */
  int32_t pic_init_qp_minus26 = 0;
  /* 指定图像的初始 QP 步长减26 */
  int32_t pic_init_qs_minus26 = 0;
};

#endif /* end of include guard: PPS_CPP_F6QSULFM */
