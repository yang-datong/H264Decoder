
#include <cstdint>
class Slice {
 public:
  // Slice();
  //~Slice();

  /* NOTE: 默认值应该设置为0,因为有时候就是需要使用到默认值为0的情况，
   * 如果将变量放在for中那么默认值为其他就会发生不可预知的错误 */
 public:
  int m_is_malloc_mem_self = 0;
  uint32_t MaxFrameNum = 0;
  uint32_t maxPicOrderCntLsb = 0;
  bool field_pic_flag = 0;
  uint32_t slice_type = 0;
  uint32_t num_ref_idx_l0_active_minus1 = 0;
  uint32_t num_ref_idx_l1_active_minus1 = 0;
  bool IdrPicFlag = 0;
  uint32_t first_mb_in_slice = 0;
  bool MbaffFrameFlag = 0;
  int32_t *mapUnitToSliceGroupMap = nullptr;
  int32_t *MbToSliceGroupMap = nullptr;
  uint32_t slice_group_change_cycle = 0;
  int MapUnitsInSliceGroup0 = 0;

  uint32_t cabac_alignment_one_bit = 0;
  uint32_t mb_skip_run = 0;
  int32_t mb_skip_flag = 0;
  int32_t end_of_slice_flag = 0;
  uint32_t mb_field_decoding_flag = 0;
  uint32_t slice_id = 0;
  uint32_t slice_number = 0;
  uint32_t CurrMbAddr = 0;
  uint32_t syntax_element_categories = 0;
  bool moreDataFlag = 1;
  uint32_t prevMbSkipped = 0;
  int32_t mb_skip_flag_next_mb = 0;
  int32_t slice_qs_delta = 0;
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
};
