#ifndef MACROBLOCK_HPP_FBNXLFQV
#define MACROBLOCK_HPP_FBNXLFQV
#include "BitStream.hpp"
#include "H264SliceData.hpp"
#include "Type.hpp"

class PictureBase;
class SliceBody;

class MacroBlock {
 public:
  /* 宏块类型，指示宏块的编码模式（如帧内、帧间、PCM等） */
  int32_t mb_type;
  /* PCM对齐零位，用于对齐PCM数据。 */
  int32_t pcm_alignment_zero_bit;
  /* PCM模式下的亮度样本数据，大小为16x16（256个样本） */
  int32_t pcm_sample_luma[256];
  /* PCM模式下的色度样本数据，大小为16x16（256个样本） */
  int32_t pcm_sample_chroma[256];
  /* 变换大小标志，指示是否使用8x8变换 */
  int32_t transform_size_8x8_flag;
  /* 编码块模式，指示哪些块包含非零系数 */
  int32_t coded_block_pattern;
  /* 宏块量化参数的变化值 */
  int32_t mb_qp_delta;

  // mb_pred
  /* 前一个4x4帧内预测模式标志 */
  int32_t prev_intra4x4_pred_mode_flag[16];
  /* 剩余的4x4帧内预测模式 */
  int32_t rem_intra4x4_pred_mode[16];
  /* 前一个8x8帧内预测模式标志 */
  int32_t prev_intra8x8_pred_mode_flag[4];
  /* 剩余的8x8帧内预测模式 */
  int32_t rem_intra8x8_pred_mode[4];
  /* 帧内色度预测模式 */
  int32_t intra_chroma_pred_mode;
  /* 参考帧索引（列表0） */
  int32_t ref_idx_l0[4];
  /* 参考帧索引（列表1） */
  int32_t ref_idx_l1[4];
  /* 运动矢量差（列表0） */
  int32_t mvd_l0[4][4][2];
  /* 运动矢量差（列表1） */
  int32_t mvd_l1[4][4][2];

  // sub_mb_pred
  /* 子宏块类型，指示子宏块的编码模式 */
  int32_t sub_mb_type[4];

  /* 色度编码块模式，指示哪些色度块包含非零系数 */
  int32_t CodedBlockPatternChroma;
  /* 亮度编码块模式，指示哪些亮度块包含非零系数 */
  int32_t CodedBlockPatternLuma;

  /* QPY: 当前宏块的亮度量化参数。
    QSY: 当前宏块的亮度量化参数（备用）。
    QP1Y: 前一个宏块的亮度量化参数。 */
  int32_t QPY;
  int32_t QSY;
  int32_t QP1Y;

  /* QPCb: 当前宏块的色度Cb量化参数。
    QP1Cb: 前一个宏块的色度Cb量化参数。 */
  int32_t QPCb;
  int32_t QP1Cb;

  /* QPCr: 当前宏块的色度Cr量化参数。
    QP1Cr: 前一个宏块的色度Cr量化参数。 */
  int32_t QPCr;
  int32_t QP1Cr;

  /* QSCb: 当前宏块的色度Cb量化参数（备用）。
    QS1Cb: 前一个宏块的色度Cb量化参数（备用）。 */
  int32_t QSCb;
  int32_t QS1Cb;

  /* QSCr: 当前宏块的色度Cr量化参数（备用）。
    QS1Cr: 前一个宏块的色度Cr量化参数（备用）。 */
  int32_t QSCr;
  int32_t QS1Cr;

  /* 变换绕过模式标志，指示是否绕过变换过程 */
  int32_t TransformBypassModeFlag;

  /* DCT变换后的直,交流系数 */
  int32_t i16x16DClevel[16];
  int32_t i16x16AClevel[16][16];

  /* 4x4/8x8 块的变换系数 */
  int32_t level4x4[16][16];
  int32_t level8x8[4][64];

  /* 4x4/8x8 亮,色度块的变换系数 */
  int32_t LumaLevel4x4[16][16];
  int32_t LumaLevel8x8[4][64];
  int32_t CbLevel4x4[16][16];
  int32_t CbLevel8x8[4][64];
  int32_t CrLevel4x4[16][16];
  int32_t CrLevel8x8[4][64];

  /* 16x16帧内预测模式下的亮,色度DC,AC系数。 */
  int32_t Intra16x16DCLevel[16];
  int32_t Intra16x16ACLevel[16][16];
  int32_t CbIntra16x16DCLevel[16];
  int32_t CbIntra16x16ACLevel[16][16];
  int32_t CrIntra16x16DCLevel[16];
  int32_t CrIntra16x16ACLevel[16][16];

  /* 色度DC,AC系数，分别为Cb和Cr分量 */
  int32_t ChromaDCLevel[2][16];
  int32_t ChromaACLevel[2][16][16];

  /* 存储当前亮度宏块的16个4x4子宏块非零系数，范围[0,16] */
  uint8_t mb_luma_4x4_non_zero_count_coeff[16];
  /* 存储当前两个色度宏块的16个4x4子宏块非零系数，范围[0,16] */
  uint8_t mb_chroma_4x4_non_zero_count_coeff[2][16];
  /* 存储当前亮度宏块的4个8x8子宏块非零系数，范围[0,64]  */
  uint8_t mb_luma_8x8_non_zero_count_coeff[4];
  /* 存储当前宏块的16个4x4子宏块预测模式的值，范围[0,8] */
  uint8_t Intra4x4PredMode[16];
  /* 存储当前宏块的4个8x8子宏块预测模式的值，范围[0,8] */
  uint8_t Intra8x8PredMode[4];
  /* 存储当前宏块的1个16x16宏块预测模式的值，范围[0,4] */
  int32_t Intra16x16PredMode;

  /* 场图像标志，指示当前图像是场图像还是帧图像 */
  int32_t field_pic_flag;
  /* 底场标志，指示当前场是底场还是顶场 */
  int32_t bottom_field_flag;
  /* 宏块跳过标志，指示当前宏块是否被跳过 */
  int32_t mb_skip_flag;
  /* 宏块场解码标志，指示当前宏块是否使用场解码模式 */
  int32_t mb_field_decoding_flag;
  /* 宏块自适应帧场编码标志，指示是否使用宏块自适应帧场编码 */
  int32_t MbaffFrameFlag;
  /* 受限帧内预测标志，指示是否使用受限帧内预测 */
  int32_t constrained_intra_pred_flag;
  /* 去块效应滤波器禁用标志，指示是否禁用去块效应滤波器。 */
  int32_t disable_deblocking_filter_idc;
  /* 当前正在处理的宏块 */
  int32_t CurrMbAddr;

  /* 指示当前宏块所属的片的ID */
  int32_t slice_id;
  /* 当前片的类型（如I片、P片、B片等） */
  int32_t m_slice_type;
  /* 指示当前片在序列中的编号 */
  int32_t slice_number;

  /* 宏块部分宽、高度，指示宏块部分的宽度 */
  int32_t MbPartWidth;
  int32_t MbPartHeight;

  /* 宏块部分数量，指示当前宏块被划分成的部分数量 */
  int32_t m_NumMbPart;

  /* 子宏块部分数量，指示每个子宏块被划分成的部分数量*/
  int32_t NumSubMbPart[4];

  /* 子宏块部分宽，高，指示每个子宏块部分的宽，高度*/
  int32_t SubMbPartWidth[4];
  int32_t SubMbPartHeight[4];

  /* 滤波器偏移A,B，用于去块效应滤波器的参数 */
  int32_t FilterOffsetA;
  int32_t FilterOffsetB;

  // 3个bit位表示CABAC残差相应4x4子宏块中的DC直流系数block的coded_block_flag值，(b7,...,b2,b1,b0)=(x,...,cr,cb,luma)
  uint8_t coded_block_flag_DC_pattern;
  // 16个bit位表示CABAC残差相应4x4子宏块中的AC交流系数block的coded_block_flag值(0或1)(全部默认为1)，[0]-luma,[1]-cb,[2]-cr
  uint16_t coded_block_flag_AC_pattern[3];

  // 本宏块的左上角像素，相对于整张图片左上角像素的x坐标
  int32_t m_mb_position_x;
  // 本宏块的左上角像素，相对于整张图片左上角像素的y坐标
  int32_t m_mb_position_y;

  MB_TYPE_I_SLICES_T mb_type_I_slice;
  MB_TYPE_SI_SLICES_T mb_type_SI_slice;
  MB_TYPE_P_SP_SLICES_T mb_type_P_SP_slice;
  MB_TYPE_B_SLICES_T mb_type_B_slice;
  SUB_MB_TYPE_P_MBS_T sub_mb_type_P_slice[4];
  SUB_MB_TYPE_B_MBS_T sub_mb_type_B_slice[4];

  int32_t m_slice_type_fixed;
  int32_t
      m_mb_type_fixed; // 码流解码出来的原始mb_type值，需要修正一次才行，原因是有的P帧里面含有帧内编码的I宏块
  H264_MB_TYPE m_name_of_mb_type;
  H264_MB_PART_PRED_MODE m_mb_pred_mode;
  H264_MB_TYPE m_name_of_sub_mb_type[4];
  H264_MB_PART_PRED_MODE m_sub_mb_pred_mode[4];

  int32_t m_MvL0[4][4][2];
  int32_t m_MvL1[4][4][2];
  int32_t m_RefIdxL0[4];
  int32_t m_RefIdxL1[4];
  int32_t m_PredFlagL0[4];
  int32_t m_PredFlagL1[4];
  uint8_t m_isDecoded[4][4];

 public:
  MacroBlock();
  ~MacroBlock();

  int printInfo();

  static std::string getNameOfMbTypeStr(H264_MB_TYPE name_of_mb_type);

  static int fix_mb_type(const int32_t slice_type_raw,
                         const int32_t mb_type_raw, int32_t &slice_type_fixed,
                         int32_t &mb_type_fixed);

  static int getMbPartWidthAndHeight(H264_MB_TYPE name_of_mb_type,
                                     int32_t &_MbPartWidth,
                                     int32_t &_MbPartHeight);
  static int MbPartPredMode(int32_t slice_type, int32_t transform_size_8x8_flag,
                            int32_t _mb_type, int32_t index, int32_t &NumMbPart,
                            int32_t &CodedBlockPatternChroma,
                            int32_t &CodedBlockPatternLuma,
                            int32_t &_Intra16x16PredMode,
                            H264_MB_TYPE &name_of_mb_type,
                            H264_MB_PART_PRED_MODE &mb_pred_mode);
  static int MbPartPredMode2(H264_MB_TYPE name_of_mb_type, int32_t mbPartIdx,
                             int32_t transform_size_8x8_flag,
                             H264_MB_PART_PRED_MODE &mb_pred_mode);
  static int SubMbPredModeFunc(int32_t slice_type, int32_t sub_mb_type,
                               int32_t &NumSubMbPart,
                               H264_MB_PART_PRED_MODE &SubMbPredMode,
                               int32_t &SubMbPartWidth,
                               int32_t &SubMbPartHeight);

  inline int set_mb_type_X_slice_info();

  int macroblock_layer(BitStream &bs, PictureBase &picture,
                       const SliceBody &slice_data, CH264Cabac &cabac);
  int macroblock_layer_mb_skip(PictureBase &picture,
                               const SliceBody &slice_data, CH264Cabac &cabac);
  int mb_pred(BitStream &bs, PictureBase &picture, const SliceBody &slice_data,
              CH264Cabac &cabac);
  int sub_mb_pred(BitStream &bs, PictureBase &picture,
                  const SliceBody &slice_data, CH264Cabac &cabac);

  int residual(BitStream &bs, PictureBase &picture, int32_t startIdx,
               int32_t endIdx, CH264Cabac &cabac);

  int residual_luma(BitStream &bs, PictureBase &picture,
                    int32_t (&i16x16DClevel)[16],
                    int32_t (&i16x16AClevel)[16][16],
                    int32_t (&level4x4)[16][16], int32_t (&level8x8)[4][64],
                    int32_t startIdx, int32_t endIdx,
                    MB_RESIDUAL_LEVEL mb_residual_level_dc,
                    MB_RESIDUAL_LEVEL mb_residual_level_ac, CH264Cabac &cabac);
};

#endif /* end of include guard: MACROBLOCK_HPP_FBNXLFQV */
