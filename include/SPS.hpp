#ifndef SPS_CPP_F6QSULFM
#define SPS_CPP_F6QSULFM

#include "BitStream.hpp"
#include "Common.hpp"
#include "Type.hpp"
#include <cstdint>

#define Extended_SAR 255
#define MAX_SPS_COUNT 32
#define MAX_OFFSET_REF_FRAME_COUNT 256

class SPS {
 public:
  int extractParameters(BitStream &bitStream);

 public:
  /* 表示编码配置文件 */
  uint8_t profile_idc = 0; // 0x64
  /* 表示编码级别 */
  uint8_t level_idc = 0; // 0
  /* SPS 的唯一标识符[0-31] */
  uint32_t seq_parameter_set_id = 0;
  /* 指示是否使用约束基线配置文件 */

  /*色度格式[0-3]。指定了亮度和色度分量的采样方式,0:none,1:420,2:422,3:444*/
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
  /* 用于指定图像顺序计数(POC)的方法：
   * 0:使用POC高低位（如果SPS中传递的POC低位，一般是最优先使用的情况）来计算POC(图像顺序计数)( 适用于大多数情况，特别是需要精确控制POC的场景)：以x264为例：
       * 当启用 B 帧时会使用
       * 当未启用 B 帧时可能使用，例如使用较高 profile 或需要支持特定功能时。
       * 当medium速度时，使用
       * 默认使用
   * 1:使用增量计数来计算POC(复杂，如B帧和P帧混合的情况)，以x264为例：
       * 几乎不使用，主要用于理论研究或特殊应用，实际编码中很少使用。
   * 2:使用帧号来计算POC(简单，如仅有I帧和P帧的情况)，以x264为例：
      * 当完全禁用 B 帧 (--bframes 0) 并且不需要支持乱序解码时使用 */
  uint32_t pic_order_cnt_type = 0;

  /* 表示解码器需要支持的最大参考帧数 */
  uint32_t max_num_ref_frames = 0;
  /* 宏块（MB）单位的图像宽度减 1。（用于计算原图像正常情况下的宽）*/
  uint32_t pic_width_in_mbs_minus1 = 0;
  /* 宏块（MB）单位的图像高度减 1。（用于计算原图像正常情况下的高）*/
  uint32_t pic_height_in_map_units_minus1 = 0;

  uint32_t MaxFrameNum = 0;
  /* 指示在宏块的直接模式（主要是指B帧，如B_Direct) 下是否可以使用8x8变换：
   * 1: 可以直接从 8x8 分区推导运动矢量，不需要进一步划分为 4x4 子块
   * 2: 需要将 8x8 分区进一步划分为 4x4 子块，并为每个 4x4 子块单独推导运动矢量 */
  bool direct_8x8_inference_flag = 0;

  /* 等于 1 指定 4:4:4 色度格式的三个颜色分量分别编码（如果为1,说明一定是YUV444);
   * 等于 0 指定颜色分量不单独编码（如果为0,YUV400,YUV420,YUV422,YUB444均有可能）;
   * 当separate_colour_plane_flag不存在时，应推断其等于0。
   * 等于1时，主编码图像由三个单独的分量组成，每个分量由一个颜色平面（Y、Cb或Cr）的编码样本组成。 ），每个都使用单色编码语法。在这种情况下，每个颜色平面都与特定的 color_plane_id 值相关联。 */
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

  // 宏块单位的图像宽度 = pic_width_in_mbs_minus1 + 1 (7-13)
  int32_t PicWidthInMbs = 0;
  // 宏块单位的图像高度 = pic_height_in_map_units_minus1 + 1
  int32_t PicHeightInMapUnits = 0;
  // 宏块单位的图像大小 = 宽 * 高
  uint32_t PicSizeInMapUnits = 0;
  uint32_t FrameHeightInMbs = 0;

  /* 是否对亮度分量中的零系数块应用变换旁路 */
  bool qpprime_y_zero_transform_bypass_flag = 0;

  /* 是否在存在缩放(量化)矩阵 */
  bool seq_scaling_matrix_present_flag = 0;

  /* 等于 1 指定缩放列表 i 的语法结构存在于序列参数集中。 seq_scaling_list_present_flag[ i ] 等于 0 指定缩放列表 i 的语法结构不存在于序列参数集中，并且表 7-2 中指定的缩放列表回退规则集 A 将用于推断序列级缩放索引 i 的列表。 */
  bool seq_scaling_list_present_flag[12] = {false};

  /*  色度宏块宽，高：
      - YUV444: 16x16像素;
      - YUV422: 8x16像素，即宽度是亮度的一半，高度相同;
      - YUV420: 8x8像素，即水平和垂直采样率都是亮度分量的一半;*/
  uint32_t MbWidthC = 0;
  uint32_t MbHeightC = 0;

  int32_t pcm_sample_chroma[256]; // 3 u(v)

  /* 用于计算非参考帧帧顺序计数偏移量的值 */
  int32_t offset_for_non_ref_pic = 0;
  /* 用于计算场顺序计数偏移量的值 */
  int32_t offset_for_top_to_bottom_field = 0;
  /* 帧顺序计数循环中参考帧的数量 */
  uint32_t num_ref_frames_in_pic_order_cnt_cycle = 0;

  int32_t Chroma_Format = 0;

  /* SubWidthC 和 SubHeightC 表示色度分量的下采样因子 */
  int32_t SubWidthC = 0;
  int32_t SubHeightC = 0;

  uint32_t MaxPicOrderCntLsb;
  int32_t ExpectedDeltaPerPicOrderCntCycle;

  /* 用于计算参考帧帧顺序计数偏移量的值，调整参考帧的显示顺序，以确保解码后的视频帧以正确的顺序显示*/
  int32_t offset_for_ref_frame[MAX_OFFSET_REF_FRAME_COUNT];
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
 public:
  uint8_t bit_rate_scale = 0;
  uint8_t cpb_size_scale = 0;
  uint32_t *bit_rate_value_minus1 = nullptr;
  uint32_t *cpb_size_value_minus1 = nullptr;
  bool *cbr_flag = nullptr;
  uint8_t initial_cpb_removal_delay_length_minus1 = 0;
  uint8_t cpb_removal_delay_length_minus1 = 0;
  uint8_t dpb_output_delay_length_minus1 = 0;
  uint32_t cpb_cnt_minus1 = 0;
  uint8_t time_offset_length = 0;

  /* VUI */
 public:
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
  /* 指示颜色原色的类型（如BT.709、BT.601,取值和含义是根据ITU-T H.273,范围为[0,255]）:
1.  BT.709
2.  未指定（图像特性和颜色空间未定义）
4.  BT.470 System M (NTSC)
5.  BT.470 System B, G (PAL & SECAM)
6.  BT.601-6 625
7.  BT.601-6 525
8.  SMPTE 240M
9.  Generic film (色彩处理用于电影)
10. BT.2020 Non-constant luminance
11. BT.2020 Constant luminance
12. SMPTE ST 2085
13. Chromaticity-derived non-constant luminance
14. Chromaticity-derived constant luminance
15. BT.2100
16. SMPTE ST 428
17. Adobe RGB
18. SMPTE RP 431
19. SMPTE EG 432
20. EBU Tech. 3213-E
21. SMPTE ST 431-2
22. SMPTE ST 432-1
23. EOTF SMPTE ST 2084 for 10, 12, 14, and 16 bit systems
24. EOTF ARIB STD-B67 (HLG) * */
  uint8_t colour_primaries = 0;
  /* 指示传输特性（如线性、伽马等） */
  uint8_t transfer_characteristics = 0;
  /* 指示矩阵系数，指定了颜色空间转换(RGB->YCbCr)的具体矩阵系数:  NOTE:如果一个视频使用BT.709的 colour_primaries，通常也应该使用BT.709的 matrix_coefficients 来确保色彩在编码过程中正确转换。
0. 矩阵系数是根据色彩原色和白点的特性推导的（Identity matrix，适用于RGB）
1. BT.709
2. 未指定
4. FCC
5. BT.470 System B, G
6. BT.601
7. SMPTE 240M
8. YCgCo
9. BT.2020 非恒定亮度
10. BT.2020 恒定亮度
14. BT.2100 ICtCp*/
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
  /* 最大重排序帧数 */
  int32_t max_num_reorder_frames = -1;
  /* 指示是否存在VCL HRD参数 */
  bool vcl_hrd_parameters_present_flag = 0;
  /* 指示是否存在图像结构信息 */
  bool pic_struct_present_flag = 0;
  /* 指示是否使用低延迟HRD */
  bool low_delay_hrd_flag = 0;
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
  /* 最大解码帧缓冲区大小 */
  uint32_t max_dec_frame_buffering = 0;

 private:
  void vui_parameters(BitStream &bitStream);
  void hrd_parameters(BitStream &bitStream);
  int seq_parameter_set_extension_rbsp();
  int derived_SubWidthC_and_SubHeightC();
};

//----------------------------------------------------------------------
//vui_parameters()
const int32_t LevelNumber_MaxDpbMbs[19][2] = {
    {10, 396},    {11, 900},    {12, 2376},   {13, 2376},   {20, 2376},
    {21, 4752},   {22, 8100},   {30, 8100},   {31, 18000},  {32, 20480},
    {40, 32768},  {41, 32768},  {42, 34816},  {50, 110400}, {51, 184320},
    {52, 184320}, {60, 696320}, {61, 696320}, {62, 696320},
};
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//Table 6-1 – SubWidthC, and SubHeightC values derived from chroma_format_idc and separate_colour_plane_flag
struct Chroma_format_idc {
  int32_t chroma_format_idc;
  int32_t separate_colour_plane_flag;
  int32_t Chroma_Format;
  int32_t SubWidthC;
  int32_t SubHeightC;
};

#define MONOCHROME 0 // 黑白图像
#define CHROMA_FORMAT_IDC_420 1
#define CHROMA_FORMAT_IDC_422 2
#define CHROMA_FORMAT_IDC_444 3

const Chroma_format_idc chroma_format_idcs[5] = {
    {0, 0, MONOCHROME, NA, NA},
    {1, 0, CHROMA_FORMAT_IDC_420, 2, 2},
    {2, 0, CHROMA_FORMAT_IDC_422, 2, 1},
    {3, 0, CHROMA_FORMAT_IDC_444, 1, 1},
    {3, 1, CHROMA_FORMAT_IDC_444, NA, NA},
};
//----------------------------------------------------------------------

// -------------------------------- H264中规定的默认量化表 --------------------------------
// Table 7-3 – Specification of default scaling lists Default_4x4_Intra and Default_4x4_Inter
/* For 4x4帧内预测 */
const uint32_t Default_4x4_Intra[16] = {6,  13, 13, 20, 20, 20, 28, 28,
                                        28, 28, 32, 32, 32, 37, 37, 42};
/* For 4x4帧间预测 */
const uint32_t Default_4x4_Inter[16] = {10, 14, 14, 20, 20, 20, 24, 24,
                                        24, 24, 27, 27, 27, 30, 30, 34};

/* For 8x8帧内预测 */
const uint32_t Default_8x8_Intra[64] = {
    6,  10, 10, 13, 11, 13, 16, 16, 16, 16, 18, 18, 18, 18, 18, 23,
    23, 23, 23, 23, 23, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27,
    27, 27, 27, 27, 29, 29, 29, 29, 29, 29, 29, 31, 31, 31, 31, 31,
    31, 33, 33, 33, 33, 33, 36, 36, 36, 36, 38, 38, 38, 40, 40, 42};
/* For 8x8帧间预测 */
const uint32_t Default_8x8_Inter[64] = {
    9,  13, 13, 15, 13, 15, 17, 17, 17, 17, 19, 19, 19, 19, 19, 21,
    21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 24, 24, 24, 24,
    24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27, 27,
    27, 28, 28, 28, 28, 28, 30, 30, 30, 30, 32, 32, 32, 33, 33, 35};

#endif /* end of include guard: SPS_CPP_F6QSULFM */
