#ifndef PICTUREBASE_HPP_ZGHBMJIH
#define PICTUREBASE_HPP_ZGHBMJIH
#include "Common.hpp"
#include "MacroBlock.hpp"
#include "Slice.hpp"
#include "SliceData.hpp"
#include "Type.hpp"
#include <vector>

class Frame;
class PictureBase {
 public:
  bool mmco_5_flag = false, mmco_6_flag = false;

  uint8_t *m_pic_buff_luma = nullptr;
  uint8_t *m_pic_buff_cb = nullptr;
  uint8_t *m_pic_buff_cr = nullptr;

  Slice *m_slice = nullptr;
  MacroBlock *m_mbs = nullptr;

  Frame *m_parent = nullptr;
  Frame *m_dpb[16] = {nullptr};
  Frame *m_RefPicList0[16] = {nullptr};
  Frame *m_RefPicList1[16] = {nullptr};

  int32_t m_pic_coded_width_pixels = 0, m_pic_coded_height_pixels = 0;

  int32_t PicWidthInMbs = 0, PicHeightInMbs = 0;
  int32_t PicSizeInMbs = 0;

  int32_t MbWidthL = 0, MbHeightL = 0;
  int32_t MbWidthC = 0, MbHeightC = 0;

  int32_t mb_x = 0, mb_y = 0;

  int32_t PicWidthInSamplesL = 0, PicWidthInSamplesC = 0;
  int32_t PicHeightInSamplesL = 0, PicHeightInSamplesC = 0;

  int32_t Chroma_Format = 0;

  int32_t m_slice_cnt = 0, mb_cnt = 0, CurrMbAddr = 0;

  int32_t TopFieldOrderCnt = 0, BottomFieldOrderCnt = 0;

  int32_t PicOrderCnt = 0;
  int32_t PicOrderCntMsb = 0, PicOrderCntLsb = 0;

  int32_t FrameNumOffset = 0, absFrameNum = 0;

  int32_t picOrderCntCycleCnt = 0, frameNumInPicOrderCntCycle = 0;
  int32_t expectedPicOrderCnt = 0;

  int32_t PicNum = 0;
  int32_t FieldNum = NA;
  int32_t m_PicNumCnt = 0;
  int32_t FrameNum = 0, FrameNumWrap = 0;
  int32_t LongTermFrameIdx = 0, LongTermPicNum = 0;
  int32_t MaxLongTermFrameIdx = NA;
  uint32_t m_RefPicList0Length = 0, m_RefPicList1Length = 0;

  int32_t m_is_malloc_mem_by_myself = 0; // 是否已经初始化

  int32_t LevelScale4x4[6][4][4] = {{{0}}};
  int32_t LevelScale8x8[6][8][8] = {{{0}}};

  PICTURE_MARKED_AS reference_marked_type = UNKOWN;
  H264_PICTURE_CODED_TYPE m_picture_coded_type = UNKNOWN;
  H264_SLICE_TYPE m_picture_type = SLICE_UNKNOWN;

 public:
  ~PictureBase();
  PictureBase &operator=(const PictureBase &src);
  int copyDataPicOrderCnt(const PictureBase &src);
  int getEmptyFrameFromDPB(Frame *&newEmptyPicture);
  int init(Slice *slice);

 private:
  int copyData(const PictureBase &src, bool isMallocAndCopyData);
  int copyData2(const PictureBase &src, int32_t copyMbsDataFlag);

  int reset();
  int unInit();
  int getOneFrameFromDPB(Frame *&pic);

  //================= 参考帧列表重排序 ========================
 private:
  int decoding_picture_order_count_type_0(const PictureBase *pic_previous_ref);
  int decoding_picture_order_count_type_1(const PictureBase *pic_previous);
  int decoding_picture_order_count_type_2(const PictureBase *pic_previous);
  int decoding_picture_numbers(Frame *(&dpb)[16]);
  int DiffPicOrderCnt(PictureBase *picA, PictureBase *picB);

 public:
  int picOrderCntFunc(PictureBase *picX);
  int decoding_picture_order_count(uint32_t pic_order_cnt_type);
  int decoding_ref_picture_lists_construction(Frame *(&dpb)[16],
                                              Frame *(&RefPicList0)[16],
                                              Frame *(&RefPicList1)[16]);

  //----------------- 构造参考帧列表 ------------------------
 private:
  int init_ref_picture_lists(Frame *(&dpb)[16], Frame *(&RefPicList0)[16],
                             Frame *(&RefPicList1)[16]);
  int init_ref_picture_list_P_SP_in_frames(Frame *(&dpb)[16],
                                           Frame *(&RefPicList0)[16],
                                           uint32_t &RefPicList0Length);
  int init_ref_picture_list_P_SP_in_fields(Frame *(&dpb)[16],
                                           Frame *(&RefPicList0)[16],
                                           uint32_t &RefPicList0Length);
  int init_ref_picture_lists_B_in_frames(Frame *(&dpb)[16],
                                         Frame *(&RefPicList0)[16],
                                         Frame *(&RefPicList1)[16],
                                         uint32_t &RefPicList0Length,
                                         uint32_t &RefPicList1Length);
  int init_ref_picture_lists_B_in_fields(Frame *(&dpb)[16],
                                         Frame *(&RefPicList0)[16],
                                         Frame *(&RefPicList1)[16],
                                         uint32_t &RefPicList0Length,
                                         uint32_t &RefPicList1Length);
  int init_ref_picture_lists_in_fields(vector<Frame *>(&refFrameListXShortTerm),
                                       vector<Frame *>(&refFrameListXLongTerm),
                                       Frame *(&RefPicListX)[16],
                                       uint32_t &RefPicListXLength,
                                       int32_t listX);

  //----------------- 修改参考帧列表 ------------------------
 private:
  int modif_ref_picture_lists(Frame *(&RefPicList0)[16],
                              Frame *(&RefPicList1)[16]);

  int modif_ref_picture_lists_for_short_ref_pictures(
      int32_t &refIdxLX, int32_t &picNumLXPred, int32_t modif_idc,
      int32_t abs_diff_pic_num_minus1, int32_t num_ref_idx_lX_active_minus1,
      Frame *(&RefPicListX)[16]);

  int modif_ref_picture_lists_for_long_ref_pictures(
      int32_t &refIdxLX, int32_t num_ref_idx_lX_active_minus1,
      int32_t long_term_pic_num, Frame *(&RefPicListX)[16]);

  int decoded_reference_picture_marking(Frame *(&dpb)[16]);
  int sliding_window_decoded_reference_picture_marking(Frame *(&dpb)[16]);
  int adaptive_memory_control_decoded_reference_picture_marking(
      Frame *(&dpb)[16]);

  //================= 帧内预测 ========================
 private:
  int transform_decoding_for_chroma_samples_with_YUV444(
      int32_t isChromaCb, int32_t PicWidthInSamples, uint8_t *pic_buff);
  int transform_decoding_for_chroma_samples_with_YUV420_or_YUV422(
      int32_t isChromaCb, int32_t PicWidthInSamples, uint8_t *pic_buff,
      bool isNeedIntraPrediction = true);
  int transform_decoding_for_residual_4x4_blocks(int32_t d[4][4],
                                                 int32_t (&r)[4][4]);
  int transform_decoding_for_residual_8x8_blocks(int32_t d[8][8],
                                                 int32_t (&r)[8][8]);

 public:
  int transform_decoding_for_chroma_samples(int32_t isChromaCb,
                                            int32_t PicWidthInSamples,
                                            uint8_t *pic_buff,
                                            bool isNeedIntraPrediction = true);
  int transform_decoding_for_4x4_luma_residual_blocks(
      int32_t isChroma, int32_t isChromaCb, int32_t BitDepth,
      int32_t PicWidthInSamples, uint8_t *pic_buff,
      bool isNeedIntraPrediction = true);
  int transform_decoding_for_8x8_luma_residual_blocks(
      int32_t isChroma, int32_t isChromaCb, int32_t BitDepth,
      int32_t PicWidthInSamples, int32_t Level8x8[4][64], uint8_t *pic_buff,
      bool isNeedIntraPrediction = true);
  int transform_decoding_for_luma_samples_of_16x16(
      int32_t isChroma, int32_t BitDepth, int32_t QP1,
      int32_t PicWidthInSamples, int32_t Intra16x16DCLevel[16],
      int32_t Intra16x16ACLevel[16][16], uint8_t *pic_buff);
  //----------------- 获取当前使用的帧内预测模式 ------------------------
 private:
  int getIntra4x4PredMode(int32_t luma4x4BlkIdx, int32_t &currMbAddrPredMode,
                          int32_t isChroma);
  int getIntra8x8PredMode(int32_t luma8x8BlkIdx, int32_t &currMbAddrPredMode,
                          int32_t isChroma);
  //----------------- 帧内预测算法(Luma) ------------------------
 private:
  int intra_4x4_sample_prediction(int32_t luma4x4BlkIdx,
                                  int32_t PicWidthInSamples,
                                  uint8_t *pic_buff_luma_pred, int32_t isChroma,
                                  int32_t BitDepth);
  int intra_8x8_sample_prediction(int32_t luma8x8BlkIdx,
                                  int32_t PicWidthInSamples,
                                  uint8_t *pic_buff_luma_pred, int32_t isChroma,
                                  int32_t BitDepth);
  int intra_16x16_sample_prediction(uint8_t *pic_buff_luma_pred,
                                    int32_t PicWidthInSamples, int32_t isChroma,
                                    int32_t BitDepth);
  int intra_residual_transform_bypass_decoding(int32_t nW, int32_t nH,
                                               int32_t horPredFlag, int32_t *r);
  //----------------- 帧内预测算法(Chroma) ------------------------
 private:
  int intra_chroma_sample_prediction(uint8_t *pic_buff_chroma_pred,
                                     int32_t PicWidthInSamples);
  int intra_chroma_sample_prediction_for_YUV420_or_YUV422(
      uint8_t *pic_buff_chroma_pred, int32_t PicWidthInSamples);
  int intra_chroma_sample_prediction_for_YUV444(uint8_t *pic_buff_chroma_pred,
                                                int32_t PicWidthInSamples);

 public:
  int sample_construction_for_I_PCM();

  //----------------- 宏块扫描 左上角位置 ------------------------
 private:
  int inverse_sub_macroblock_partition_scanning_process(
      H264_MB_TYPE m_name_of_mb_type, int32_t mbPartIdx, int32_t subMbPartIdx,
      int32_t &x, int32_t &y);

 public:
  int inverse_mb_scanning_process(int32_t MbaffFrameFlag, int32_t mbAddr,
                                  int32_t mb_field_decoding_flag, int32_t &x,
                                  int32_t &y);

  //----------------- 相邻宏块地址推导 ------------------------
 private:
  int derivation_of_availability_for_macroblock_addresses(
      int32_t mbAddr, int32_t &is_mbAddr_available);
  int derivation_of_availability_macroblock_addresses(
      int32_t _mbAddr, int32_t CurrMbAddr, MB_ADDR_TYPE &mbAddrN_type,
      int32_t &mbAddrN);
  int derivation_for_neighbouring_locations(
      int32_t MbaffFrameFlag, int32_t xN, int32_t yN, int32_t currMbAddr,
      MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN, int32_t &b4x4BlkIdxN,
      int32_t &b8x8BlkIdxN, int32_t &xW, int32_t &yW, int32_t isChroma);
  int derivation_for_neighbouring_macroblock_addr_availability(
      int32_t xN, int32_t yN, int32_t maxW, int32_t maxH, int32_t CurrMbAddr,
      MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN);

 public:
  int derivation_neighbouring_partitions(int32_t xN, int32_t yN,
                                         int32_t mbPartIdx,
                                         H264_MB_TYPE currSubMbType,
                                         int32_t subMbPartIdx, int32_t isChroma,
                                         int32_t &mbAddrN, int32_t &mbPartIdxN,
                                         int32_t &subMbPartIdxN);
  int derivation_for_neighbouring_macroblocks(int32_t MbaffFrameFlag,
                                              int32_t currMbAddr,
                                              int32_t &mbAddrA,
                                              int32_t &mbAddrB,
                                              const int32_t isChroma);
  int derivation_for_neighbouring_8x8_luma_block(
      int32_t luma8x8BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
      int32_t &luma8x8BlkIdxA, int32_t &luma8x8BlkIdxB, int32_t isChroma);
  int derivation_for_neighbouring_8x8_chroma_blocks_for_YUV444(
      int32_t chroma8x8BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
      int32_t &chroma8x8BlkIdxA, int32_t &chroma8x8BlkIdxB);
  int derivation_for_neighbouring_4x4_luma_blocks(
      int32_t luma4x4BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
      int32_t &luma4x4BlkIdxA, int32_t &luma4x4BlkIdxB, int32_t isChroma);
  int derivation_for_neighbouring_4x4_chroma_blocks(int32_t chroma4x4BlkIdx,
                                                    int32_t &mbAddrA,
                                                    int32_t &mbAddrB,
                                                    int32_t &chroma4x4BlkIdxA,
                                                    int32_t &chroma4x4BlkIdxB);
  int neighbouring_locations_non_MBAFF(int32_t xN, int32_t yN, int32_t maxW,
                                       int32_t maxH, int32_t CurrMbAddr,
                                       MB_ADDR_TYPE &mbAddrN_type,
                                       int32_t &mbAddrN, int32_t &b4x4BlkIdx,
                                       int32_t &b8x8BlkIdxN, int32_t &xW,
                                       int32_t &yW, int32_t isChroma);
  int neighbouring_locations_MBAFF(int32_t xN, int32_t yN, int32_t maxW,
                                   int32_t maxH, int32_t CurrMbAddr,
                                   MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN,
                                   int32_t &b4x4BlkIdxN, int32_t &b8x8BlkIdxN,
                                   int32_t &xW, int32_t &yW, int32_t isChroma);
  int derivation_for_neighbouring_macroblock_addr_availability_in_MBAFF(
      int32_t &mbAddrA, int32_t &mbAddrB, int32_t &mbAddrC, int32_t &mbAddrD);
  //----------------- 量化 ------------------------
 private:
  int scaling_for_residual_4x4_blocks(
      int32_t d[4][4], int32_t c[4][4], int32_t isChroma,
      const H264_MB_PART_PRED_MODE &m_mb_pred_mode, int32_t qP);
  int scaling_for_residual_8x8_blocks(
      int32_t d[8][8], int32_t c[8][8], int32_t isChroma,
      const H264_MB_PART_PRED_MODE &m_mb_pred_mode, int32_t qP);

  int scaling_and_transform_for_chroma_DC(int32_t isChromaCb, int32_t c[4][2],
                                          int32_t nW, int32_t nH,
                                          int32_t (&dcC)[4][2]);
  int scaling_and_transform_for_residual_4x4_blocks(int32_t c[4][4],
                                                    int32_t (&r)[4][4],
                                                    int32_t isChroma,
                                                    int32_t isChromaCb);
  int scaling_and_transform_for_residual_8x8_blocks(int32_t c[8][8],
                                                    int32_t (&r)[8][8],
                                                    int32_t isChroma,
                                                    int32_t isChromaCb);
  int scaling_functions(int32_t isChroma, int32_t isChromaCb);
  int scaling_and_transform_for_DC_Intra16x16(int32_t bitDepth, int32_t qP,
                                              int32_t c[4][4],
                                              int32_t (&dcY)[4][4]);
  int derivation_chroma_quantisation_parameters(int32_t isChromaCb);

 public:
  int get_chroma_quantisation_parameters2(int32_t QPY, int32_t isChromaCb,
                                          int32_t &QPC);

  //--------------帧间预测------------------------
 private:
  int derivation_motion_vector_components_and_reference_indices(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
      int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
      int32_t (&mvCL0)[2], int32_t (&mvCL1)[2], int32_t &subMvCnt,
      bool &predFlagL0, bool &predFlagL1, PictureBase *&refPicL0,
      PictureBase *&refPicL1);
  int derivation_luma_motion_vectors_for_P_Skip(
      int32_t &refIdxL0, int32_t &refIdxL1, int32_t (&mvL0)[2],
      int32_t (&mvL1)[2], int32_t &subMvCnt, bool &predFlagL0,
      bool &predFlagL1);
  int derivation_luma_motion_vectors_for_B_Skip_or_Direct_16x16_8x8(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
      int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
      int32_t &subMvCnt, bool &predFlagL0, bool &predFlagL1);
  int derivation_the_coLocated_4x4_sub_macroblock_partitions(
      int32_t mbPartIdx, int32_t subMbPartIdx, PictureBase *&colPic,
      int32_t &mbAddrCol, int32_t (&mvCol)[2], int32_t &refIdxCol,
      int32_t &vertMvScale, bool useRefPicList1);
  int derivation_spatial_direct_luma_motion_vector_and_ref_index_prediction(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
      int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
      int32_t &subMvCnt, bool &predFlagL0, bool &predFlagL1);
  int derivation_temporal_direct_luma_motion_vector_and_ref_index_prediction(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
      int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
      int32_t &subMvCnt, bool &predFlagL0, bool &predFlagL1);
  int derivation_luma_motion_vector_prediction(
      int32_t mbPartIdx, int32_t subMbPartIdx, H264_MB_TYPE currSubMbType,
      int32_t listSuffixFlag, int32_t refIdxLX, int32_t (&mvpLX)[2]);
  int derivation_median_luma_motion_vector_prediction(
      int32_t &mbAddrN_A, int32_t (&mvLXN_A)[2], int32_t &refIdxLXN_A,
      int32_t &mbAddrN_B, int32_t (&mvLXN_B)[2], int32_t &refIdxLXN_B,
      int32_t &mbAddrN_C, int32_t (&mvLXN_C)[2], int32_t &refIdxLXN_C,
      int32_t refIdxLX, int32_t (&mvpLX)[2]);
  int derivation_motion_data_of_neighbouring_partitions(
      int32_t mbPartIdx, int32_t subMbPartIdx, H264_MB_TYPE currSubMbType,
      int32_t listSuffixFlag, int32_t &mbAddrN_A, int32_t (&mvLXN_A)[2],
      int32_t &refIdxLXN_A, int32_t &mbAddrN_B, int32_t (&mvLXN_B)[2],
      int32_t &refIdxLXN_B, int32_t &mbAddrN_C, int32_t (&mvLXN_C)[2],
      int32_t &refIdxLXN_C);
  int derivation_chroma_motion_vectors(int32_t ChromaArrayType, int32_t mvLX[2],
                                       PictureBase *refPic,
                                       int32_t (&mvCLX)[2]);
  int MapColToList0(int32_t refIdxCol, PictureBase *colPic, int32_t mbAddrCol,
                    int32_t vertMvScale, bool field_pic_flag);

  int decoding_inter_prediction_samples(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t partWidth,
      int32_t partHeight, int32_t partWidthC, int32_t partHeightC, int32_t xAL,
      int32_t yAL, int32_t (&mvL0)[2], int32_t (&mvL1)[2], int32_t (&mvCL0)[2],
      int32_t (&mvCL1)[2], PictureBase *refPicL0, PictureBase *refPicL1,
      bool predFlagL0, bool predFlagL1, int32_t logWDL, int32_t w0L,
      int32_t w1L, int32_t o0L, int32_t o1L, int32_t logWDCb, int32_t w0Cb,
      int32_t w1Cb, int32_t o0Cb, int32_t o1Cb, int32_t logWDCr, int32_t w0Cr,
      int32_t w1Cr, int32_t o0Cr, int32_t o1Cr, uint8_t *predPartL,
      uint8_t *predPartCb, uint8_t *predPartCr);
  int reference_picture_selection(int32_t refIdxLX, Frame *RefPicListX[16],
                                  int32_t RefPicListXLength,
                                  PictureBase *&refPic);
  int fractional_sample_interpolation(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t partWidth,
      int32_t partHeight, int32_t partWidthC, int32_t partHeightC, int32_t xAL,
      int32_t yAL, int32_t (&mvLX)[2], int32_t (&mvCLX)[2],
      PictureBase *refPicLX, uint8_t *predPartLXL, uint8_t *predPartLXCb,
      uint8_t *predPartLXCr);

  int luma_sample_interpolation(int32_t xIntL, int32_t yIntL, int32_t xFracL,
                                int32_t yFracL, PictureBase *refPic,
                                uint8_t &predPartLXL_xL_yL);
  int chroma_sample_interpolation(int32_t xIntC, int32_t yIntC, int32_t xFracC,
                                  int32_t yFracC, PictureBase *refPic,
                                  int32_t isChromaCb,
                                  uint8_t &predPartLXC_xC_yC);
  int weighted_sample_prediction(
      int32_t mbPartIdx, int32_t subMbPartIdx, bool predFlagL0, bool predFlagL1,
      int32_t partWidth, int32_t partHeight, int32_t partWidthC,
      int32_t partHeightC, int32_t logWDL, int32_t w0L, int32_t w1L,
      int32_t o0L, int32_t o1L, int32_t logWDCb, int32_t w0Cb, int32_t w1Cb,
      int32_t o0Cb, int32_t o1Cb, int32_t logWDCr, int32_t w0Cr, int32_t w1Cr,
      int32_t o0Cr, int32_t o1Cr, uint8_t *predPartL0L, uint8_t *predPartL0Cb,
      uint8_t *predPartL0Cr, uint8_t *predPartL1L, uint8_t *predPartL1Cb,
      uint8_t *predPartL1Cr, uint8_t *predPartL, uint8_t *predPartCb,
      uint8_t *predPartCr);

  int derivation_prediction_weights(
      int32_t refIdxL0, int32_t refIdxL1, bool predFlagL0, bool predFlagL1,
      int32_t &logWDL, int32_t &w0L, int32_t &w1L, int32_t &o0L, int32_t &o1L,
      int32_t &logWDCb, int32_t &w0Cb, int32_t &w1Cb, int32_t &o0Cb,
      int32_t &o1Cb, int32_t &logWDCr, int32_t &w0Cr, int32_t &w1Cr,
      int32_t &o0Cr, int32_t &o1Cr);
  int weighted_sample_prediction_default(
      bool predFlagL0, bool predFlagL1, int32_t partWidth, int32_t partHeight,
      int32_t partWidthC, int32_t partHeightC, uint8_t *predPartL0L,
      uint8_t *predPartL0Cb, uint8_t *predPartL0Cr, uint8_t *predPartL1L,
      uint8_t *predPartL1Cb, uint8_t *predPartL1Cr, uint8_t *predPartL,
      uint8_t *predPartCb, uint8_t *predPartCr);
  int weighted_sample_prediction_Explicit_or_Implicit(
      int32_t mbPartIdx, int32_t subMbPartIdx, bool predFlagL0, bool predFlagL1,
      int32_t partWidth, int32_t partHeight, int32_t partWidthC,
      int32_t partHeightC, int32_t logWDL, int32_t w0L, int32_t w1L,
      int32_t o0L, int32_t o1L, int32_t logWDCb, int32_t w0Cb, int32_t w1Cb,
      int32_t o0Cb, int32_t o1Cb, int32_t logWDCr, int32_t w0Cr, int32_t w1Cr,
      int32_t o0Cr, int32_t o1Cr, uint8_t *predPartL0L, uint8_t *predPartL0Cb,
      uint8_t *predPartL0Cr, uint8_t *predPartL1L, uint8_t *predPartL1Cb,
      uint8_t *predPartL1Cr, uint8_t *predPartL, uint8_t *predPartCb,
      uint8_t *predPartCr);
  int picture_construction_process_prior_to_deblocking_filter(
      int32_t *u, int32_t nW, int32_t nH, int32_t BlkIdx, int32_t isChroma,
      int32_t PicWidthInSamples, uint8_t *pic_buff);

 public:
  int inter_prediction_process();
  int derivation_macroblock_and_sub_macroblock_partition_indices(
      H264_MB_TYPE mb_type_, H264_MB_TYPE subMbType[4], int32_t xP, int32_t yP,
      int32_t &mbPartIdxN, int32_t &subMbPartIdxN);
};

#endif /* end of include guard: PICTUREBASE_HPP_ZGHBMJIH */
