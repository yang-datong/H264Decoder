#ifndef SPS_CPP_F6QSULFM
#define SPS_CPP_F6QSULFM

#include "Common.hpp"
#include "RBSP.hpp"

#define Extended_SAR 255

#define H264_MAX_SPS_COUNT                                                     \
  32 // 7.4.2.1.1: seq_parameter_set_id shall be in the range of 0 to 31,
     // inclusive.
#define H264_MAX_OFFSET_REF_FRAME_COUNT                                        \
  256 // 7.4.2.1.1: num_ref_frames_in_pic_order_cnt_cycle shall be in the range
      // of 0 to 255, inclusive.

struct CHROMA_FORMAT_IDC_T {
  int32_t chroma_format_idc;
  int32_t separate_colour_plane_flag;
  int32_t Chroma_Format;
  int32_t SubWidthC;
  int32_t SubHeightC;
};

class SPS : public RBSP {
 public:
  SPS();
  ~SPS();
  uint8_t *_buf = nullptr;
  int _len = 0;
  int extractParameters();

 public:
  /* 表示编码配置文件 */
  uint8_t profile_idc = 0; // 0x64
  /* 表示编码级别 */
  uint8_t level_idc = 0; // 0
  /* SPS 的唯一标识符 */
  uint32_t seq_parameter_set_id = 0;
  /* 指示是否使用约束基线配置文件 */
  uint8_t constraint_set0_5_flag = 0;
  uint8_t reserved_zero_2bits = 0;
  /* 用于计算帧号的最大值 */
  uint32_t log2_max_pic_order_cnt_lsb_minus4 = 0;
  /* 最大帧号减去 4 的对数 */
  uint32_t log2_max_frame_num_minus4 = 0;
  /* 用于指定图像顺序计数的方法(一般处理 B 帧插入) */
  uint32_t pic_order_cnt_type = 0;
  /* 表示解码器需要支持的最大参考帧数 */
  uint32_t max_num_ref_frames = 0;
  /* 宏块（MB）单位的图像宽度减 1。（用于计算原图像正常情况下的宽）*/
  uint32_t pic_width_in_mbs_minus1 = 0;
  /* 宏块（MB）单位的图像高度减 1。（用于计算原图像正常情况下的高）*/
  uint32_t pic_height_in_map_units_minus1 = 0;
  /*色度格式。指定了亮度和色度分量的采样方式,444,420,422*/
  uint32_t chroma_format_idc = 0;

  uint32_t MaxFrameNum = 0;
  /* 是否使用直接 8x8 推断 */
  bool direct_8x8_inference_flag = 0;

  bool separate_colour_plane_flag = 0;

  /* 亮度分量的比特深度减去 8 */
  uint32_t bit_depth_luma_minus8 = 0;
  /* 色度分量的比特深度减去 8 */
  uint32_t bit_depth_chroma_minus8 = 0;
  /* 图像是否仅包含帧（否则为场编码） */
  bool frame_mbs_only_flag = 0;
  /* 帧顺序计数增量是否始终为零 */
  bool delta_pic_order_always_zero_flag = 0;

  uint32_t ChromaArrayType = 0;

  /* 是否使用基于宏块的自适应帧/场编码 */
  bool mb_adaptive_frame_field_flag = 0;

  uint32_t PicWidthInMbs = 0;
  uint32_t PicHeightInMapUnits = 0;
  uint32_t PicSizeInMapUnits = 0;
  uint32_t frameHeightInMbs = 0;

  /* 是否对亮度分量中的零系数块应用变换旁路 */
  bool qpprime_y_zero_transform_bypass_flag = 0;
  /* 是否在序列级别存在缩放矩阵 */
  bool seq_scaling_matrix_present_flag = 0;
  bool seq_scaling_list_present_flag[12] = {false};

  uint32_t BitDepthY = 0;
  uint32_t QpBdOffsetY = 0;
  uint32_t BitDepthC = 0;
  uint32_t QpBdOffsetC = 0;
  uint32_t MbWidthC = 0;          // 色度宏块宽度
  uint32_t MbHeightC = 0;         // 色度宏块高度
  int32_t pcm_sample_chroma[256]; // 3 u(v)

  /* 用于计算非参考帧帧顺序计数偏移量的值 */
  int32_t offset_for_non_ref_pic = 0;
  /* 用于计算场顺序计数偏移量的值 */
  int32_t offset_for_top_to_bottom_field = 0;
  /* 帧顺序计数循环中参考帧的数量 */
  uint32_t num_ref_frames_in_pic_order_cnt_cycle = 0;

  int Chroma_Format = 0;
  int SubWidthC = 0;
  int SubHeightC = 0;
  uint32_t pic_parametter_set_id = 0;
  uint8_t colour_plane_id = 0;
  uint32_t frame_num = 0; // u(v)
  bool bottom_field_flag = 0;
  uint32_t idr_pic_id = 0;
  uint32_t pic_order_cnt_lsb = 0;
  int32_t delta_pic_order_cnt_bottom = 0;
  int32_t delta_pic_order_cnt[2] = {0};
  uint32_t redundant_pic_cnt = 0;
  bool direct_spatial_mv_pred_flag = 0;
  bool num_ref_idx_active_override_flag = 0;
  uint32_t cabac_init_idc = 0;
  int32_t slice_qp_delta = 0;
  bool sp_for_switch_flag = 0;
  uint32_t disable_deblocking_filter_idc = 0;

  int32_t MaxPicOrderCntLsb;
  int32_t ExpectedDeltaPerPicOrderCntCycle;

  /* 用于计算参考帧帧顺序计数偏移量的值，调整参考帧的显示顺序，以确保解码后的视频帧以正确的顺序显示*/
  int32_t offset_for_ref_frame[H264_MAX_OFFSET_REF_FRAME_COUNT];
  /* 是否允许帧号值中的间隙 */
  bool gaps_in_frame_num_value_allowed_flag = 0;
  /* 是否应用帧裁剪 */
  bool frame_cropping_flag = 0;
  /* 帧裁剪左、右、顶、底偏移量 */
  uint32_t frame_crop_left_offset = 0;
  uint32_t frame_crop_right_offset = 0;
  uint32_t frame_crop_top_offset = 0;
  uint32_t frame_crop_bottom_offset = 0;

  /* 是否存在视频用户界面 (VUI) 参数 */
  bool vui_parameters_present_flag = 0;

  /* 缩放矩阵 */
  uint32_t ScalingList4x4[6][16];
  uint32_t ScalingList8x8[6][64];
  /* 是否使用默认 4x4/8x8 缩放矩阵 */
  uint32_t UseDefaultScalingMatrix4x4Flag[6];
  uint32_t UseDefaultScalingMatrix8x8Flag[6];

  uint32_t picWidthInSamplesL = 0;
  // 亮度分量的采样宽度，等于宏块宽度乘以 16
  uint32_t picWidthInSamplesC = 0;
  // 色度分量的采样宽度，等于宏块宽度乘以 MbWidthC。
  uint32_t RawMbBits = 0;

 public:
  void vui_parameters(BitStream &bitStream);
  void hrd_parameters(BitStream &bitStream);
};

#endif /* end of include guard: SPS_CPP_F6QSULFM */
