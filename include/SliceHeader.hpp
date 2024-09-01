#ifndef SLICEHEADER_HPP_JYXLKOEI
#define SLICEHEADER_HPP_JYXLKOEI

#include "PPS.hpp"
#include "SPS.hpp"
#include <cstdint>

class Nalu;

typedef struct _DEC_REF_PIC_MARKING_ {
  int32_t memory_management_control_operation; // 2 | 5 ue(v)
  int32_t difference_of_pic_nums_minus1;       // 2 | 5 ue(v)
  int32_t long_term_pic_num_2;                 // 2 | 5 ue(v)
  int32_t long_term_frame_idx;                 // 2 | 5 ue(v)
  int32_t max_long_term_frame_idx_plus1;       // 2 | 5 ue(v)
} DEC_REF_PIC_MARKING;

class SliceHeader {
 private:
  /* NOTE: 默认值应该设置为0,因为有时候就是需要使用到默认值为0的情况，
   * 如果将变量放在for中那么默认值为其他就会发生不可预知的错误 */
  SPS m_sps;
  PPS m_pps;

 private:
  /* 私有化SliceBody，不提供给外界，只能通过Slice来访问本类 */
  SliceHeader(SPS &sps, PPS &pps) : m_sps(sps), m_pps(pps){};

 public:
  /* 允许Slice类访问 */
  friend class Slice;
  void setSPS(SPS &sps) { this->m_sps = sps; }
  void setPPS(PPS &pps) { this->m_pps = pps; }

 public:
  /* 引用自Nalu */
  char nal_unit_type = 0;
  char nal_ref_idc = 0;

  /* 由slice_type计算出来 */
  bool IdrPicFlag = 0;

  //uint32_t slice_id = 0;
  //uint32_t slice_number = 0;
  //uint32_t syntax_element_categories = 0;

  /* Slice中第一个宏块的索引。 
  如果first_mb_in_slice == 0，则表示这是该帧的第一个Slice（可以独立解码）。
  如果first_mb_in_slice != 0，则表示该Slice与前面的Slice共同组成一帧数据（需要组合解码）。*/
  uint32_t first_mb_in_slice = 0;
  /* Slice的量化步长调整值 */
  int32_t slice_qs_delta = 0;
  /* PPS ID */
  uint32_t pic_parameter_set_id = 0;

  /* 去块效应滤波器的Alpha偏移值 */
  int32_t slice_alpha_c0_offset_div2 = 0;
  /* 去块效应滤波器的Beta偏移值 */
  int32_t slice_beta_offset_div2 = 0;

  /* Slice的类型（I, P, B等） */
  uint32_t slice_type = 0;
  /* 内存分配标志 */
  int m_is_malloc_mem_self = 0;
  /* 参考帧列表0的活动参考帧数减1 */
  uint32_t num_ref_idx_l0_active_minus1 = 0;
  /* 参考帧列表1的活动参考帧数减1 */
  uint32_t num_ref_idx_l1_active_minus1 = 0;
  /* 映射单元到Slice组的映射表 */
  int32_t *mapUnitToSliceGroupMap = nullptr;
  /* 宏块到Slice组的映射表 */
  int32_t *MbToSliceGroupMap = nullptr;
  /* Slice组改变周期 */
  uint32_t slice_group_change_cycle = 0;
  /* Slice组0中的映射单元数 */
  int MapUnitsInSliceGroup0 = 0;

  /* 宏块自适应帧场标志：MBAFF是一种编码模式，允许在帧内使用宏块级别的自适应帧场编码。它可以在同一帧中混合使用帧宏块和场宏块：
   * MbaffFrameFlag == 1: 表示当前帧使用MBAFF编码模式。在这种模式下，每个宏块对（MB pair）可以独立地选择是作为帧宏块对还是场宏块对进行编码。
   * MbaffFrameFlag == 0: 表示当前帧不使用MBAFF编码模式。所有宏块都作为帧宏块进行编码。 */
  bool MbaffFrameFlag = 0;
  /* 场图像标志 */
  bool field_pic_flag = 0;

  /* 颜色平面ID */
  uint8_t colour_plane_id = 0;
  /* 当前帧的编号 */
  uint32_t frame_num = 0;
  /* 底场标志 */
  bool bottom_field_flag = 0;
  /* IDR图像ID */
  uint32_t idr_pic_id = 0;
  /* 图像顺序计数LSB */
  uint32_t pic_order_cnt_lsb = 0;
  /* 底场的图像顺序计数增量 */
  int32_t delta_pic_order_cnt_bottom = 0;
  //bool delta_pic_order_always_zero_flag = 0;
  /* 冗余图像计数 */
  uint32_t redundant_pic_cnt = 0;
  /* 直接空间运动矢量预测标志 */
  bool direct_spatial_mv_pred_flag = 0;
  /* 覆盖活动参考帧数标志 */
  bool num_ref_idx_active_override_flag = 0;
  /* 图像顺序计数增量 */
  int32_t delta_pic_order_cnt[2] = {0};
  /* CABAC初始化索引 */
  uint32_t cabac_init_idc = 0;
  /* Slice的量化参数调整值 */
  int32_t slice_qp_delta = 0;
  /* SP切换标志 */
  bool sp_for_switch_flag = 0;
  /* Slice的量化参数 */
  int SliceQPY = 0;
  /* 图像高度（亮度样本） */
  int PicHeightInSamplesL = 0;
  /* 图像高度（色度样本） */
  int PicHeightInSamplesC = 0;
  /* 禁用去块效应滤波器标志 */
  uint32_t disable_deblocking_filter_idc = 0;
  /* 前一个Slice的量化参数 */
  int QPY_prev = 0;
  /* 最大图像编号 */
  int MaxPicNum = 0;
  /* 当前图像编号 */
  int CurrPicNum = 0;
  /* Slice的量化参数（色度） */
  int QSY = 0;
  /* 去块效应滤波器的A偏移值 */
  int FilterOffsetA = 0;
  /* 去块效应滤波器的B偏移值 */
  int FilterOffsetB = 0;

  /* 图像高度（宏块数） */
  int32_t PicHeightInMbs = 0;
  /* 图像大小（宏块数） */
  int32_t PicSizeInMbs = 0;

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

  /* 参考帧列表0的预测图像编号 */
  int32_t picNumL0Pred = 0;
  /* 参考帧列表1的预测图像编号 */
  int32_t picNumL1Pred = 0;
  /* 参考帧列表0的参考索引 */
  int32_t refIdxL0 = 0;
  /* 参考帧列表1的参考索引 */
  int32_t refIdxL1 = 0;

  // ref_pic_list_modification
  int32_t ref_pic_list_modification_flag_l0;   // 2 u(1)
  int32_t modification_of_pic_nums_idc[2][32]; // 2 ue(v)
  int32_t abs_diff_pic_num_minus1[2][32];      // 2 ue(v)
  int32_t long_term_pic_num[2][32];            // 2 ue(v)
  int32_t ref_pic_list_modification_flag_l1;   // 2 u(1)
  int32_t
      ref_pic_list_modification_count_l0; // modification_of_pic_nums_idc[0]数组大小
  int32_t
      ref_pic_list_modification_count_l1; // modification_of_pic_nums_idc[1]数组大小
                                          //
                                          //
  // dec_ref_pic_marking
  int32_t no_output_of_prior_pics_flag;       // 2 | 5 u(1)
  int32_t long_term_reference_flag;           // 2 | 5 u(1)
  int32_t adaptive_ref_pic_marking_mode_flag; // 2 | 5 u(1)
  DEC_REF_PIC_MARKING m_dec_ref_pic_marking[32];
  int32_t dec_ref_pic_marking_count; // m_dec_ref_pic_marking[]数组大小
                                     //
  uint32_t ScalingList4x4[6][16] = {{0}};
  uint32_t ScalingList8x8[6][64] = {{0}};

  int set_scaling_lists_values();

  void ref_pic_list_mvc_modification(BitStream &bitStream);
  void ref_pic_list_modification(BitStream &bitStream);

  void pred_weight_table(BitStream &bitStream);
  void dec_ref_pic_marking(BitStream &bitStream);
  int parseSliceHeader(BitStream &bitStream);
};
#endif /* end of include guard: SLICEHEADER_HPP_JYXLKOEI */
