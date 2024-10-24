#ifndef PPS_CPP_F6QSULFM
#define PPS_CPP_F6QSULFM

#include "Common.hpp"
#include "SPS.hpp"
#include <cstdint>

#define MAX_PPS_COUNT 256

class PPS {
 public:
  int extractParameters(BitStream &bs, uint32_t chroma_format_idc,
                        SPS spss[MAX_SPS_COUNT]);

 public:
  // PPS的唯一标识符。
  int32_t pic_parameter_set_id = 0;
  // 引用的序列参数集（SPS）的ID。
  int32_t seq_parameter_set_id = 0;
  // 指示是否允许依赖片段分割。
  int32_t dependent_slice_segments_enabled_flag = 0;
  // 指示输出标志的存在性。
  int32_t output_flag_present_flag = 0;
  // 片头额外的位数。
  int32_t num_extra_slice_header_bits = 0;
  // 数据隐藏的启用标志，用于控制编码噪声。
  int32_t sign_data_hiding_enabled_flag = 0;
  // 指示是否为每个切片指定CABAC初始化参数。
  int32_t cabac_init_present_flag = 0;
  // 默认的L0和L1参考图像索引数量减1。
  int32_t num_ref_idx_l0_default_active = 0;
  int32_t num_ref_idx_l1_default_active = 0;
  // 初始量化参数减26，用于计算初始量化值。
  int32_t init_qp = 0;
  // 限制内部预测的启用标志。
  int32_t constrained_intra_pred_flag = 0;
  // 转换跳过的启用标志。
  int32_t transform_skip_enabled_flag = 0;
  // 控制单元（CU）QP差分的启用标志。
  int32_t cu_qp_delta_enabled_flag = 0;
  // CU的QP差分深度。
  int32_t diff_cu_qp_delta_depth = 0;
  int Log2MinCuQpDeltaSize = 0;
  // 色度QP偏移。
  int32_t pps_cb_qp_offset = 0;
  int32_t pps_cr_qp_offset = 0;
  // 指示切片层面的色度QP偏移是否存在。
  int32_t pps_slice_chroma_qp_offsets_present_flag = 0;
  // 加权预测和双向加权预测的启用标志。
  int32_t weighted_pred_flag = 0;
  int32_t weighted_bipred_flag = 0;
  // 量化和变换绕过的启用标志。
  int32_t transquant_bypass_enabled_flag = 0;
  // 瓦片的启用标志。
  int32_t tiles_enabled_flag = 0;
  // 熵编码同步的启用标志：熵编码同步是一种并行化技术，允许在不同的 CTB 行之间同步 CABAC 状态。
  int32_t entropy_coding_sync_enabled_flag = 0;
  // 瓦片列数和行数减1。
  int32_t num_tile_columns = 0;
  int32_t num_tile_rows = 0;
  // 指示瓦片是否均匀分布。
  int32_t uniform_spacing_flag = 0;
  // 定义瓦片的列宽和行高减1。
  int32_t column_width_minus1[32] = {0};
  int32_t row_height_minus1[32] = {0};
  // 指示是否允许循环滤波器跨瓦片工作。NOTE:When not present, the value of loop_filter_across_tiles_enabled_flag is inferred to be equal to 1
  int32_t loop_filter_across_tiles_enabled_flag = 1;
  // 指示是否允许循环滤波器跨切片工作。
  int32_t pps_loop_filter_across_slices_enabled_flag = 0;
  // 解块滤波器覆盖的启用标志。
  int32_t deblocking_filter_override_enabled_flag = 0;
  // 解块滤波器禁用的标志。
  int32_t pps_deblocking_filter_disabled_flag = 0;
  // 解块滤波器的β偏移和tc偏移除以2。
  int32_t pps_beta_offset_div2 = 0;
  // 指示PPS是否包含量化缩放列表。
  int32_t pps_tc_offset_div2 = 0;
  int32_t pps_scaling_list_data_present_flag = 0;
  // 指示是否允许修改参考列表。
  int32_t lists_modification_present_flag = 0;
  // 并行合并级别的对数值减2。
  int32_t log2_parallel_merge_level = 0;
  int Log2ParMrgLevel = 0;

  // 切片段头扩展的存在标志。
  int32_t slice_segment_header_extension_present_flag = 0;
  // PPS扩展的存在标志。
  int32_t pps_extension_present_flag = 0;
  // 各种PPS扩展的启用标志。
  int32_t pps_range_extension_flag = false;
  int32_t pps_multilayer_extension_flag = false;
  int32_t pps_3d_extension_flag = false;
  int32_t pps_scc_extension_flag = false;
  // 指示PPS扩展数据是否存在。
  int32_t pps_extension_4bits = false;
  int residual_adaptive_colour_transform_enabled_flag = 0;

  int32_t pps_slice_act_qp_offsets_present_flag = false;
  int32_t chroma_qp_offset_list_enabled_flag = false;
  int32_t diff_cu_chroma_qp_offset_depth = 0;

  int log2_max_transform_skip_block_size_minus2 = 0;
  int cross_component_prediction_enabled_flag = 0;
  int cb_qp_offset_list[32] = {0};
  int cr_qp_offset_list[32] = {0};
  int log2_sao_offset_scale_luma = 0;
  int log2_sao_offset_scale_chroma = 0;

  int32_t pps_curr_pic_ref_enabled_flag = 0;
  int pps_act_y_qp_offset_plus5 = {0};

  int pps_act_cb_qp_offset_plus5 = {0};
  int pps_act_cr_qp_offset_plus3 = {0};
  int pps_palette_predictor_initializers_present_flag = 0;
  int pps_num_palette_predictor_initializers = 0;

  int monochrome_palette_flag = {0};
  int luma_bit_depth_entry_minus8 = {0};
  int chroma_bit_depth_entry_minus8 = 0;
  int pps_palette_predictor_initializer[3][128] = {{0}};

  uint8_t *CtbAddrTsToRs = nullptr;
  uint8_t *CtbAddrRsToTs = nullptr;
  int col_idxX[32] = {0};
  int rowHeight[32] = {0};
  int colWidth[32] = {0};

  SPS *m_sps = nullptr;
  int TileId[32] = {0};
  int scaling_list_data(BitStream &bs);
  int scaling_list_pred_mode_flag[4][6] = {0};
  int scaling_list_pred_matrix_id_delta[4][6] = {0};
  int scaling_list_dc_coef_minus8[4][6] = {0};
  int ScalingList[4][6][32] = {0};
  int pps_range_extension(BitStream &bs);
  int pps_scc_extension(BitStream &bs);
  // ------------------------------------------- Old -------------------------------------------
  /* PPS 的唯一标识符 */
  //uint32_t pic_parameter_set_id = 0;
  /* 该PPS对应的SSP标识符 */
  /* TODO：这里的sps id并没有使用到 */
  //uint32_t seq_parameter_set_id = 0;
  /* 表示使用的熵编码模式，其中：0: 表示使用 CAVLC, 1: 表示使用 CABAC */
  bool entropy_coding_mode_flag = 0;
  /* 指示是否存在场序信息 */
  bool bottom_field_pic_order_in_frame_present_flag = 0;
  /* 图像大小（以宏块单位表示） */
  uint32_t pic_size_in_map_units_minus1 = 0;
  /* 指定默认激活的 L0 参考索引数量减 1 */
  //uint32_t num_ref_idx_l0_default_active_minus1 = 0;
  //uint32_t num_ref_idx_l1_default_active_minus1 = 0;
  /* 指定第一个色度量化参数索引偏移(Cb) */
  int32_t chroma_qp_index_offset = 0;
  /* 指定第二个色度量化参数索引偏移(Cr) */
  int32_t second_chroma_qp_index_offset = 0;
  /* 指示是否存在约束的帧内预测 */
  //bool constrained_intra_pred_flag = 0;
  /* 指示是否存在 8x8 变换模式 */
  bool transform_8x8_mode_flag = 0;
  /* 指定图像缩放列表的最大数量 */
  uint32_t maxPICScalingList = 0;

  /* 指示是否存在冗余图像计数：主要用于错误恢复，I帧的备份 */
  bool redundant_pic_cnt_present_flag = 0;
  /* 指示是否存在加权预测 */
  //bool weighted_pred_flag = 0;
  /* 指定加权双向预测类型：
   * 0: 表示不使用加权双向预测。
   * 1: 表示使用加权双向预测。权重是显示计算的（由编码器提供）
   * 2: 表示使用加权双向预测，但权重是隐式计算的。 */
  uint32_t weighted_bipred_idc = 0;
  /* 指示是否存在去块滤波器控制信息 */
  bool deblocking_filter_control_present_flag = 0;
  /* 表示Slice group的数量 */
  uint32_t num_slice_groups_minus1 = 0;
  /* 指定切片组映射类型：
   * type=[0-2]：定义了固定的划分模式，比如交错或者基于矩形区域划分。
   * type=[3-5]：这些模式允许更灵活的分割，并且通常依赖于动态参数来定义宏块的分配。*/
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
  /* 帧内和帧间宏块的量化参数 */
  int32_t pic_init_qp_minus26 = 0;
  /* 场景切换(SI,SP Slice)或 B Slice 中帧间预测的量化参数 */
  int32_t pic_init_qs_minus26 = 0;
};

#endif /* end of include guard: PPS_CPP_F6QSULFM */
