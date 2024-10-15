#ifndef DEBLOCKINGFILTER_HPP_CO4HVBTK
#define DEBLOCKINGFILTER_HPP_CO4HVBTK

#include "MacroBlock.hpp"
#include "PictureBase.hpp"
#include <cstdint>
class DeblockingFilter {
 private:
  PictureBase *pic = nullptr;

  // 在场模式下是否应用帧内滤波
  bool fieldModeInFrameFilteringFlag = false;
  // 宏块是场编码的一部分还是帧编码的一部分
  bool fieldMbInFrameFlag = false;

  // 垂直边缘(0)或水平边缘(1)
  bool verticalEdgeFlag = false;
  // 对宏块的左边缘进行滤波
  bool leftMbEdgeFlag = false;
  // 色度（U和V）边缘进行滤波
  bool chromaEdgeFlag = false;

  // -------------- 临时引用变量(别名) ------------
  bool transform_size_8x8_flag = false;
  bool MbaffFrameFlag = false;
  uint32_t ChromaArrayType = 0;
  int32_t mb_field_decoding_flag = false;
  int32_t SubWidthC = 0;
  int32_t SubHeightC = 0;
  uint32_t BitDepthY = 0;
  uint32_t BitDepthC = 0;
  //-----------------------------------------------

 public:
  int deblocking_filter_process(PictureBase *picture);

 private:
  int filtering_for_block_edges(int32_t _CurrMbAddr, int32_t iCbCr,
                                int32_t mbAddrN, int32_t (&E)[16][2]);
  int filtering_for_a_set_of_samples_across_a_horizontal_or_vertical_block_edge(
      int32_t _CurrMbAddr, int32_t isChromaCb, uint8_t mb_x_p0, uint8_t mb_y_p0,
      uint8_t mb_x_q0, uint8_t mb_y_q0, int32_t mbAddrN, const uint8_t (&p)[4],
      const uint8_t (&q)[4], uint8_t (&pp)[3], uint8_t (&qq)[3]);
  int derivation_the_luma_content_dependent_boundary_filtering_strength(
      int32_t p0, int32_t q0, uint8_t mb_x_p0, uint8_t mb_y_p0, uint8_t mb_x_q0,
      uint8_t mb_y_q0, int32_t mbAddr_p0, int32_t mbAddr_q0, int32_t &bS);
  int derivation_for_the_thresholds_for_each_block_edge(
      int32_t p0, int32_t q0, int32_t p1, int32_t q1, int32_t bS,
      int32_t filterOffsetA, int32_t filterOffsetB, int32_t qPp, int32_t qPq,
      int32_t &filterSamplesFlag, int32_t &indexA, int32_t &alpha,
      int32_t &beta);
  int filtering_for_edges_with_bS_less_than_4(const uint8_t (&p)[4],
                                              const uint8_t (&q)[4],
                                              int32_t chromaStyleFilteringFlag,
                                              int32_t bS, int32_t beta,
                                              int32_t indexA, uint8_t (&pp)[3],
                                              uint8_t (&qq)[3]);
  int filtering_for_edges_for_bS_equal_to_4(const uint8_t (&p)[4],
                                            const uint8_t (&q)[4],
                                            int32_t chromaStyleFilteringFlag,
                                            int32_t alpha, int32_t beta,
                                            uint8_t (&pp)[3], uint8_t (&qq)[3]);

  int process_filterLeftMbEdge(bool _chromaEdgeFlag, int32_t _CurrMbAddr,
                               int32_t mbAddrN, int32_t (&E)[16][2]);
  int process_filterInternalEdges(bool _verticalEdgeFlag, int32_t _CurrMbAddr,
                                  int32_t mbAddrN, int32_t (&E)[16][2]);
  int process_filterInternalEdges_chrome(bool _verticalEdgeFlag,
                                         int32_t _CurrMbAddr, int32_t mbAddrN,
                                         int32_t (&E)[16][2]);
  int process_filterTopMbEdge(bool _chromaEdgeFlag, int32_t _CurrMbAddr,
                              int32_t mbAddrN, int32_t (&E)[16][2]);
};

#endif /* end of include guard: DEBLOCKINGFILTER_HPP_CO4HVBTK */
