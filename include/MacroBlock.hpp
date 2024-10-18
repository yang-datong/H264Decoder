#ifndef MACROBLOCK_HPP_FBNXLFQV
#define MACROBLOCK_HPP_FBNXLFQV
#include "BitStream.hpp"
#include "Cabac.hpp"
#include "SliceHeader.hpp"
#include "Type.hpp"
#include <cstdint>

class Cavlc;
class PictureBase;
class SliceData;

class MacroBlock {
 public:
  MacroBlock(){};
  ~MacroBlock();
  /* 宏块类型，指示宏块的编码模式（如帧内、帧间、PCM等），取决于切片类型 */
  /* 对于I Slice: mb_type取值区间为：[0-25]
    * 对于P Slice: mb_type取值区间为：[0-4] + inferred (P_Skip)
    * 对于B Slice: mb_type取值区间为：[0-22] + inferred (B_Skip)
    * 对于SI Slice: mb_type取值区间为：[0]
    * 对于SP Slice: mb_type取值区间为：同P Slice */

  /* 对应的宏块名称，见m_name_of_mb_type（考虑字节对齐，不放在这里），一般来说特殊类型的宏块都是对应类型宏块的最后一个类型：
      * 对于I宏块来说，存在一个特殊宏块类型，mb_type = 25， 称为I_PCM宏块,I宏块只有16x16大小，但可以使用4x4,8x8进行宏块预测。
      * 对于P宏块来说，存在一个特殊宏块类型，mb_type = 5， 称为P_Skip宏块。
      * 对于B宏块来说，存在一个特殊宏块类型，mb_type = 23， 称为B_Skip宏块。
    * */
  int32_t mb_type = 0;

  /* PCM对齐零位，用于对齐PCM数据。 */
  int32_t pcm_alignment_zero_bit = 0;
  /* PCM模式下的亮度样本数据，大小为16x16（256个样本） */
  int32_t pcm_sample_luma[256] = {0};
  /* PCM模式下的色度样本数据，大小为16x16（256个样本） */
  int32_t pcm_sample_chroma[256] = {0};
  /* 变换大小标志，指示是否使用8x8变换 */
  int32_t transform_size_8x8_flag = 0;
  /* 编码块模式，指示哪些块包含非零系数 */
  int32_t coded_block_pattern = 0;
  /* 宏块量化参数的变化值 */
  int32_t mb_qp_delta = 0;

  // mb_pred
  /* 前一个4x4帧内预测模式标志 */
  int32_t prev_intra4x4_pred_mode_flag[16] = {0};
  /* 剩余的4x4帧内预测模式（由编码器提供，通过解码得到） */
  int32_t rem_intra4x4_pred_mode[16] = {0};
  /* 前一个8x8帧内预测模式标志 */
  int32_t prev_intra8x8_pred_mode_flag[4] = {0};
  /* 剩余的8x8帧内预测模式 */
  int32_t rem_intra8x8_pred_mode[4] = {0};
  /* 帧内色度预测模式: 0: DC, 1: Horizontal, 2: Vertical, 3: Plane*/
  int32_t intra_chroma_pred_mode = 0;
  /* 参考帧索引（列表0） */
  int32_t ref_idx_l0[4] = {0};
  /* 参考帧索引（列表1） */
  int32_t ref_idx_l1[4] = {0};
  /* 运动矢量差（列表0） */
  int32_t mvd_l0[4][4][2] = {{{0}}};
  /* 运动矢量差（列表1） */
  int32_t mvd_l1[4][4][2] = {{{0}}};

  // sub_mb_pred
  /* 子宏块类型，指示子宏块的编码模式 */
  int32_t sub_mb_type[4] = {0};

  /* 亮度编码块模式，指示哪些亮度块包含非零系数，亮度块模式的值范围是0到15，即2^4 - 1 = 15，表示为4个bits位：
   * 比如 0b1010（即二进制的 1010），从低位往高位数，这意味着第 2 和第 4 个 8x8 块包含亮度残差数据，而第 1 和第 3 个 8x8 块没有亮度残差数据，在后面可以看到 if (CodedBlockPatternLuma & (1 << i8x8)) 这样的判断就是在做这件事情 */
  int32_t CodedBlockPatternLuma = -1;

  //Table 7-15 – Specification of CodedBlockPatternChroma values
  /* 色度编码块模式，指示哪些色度块包含非零系数，色度块模式的值范围是0到2：
   * 0: 所有色度变换系数级别都等于 0。
   * 1: 一个或多个色度 DC 变换系数级别应为非零值。所有色度 AC 变换系数级别都等于 0。
   * 2: 零个或多个色度 DC 变换系数级别是非零值。一个或多个色度 AC 变换系数级别应为非零值。*/
  int32_t CodedBlockPatternChroma = -1;

  /* QPY: 当前宏块的亮度量化参数。
    QSY: 当前宏块的亮度量化参数（场景切换状态下的量化值）。
    QP1Y: 加上QpBdOffset后的QPY */
  int32_t QPY = 0;
  int32_t QSY = 0;
  int32_t QP1Y = 0;

  /* QPCb: 当前宏块的色度Cb量化参数。
    QP1Cb: 前一个宏块的色度Cb量化参数。 */
  int32_t QPCb = 0;
  int32_t QP1Cb = 0;

  /* QPCr: 当前宏块的色度Cr量化参数。
    QP1Cr: 前一个宏块的色度Cr量化参数。 */
  int32_t QPCr = 0;
  int32_t QP1Cr = 0;

  /* QSCb: 当前宏块的色度Cb量化参数（备用）。
    QS1Cb: 前一个宏块的色度Cb量化参数（备用）。 */
  int32_t QSCb = 0;
  int32_t QS1Cb = 0;

  /* QSCr: 当前宏块的色度Cr量化参数（备用）。
    QS1Cr: 前一个宏块的色度Cr量化参数（备用）。 */
  int32_t QSCr = 0;
  int32_t QS1Cr = 0;

  /* 变换绕过模式标志，指示是否绕过变换过程 */
  int32_t TransformBypassModeFlag = 0;

  /* DCT变换后的直,交流系数 */
  int32_t i16x16DClevel[16] = {0};
  int32_t i16x16AClevel[16][16] = {{0}};

  /* 4x4/8x8 块的变换系数 */
  int32_t level4x4[16][16] = {{0}};
  int32_t level8x8[4][64] = {{0}};

  /* 4x4/8x8 亮,色度块残差值 */
  int32_t LumaLevel4x4[16][16] = {{0}};
  int32_t LumaLevel8x8[4][64] = {{0}};
  int32_t CbLevel4x4[16][16] = {{0}};
  int32_t CbLevel8x8[4][64] = {{0}};
  int32_t CrLevel4x4[16][16] = {{0}};
  int32_t CrLevel8x8[4][64] = {{0}};

  /* 16x16帧内预测模式下的亮,色度DC,AC系数。 */
  int32_t Intra16x16DCLevel[16] = {0};
  int32_t Intra16x16ACLevel[16][16] = {{0}};
  int32_t CbIntra16x16DCLevel[16] = {0};
  int32_t CbIntra16x16ACLevel[16][16] = {{0}};
  int32_t CrIntra16x16DCLevel[16] = {0};
  int32_t CrIntra16x16ACLevel[16][16] = {{0}};

  /* 色度DC,AC系数，分别为Cb和Cr分量 */
  int32_t ChromaDCLevel[2][16] = {{0}};
  int32_t ChromaACLevel[2][16][16] = {{{0}}};

  /* 存储当前亮度宏块的16个4x4子宏块非零系数，范围[0,16] */
  uint8_t mb_luma_4x4_non_zero_count_coeff[16] = {0};
  /* 存储当前两个色度宏块的16个4x4子宏块非零系数，范围[0,16] */
  uint8_t mb_chroma_4x4_non_zero_count_coeff[2][16] = {{0}};
  /* 存储当前亮度宏块的4个8x8子宏块非零系数，范围[0,64]  */
  uint8_t mb_luma_8x8_non_zero_count_coeff[4] = {0};
  /* 存储当前宏块的16个4x4子宏块预测模式的值，范围[0,8] */
  uint8_t Intra4x4PredMode[16] = {0};
  /* 存储当前宏块的4个8x8子宏块预测模式的值，范围[0,8] */
  uint8_t Intra8x8PredMode[4] = {0};
  /* 存储当前宏块的1个16x16宏块预测模式的值，范围[0,4] */
  int32_t Intra16x16PredMode = 0;

  /* 这里都是已有的变量，不应该作为一个类成员，后续需要删除 TODO  <24-09-01 01:28:00, YangJing>  */
  /* 场图像标志，指示当前图像是场图像还是帧图像 */
  int32_t field_pic_flag;
  /* 底场标志，指示当前场是底场还是顶场 */
  int32_t bottom_field_flag;
  /* 宏块跳过标志，指示当前宏块是否被跳过 */
  int32_t mb_skip_flag;
  /* 宏块场解码标志，指示当前宏块是否使用场解码模式
    Mb_field_decoding_flag:0 -> 帧宏块，宏块对中两个宏块合并处理。
    Mb_field_decoding_flag:1 -> 场宏块，某些部分需要场间隔解码，宏块对中两个宏块单独处理。
   */
  int32_t mb_field_decoding_flag;
  /* 宏块自适应帧场编码标志，指示是否使用宏块自适应帧场编码 */
  int32_t MbaffFrameFlag;

  /* 当前正在处理的宏块 */
  int32_t CurrMbAddr;
  /* 指示当前宏块所属的片的ID */
  //int32_t slice_id;
  /* 指示当前片在序列中的编号 */
  uint32_t slice_number;
  /* 当前片的类型（如I片、P片、B片等） */
  int32_t m_slice_type;

  /* 滤波器，见Slice Header */
  int32_t disable_deblocking_filter_idc;
  int32_t FilterOffsetA;
  int32_t FilterOffsetB;
  //是否允许使用邻近样本作为当前样本预测
  int32_t constrained_intra_pred_flag;

  /* 宏块部分宽、高度，指示宏块部分的宽度 */
  int32_t MbPartWidth = 0;
  int32_t MbPartHeight = 0;

  /* 宏块部分数量，指示当前宏块被划分成的部分数量 */
  int32_t m_NumMbPart = 0;

  /* 子宏块部分数量，指示每个子宏块被划分成的部分数量*/
  int32_t NumSubMbPart[4] = {0};

  /* 子宏块部分宽，高，指示每个子宏块部分的宽，高度*/
  int32_t SubMbPartWidth[4] = {0};
  int32_t SubMbPartHeight[4] = {0};

  // 3个bit位表示CABAC残差相应4x4子宏块中的DC直流系数block的coded_block_flag值，(b7,...,b2,b1,b0)=(x,...,cr,cb,luma)
  uint8_t coded_block_flag_DC_pattern = 7;
  // 16个bit位表示CABAC残差相应4x4子宏块中的AC交流系数block的coded_block_flag值(0或1)(全部默认为1)，[0]-luma,[1]-cb,[2]-cr
  uint16_t coded_block_flag_AC_pattern[3] = {0xFFFF, 0xFFFF, 0xFFFF};

  // 本宏块的左上角像素，相对于整张图片左上角像素的x坐标
  int32_t m_mb_position_x = -1;
  // 本宏块的左上角像素，相对于整张图片左上角像素的y坐标
  int32_t m_mb_position_y = -1;

  MB_TYPE_I_SLICES_T mb_type_I_slice;
  MB_TYPE_SI_SLICES_T mb_type_SI_slice;
  MB_TYPE_P_SP_SLICES_T mb_type_P_SP_slice;
  MB_TYPE_B_SLICES_T mb_type_B_slice;
  SUB_MB_TYPE_P_MBS_T sub_mb_type_P_slice[4];
  SUB_MB_TYPE_B_MBS_T sub_mb_type_B_slice[4];

  int32_t m_slice_type_fixed = -1;
  // 码流解码出来的原始mb_type值，需要修正一次才行，原因是有的P帧里面含有帧内编码的I宏块
  int32_t m_mb_type_fixed = -1;

  /* 对应宏块类型的名称 */
  H264_MB_TYPE m_name_of_mb_type;
  /* 对应宏块类型所使用的预测模式 */
  H264_MB_PART_PRED_MODE m_mb_pred_mode;
  H264_MB_TYPE m_name_of_sub_mb_type[4];
  H264_MB_PART_PRED_MODE m_sub_mb_pred_mode[4];

  int32_t m_MvL0[4][4][2] = {{{0}}};
  int32_t m_MvL1[4][4][2] = {{{0}}};
  int32_t m_RefIdxL0[4] = {0};
  int32_t m_RefIdxL1[4] = {0};
  int32_t m_PredFlagL0[4] = {0};
  int32_t m_PredFlagL1[4] = {0};
  bool m_isDecoded[4][4] = {{0}};

 private:
  /* 用于内部类中使用 */
  bool _is_cabac = 0;
  Cabac *_cabac = nullptr;
  BitStream *_bs = nullptr;
  Cavlc *_cavlc = nullptr;
  PictureBase *_pic = nullptr;
  MB_RESIDUAL_LEVEL _mb_residual_level_dc = MB_RESIDUAL_UNKOWN;
  MB_RESIDUAL_LEVEL _mb_residual_level_ac = MB_RESIDUAL_UNKOWN;

 public:
  int decode(BitStream &bs, PictureBase &picture, const SliceData &slice_data,
             Cabac &cabac);

  int decode_skip(PictureBase &picture, const SliceData &slice_data,
                  Cabac &cabac);

  static int getMbPartWidthAndHeight(H264_MB_TYPE name_of_mb_type,
                                     int32_t &_MbPartWidth,
                                     int32_t &_MbPartHeight);

  static int SubMbPredMode(int32_t slice_type, int32_t sub_mb_type,
                           int32_t &NumSubMbPart,
                           H264_MB_PART_PRED_MODE &SubMbPredMode,
                           int32_t &SubMbPartWidth, int32_t &SubMbPartHeight);

 private:
  int pcm_sample_copy(const SPS *sps);
  int process_mb_qp(SliceHeader *&header, int32_t min = 0, int32_t max = 0);
  int check_sub_mb_size(bool &noSubMbPartSizeLessThan8x8Flag,
                        bool direct_8x8_inference_flag);
  int fix_mb_type(int32_t slice_type_raw, int32_t mb_type_raw,
                  int32_t &slice_type_fixed, int32_t &mb_type_fixed);

 public:
  static int MbPartPredMode(H264_MB_TYPE name_of_mb_type, int32_t mbPartIdx,
                            int32_t transform_size_8x8_flag,
                            H264_MB_PART_PRED_MODE &mb_pred_mode);

 private:
  int MbPartPredMode();
  int MbPartPredMode(int32_t _mb_type, int32_t index,
                     H264_MB_TYPE &name_of_mb_type,
                     H264_MB_PART_PRED_MODE &mb_pred_mode);

  void initFromSlice(const SliceHeader &header, const SliceData &slice_data);

  int process_mb_type(const SliceHeader &header, int32_t slice_type);
  int process_sub_mb_type(const int mbPartIdx);
  int process_transform_size_8x8_flag(int32_t &transform_size_8x8_flag_temp);
  int process_coded_block_pattern(const uint32_t ChromaArrayType);
  int process_mb_qp_delta();
  int process_prev_intra4x4_pred_mode_flag(const int luma4x4BlkIdx);
  int process_rem_intra4x4_pred_mode(const int luma4x4BlkIdx);
  int process_prev_intra8x8_pred_mode_flag(const int luma8x8BlkIdx);
  int process_rem_intra8x8_pred_mode(const int luma8x8BlkIdx);
  int process_intra_chroma_pred_mode();
  int process_ref_idx_l0(int mbPartIdx, uint32_t num_ref_idx_l0_active_minus1);
  int process_ref_idx_l1(int mbPartIdx, uint32_t num_ref_idx_l1_active_minus1);
  int process_mvd_l0(const int mbPartIdx, const int compIdx,
                     int32_t subMbPartIdx = 0);
  int process_mvd_l1(const int mbPartIdx, const int compIdx,
                     int32_t subMbPartIdx = 0);

  int NumSubMbPartFunc(int mbPartIdx);

  int mb_pred(const SliceData &slice_data);
  int sub_mb_pred(const SliceData &slice_data);
  void set_current_mb_info(SUB_MB_TYPE_P_MBS_T type, int mbPartIdx);
  void set_current_mb_info(SUB_MB_TYPE_B_MBS_T type, int mbPartIdx);

  int residual(int32_t startIdx, int32_t endIdx);
  int residual_luma(int32_t startIdx, int32_t endIdx);
  int residual_chroma(int32_t startIdx, int32_t endIdx, int32_t NumC8x8);

  int residual_block_DC(int32_t coeffLevel[], int32_t startIdx, int32_t endIdx,
                        int32_t maxNumCoeff, int iCbCr, int32_t BlkIdx);
  int residual_block_AC(int32_t coeffLevel[], int32_t startIdx, int32_t endIdx,
                        int32_t maxNumCoeff, int iCbCr, int32_t BlkIdx);
  int residual_block2(int32_t coeffLevel[], int32_t startIdx, int32_t endIdx,
                      int32_t maxNumCoeff, MB_RESIDUAL_LEVEL mb_block_level,
                      int iCbCr, int32_t BlkIdx, int &TotalCoeff);
};

#endif /* end of include guard: MACROBLOCK_HPP_FBNXLFQV */
