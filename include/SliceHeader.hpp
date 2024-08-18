#ifndef SLICEHEADER_HPP_JYXLKOEI
#define SLICEHEADER_HPP_JYXLKOEI

#include "IDR.hpp"
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
 public:
  /* NOTE: 默认值应该设置为0,因为有时候就是需要使用到默认值为0的情况，
   * 如果将变量放在for中那么默认值为其他就会发生不可预知的错误 */
 public:
  SPS m_sps;
  PPS m_pps;
  IDR m_idr;
  //Nalu *m_nalu;
  char nal_unit_type = 0;
  char nal_ref_idc = 0;

  int m_is_malloc_mem_self = 0;
  uint32_t slice_type = 0;
  uint32_t num_ref_idx_l0_active_minus1 = 0;
  uint32_t num_ref_idx_l1_active_minus1 = 0;
  bool IdrPicFlag = 0;
  int32_t *mapUnitToSliceGroupMap = nullptr;
  int32_t *MbToSliceGroupMap = nullptr;
  uint32_t slice_group_change_cycle = 0;
  int MapUnitsInSliceGroup0 = 0;

  bool MbaffFrameFlag = 0;
  bool field_pic_flag = 0;
  uint32_t mb_skip_run = 0;
  int32_t mb_skip_flag = 0;
  int32_t mb_skip_flag_next_mb = 0;
  int32_t end_of_slice_flag = 0;
  uint32_t slice_id = 0;
  uint32_t slice_number = 0;
  uint32_t syntax_element_categories = 0;
  int32_t slice_qs_delta = 0;
  uint32_t first_mb_in_slice = 0;
  int32_t slice_alpha_c0_offset_div2 = 0;
  int32_t slice_beta_offset_div2 = 0;

  /* TODO YangJing  <24-07-29 19:11:15> */
  uint32_t pic_parametter_set_id = 0;

  uint8_t colour_plane_id = 0;
  uint32_t frame_num = 0; // u(v)
  bool bottom_field_flag = 0;
  uint32_t idr_pic_id = 0;
  uint32_t pic_order_cnt_lsb = 0;
  int32_t delta_pic_order_cnt_bottom = 0;
  bool delta_pic_order_always_zero_flag = 0;
  uint32_t redundant_pic_cnt = 0;
  bool direct_spatial_mv_pred_flag = 0;
  bool num_ref_idx_active_override_flag = 0;
  int32_t delta_pic_order_cnt[2] = {0};
  uint32_t cabac_init_idc = 0;
  int32_t slice_qp_delta = 0;
  bool sp_for_switch_flag = 0;
  int SliceQPY = 0;
  int PicHeightInSamplesL = 0;
  uint32_t disable_deblocking_filter_idc = 0;
  int QPY_prev = 0;
  int PicHeightInSamplesC = 0;
  int MaxPicNum = 0;
  int CurrPicNum = 0;
  int QSY = 0;
  int FilterOffsetA = 0;
  int FilterOffsetB = 0;

  int32_t PicHeightInMbs = 0;
  int32_t PicSizeInMbs = 0;

  uint32_t luma_log2_weight_denom = 0;
  uint32_t chroma_log2_weight_denom = 0;
  int32_t luma_weight_l0[32] = {0};
  int32_t luma_offset_l0[32] = {0};
  int32_t chroma_weight_l0[32][2] = {{0}};
  int32_t chroma_offset_l0[32][2] = {{0}};

  int32_t luma_weight_l1[32] = {0};
  int32_t luma_offset_l1[32] = {0};
  int32_t chroma_weight_l1[32][2] = {{0}};
  int32_t chroma_offset_l1[32][2] = {{0}};

  int32_t picNumL0Pred;
  int32_t picNumL1Pred;
  int32_t refIdxL0;
  int32_t refIdxL1;

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
  int setMapUnitToSliceGroupMap();
  int setMbToSliceGroupMap();

  void ref_pic_list_mvc_modification(BitStream &bitStream);
  void ref_pic_list_modification(BitStream &bitStream);

  void pred_weight_table(BitStream &bitStream);
  void dec_ref_pic_marking(BitStream &bitStream);
  int parseSliceHeader(BitStream &bitStream, Nalu *nalu);
};
#endif /* end of include guard: SLICEHEADER_HPP_JYXLKOEI */
