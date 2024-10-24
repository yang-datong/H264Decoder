#ifndef SLICEHEADER_HPP_JYXLKOEI
#define SLICEHEADER_HPP_JYXLKOEI

#include "BitStream.hpp"
#include "GOP.hpp"
#include "PPS.hpp"
#include "SPS.hpp"
#include <cstdint>

typedef struct _DEC_REF_PIC_MARKING_ {
  int32_t memory_management_control_operation; // 2 | 5 ue(v)
  int32_t difference_of_pic_nums_minus1;       // 2 | 5 ue(v)
  int32_t long_term_pic_num_2;                 // 2 | 5 ue(v)
  int32_t long_term_frame_idx;                 // 2 | 5 ue(v)
  int32_t max_long_term_frame_idx_plus1;       // 2 | 5 ue(v)
} DEC_REF_PIC_MARKING;

class SliceHeader {
 public:
  /* 需要对外提供，因为SliceHeader是最先知道当前所使用的SPS ID，PPS ID的 */
  SPS *m_sps;
  PPS *m_pps;

  /* 引用自Nalu */
  const uint8_t nal_unit_type;
  uint8_t nal_ref_idc;

 private:
  /* 私有化SliceBody，不提供给外界，只能通过Slice来访问本类 */
  /* 允许Slice类访问 */
  friend class Slice;
  SliceHeader(uint8_t nal_type, uint8_t nal_ref_idc)
      : nal_unit_type(nal_type), nal_ref_idc(nal_ref_idc){};
  ~SliceHeader();

  // ------------ NEW -------------
 public:
  int32_t slice_pic_parameter_set_id = {0};
  int32_t dependent_slice_segment_flag = false;
  int32_t slice_segment_address = 0;
  int32_t slice_ctb_addr_rs = 0;

  int32_t SliceAddrRs = 0;

  int32_t CuQpDeltaVal = 0;
  //NOTE: When pic_output_flag is not present, it is inferred to be equal to 1.
  int32_t pic_output_flag = 1;
  int32_t colour_plane_id = 0;
  int32_t slice_pic_order_cnt_lsb = 0;
  int32_t short_term_ref_pic_set_sps_flag = {0};
  int32_t short_term_ref_pic_set_idx = 0;
  int32_t num_long_term_sps = 0;
  int32_t num_long_term_pics = 0;
  int32_t delta_poc_msb_present_flag[32] = {0};
  int32_t delta_poc_msb_cycle_lt[32] = {0};
  int DeltaPocMsbCycleLt[32] = {0};

  int32_t slice_temporal_mvp_enabled_flag = {0};
  int32_t lt_idx_sps[32] = {0};
  int32_t poc_lsb_lt[32] = {0};
  int32_t used_by_curr_pic_lt_flag[32] = {0};
  int PocLsbLt[32] = {0};

  int32_t slice_sao_luma_flag = 0;
  int32_t slice_sao_chroma_flag = 0;

  int32_t mvd_l1_zero_flag = 0;
  int32_t cabac_init_flag = 0;
  // NOTE: collocated_from_l0_flag equal to 1 specifies that the collocated picture used for temporal motion vector prediction is derived from reference picture list 0. collocated_from_l0_flag equal to 0 specifies that the collocated picture used for temporal motion vector prediction is derived from reference picture list 1. When collocated_from_l0_flag is not present, it is inferred to be equal to 1.
  int32_t collocated_from_l0_flag = 1;
  int32_t collocated_ref_idx = 0;
  int32_t weighted_pred_flag = 0;
  int32_t weighted_bipred_flag = 0;
  int32_t five_minus_max_num_merge_cand = 0;
  int32_t motion_vector_resolution_control_idc = 0;
  int32_t use_integer_mv_flag = 0;

  int32_t slice_cb_qp_offset = 0;
  int32_t slice_cr_qp_offset = 0;
  int32_t slice_act_y_qp_offset = 0;
  int32_t slice_act_cb_qp_offset = 0;
  int32_t slice_act_cr_qp_offset = 0;

  int32_t cu_chroma_qp_offset_enabled_flag = 0;
  int32_t deblocking_filter_override_flag = 0;
  int32_t slice_deblocking_filter_disabled_flag = 0;
  int32_t slice_beta_offset_div2 = 0;
  int32_t slice_tc_offset_div2 = 0;

  int32_t slice_loop_filter_across_slices_enabled_flag = 0;

  int32_t num_entry_point_offsets = 0;
  int32_t offset_len_minus1 = 0;
  int32_t entry_point_offset_minus1[32] = {0};
  int32_t slice_segment_header_extension_length = 0;
  int32_t slice_segment_header_extension_data_byte[32] = {0};

  int32_t list_entry_lx[2][32] = {{0}};

  void ref_pic_lists_modification(BitStream *bs, int32_t nb_refs);

  int st_ref_pic_set(BitStream *bs, int32_t stRpsIdx);
  int32_t inter_ref_pic_set_prediction_flag = 0;
  int32_t delta_idx_minus1 = 0;
  int32_t delta_rps_sign = 0;
  int32_t abs_delta_rps_minus1 = 0;
  int32_t used_by_curr_pic_flag[32] = {0};
  int32_t use_delta_flag[32] = {0};
  int32_t num_negative_pics = 0;
  int32_t num_positive_pics = 0;
  int32_t delta_poc_s0_minus1[32] = {0};
  int32_t used_by_curr_pic_s0_flag[32] = {0};
  int32_t delta_poc_s1_minus1[32] = {0};
  int32_t used_by_curr_pic_s1_flag[32] = {0};

  int32_t NumNegativePics[32] = {0};
  int32_t NumPositivePics[32] = {0};
  int32_t NumDeltaPocs[32] = {0};
  int32_t UsedByCurrPicS0[32][32] = {0};
  int32_t UsedByCurrPicS1[32][32] = {0};
  int32_t UsedByCurrPicLt[32] = {0};
  int32_t DeltaPocS0[32][32] = {0};
  int32_t DeltaPocS1[32][32] = {0};

  int32_t RefRpsIdx = 0;
  int deltaRps = 0;

  int SliceQpY = 0;
  int32_t NumPicTotalCurr = 0;

  int CurrRpsIdx = 0;

 public:
  /* Slice中第一个宏块的索引。 （可判断一帧图像是否由多个Slice组成）
  如果first_mb_in_slice == 0，则表示这是该帧的第一个Slice（可以独立解码）。
  如果first_mb_in_slice != 0，则表示该Slice与前面的Slice共同组成一帧数据（需要组合解码）。*/
  /* TODO YangJing 一帧图像多个Slice的情况还没有测试过 <24-09-15 00:00:19> */
  uint32_t first_slice_segment_in_pic_flag = 0;
  /* Slice的类型（I, P, B等） */
  uint32_t slice_type = 0;

 private:
  /* PPS ID */
  uint32_t pic_parameter_set_id = 0;

 public:
  /* 颜色平面ID：当存在色度子采样时，两个相应的色度采样阵列，颜色分量的每个宏块恰好包含在一个切片中（即，图片的每个宏块的信息恰好存在于三个切片中，并且这三个切片具有不同的colour_plane_id值）。*/
  //uint8_t colour_plane_id = 0;
  /* 当前帧的编号 */
  uint32_t frame_num = 0;
  /* 场图像标志 */
  bool field_pic_flag = 0;
  /* 底场标志 */
  bool bottom_field_flag = 0;
  /* 由slice_type计算出来 */
  bool IdrPicFlag = 0;
  /* IDR图像ID */
  uint32_t idr_pic_id = 0;

  /* 图像顺序计数(POC低位，对应的还有高位，高位需要在解码POC时进行计算) */
  uint32_t pic_order_cnt_lsb = 0;
  /* 底场的图像顺序计数增量 */
  int32_t delta_pic_order_cnt_bottom = 0;

 public:
  /* 参考帧列表0的活动参考帧数减1 */
  uint32_t num_ref_idx_l0_active_minus1 = 0;
  /* 参考帧列表1的活动参考帧数减1 */
  uint32_t num_ref_idx_l1_active_minus1 = 0;
  /* 映射单元（或一个宏块或一对宏块）到Slice组的映射表 */
  int32_t *mapUnitToSliceGroupMap = nullptr;
  /* 最终的宏块到切片组的映射 */
  int32_t *MbToSliceGroupMap = nullptr;

  /* 图像顺序计数增量 */
  int32_t delta_pic_order_cnt[2] = {0};
  /* 冗余图像计数 */
  uint32_t redundant_pic_cnt = 0;

 private:
  /* 内存分配标志 */
  int m_is_malloc_mem_self = 0;

 public:
  /* 直接空间运动矢量预测标志 */
  bool direct_spatial_mv_pred_flag = 0;
  /* CABAC初始化索引 */
  uint32_t cabac_init_idc = 0;

 private:
  /* 覆盖活动参考帧数标志 */
  bool num_ref_idx_active_override_flag = 0;

  /* SP切换标志 */
  bool sp_for_switch_flag = 0;
  /* Slice 的量化参数的差值，参考PPS中的全局量化值 */
  int32_t slice_qp_delta = 0;
  /* SP Slice 的量化参数的差值，参考PPS中的全局量化值，但仅帧对于SP Slice */
  int32_t slice_qs_delta = 0;

  /* Slice组改变周期 */
  uint32_t slice_group_change_cycle = 0;

 public:
  /* 禁用去块效应滤波器标志：
   * 0：应用去块滤波器
   * 1：禁用去块滤波器
   * 2：部分应用去块滤波器：只应用于slice内部的宏块边缘，而不应用于slice边界的宏块边缘。这可以用于当多个slice被并行处理时，以减少并行处理边界上的潜在视觉不一致。*/
  uint32_t disable_deblocking_filter_idc = 0;
  /* 去块效应滤波器的Alpha偏移值：控制着去块滤波器是否启动的阈值 */
  int32_t slice_alpha_c0_offset_div2 = 0;
  /* 去块效应滤波器的Beta偏移值：决定滤波器强度的阈值 */
  //int32_t slice_beta_offset_div2 = 0;
  /* 前一个Slice的量化参数（这里的Y表示亮度块，一般来说QP也以QPY为准） */
  int32_t QPY_prev = 0;
  /* Slice的量化参数（色度） */
  int32_t QSY = 0;

  /* Slice组0中的映射单元数 */
  int MapUnitsInSliceGroup0 = 0;

  /* 宏块自适应帧场标志：MBAFF是一种编码模式，允许在帧内使用宏块级别的自适应帧场编码。它可以在同一帧中混合使用帧宏块和场宏块：
   * MbaffFrameFlag == 1: 表示当前帧使用MBAFF编码模式。在这种模式下，每个宏块对（MB pair）可以独立地选择是作为帧宏块对还是场宏块对进行编码。
   * MbaffFrameFlag == 0: 表示当前帧不使用MBAFF编码模式。所有宏块都作为帧宏块进行编码。
   * 
   * 注:在MBAFF（宏块对自适应帧场编码）模式下，宏块对中的每个宏块（顶宏块和底宏块）是独立处理的，即第一次循环的宏块是顶宏块，那么底宏块会在第二次循环时才出现，并不是两个宏块打包为一个宏块对一并处理
   * */
  bool MbaffFrameFlag = 0;

  /* Slice的量化参数 */
  int SliceQPY = 0;

 public:
  /* 最大图像编号 */
  int MaxPicNum = 0;
  /* 当前图像编号 */
  int CurrPicNum = 0;
  /* 去块效应滤波器的A偏移值 */
  int FilterOffsetA = 0;
  /* 去块效应滤波器的B偏移值 */
  int FilterOffsetB = 0;

  /* 图像大小（宏块数） */
  int32_t PicSizeInMbs = 0;

  /* 图像宽度（宏块数） */
  int32_t PicWidthInMbs = 0;
  /* 图像宽度（亮度样本） */
  int32_t PicWidthInSamplesL = 0;
  /* 图像宽度（色度样本） */
  int32_t PicWidthInSamplesC = 0;

  /* 图像高度（宏块数） */
  int32_t PicHeightInMbs = 0;
  /* 图像高度（亮度样本） */
  int32_t PicHeightInSamplesL = 0;
  /* 图像高度（色度样本） */
  int32_t PicHeightInSamplesC = 0;

  //-------------- 显示加权预测 ---------------
  /* 亮度权重的对数基数 */
  uint32_t luma_log2_weight_denom = 0;
  /* 色度权重的对数基数 */
  uint32_t chroma_log2_weight_denom = 0;
  /* 参考帧列表0的亮度权重 */
  int32_t luma_weight_l0[32] = {0};
  /* 参考帧列表0的亮度偏移 */
  int32_t luma_offset_l0[32] = {0};
  /* 参考帧列表0的色度权重 */
  int32_t chroma_weight_l0[32][2] = {{0}};
  /* 参考帧列表0的色度偏移 */
  int32_t chroma_offset_l0[32][2] = {{0}};

  /* 参考帧列表1的亮度权重 */
  int32_t luma_weight_l1[32] = {0};
  /* 参考帧列表1的亮度偏移 */
  int32_t luma_offset_l1[32] = {0};
  /* 参考帧列表1的色度权重 */
  int32_t chroma_weight_l1[32][2] = {{0}};
  /* 参考帧列表1的色度偏移 */
  int32_t chroma_offset_l1[32][2] = {{0}};

  // ref_pic_list_modification
 public:
  /* 参考帧列表0的预测图像编号 */
  int32_t picNumL0Pred = 0;
  /* 参考帧列表1的预测图像编号 */
  int32_t picNumL1Pred = 0;
  /* 参考帧列表0的参考索引 */
  int32_t refIdxL0 = 0;
  /* 参考帧列表1的参考索引 */
  int32_t refIdxL1 = 0;

  int32_t ref_pic_list_modification_flag_l0 = 0;
  int32_t ref_pic_list_modification_flag_l1 = 0;
  int32_t ref_pic_list_modification_count_l0 = 0;
  int32_t ref_pic_list_modification_count_l1 = 0;
  /* modification_of_pic_nums_idc的取值如下：
 * 0: 减去帧编号差值
 * 1: 加上帧编号差值
 * 2: 使用长编码号的参考帧
 * 3: 结束参考帧列表的修改*/
  int32_t modification_of_pic_nums_idc[2][32] = {{0}};
  /* 当前帧的预测帧编号与目标参考帧编号的绝对差值，这个差值可以是正值，也可以是负值，具体取决于要向前或向后参考哪一帧 */
  int32_t abs_diff_pic_num_minus1[2][32] = {{0}};
  int32_t long_term_pic_num[2][32] = {{0}};

  // dec_ref_pic_marking
 public:
  bool no_output_of_prior_pics_flag = false;
  bool long_term_reference_flag = false;
  bool adaptive_ref_pic_marking_mode_flag = false;
  int32_t dec_ref_pic_marking_count = 0;
  /* 当前使用的缩放矩阵值: 
   * 对于4x4:
     * 0-2为Sl_4x4_Intra_Y/Cb/Cr
     * 3-5为Sl_4x4_Inter_Y/Cb/Cr
   * 对于8x8:
     * 0-1为Sl_8x8_Intra/Inter_Y
     * 2-3为Sl_8x8_Intra/Inter_Cb
     * 4-5为Sl_8x8_Intra/Inter_Cr */
  uint32_t ScalingList4x4[6][16] = {{0}};
  uint32_t ScalingList8x8[6][64] = {{0}};
  DEC_REF_PIC_MARKING m_dec_ref_pic_marking[32];

 private:
  BitStream *_bs = nullptr;
  int set_scaling_lists_values();
  int seq_scaling_matrix(int32_t scaling_list_size);
  int pic_scaling_matrix(int32_t scaling_list_size);
  void printf_scaling_lists_values();

  void ref_pic_list_mvc_modification();
  void ref_pic_lists_modification();
  void pred_weight_table();
  void dec_ref_pic_marking();

 public:
  int slice_segment_header(BitStream &bitStream, GOP &gop);
};
#endif /* end of include guard: SLICEHEADER_HPP_JYXLKOEI */
