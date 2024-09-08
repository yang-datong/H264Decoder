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
  int32_t mb_x; // 存储当前正在解码的宏块的X坐标（相对于图片左上角位置）
  int32_t mb_y; // 存储当前正在解码的宏块的Y坐标（相对于图片左上角位置）
  /* 比如(0,0)表示最左上角的宏块，假设图像的分辨率是 1920x1080，宏块大小是 16x16 像素：
 * 共有宏块： 1920 / 16 = 120,  1080 / 16 = 67.5, 120 * 68 = 8160
 * 若图像像素坐标为：(1919,1079)，对应映射的宏块坐标为：(1919 / 16 = 119.9375, 1079 / 16 = 67.4375) = (119,67)
 * */
  int32_t m_pic_coded_width_pixels; // 图片宽（单位：像素），例如：1920x1088
  int32_t m_pic_coded_height_pixels; // 图片高（单位：像素），例如：1920x1088
  int32_t PicWidthInMbs;             // 图片宽（单位：16x16的宏块）
  int32_t PicHeightInMbs;            // 图片高（单位：16x16的宏块）
  int32_t PicSizeInMbs;       // PicSizeInMbs = PicWidthInMbs * PicHeightInMbs;
  int32_t MbWidthL;           // 亮度宏块的宽（单位：像素）
  int32_t MbHeightL;          // 亮度宏块的宽（单位：像素）
  int32_t MbWidthC;           // 色度宏块的宽（单位：像素）
  int32_t MbHeightC;          // 色度宏块的宽（单位：像素）
  int32_t PicWidthInSamplesL; // PicWidthInSamplesL = PicWidthInMbs * 16;
                              // //解码后图片的宽度（单位：像素）
  int32_t PicWidthInSamplesC; // PicWidthInSamplesC = PicWidthInMbs * MbWidthC;
  int32_t PicHeightInSamplesL; // PicHeightInSamplesL = PicHeightInMbs * 16;
      // //解码后图片的高度（单位：像素）
  int32_t
      PicHeightInSamplesC; // PicHeightInSamplesC = PicHeightInMbs * MbHeightC;
  int32_t Chroma_Format;   // CHROMA_FORMAT_IDC_420
  int32_t mb_cnt;          // 解码的宏块的计数
  int32_t CurrMbAddr; // 当前解码的宏块在图片中的坐标位置(此字段非常重要)

  uint8_t *m_pic_buff_luma; // 存储解码后图片的Y分量数据 cSL[1920x1088]
  uint8_t *m_pic_buff_cb; // 存储解码后图片的cb分量数据
  uint8_t *m_pic_buff_cr; // 存储解码后图片的cr分量数据

  int32_t TopFieldOrderCnt;
  int32_t BottomFieldOrderCnt;
  int32_t PicOrderCntMsb;
  int32_t PicOrderCntLsb;
  int32_t FrameNumOffset;
  int32_t absFrameNum;
  int32_t picOrderCntCycleCnt;
  int32_t frameNumInPicOrderCntCycle;
  int32_t expectedPicOrderCnt;
  int32_t
      PicOrderCnt; //(Picture Order Count）用于表示图像顺序的计数器，用于确定图像的显示顺序
  int32_t FrameNum;     // To each short-term reference picture 短期参考帧
  int32_t FrameNumWrap; // To each short-term reference picture 短期参考帧
  int32_t LongTermFrameIdx; // Each long-term reference picture 长期参考帧
  int32_t PicNum; // To each short-term reference picture 短期参考图像
  int32_t LongTermPicNum; // To each long-term reference picture 长期参考图像
  int32_t FieldNum;
  int32_t MaxLongTermFrameIdx; // -1: "no long-term frame indices"
  int32_t
      memory_management_control_operation_5_flag; // 所有参考图像标记为“不用于参考”
  int32_t memory_management_control_operation_6_flag;
  H264_PICTURE_MARKED_AS reference_marked_type = H264_PICTURE_MARKED_AS_unkown; // I,P作为参考帧的mark状态

  Slice m_slice;
  //SliceHeader m_h264_slice_header;
  //SliceBody m_h264_slice_data; // 注意：一个picture中可能有多个slice data
  MacroBlock *m_mbs; // 存储当前图像的所有宏块 m_mbs[PicSizeInMbs] =
                     // m_mbs[PicWidthInMbs * PicHeightInMbs];
  int32_t LevelScale4x4[6][4][4];
  int32_t LevelScale8x8[6][8][8];

  H264_PICTURE_CODED_TYPE m_picture_coded_type;
  H264_PICTURE_TYPE m_picture_type;
  int32_t m_is_malloc_mem_by_myself; // 是否已经初始化
  int32_t m_is_decode_finished;      // 本帧/场是否解码完毕
  int32_t m_slice_cnt; // 一个picture中可能有多个slice data

  Frame *m_dpb[16]; //[16] decoded picture buffer
  Frame *m_parent;
  Frame *m_RefPicList0[16];    //[16] decoding a P or SP slice;
  Frame *m_RefPicList1[16];    //[16] decoding a B slice;
  int32_t m_RefPicList0Length; // RefPicList0排序后的参考图像数目
  int32_t m_RefPicList1Length; // RefPicList1排序后的参考图像数目
  int32_t m_PicNumCnt;         // 图片递增计数

 public:
  PictureBase();
  ~PictureBase();
  int printInfo();
  int reset();
  int init(Slice &slice);
  int unInit();
  PictureBase &operator=(const PictureBase &src); // 重载等号运算符
  int copyData(const PictureBase &src, bool isMallocAndCopyData);
  int copyData2(const PictureBase &src, int32_t copyMbsDataFlag);
  int copyDataPicOrderCnt(const PictureBase &src);

  //-------------------------------------
  int convertYuv420pToBgr24(
      uint32_t width, uint32_t height, const uint8_t *yuv420p, uint8_t *bgr24,
      uint32_t widthBytesBgr24); // 图像不是上下翻转的，主要用于保存成BMP图片
  int convertYuv420pToBgr24FlipLines(
      uint32_t width, uint32_t height, const uint8_t *yuv420p, uint8_t *bgr24,
      uint32_t widthBytesBgr24); // 图像是上下翻转的，主要用于播放器画图
  int createEmptyImage(MY_BITMAP &bitmap, int32_t width, int32_t height,
                       int32_t bmBitsPixel); // 在内存中创建一幅空白位图
  int saveToBmpFile(const char *filename);
  int saveBmp(const char *filename, MY_BITMAP *pBitmap);
  int writeYUV(const char *filename);
  int getOneEmptyPicture(Frame *&pic);
  int end_decode_the_picture_and_get_a_new_empty_picture(
      Frame *&newEmptyPicture);

  //--------------参考帧列表重排序------------------------
  int decoding_picture_order_count();
  int decoding_picture_order_count_type_0(
      const PictureBase *picture_previous_ref); // 8.2.1.1
  int decoding_picture_order_count_type_1(
      const PictureBase *picture_previous); // 8.2.1.2
  int decoding_picture_order_count_type_2(
      const PictureBase *picture_previous); // 8.2.1.3

  int decoding_reference_picture_lists_construction(
      Frame *(&dpb)[16], Frame *(&RefPicList0)[16],
      Frame *(&RefPicList1)[16]); // 8.2.4 参考图像列表的重排序过程
  int decoding_picture_numbers(Frame *(&dpb)[16]); // 8.2.4.1

  int init_reference_picture_lists(Frame *(&dpb)[16], Frame *(&RefPicList0)[16],
                                   Frame *(&RefPicList1)[16]); // 8.2.4.2
  int init_reference_picture_list_P_SP_slices_in_frames(
      Frame *(&dpb)[16], Frame *(&RefPicList0)[16],
      int32_t &RefPicList0Length); // 8.2.4.2.1
  int init_reference_picture_list_P_SP_slices_in_fields(
      Frame *(&dpb)[16], Frame *(&RefPicList0)[16],
      int32_t &RefPicList0Length); // 8.2.4.2.2
  int init_reference_picture_lists_B_slices_in_frames(
      Frame *(&dpb)[16], Frame *(&RefPicList0)[16], Frame *(&RefPicList1)[16],
      int32_t &RefPicList0Length,
      int32_t &RefPicList1Length); // 8.2.4.2.3
  int init_reference_picture_lists_B_slices_in_fields(
      Frame *(&dpb)[16], Frame *(&RefPicList0)[16], Frame *(&RefPicList1)[16],
      int32_t &RefPicList0Length,
      int32_t &RefPicList1Length); // 8.2.4.2.4
  //int init_reference_picture_lists_in_fields(
  //Frame *(&refFrameListXShortTerm)[16], Frame *(&refFrameListXLongTerm)[16],
  //Frame *(&RefPicListX)[16], int32_t &RefPicListXLength,
  //int32_t listX); // 8.2.4.2.5
  int init_reference_picture_lists_in_fields(
      vector<Frame *>(&refFrameListXShortTerm),
      vector<Frame *>(&refFrameListXLongTerm), Frame *(&RefPicListX)[16],
      int32_t &RefPicListXLength,
      int32_t listX); // 8.2.4.2.5

  int modif_reference_picture_lists(
      Frame *(&RefPicList0)[16],
      Frame *(&RefPicList1)[16]); // 8.2.4.3 参考图像列表的重排序过程

  int modif_reference_picture_lists_for_short_ref_pictures(
      int32_t &refIdxLX, int32_t &picNumLXPred, const int32_t modif_idc,
      const int32_t abs_diff_pic_num_minus1,
      const int32_t num_ref_idx_lX_active_minus1, Frame *(&RefPicListX)[16]);

  int modif_reference_picture_lists_for_long_ref_pictures(
      int32_t &refIdxLX, const int32_t num_ref_idx_lX_active_minus1,
      const int32_t long_term_pic_num, Frame *(&RefPicListX)[16]);

  int Decoded_reference_picture_marking_process(Frame *(
      &dpb)[16]); // 8.2.5 每一张图片解码完成后，都需要标记一次图像参考列表
  int Sequence_of_operations_for_decoded_reference_picture_marking_process(
      Frame *(&dpb)[16]);                       // 8.2.5.1
  int Decoding_process_for_gaps_in_frame_num(); // 8.2.5.2
  int Sliding_window_decoded_reference_picture_marking_process(
      Frame *(&dpb)[16]); // 8.2.5.3
  int Adaptive_memory_control_decoded_reference_picture_marking_process(
      Frame *(&dpb)[16]); // 8.2.5.4

  //--------------帧内预测------------------------
  int getIntra4x4PredMode(int32_t luma4x4BlkIdx,
                          int32_t &Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr,
                          int32_t isChroma); // 8.3.1.1
  int getIntra8x8PredMode(int32_t luma8x8BlkIdx,
                          int32_t &Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr,
                          int32_t isChroma); // 8.3.2.1
  int Intra_4x4_sample_prediction(int32_t luma4x4BlkIdx,
                                  int32_t PicWidthInSamples,
                                  uint8_t *pic_buff_luma_pred, int32_t isChroma,
                                  int32_t BitDepth); // 8.3.1.2
  int Intra_8x8_sample_prediction(int32_t luma8x8BlkIdx,
                                  int32_t PicWidthInSamples,
                                  uint8_t *pic_buff_luma_pred, int32_t isChroma,
                                  int32_t BitDepth); // 8.3.2.2
  int Intra_16x16_sample_prediction(uint8_t *pic_buff_luma_pred,
                                    int32_t PicWidthInSamples, int32_t isChroma,
                                    int32_t BitDepth); // 8.3.3
  int Intra_chroma_sample_prediction(uint8_t *pic_buff_chroma_pred,
                                     int32_t PicWidthInSamples); // 8.3.4
  int Intra_chroma_sample_prediction_for_YUV420_or_YUV422(
      uint8_t *pic_buff_chroma_pred, int32_t PicWidthInSamples); // 8.3.4
  int Intra_chroma_sample_prediction_for_YUV444(
      uint8_t *pic_buff_chroma_pred, int32_t PicWidthInSamples); // 8.3.4.5
  int Sample_construction_process_for_I_PCM_macroblocks();       // 8.3.5

  int inverse_macroblock_scanning_process(int32_t MbaffFrameFlag,
                                          int32_t mbAddr,
                                          int32_t mb_field_decoding_flag,
                                          int32_t &x,
                                          int32_t &y); // 6.4.1
  int Inverse_sub_macroblock_partition_scanning_process(
      H264_MB_TYPE m_name_of_mb_type, int32_t mbPartIdx, int32_t subMbPartIdx,
      int32_t &x, int32_t &y); // 6.4.2.2
  int Derivation_process_of_the_availability_for_macroblock_addresses(
      int32_t mbAddr, int32_t &is_mbAddr_available); // 6.4.8

  int derivation_for_neighbouring_macroblocks(const int32_t MbaffFrameFlag,
                                              const int32_t currMbAddr,
                                              int32_t &mbAddrA,
                                              int32_t &mbAddrB,
                                              const int32_t isChroma);

  int Derivation_process_for_neighbouring_8x8_luma_block(
      int32_t luma8x8BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
      int32_t &luma8x8BlkIdxA, int32_t &luma8x8BlkIdxB,
      int32_t isChroma); // 6.4.11.2
  int Derivation_process_for_neighbouring_8x8_chroma_blocks_for_ChromaArrayType_equal_to_3(
      int32_t chroma8x8BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
      int32_t &chroma8x8BlkIdxA, int32_t &chroma8x8BlkIdxB); // 6.4.11.3
  int Derivation_process_for_neighbouring_4x4_luma_blocks(
      int32_t luma4x4BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
      int32_t &luma4x4BlkIdxA, int32_t &luma4x4BlkIdxB,
      int32_t isChroma); // 6.4.11.4
  int Derivation_process_for_neighbouring_4x4_chroma_blocks(
      int32_t chroma4x4BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
      int32_t &chroma4x4BlkIdxA, int32_t &chroma4x4BlkIdxB); // 6.4.11.5
                                                             //
  int derivation_for_neighbouring_locations(
      const int32_t MbaffFrameFlag, const int32_t xN, const int32_t yN,
      const int32_t currMbAddr, MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN,
      int32_t &b4x4BlkIdxN, int32_t &b8x8BlkIdxN, int32_t &xW, int32_t &yW,
      const int32_t isChroma);

  int neighbouring_locations_non_MBAFF(const int32_t xN, const int32_t yN,
                                       const int32_t maxW, const int32_t maxH,
                                       const int32_t CurrMbAddr,
                                       MB_ADDR_TYPE &mbAddrN_type,
                                       int32_t &mbAddrN, int32_t &b4x4BlkIdx,
                                       int32_t &b8x8BlkIdxN, int32_t &xW,
                                       int32_t &yW, const int32_t isChroma);

  int derivation_for_neighbouring_macroblock_addr_availability(
      const int32_t xN, const int32_t yN, const int32_t maxW,
      const int32_t maxH, const int32_t CurrMbAddr, MB_ADDR_TYPE &mbAddrN_type,
      int32_t &mbAddrN);

  inline int derivation_of_availability_macroblock_addresses(
      int32_t _mbAddr, int32_t CurrMbAddr, MB_ADDR_TYPE &mbAddrN_type,
      int32_t &mbAddrN);

  int neighbouring_locations_MBAFF(const int32_t xN, const int32_t yN,
                                   const int32_t maxW, const int32_t maxH,
                                   const int32_t CurrMbAddr,
                                   MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN,
                                   int32_t &b4x4BlkIdxN, int32_t &b8x8BlkIdxN,
                                   int32_t &xW, int32_t &yW,
                                   const int32_t isChroma);

  int derivation_for_neighbouring_macroblock_addr_availability_in_MBAFF(
      int32_t &mbAddrA, int32_t &mbAddrB, int32_t &mbAddrC, int32_t &mbAddrD);

  int Derivation_process_for_4x4_luma_block_indices(
      uint8_t xP, uint8_t yP, uint8_t &luma4x4BlkIdx); // 6.4.13.1
  int Derivation_process_for_4x4_chroma_block_indices(
      uint8_t xP, uint8_t yP, uint8_t &chroma4x4BlkIdx); // 6.4.13.2
  int Derivation_process_for_8x8_luma_block_indices(
      uint8_t xP, uint8_t yP, uint8_t &luma8x8BlkIdx); // 6.4.13.3

  int transform_decoding_for_4x4_luma_residual_blocks(
      int32_t isChroma, int32_t isChromaCb, int32_t BitDepth,
      int32_t PicWidthInSamples, uint8_t *pic_buff); // 8.5.1
  int transform_decoding_for_luma_samples_of_Intra_16x16_macroblock_prediction(
      int32_t isChroma, int32_t BitDepth, int32_t QP1,
      int32_t PicWidthInSamples, int32_t Intra16x16DCLevel[16],
      int32_t Intra16x16ACLevel[16][16], uint8_t *pic_buff); // 8.5.2
  int transform_decoding_for_8x8_luma_residual_blocks(
      int32_t isChroma, int32_t isChromaCb, int32_t BitDepth,
      int32_t PicWidthInSamples, int32_t Level8x8[4][64],
      uint8_t *pic_buff); // 8.5.3
  int transform_decoding_for_chroma_samples(int32_t isChromaCb,
                                            int32_t PicWidthInSamples,
                                            uint8_t *pic_buff); // 8.5.4
  int transform_decoding_for_chroma_samples_with_YUV444(
      int32_t isChromaCb, int32_t PicWidthInSamples,
      uint8_t *pic_buff); // 8.5.5
  int inverse_scanning_for_4x4_transform_coefficients_and_scaling_lists(
      int32_t values[16], int32_t (&c)[4][4],
      int32_t field_scan_flag); // 8.5.6
  int scaling_and_transformation_for_chroma_DC_transform_coefficients(
      int32_t isChromaCb, int32_t c[4][2], int32_t nW, int32_t nH,
      int32_t (&dcC)[4][2]); // 8.5.11
  int scaling_and_transformation_process_for_residual_4x4_blocks(
      int32_t c[4][4], int32_t (&r)[4][4], int32_t isChroma,
      int32_t isChromaCb); // 8.5.12
  int scaling_for_residual_4x4_blocks(
      int32_t d[4][4], int32_t c[4][4], int32_t isChroma,
      const H264_MB_PART_PRED_MODE &m_mb_pred_mode, int32_t qP);
  int transformation_for_residual_4x4_blocks(int32_t d[4][4],
                                             int32_t (&r)[4][4]);

  int Scaling_and_transformation_process_for_residual_8x8_blocks(
      int32_t c[8][8], int32_t (&r)[8][8], int32_t isChroma,
      int32_t isChromaCb); // 8.5.13
  int picture_construction_process_prior_to_deblocking_filter(
      int32_t *u, int32_t nW, int32_t nH, int32_t BlkIdx, int32_t isChroma,
      int32_t PicWidthInSamples, uint8_t *pic_buff); // 8.5.14
  int Decoding_process_for_P_macroblocks_in_SP_slices_or_SI_macroblocks(); // 8.6
  int Inverse_scanning_process_for_8x8_transform_coefficients_and_scaling_lists(
      int32_t values[64], int32_t (&c)[8][8],
      int32_t field_scan_flag);                                      // 8.5.7
  int derivation_chroma_quantisation_parameters(int32_t isChromaCb); // 8.5.8
  int get_chroma_quantisation_parameters2(int32_t QPY, int32_t isChromaCb,
                                          int32_t &QPC);       // 8.5.8
  int scaling_functions(int32_t isChroma, int32_t isChromaCb); // 8.5.9
  int Scaling_and_transformation_process_for_DC_transform_coefficients_for_Intra_16x16_macroblock_type(
      int32_t bitDepth, int32_t qP, int32_t c[4][4],
      int32_t (&dcY)[4][4]); // 8.5.10

  //--------------帧间预测------------------------
  int transform_decoding_for_4x4_luma_residual_blocks_inter(
      int32_t isChroma, int32_t isChromaCb, int32_t BitDepth,
      int32_t PicWidthInSamples, uint8_t *pic_buff);
  int transform_decoding_for_8x8_luma_residual_blocks_inter(
      int32_t isChroma, int32_t isChromaCb, int32_t BitDepth,
      int32_t PicWidthInSamples, int32_t Level8x8[4][64], uint8_t *pic_buff);
  int transform_decoding_for_chroma_samples_inter(int32_t isChromaCb,
                                                  int32_t PicWidthInSamples,
                                                  uint8_t *pic_buff);
  int intra_residual_transform_bypass_decoding(int32_t nW, int32_t nH,
                                               int32_t horPredFlag, int32_t *r);
  int inter_prediction_process(); // 8.4
  int derivation_motion_vector_components_and_reference_indices(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
      int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
      int32_t (&mvCL0)[2], int32_t (&mvCL1)[2], int32_t &subMvCnt,
      int32_t &predFlagL0, int32_t &predFlagL1, PictureBase *&refPicL0,
      PictureBase *&refPicL1); // 8.4.1
  int Derivation_process_for_luma_motion_vectors_for_B_Skip_or_B_Direct_16x16_or_B_Direct_8x8(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
      int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
      int32_t &subMvCnt, int32_t &predFlagL0,
      int32_t &predFlagL1); // 8.4.1.2
  int Derivation_process_for_the_co_located_4x4_sub_macroblock_partitions(
      int32_t mbPartIdx, int32_t subMbPartIdx, PictureBase *&colPic,
      int32_t &mbAddrCol, int32_t (&mvCol)[2], int32_t &refIdxCol,
      int32_t &vertMvScale); // 8.4.1.2.1
  int Derivation_process_for_spatial_direct_luma_motion_vector_and_reference_index_prediction_mode(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
      int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
      int32_t &subMvCnt, int32_t &predFlagL0,
      int32_t &predFlagL1); // 8.4.1.2.2
  int Derivation_process_for_temporal_direct_luma_motion_vector_and_reference_index_prediction_mode(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t &refIdxL0,
      int32_t &refIdxL1, int32_t (&mvL0)[2], int32_t (&mvL1)[2],
      int32_t &subMvCnt, int32_t &predFlagL0,
      int32_t &predFlagL1); // 8.4.1.2.3
  int Derivation_process_for_luma_motion_vector_prediction(
      int32_t mbPartIdx, int32_t subMbPartIdx, H264_MB_TYPE currSubMbType,
      int32_t listSuffixFlag, int32_t refIdxLX,
      int32_t (&mvpLX)[2]); // 8.4.1.3
  int Derivation_process_for_motion_data_of_neighbouring_partitions(
      int32_t mbPartIdx, int32_t subMbPartIdx, H264_MB_TYPE currSubMbType,
      int32_t listSuffixFlag, int32_t &mbAddrN_A, int32_t (&mvLXN_A)[2],
      int32_t &refIdxLXN_A, int32_t &mbAddrN_B, int32_t (&mvLXN_B)[2],
      int32_t &refIdxLXN_B, int32_t &mbAddrN_C, int32_t (&mvLXN_C)[2],
      int32_t &refIdxLXN_C); // 8.4.1.3.2
  int Derivation_process_for_chroma_motion_vectors(
      int32_t ChromaArrayType, int32_t mvLX[2], PictureBase *refPic,
      int32_t (&mvCLX)[2]); // 8.4.1.4

  int decoding_Inter_prediction_samples(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t partWidth,
      int32_t partHeight, int32_t partWidthC, int32_t partHeightC, int32_t xAL,
      int32_t yAL, int32_t (&mvL0)[2], int32_t (&mvL1)[2], int32_t (&mvCL0)[2],
      int32_t (&mvCL1)[2], PictureBase *refPicL0, PictureBase *refPicL1,
      int32_t predFlagL0, int32_t predFlagL1, int32_t logWDL, int32_t w0L,
      int32_t w1L, int32_t o0L, int32_t o1L, int32_t logWDCb, int32_t w0Cb,
      int32_t w1Cb, int32_t o0Cb, int32_t o1Cb, int32_t logWDCr, int32_t w0Cr,
      int32_t w1Cr, int32_t o0Cr, int32_t o1Cr,
      uint8_t *predPartL,   // predPartL[partHeight][partWidth]
      uint8_t *predPartCb,  // predPartCb[partHeightC][partWidthC]
      uint8_t *predPartCr); // predPartCr[partHeightC][partWidthC] //8.4.2
  int Reference_picture_selection_process(int32_t refIdxLX,
                                          Frame *RefPicListX[16],
                                          int32_t RefPicListXLength,
                                          PictureBase *&refPic); // 8.4.2.1
  int fractional_sample_interpolation(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t partWidth,
      int32_t partHeight, int32_t partWidthC, int32_t partHeightC, int32_t xAL,
      int32_t yAL, int32_t (&mvLX)[2], int32_t (&mvCLX)[2],
      PictureBase *refPicLX,
      uint8_t *predPartLXL,   // predPartL[partHeight][partWidth]
      uint8_t *predPartLXCb,  // predPartCb[partHeightC][partWidthC]
      uint8_t *predPartLXCr); // predPartCr[partHeightC][partWidthC] //8.4.2.2
  int luma_sample_interpolation_process(
      int32_t xIntL, int32_t yIntL, int32_t xFracL, int32_t yFracL,
      PictureBase *refPic, uint8_t &predPartLXL_xL_yL); // 8.4.2.2.1
  int chroma_sample_interpolation_process(
      int32_t xIntC, int32_t yIntC, int32_t xFracC, int32_t yFracC,
      PictureBase *refPic, int32_t isChromaCb,
      uint8_t &predPartLXC_xC_yC); // 8.4.2.2.2
  int weighted_sample_prediction(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t predFlagL0,
      int32_t predFlagL1, int32_t partWidth, int32_t partHeight,
      int32_t partWidthC, int32_t partHeightC, int32_t logWDL, int32_t w0L,
      int32_t w1L, int32_t o0L, int32_t o1L, int32_t logWDCb, int32_t w0Cb,
      int32_t w1Cb, int32_t o0Cb, int32_t o1Cb, int32_t logWDCr, int32_t w0Cr,
      int32_t w1Cr, int32_t o0Cr, int32_t o1Cr,
      uint8_t *predPartL0L,  // predPartLXL[partHeight][partWidth]
      uint8_t *predPartL0Cb, // predPartLXCb[partHeightC][partWidthC]
      uint8_t *predPartL0Cr, // predPartLXCr[partHeightC][partWidthC]
      uint8_t *predPartL1L,  // predPartLXL[partHeight][partWidth]
      uint8_t *predPartL1Cb, // predPartLXCb[partHeightC][partWidthC]
      uint8_t *predPartL1Cr, // predPartLXCr[partHeightC][partWidthC]
      uint8_t *predPartL,    // out: predPartL[partHeight][partWidth]
      uint8_t *predPartCb,   // out: predPartLXCb[partHeightC][partWidthC]
      uint8_t
          *predPartCr); // out: predPartLXCr[partHeightC][partWidthC] //8.4.2.3

  int derivation_prediction_weights(int32_t refIdxL0, int32_t refIdxL1,
                                    int32_t predFlagL0, int32_t predFlagL1,
                                    int32_t &logWDL, int32_t &w0L, int32_t &w1L,
                                    int32_t &o0L, int32_t &o1L,
                                    int32_t &logWDCb, int32_t &w0Cb,
                                    int32_t &w1Cb, int32_t &o0Cb, int32_t &o1Cb,
                                    int32_t &logWDCr, int32_t &w0Cr,
                                    int32_t &w1Cr, int32_t &o0Cr,
                                    int32_t &o1Cr); // 8.4.3
  int default_weighted_sample_prediction(
      int32_t predFlagL0, int32_t predFlagL1, int32_t partWidth,
      int32_t partHeight, int32_t partWidthC, int32_t partHeightC,
      uint8_t *predPartL0L, uint8_t *predPartL0Cb, uint8_t *predPartL0Cr,
      uint8_t *predPartL1L, uint8_t *predPartL1Cb, uint8_t *predPartL1Cr,
      uint8_t *predPartL, uint8_t *predPartCb,
      uint8_t *predPartCr); // 8.4.2.3.1
  int weighted_sample_prediction_process_2(
      int32_t mbPartIdx, int32_t subMbPartIdx, int32_t predFlagL0,
      int32_t predFlagL1, int32_t partWidth, int32_t partHeight,
      int32_t partWidthC, int32_t partHeightC, int32_t logWDL, int32_t w0L,
      int32_t w1L, int32_t o0L, int32_t o1L, int32_t logWDCb, int32_t w0Cb,
      int32_t w1Cb, int32_t o0Cb, int32_t o1Cb, int32_t logWDCr, int32_t w0Cr,
      int32_t w1Cr, int32_t o0Cr, int32_t o1Cr, uint8_t *predPartL0L,
      uint8_t *predPartL0Cb, uint8_t *predPartL0Cr, uint8_t *predPartL1L,
      uint8_t *predPartL1Cb, uint8_t *predPartL1Cr, uint8_t *predPartL,
      uint8_t *predPartCb, uint8_t *predPartCr); // 8.4.2.3.2

  int Inverse_sub_macroblock_partition_scanning_process(MacroBlock *mb,
                                                        int32_t mbPartIdx,
                                                        int32_t subMbPartIdx,
                                                        int32_t &x,
                                                        int32_t &y); // 6.4.2.2
  int Derivation_process_for_neighbouring_partitions(
      int32_t xN, int32_t yN, int32_t mbPartIdx, H264_MB_TYPE currSubMbType,
      int32_t subMbPartIdx, int32_t isChroma, int32_t &mbAddrN,
      int32_t &mbPartIdxN, int32_t &subMbPartIdxN); // 6.4.11.7
  int Derivation_process_for_macroblock_and_sub_macroblock_partition_indices(
      H264_MB_TYPE mb_type_, H264_MB_TYPE subMbType[4], int32_t xP, int32_t yP,
      int32_t &mbPartIdxN, int32_t &subMbPartIdxN); // 6.4.13.4

  int PicOrderCntFunc(
      PictureBase *picX); // 8.2.1 POC: picture order count 图像序列号
  int DiffPicOrderCnt(PictureBase *picA, PictureBase *picB);

  //--------------去方块(环路)滤波过程------------------------
  int Deblocking_filter_process(); // 8.7
  int Filtering_process_for_block_edges(
      int32_t MbaffFrameFlag, int32_t _CurrMbAddr,
      int32_t mb_field_decoding_flag, int32_t chromaEdgeFlag, int32_t iCbCr,
      int32_t mbAddrN, int32_t verticalEdgeFlag,
      int32_t fieldModeInFrameFilteringFlag, int32_t leftMbEdgeFlag,
      int32_t (&E)[16][2]); // 8.7.1
  int Filtering_process_for_a_set_of_samples_across_a_horizontal_or_vertical_block_edge(
      int32_t MbaffFrameFlag, int32_t _CurrMbAddr, int32_t chromaEdgeFlag,
      int32_t isChromaCb, uint8_t mb_x_p0, uint8_t mb_y_p0, uint8_t mb_x_q0,
      uint8_t mb_y_q0, int32_t verticalEdgeFlag, int32_t mbAddrN,
      const uint8_t (&p)[4], const uint8_t (&q)[4], uint8_t (&pp)[3],
      uint8_t (&qq)[3]); // 8.7.2
  int Derivation_process_for_the_luma_content_dependent_boundary_filtering_strength(
      int32_t MbaffFrameFlag, int32_t p0, int32_t q0, uint8_t mb_x_p0,
      uint8_t mb_y_p0, uint8_t mb_x_q0, uint8_t mb_y_q0, int32_t mbAddr_p0,
      int32_t mbAddr_q0, int32_t verticalEdgeFlag, int32_t &bS); // 8.7.2.1
  int Derivation_process_for_the_thresholds_for_each_block_edge(
      int32_t p0, int32_t q0, int32_t p1, int32_t q1, int32_t chromaEdgeFlag,
      int32_t bS, int32_t filterOffsetA, int32_t filterOffsetB, int32_t qPp,
      int32_t qPq, int32_t &filterSamplesFlag, int32_t &indexA, int32_t &alpha,
      int32_t &beta); // 8.7.2.2
  int Filtering_process_for_edges_with_bS_less_than_4(
      const uint8_t (&p)[4], const uint8_t (&q)[4], int32_t chromaEdgeFlag,
      int32_t chromaStyleFilteringFlag, int32_t bS, int32_t beta,
      int32_t indexA, uint8_t (&pp)[3], uint8_t (&qq)[3]); // 8.7.2.3
  int Filtering_process_for_edges_for_bS_equal_to_4(
      const uint8_t (&p)[4], const uint8_t (&q)[4], int32_t chromaEdgeFlag,
      int32_t chromaStyleFilteringFlag, int32_t alpha, int32_t beta,
      uint8_t (&pp)[3], uint8_t (&qq)[3]); // 8.7.2.4
};

#endif /* end of include guard: PICTUREBASE_HPP_ZGHBMJIH */
