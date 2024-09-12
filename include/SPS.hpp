#ifndef SPS_CPP_F6QSULFM
#define SPS_CPP_F6QSULFM

#include "Common.hpp"
#include <cstdint>

#define Extended_SAR 255

#define H264_MAX_SPS_COUNT                                                     \
  32 // 7.4.2.1.1: seq_parameter_set_id shall be in the range of 0 to 31,     \
      // inclusive.
#define H264_MAX_OFFSET_REF_FRAME_COUNT                                        \
  256 // 7.4.2.1.1: num_ref_frames_in_pic_order_cnt_cycle shall be in the range \
      // of 0 to 255, inclusive.

const int32_t LevelNumber_MaxDpbMbs[19][2] = {
    {10, 396},    {11, 900},    {12, 2376},   {13, 2376},   {20, 2376},
    {21, 4752},   {22, 8100},   {30, 8100},   {31, 18000},  {32, 20480},
    {40, 32768},  {41, 32768},  {42, 34816},  {50, 110400}, {51, 184320},
    {52, 184320}, {60, 696320}, {61, 696320}, {62, 696320},
};

struct CHROMA_FORMAT_IDC_T {
  int32_t chroma_format_idc;
  int32_t separate_colour_plane_flag;
  int32_t Chroma_Format;
  int32_t SubWidthC;
  int32_t SubHeightC;
};

class SPS {
 public:
  uint8_t *_buf = nullptr;
  int _len = 0;
  int extractParameters();

 public:
  /* 表示编码配置文件 */
  uint8_t profile_idc = 0; // 0x64
  /* 表示编码级别 */
  uint8_t level_idc = 0; // 0
  /* SPS 的唯一标识符[0-31] */
  uint32_t seq_parameter_set_id = 0;
  /* 指示是否使用约束基线配置文件 */

  /*色度格式[0-3]。指定了亮度和色度分量的采样方式,444,420,422*/
  /* 当 chroma_format_idc 不存在时，应推断其等于 1（4:2:0 色度格式）。page 74 */
  uint32_t chroma_format_idc = 1;

  uint8_t constraint_set0_flag = 0;
  uint8_t constraint_set1_flag = 0;
  uint8_t constraint_set2_flag = 0;
  uint8_t constraint_set3_flag = 0;
  uint8_t constraint_set4_flag = 0;
  uint8_t constraint_set5_flag = 0;
  uint8_t reserved_zero_2bits = 0;
  /* 用于计算帧号的最大值 */
  uint32_t log2_max_pic_order_cnt_lsb_minus4 = 0;
  /* 最大帧号减去 4 的对数 */
  uint32_t log2_max_frame_num_minus4 = 0;
  /* 用于指定图像顺序计数的方法：
   * 0:使用帧号和帧场号来计算POC(图像顺序计数)( 适用于大多数情况，特别是需要精确控制POC的场景)
   * 1:使用增量计数来计算POC(复杂，如B帧和P帧混合的情况)
   * 2:使用帧号来计算POC(简单，如仅有I帧和P帧的情况)*/
  uint32_t pic_order_cnt_type = 0;

  /* 表示解码器需要支持的最大参考帧数 */
  uint32_t max_num_ref_frames = 0;
  /* 宏块（MB）单位的图像宽度减 1。（用于计算原图像正常情况下的宽）*/
  uint32_t pic_width_in_mbs_minus1 = 0;
  /* 宏块（MB）单位的图像高度减 1。（用于计算原图像正常情况下的高）*/
  uint32_t pic_height_in_map_units_minus1 = 0;

  uint32_t MaxFrameNum = 0;
  /* 指示在宏块的直接模式（主要是指B帧，如B_Direct) 下是否可以使用8x8变换 */
  bool direct_8x8_inference_flag = 0;

  /* 等于 1 指定 4:4:4 色度格式的三个颜色分量分别编码。 separate_colour_plane_flag 等于 0 指定颜色分量不单独编码。当separate_colour_plane_flag不存在时，应推断其等于0。当separate_colour_plane_flag等于1时，主编码图像由三个单独的分量组成，每个分量由一个颜色平面（Y、Cb或Cr）的编码样本组成。 ），每个都使用单色编码语法。在这种情况下，每个颜色平面都与特定的 color_plane_id 值相关联。 */
  bool separate_colour_plane_flag = 0;

  /* 亮度分量的比特深度减去 8 */
  uint32_t bit_depth_luma_minus8 = 0;
  /* 色度分量的比特深度减去 8 */
  uint32_t bit_depth_chroma_minus8 = 0;
  /* 图像是否仅包含帧（否则为场编码） */

  // 上面bit_depth_luma_minus8.bit_depth_chroma_minus8计算还原的结果
  uint32_t BitDepthY = 0;
  uint32_t QpBdOffsetY = 0;
  uint32_t BitDepthC = 0;
  uint32_t QpBdOffsetC = 0;

  /* 整个视频序列是以帧为单位编码，还是允许场编码：
   * 只有帧编码，即每个宏块（macroblock）都对应一整帧的图像。
   * 允许场编码，即可以使用场编码模式（每个宏块可以对应一个场，而不是一整帧）
   * 这个flag为场编码的前提下，需要进步一步判断Slice Header中的field_pic_flag，指示当前图片（picture）是帧图像还是场图像。*/
  bool frame_mbs_only_flag = 0;
  /* 帧顺序计数增量是否始终为零 */
  bool delta_pic_order_always_zero_flag = 0;

  /* 色度子采样的类型：
    0: 4:0:0 色度子采样（没有色度信息，只有亮度信息）
    1: 4:2:0 色度子采样（色度分辨率为亮度分辨率的一半）
    2: 4:2:2 色度子采样（色度分辨率为亮度分辨率的水平一半，垂直方向相同）
    3: 4:4:4 色度子采样（没有子采样，色度和亮度分辨率相同）
*/
  uint32_t ChromaArrayType = 0;

  /* 是否使用基于宏块的自适应帧/场编码 */
  bool mb_adaptive_frame_field_flag = 0;

  int32_t PicWidthInMbs = 0;
  int32_t PicHeightInMapUnits = 0;
  uint32_t PicSizeInMapUnits = 0;
  uint32_t frameHeightInMbs = 0;

  /* 是否对亮度分量中的零系数块应用变换旁路 */
  bool qpprime_y_zero_transform_bypass_flag = 0;

  /* 是否在序列级别存在缩放矩阵 */
  bool seq_scaling_matrix_present_flag = 0;

  /* 等于 1 指定缩放列表 i 的语法结构存在于序列参数集中。 seq_scaling_list_present_flag[ i ] 等于 0 指定缩放列表 i 的语法结构不存在于序列参数集中，并且表 7-2 中指定的缩放列表回退规则集 A 将用于推断序列级缩放索引 i 的列表。 */
  bool seq_scaling_list_present_flag[12] = {false};

  uint32_t MbWidthC = 0;          // 色度宏块宽度
  uint32_t MbHeightC = 0;         // 色度宏块高度
  int32_t pcm_sample_chroma[256]; // 3 u(v)

  /* 用于计算非参考帧帧顺序计数偏移量的值 */
  int32_t offset_for_non_ref_pic = 0;
  /* 用于计算场顺序计数偏移量的值 */
  int32_t offset_for_top_to_bottom_field = 0;
  /* 帧顺序计数循环中参考帧的数量 */
  uint32_t num_ref_frames_in_pic_order_cnt_cycle = 0;

  int32_t Chroma_Format = 0;
  int32_t SubWidthC = 0;
  int32_t SubHeightC = 0;

  uint32_t MaxPicOrderCntLsb;
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

  /* SPS::hrd_parameters() */
  uint32_t cpb_cnt_minus1 = 0;
  uint8_t bit_rate_scale = 0;
  uint8_t cpb_size_scale = 0;
  uint32_t *bit_rate_value_minus1 = nullptr;
  uint32_t *cpb_size_value_minus1 = nullptr;
  bool *cbr_flag = nullptr;
  uint8_t initial_cpb_removal_delay_length_minus1 = 0;
  uint8_t cpb_removal_delay_length_minus1 = 0;
  uint8_t dpb_output_delay_length_minus1 = 0;
  uint8_t time_offset_length = 0;

  /* VUI */
  /* 指示是否存在宽高比信息 */
  bool aspect_ratio_info_present_flag = 0;
  /* 宽高比标识符，指示视频的宽高比类型 */
  uint8_t aspect_ratio_idc = 0;
  /* 表示样本的宽度（SAR，样本宽高比） */
  uint16_t sar_width = 0;
  /* 表示样本的高度（SAR，样本宽高比） */
  uint16_t sar_height = 0;
  /* 指示是否存在超扫描信息 */
  bool overscan_info_present_flag = 0;
  /* 指示视频是否适合超扫描显示 */
  bool overscan_appropriate_flag = 0;
  /* 指示是否存在视频信号类型信息 */
  bool video_signal_type_present_flag = 0;
  /* 视频格式标识符，指示视频的类型（如未压缩、压缩等） */
  uint8_t video_format = 0;
  /* 指示视频是否使用全范围色彩（0-255）或限范围色彩（16-235） */
  bool video_full_range_flag = 0;
  /* 指示是否存在颜色描述信息 */
  bool colour_description_present_flag = 0;
  /* 指示颜色原色的类型（如BT.709、BT.601等） */
  uint8_t colour_primaries = 0;
  /* 指示传输特性（如线性、伽马等） */
  uint8_t transfer_characteristics = 0;
  /* 指示矩阵系数，用于颜色空间转换 */
  uint8_t matrix_coefficients = 0;
  /* 指示是否存在色度样本位置的信息 */
  bool chroma_loc_info_present_flag = 0;
  /* 顶场色度样本位置类型 */
  int32_t chroma_sample_loc_type_top_field = 0;
  /* 底场色度样本位置类型 */
  int32_t chroma_sample_loc_type_bottom_field = 0;
  /* 指示是否存在时间信息 */
  bool timing_info_present_flag = 0;
  /* 每个时钟周期的单位数 */
  uint32_t num_units_in_tick = 0;
  /* 时间尺度，表示每秒的单位数 */
  uint32_t time_scale = 0;
  /* 指示是否使用固定帧率 */
  bool fixed_frame_rate_flag = 0;
  /* 指示是否存在NAL HRD（网络提取率控制）参数 */
  bool nal_hrd_parameters_present_flag = 0;
  /* 指示是否存在VCL HRD参数 */
  bool vcl_hrd_parameters_present_flag = 0;
  /* 指示是否使用低延迟HRD */
  bool low_delay_hrd_flag = 0;
  /* 指示是否存在图像结构信息 */
  bool pic_struct_present_flag = 0;
  /* 指示是否存在比特流限制 */
  bool bitstream_restriction_flag = 0;
  /* 指示是否允许运动矢量跨越图像边界 */
  bool motion_vectors_over_pic_boundaries_flag = 0;
  /* 每帧最大字节数的分母 */
  uint32_t max_bytes_per_pic_denom = 0;
  /* 每个宏块最大比特数的分母 */
  uint32_t max_bits_per_mb_denom = 0;
  /* 水平运动矢量的最大长度的对数值 */
  uint32_t log2_max_mv_length_horizontal = 0;
  /* 垂直运动矢量的最大长度的对数值 */
  uint32_t log2_max_mv_length_vertical = 0;
  /* 最大重排序帧数 */
  int32_t max_num_reorder_frames = -1;
  /* 最大解码帧缓冲区大小 */
  uint32_t max_dec_frame_buffering = 0;

 public:
  void vui_parameters(BitStream &bitStream);
  void hrd_parameters(BitStream &bitStream);
  int seq_parameter_set_extension_rbsp();
};

#endif /* end of include guard: SPS_CPP_F6QSULFM */
