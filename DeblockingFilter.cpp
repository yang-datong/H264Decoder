#include "DeblockingFilter.hpp"
#include "MacroBlock.hpp"
#include "PictureBase.hpp"
#include <cstdint>

bool UseMacroblockLeftEdgeFiltering(bool MbaffFrameFlag, int32_t mbAddr,
                                    int32_t mbAddrA, int32_t PicWidthInMbs,
                                    int32_t disable_deblocking_filter_idc) {
  bool filterLeftMbEdgeFlag = true;
  // 1. 非MBAFF模式，且宏块位于行的开始，不进行滤波
  if (MbaffFrameFlag == false && mbAddr % PicWidthInMbs == 0)
    filterLeftMbEdgeFlag = false;
  // 2. Mbaff模式，且宏块为宏块对中的首宏块，不进行滤波
  else if (MbaffFrameFlag && (mbAddr >> 1) % PicWidthInMbs == 0)
    filterLeftMbEdgeFlag = false;
  // 3. 去块滤波被完全禁用，不进行滤波
  else if (disable_deblocking_filter_idc == 1)
    filterLeftMbEdgeFlag = false;
  // 4. 去块滤波只在片边界禁用，但相邻左宏块不可用，不进行滤波
  else if (disable_deblocking_filter_idc == 2 && mbAddrA < 0)
    filterLeftMbEdgeFlag = false;
  return filterLeftMbEdgeFlag;
}

bool UseMacroblockTopEdgeFiltering(bool MbaffFrameFlag, int32_t mbAddr,
                                   int32_t mbAddrB, int32_t PicWidthInMbs,
                                   int32_t disable_deblocking_filter_idc,
                                   bool mb_field_decoding_flag) {
  bool filterTopMbEdgeFlag = true;
  // 1. 非MBAFF模式，且宏块位于首行，不进行滤波
  if (MbaffFrameFlag == false && mbAddr < PicWidthInMbs)
    filterTopMbEdgeFlag = false;
  // 2. Mbaff模式，且宏块对中为首行（场宏块），不进行滤波
  else if (MbaffFrameFlag && (mbAddr >> 1) < PicWidthInMbs &&
           mb_field_decoding_flag)
    filterTopMbEdgeFlag = false;
  // 3. Mbaff模式，且宏块对中为首行（帧宏块），且为帧宏块的首宏块，不进行滤波
  else if (MbaffFrameFlag && (mbAddr >> 1) < PicWidthInMbs &&
           mb_field_decoding_flag == false && (mbAddr % 2) == 0)
    filterTopMbEdgeFlag = false;
  // 4. 去块滤波被完全禁用，不进行滤波
  else if (disable_deblocking_filter_idc == 1)
    filterTopMbEdgeFlag = false;
  // 5. 去块滤波只在片边界禁用，但相邻上宏块不可用，不进行滤波
  else if (disable_deblocking_filter_idc == 2 && mbAddrB < 0)
    filterTopMbEdgeFlag = false;
  return filterTopMbEdgeFlag;
}

// 8.7 Deblocking filter process
int DeblockingFilter::deblocking_filter_process(PictureBase *picture) {
  this->pic = picture;
  this->ChromaArrayType = pic->m_slice->slice_header->m_sps->ChromaArrayType;
  this->SubWidthC = pic->m_slice->slice_header->m_sps->SubWidthC;
  this->SubHeightC = pic->m_slice->slice_header->m_sps->SubHeightC;
  this->BitDepthY = pic->m_slice->slice_header->m_sps->BitDepthY;
  this->BitDepthC = pic->m_slice->slice_header->m_sps->BitDepthC;

  // 宏块内部的,左,上边缘进行滤波
  for (int32_t mbAddr = 0; mbAddr < pic->PicSizeInMbs; mbAddr++) {
    const MacroBlock &mb = pic->m_mbs[mbAddr];
    this->MbaffFrameFlag = mb.MbaffFrameFlag;
    this->mb_field_decoding_flag = mb.mb_field_decoding_flag;
    this->transform_size_8x8_flag = mb.transform_size_8x8_flag;

    this->fieldModeInFrameFilteringFlag = false;
    this->fieldMbInFrameFlag = (MbaffFrameFlag && mb.mb_field_decoding_flag);
    this->verticalEdgeFlag = false;
    this->leftMbEdgeFlag = false;
    this->chromaEdgeFlag = false;

    int32_t mbAddrA = 0, mbAddrB = 0;
    RET(pic->derivation_for_neighbouring_macroblocks(MbaffFrameFlag, mbAddr,
                                                     mbAddrA, mbAddrB, 0));

    // 是否对宏块左边缘进行滤波
    bool filterLeftMbEdgeFlag = UseMacroblockLeftEdgeFiltering(
        MbaffFrameFlag, mbAddr, mbAddrA, pic->PicWidthInMbs,
        mb.disable_deblocking_filter_idc);
    // 是否对宏块上边缘进行滤波
    bool filterTopMbEdgeFlag = UseMacroblockTopEdgeFiltering(
        MbaffFrameFlag, mbAddr, mbAddrB, pic->PicWidthInMbs,
        mb.disable_deblocking_filter_idc, mb.mb_field_decoding_flag);
    // 是否对宏块的内部的边缘进行滤波
    bool filterInternalEdgesFlag = (mb.disable_deblocking_filter_idc != 1);

    //  水平、垂直方向上的像素偏移值
    int32_t E[16][2] = {{0}};

    // 对宏块的左边缘进行滤波
    if (filterLeftMbEdgeFlag)
      process_filterLeftMbEdge(false, mbAddr, mbAddrA, E);

    // 对宏块的内部的边缘进行滤波（水平方向）
    if (filterInternalEdgesFlag)
      process_filterInternalEdges(true, mbAddr, mbAddrA, E);

    // 对宏块的上边缘进行滤波
    if (filterTopMbEdgeFlag) process_filterTopMbEdge(false, mbAddr, mbAddrB, E);

    // 对宏块的内部的边缘进行滤波（垂直方向）
    if (filterInternalEdgesFlag)
      process_filterInternalEdges(false, mbAddr, mbAddrA, E);

    if (ChromaArrayType != 0) {
      // 对宏块的左边缘进行滤波
      if (filterLeftMbEdgeFlag)
        process_filterLeftMbEdge(true, mbAddr, mbAddrA, E);

      // 对宏块的内部的边缘进行滤波（水平方向）
      if (filterInternalEdgesFlag)
        process_filterInternalEdges_chrome(true, mbAddr, mbAddrA, E);

      // 对宏块的上边缘进行滤波
      if (filterTopMbEdgeFlag)
        process_filterTopMbEdge(true, mbAddr, mbAddrB, E);

      // 对宏块的内部的边缘进行滤波（垂直方向）
      if (filterInternalEdgesFlag)
        process_filterInternalEdges_chrome(false, mbAddr, mbAddrA, E);
    }
  }

  return 0;
}

// currMbAddr：当前宏块 mbAddrN：参考的宏块
int DeblockingFilter::process_filterLeftMbEdge(bool _chromaEdgeFlag,
                                               int32_t currMbAddr,
                                               int32_t mbAddrN,
                                               int32_t (&E)[16][2]) {

  //垂直的的左边缘
  verticalEdgeFlag = true, chromaEdgeFlag = _chromaEdgeFlag,
  fieldModeInFrameFilteringFlag = fieldMbInFrameFlag;

  // MBAFF模式，当前宏块为Slice的至少第3个，当前宏块是帧宏块，同时前两个宏块是场模式宏块，则需要对宏块的左边缘进行处理。NOTE: 主要是解决帧与场模式宏块之间的不连续性
  leftMbEdgeFlag = false;
  if (MbaffFrameFlag && currMbAddr >= 2 && mb_field_decoding_flag == false &&
      pic->m_mbs[(currMbAddr - 2)].mb_field_decoding_flag)
    leftMbEdgeFlag = true;

  int32_t n = chromaEdgeFlag ? pic->MbHeightC : 16;
  for (int32_t k = 0; k < n; k++)
    E[k][0] = 0, E[k][1] = k;

  RET(filtering_for_block_edges(currMbAddr, 0, mbAddrN, E));
  if (chromaEdgeFlag) RET(filtering_for_block_edges(currMbAddr, 1, mbAddrN, E));
  return 0;
}

int DeblockingFilter::process_filterTopMbEdge(bool _chromaEdgeFlag,
                                              int32_t currMbAddr,
                                              int32_t mbAddrN,
                                              int32_t (&E)[16][2]) {

  verticalEdgeFlag = false, leftMbEdgeFlag = false,
  chromaEdgeFlag = _chromaEdgeFlag,
  fieldModeInFrameFilteringFlag = fieldMbInFrameFlag;

  int32_t n = chromaEdgeFlag ? pic->MbHeightC : 16;
  if (MbaffFrameFlag && (currMbAddr % 2) == 0 &&
      currMbAddr >= 2 * pic->PicWidthInMbs && mb_field_decoding_flag == false &&
      pic->m_mbs[(currMbAddr - 2 * pic->PicWidthInMbs + 1)]
          .mb_field_decoding_flag) {
    fieldModeInFrameFilteringFlag = true;
    for (int32_t k = 0; k < n - !chromaEdgeFlag; k++)
      E[k][0] = k, E[k][1] = 0;

    RET(filtering_for_block_edges(currMbAddr, 0, mbAddrN - !chromaEdgeFlag, E));
    if (chromaEdgeFlag)
      RET(filtering_for_block_edges(currMbAddr, 1, mbAddrN, E));

    for (int32_t k = 0; k < n; k++)
      E[k][0] = k, E[k][1] = 1;

  } else {
    for (int32_t k = 0; k < n; k++)
      E[k][0] = k, E[k][1] = 0;
  }

  RET(filtering_for_block_edges(currMbAddr, 0, mbAddrN, E));
  if (chromaEdgeFlag) RET(filtering_for_block_edges(currMbAddr, 1, mbAddrN, E));
  return 0;
}

int DeblockingFilter::process_filterInternalEdges(bool _verticalEdgeFlag,
                                                  int32_t currMbAddr,
                                                  int32_t mbAddrN,
                                                  int32_t (&E)[16][2]) {
  chromaEdgeFlag = false, verticalEdgeFlag = _verticalEdgeFlag;
  leftMbEdgeFlag = false;
  fieldModeInFrameFilteringFlag = fieldMbInFrameFlag;

  if (transform_size_8x8_flag == false) {
    for (int32_t k = 0; k < 16; k++) {
      if (verticalEdgeFlag)
        E[k][0] = 4, E[k][1] = k;
      else
        E[k][0] = k, E[k][1] = 4;
    }

    RET(filtering_for_block_edges(currMbAddr, -1, mbAddrN, E));
  }

  for (int32_t k = 0; k < 16; k++) {
    if (verticalEdgeFlag)
      E[k][0] = 8, E[k][1] = k;
    else
      E[k][0] = k, E[k][1] = 8;
  }

  RET(filtering_for_block_edges(currMbAddr, -1, mbAddrN, E));

  if (transform_size_8x8_flag == false) {
    for (int32_t k = 0; k < 16; k++) {
      if (verticalEdgeFlag)
        E[k][0] = 12, E[k][1] = k;
      else
        E[k][0] = k, E[k][1] = 12;
    }

    RET(filtering_for_block_edges(currMbAddr, -1, mbAddrN, E));
  }
  return 0;
}

int DeblockingFilter::process_filterInternalEdges_chrome(bool _verticalEdgeFlag,
                                                         int32_t currMbAddr,
                                                         int32_t mbAddrN,
                                                         int32_t (&E)[16][2]) {

  chromaEdgeFlag = true, verticalEdgeFlag = _verticalEdgeFlag,
  leftMbEdgeFlag = false;
  fieldModeInFrameFilteringFlag = fieldMbInFrameFlag;

  if (ChromaArrayType != 3 || transform_size_8x8_flag == false) {
    for (int32_t k = 0; k < pic->MbHeightC; k++)
      if (verticalEdgeFlag)
        E[k][0] = 4, E[k][1] = k;
      else
        E[k][0] = k, E[k][1] = 4;

    RET(filtering_for_block_edges(currMbAddr, 0, mbAddrN, E));
    RET(filtering_for_block_edges(currMbAddr, 1, mbAddrN, E));
  }

  if (verticalEdgeFlag) {
    if (ChromaArrayType == 3) {
      for (int32_t k = 0; k < pic->MbHeightC; k++)
        E[k][0] = 8, E[k][1] = k;

      RET(filtering_for_block_edges(currMbAddr, 0, mbAddrN, E));
      RET(filtering_for_block_edges(currMbAddr, 1, mbAddrN, E));
    }
  } else {
    if (ChromaArrayType != 1) {
      for (int32_t k = 0; k < pic->MbWidthC - 1; k++)
        E[k][0] = k, E[k][1] = 8;

      RET(filtering_for_block_edges(currMbAddr, 0, mbAddrN, E));
      RET(filtering_for_block_edges(currMbAddr, 1, mbAddrN, E));
    }

    if (ChromaArrayType == 2) {
      for (int32_t k = 0; k < pic->MbWidthC; k++)
        E[k][0] = k, E[k][1] = 12;

      RET(filtering_for_block_edges(currMbAddr, 0, mbAddrN, E));
      RET(filtering_for_block_edges(currMbAddr, 1, mbAddrN, E));
    }
  }

  // YUV444 水平，垂直方向都需要过滤两次
  if (ChromaArrayType == 3 && transform_size_8x8_flag == false) {
    for (int32_t k = 0; k < pic->MbHeightC; k++)
      if (verticalEdgeFlag)
        E[k][0] = 12, E[k][1] = k;
      else
        E[k][0] = k, E[k][1] = 12;

    RET(filtering_for_block_edges(currMbAddr, 0, mbAddrN, E));
    RET(filtering_for_block_edges(currMbAddr, 1, mbAddrN, E));
  }
  return 0;
}

// 8.7.1 Filtering process for block edges
// iCbCr: 0 -> Cb , 1 -> Cr , -1 -> Y
int DeblockingFilter::filtering_for_block_edges(int32_t currMbAddr,
                                                int32_t iCbCr, int32_t mbAddrN,
                                                int32_t (&E)[16][2]) {

  int32_t picWidth = 0;
  uint8_t *pic_buff = nullptr;

  // 判断处理的是 亮度 或 色度 分量
  if (chromaEdgeFlag == false)
    pic_buff = pic->m_pic_buff_luma, picWidth = pic->PicWidthInSamplesL;
  else if (chromaEdgeFlag && iCbCr == 0)
    pic_buff = pic->m_pic_buff_cb, picWidth = pic->PicWidthInSamplesC;
  else if (chromaEdgeFlag && iCbCr == 1)
    pic_buff = pic->m_pic_buff_cr, picWidth = pic->PicWidthInSamplesC;

  int32_t xI = 0, yI = 0;
  pic->inverse_mb_scanning_process(MbaffFrameFlag, currMbAddr,
                                   mb_field_decoding_flag, xI, yI);

  // 不同色度子采样的宽度,亮度位置
  int32_t xP = xI, yP = yI;
  if (chromaEdgeFlag)
    xP = xI / SubWidthC, yP = (yI + SubHeightC - 1) / SubHeightC;

  // 亮度、色度分量和边缘方向（垂直 或 水平），确定滤波操作的范围nE，即处理多少行或列的像素
  int32_t nE = 16;
  if (chromaEdgeFlag) nE = (verticalEdgeFlag) ? pic->MbHeightC : pic->MbWidthC;

  // 循环遍历每一个要处理的边缘宏块（由E数组定义），对每个像素位置执行滤波
  int32_t dy = 1 + fieldModeInFrameFilteringFlag;
  for (int32_t k = 0; k < nE; k++) {
    // 当前处理边缘的两侧像素值，inside[]是边缘内侧，outside[]是边缘外侧
    uint8_t inside[4] = {0}, outside[4] = {0};
    // 加上不同的边缘滤波指定的像素偏移
    int32_t y = yP + dy * E[k][1], x = xP + E[k][0];
    for (int32_t i = 0; i < 4; i++) {
      // 根据边缘方向，从图像缓冲区中获取相应的像素值
      if (verticalEdgeFlag)
        outside[i] = pic_buff[y * picWidth + x + i],
        inside[i] = pic_buff[y * picWidth + x - (i + 1)];
      else
        y = yP - (E[k][1] % 2),
        outside[i] = pic_buff[(y + dy * (E[k][1] + i)) * picWidth + x],
        inside[i] = pic_buff[(y + dy * (E[k][1] - (i + 1))) * picWidth + x];
    }

    int32_t mbAddr_p0 = currMbAddr;
    int8_t mb_x_inside = 0, mb_y_inside = 0, mb_x_outside = 0, mb_y_outside = 0;
    // 处理垂直边缘
    if (verticalEdgeFlag) {
      // 边缘内侧、外侧的像素坐标，在垂直方向上靠内侧需要-1
      mb_x_inside = E[k][0] - 1, mb_y_inside = E[k][1];
      mb_x_outside = E[k][0], mb_y_outside = E[k][1];
      // 边缘内侧的像素位置在当前宏块外时，调整相关参数以引用正确的宏块
      if (mb_x_inside < 0) {
        mb_x_inside += (chromaEdgeFlag) ? 8 : 16;
        //相邻宏块可用
        if (mbAddrN >= 0)
          mbAddr_p0 = (leftMbEdgeFlag) ? mbAddrN + (E[k][1] % 2) : mbAddrN;
      }
    }
    // 处理水平边缘
    else {
      // 边缘内侧、外侧的像素坐标，在水平方向上靠内侧需要-1
      mb_x_inside = E[k][0], mb_y_inside = E[k][1] - (E[k][1] % 2) - 1,
      mb_x_outside = E[k][0], mb_y_outside = E[k][1] - (E[k][1] % 2);
      if (mb_y_inside < 0) {
        mb_y_inside += (chromaEdgeFlag) ? 8 : 16;
        if (mbAddrN >= 0) mbAddr_p0 = mbAddrN;
      }
    }

    uint8_t pp[3] = {0}, qq[3] = {0};
    RET(_filtering_for_block_edges(currMbAddr, iCbCr, mb_x_inside, mb_y_inside,
                                   mb_x_outside, mb_y_outside, mbAddr_p0,
                                   inside, outside, pp, qq));
    // 使用滤波后的样本
    for (int32_t i = 0; i < 3; i++) {
      if (verticalEdgeFlag) {
        pic_buff[y * picWidth + x + i] = qq[i];
        pic_buff[y * picWidth + x - i - 1] = pp[i];
      } else {
        y = yP - (E[k][1] % 2);
        pic_buff[(y + dy * (E[k][1] + i)) * picWidth + x] = qq[i];
        pic_buff[(y + dy * (E[k][1] - i - 1)) * picWidth + x] = pp[i];
      }
    }
  }

  return 0;
}

// 8.7.2 Filtering process for a set of samples across a horizontal or vertical block edge
int DeblockingFilter::_filtering_for_block_edges(
    int32_t currMbAddr, int32_t isChromaCb, uint8_t mb_x_inside,
    uint8_t mb_y_inside, uint8_t mb_x_outside, uint8_t mb_y_outside,
    int32_t mbAddrN, const uint8_t (&inside)[4], const uint8_t (&outside)[4],
    uint8_t (&pp)[3], uint8_t (&qq)[3]) {

  // 根据亮度或色度分量的不同，计算用于决定滤波强度的bS值，该值是通过比较边界两侧的样本值来确定
  int32_t bS = 0, mbAddr_inside = mbAddrN, mbAddr_outside = currMbAddr;
  if (chromaEdgeFlag == false) {
    RET(derivation_luma_content_dependent_boundary_filtering_strength(
        inside[0], outside[0], mb_x_inside, mb_y_inside, mb_x_outside,
        mb_y_outside, mbAddr_inside, mbAddr_outside, bS));
  } else
    RET(derivation_luma_content_dependent_boundary_filtering_strength(
        inside[0], outside[0], SubWidthC * mb_x_inside,
        SubHeightC * mb_y_inside, SubWidthC * mb_x_outside,
        SubHeightC * mb_y_outside, mbAddr_inside, mbAddr_outside, bS));

  const MacroBlock &mb_inside = pic->m_mbs[mbAddr_inside];
  const MacroBlock &mb_outside = pic->m_mbs[mbAddr_outside];

  int32_t qPInside = 0, qPOutside = 0;
  if (chromaEdgeFlag == false) {
    qPInside = (mb_inside.m_name_of_mb_type != I_PCM) ? mb_inside.QPY : 0;
    qPOutside = (mb_outside.m_name_of_mb_type != I_PCM) ? mb_outside.QPY : 0;
  } else {
    int32_t QPY = 0;
    if (mb_inside.m_name_of_mb_type != I_PCM) QPY = mb_inside.QPY;
    RET(pic->get_chroma_quantisation_parameters2(QPY, isChromaCb, qPInside));

    if (mb_outside.m_name_of_mb_type != I_PCM) QPY = mb_outside.QPY;
    RET(pic->get_chroma_quantisation_parameters2(QPY, isChromaCb, qPOutside));
  }

  const int32_t filterOffsetA = pic->m_mbs[mbAddr_outside].FilterOffsetA;
  const int32_t filterOffsetB = pic->m_mbs[mbAddr_outside].FilterOffsetB;

  // 计算滤波的阈值（alpha, beta），以及最后确认是否进行滤波
  bool filterSamplesFlag = false;
  int32_t indexA = 0, alpha = 0, beta = 0;
  RET(derivation_thresholds_for_each_block_edge(
      inside[0], outside[0], inside[1], outside[1], bS, filterOffsetA,
      filterOffsetB, qPInside, qPOutside, filterSamplesFlag, indexA, alpha,
      beta));

  // YUV420,YUV422
  int32_t chromaStyleFilteringFlag = chromaEdgeFlag && (ChromaArrayType != 3);
  // 进行滤波操作(相对某个宏块而言)
  if (filterSamplesFlag) {
    if (bS < 4) {
      /* 低强度滤波 */
      RET(filtering_for_edges_for_low_strength(
          inside, outside, chromaStyleFilteringFlag, bS, beta, indexA, pp, qq));
    } else
      /* 高强度滤波 */
      RET(filtering_for_edges_for_high_strength(
          inside, outside, chromaStyleFilteringFlag, alpha, beta, pp, qq));
  }
  // 不进行滤波操作，复制原有的样本(相对某个宏块而言)
  else {
    pp[0] = inside[0], pp[1] = inside[1], pp[2] = inside[2];
    qq[0] = outside[0], qq[1] = outside[1], qq[2] = outside[2];
  }

  return 0;
}

// 8.7.2.3 Filtering process for edges with bS less than 4
int DeblockingFilter::filtering_for_edges_for_low_strength(
    const uint8_t (&p)[4], const uint8_t (&q)[4],
    int32_t chromaStyleFilteringFlag, int32_t bS, int32_t beta, int32_t indexA,
    uint8_t (&pp)[3], uint8_t (&qq)[3]) {

  // Table 8-17 – Value of variable t´C0 as a function of indexA and bS indexA
  int32_t ttC0[3][52] = {
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0, 0,
       0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1,  2, 2, 2,
       2, 3, 3, 3, 4, 4, 4, 5, 6, 6, 7, 8, 9, 10, 11, 13},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
       0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,  1,  1,  2,  2,  2, 2, 3,
       3, 3, 4, 4, 5, 5, 6, 7, 8, 8, 10, 11, 12, 13, 15, 17},
      {0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 1,
       1, 1, 1, 1, 1, 1, 1, 1,  1,  2,  2,  2,  2,  3,  3,  3, 4, 4,
       4, 5, 6, 6, 7, 8, 9, 10, 11, 13, 14, 16, 18, 20, 23, 25}};

  // tC0 基于 bS 和 滤波的阈值alpha，为滤波强度的基本阈值，位深越大，则强度越高
  int32_t tC0 = 0;
  if (chromaEdgeFlag == false)
    tC0 = ttC0[bS - 1][indexA] * (1 << (BitDepthY - 8));
  else
    tC0 = ttC0[bS - 1][indexA] * (1 << (BitDepthC - 8));

  int32_t ap = ABS(p[2] - p[0]), aq = ABS(q[2] - q[0]);

  // 基本阈值 tC0 可以根据边界像素值的差异进一步调整。对于亮度(包括YUV444的色度)的滤波，则 tC 可以基于 beta 阈值进一步增加
  int32_t tC = tC0 + 1;
  if (chromaStyleFilteringFlag == false)
    tC = tC0 + ((ap < beta) ? 1 : 0) + ((aq < beta) ? 1 : 0);

  /* 
   q[0] - p[0]：计算边界两侧最靠近边界的像素之间的差值
   ((q[0] - p[0]) << 2)：将差值乘以4。放大差值，使得滤波算法对边界的变化更敏感
   p[1] - q[1]：计算边界内外各自第二个像素之间的差值，评估边界两边像素值的整体趋势
   + 4：为了后续的>>3准备，目的是实现数值的四舍五入
   >> 3：将总和除以8, 将前面计算的结果缩放回合适的范围，以便用作实际的像素调整值。
 */
  int32_t delta = CLIP3(-tC, tC, (((q[0] - p[0]) << 2) + p[1] - q[1] + 4) >> 3);

  int32_t BitDepth = (chromaEdgeFlag) ? BitDepthC : BitDepthY;
  pp[0] = Clip1C(p[0] + delta, BitDepth);
  qq[0] = Clip1C(q[0] - delta, BitDepth);

  // 色度(或子采样的像素）则不进行滤波
  pp[1] = p[1], qq[1] = q[1];
  pp[2] = p[2], qq[2] = q[2];
  // 亮度(未子采样的像素，亮度，YUV444）则进行滤波
  if (chromaStyleFilteringFlag == false && ap < beta)
    pp[1] = p[1] + CLIP3(-tC0, tC0,
                         (p[2] + ((p[0] + q[0] + 1) >> 1) - (p[1] << 1)) >> 1);

  if (chromaStyleFilteringFlag == false && aq < beta)
    qq[1] = q[1] + CLIP3(-tC0, tC0,
                         (q[2] + ((p[0] + q[0] + 1) >> 1) - (q[1] << 1)) >> 1);

  return 0;
}

// 8.7.2.4 Filtering process for edges for bS equal to 4
int DeblockingFilter::filtering_for_edges_for_high_strength(
    const uint8_t (&p)[4], const uint8_t (&q)[4],
    int32_t chromaStyleFilteringFlag, int32_t alpha, int32_t beta,
    uint8_t (&pp)[3], uint8_t (&qq)[3]) {

  // 计算边界内外第一个和第三个像素之间的差异，用于评估边界的局部平滑性
  int32_t ap = ABS(p[2] - p[0]), aq = ABS(q[2] - q[0]);

  // 1. 边界足够平滑，适合进行整体像素的滤波
  if (chromaStyleFilteringFlag == false &&
      (ap < beta && ABS(p[0] - q[0]) < ((alpha >> 2) + 2))) {
    pp[0] = (p[2] + 2 * p[1] + 2 * p[0] + 2 * q[0] + q[1] + 4) >> 3;
    pp[1] = (p[2] + p[1] + p[0] + q[0] + 2) >> 2;
    pp[2] = (2 * p[3] + 3 * p[2] + p[1] + p[0] + q[0] + 4) >> 3;
  }
  // 2. 边界不够平滑，适合进行相邻像素的滤波
  else {
    pp[0] = (2 * p[1] + p[0] + q[1] + 2) >> 2;
    pp[1] = p[1];
    pp[2] = p[2];
  }

  if (chromaStyleFilteringFlag == false &&
      (aq < beta && ABS(p[0] - q[0]) < ((alpha >> 2) + 2))) {
    qq[0] = (p[1] + 2 * p[0] + 2 * q[0] + 2 * q[1] + q[2] + 4) >> 3;
    qq[1] = (p[0] + q[0] + q[1] + q[2] + 2) >> 2;
    qq[2] = (2 * q[3] + 3 * q[2] + q[1] + q[0] + p[0] + 4) >> 3;
  } else {
    qq[0] = (2 * q[1] + q[0] + p[1] + 2) >> 2;
    qq[1] = q[1];
    qq[2] = q[2];
  }

  return 0;
}

// 8.7.2.1 Derivation process for the luma content dependent boundary filtering strength
int DeblockingFilter::
    derivation_luma_content_dependent_boundary_filtering_strength(
        int32_t p0, int32_t q0, uint8_t mb_x_p0, uint8_t mb_y_p0,
        uint8_t mb_x_q0, uint8_t mb_y_q0, int32_t mbAddr_p0, int32_t mbAddr_q0,
        int32_t &bS) {

  bS = 0;
  MacroBlock &mb_p = pic->m_mbs[mbAddr_p0];
  MacroBlock &mb_q = pic->m_mbs[mbAddr_q0];
  const bool mb_field_p = mb_p.mb_field_decoding_flag;
  const bool mb_field_q = mb_q.mb_field_decoding_flag;
  const int32_t slice_type_p = mb_p.m_slice_type;
  const int32_t slice_type_q = mb_q.m_slice_type;
  const H264_MB_PART_PRED_MODE &mb_pred_mode_p = mb_p.m_mb_pred_mode;
  const H264_MB_PART_PRED_MODE &mb_pred_mode_q = mb_q.m_mb_pred_mode;

  bool mixedModeEdgeFlag = false;
  if (MbaffFrameFlag && mbAddr_p0 != mbAddr_q0 && mb_field_p != mb_field_q)
    mixedModeEdgeFlag = true;

  if (mbAddr_p0 != mbAddr_q0) {
    if (mb_field_p == false && mb_field_q == false &&
        (IS_INTRA_Prediction_Mode(mb_pred_mode_p) ||
         IS_INTRA_Prediction_Mode(mb_pred_mode_q)))
      bS = 4;
    else if (mb_field_p == false && mb_field_q == false &&
             (slice_type_p == SLICE_SP || slice_type_p == SLICE_SI ||
              slice_type_q == SLICE_SP || slice_type_q == SLICE_SI))
      bS = 4;
    else if ((MbaffFrameFlag || mb_q.field_pic_flag) && verticalEdgeFlag &&
             (IS_INTRA_Prediction_Mode(mb_pred_mode_p) ||
              IS_INTRA_Prediction_Mode(mb_pred_mode_q)))
      bS = 4;
    else if ((MbaffFrameFlag || mb_q.field_pic_flag) && verticalEdgeFlag &&
             (slice_type_p == SLICE_SP || slice_type_p == SLICE_SI ||
              slice_type_q == SLICE_SP || slice_type_q == SLICE_SI))
      bS = 4;
  }
  if (bS == 4) return 0;

  if (mixedModeEdgeFlag == false && (IS_INTRA_Prediction_Mode(mb_pred_mode_p) ||
                                     IS_INTRA_Prediction_Mode(mb_pred_mode_q)))
    bS = 3;
  else if (mixedModeEdgeFlag == false &&
           (slice_type_p == SLICE_SP || slice_type_p == SLICE_SI ||
            slice_type_q == SLICE_SP || slice_type_q == SLICE_SI))
    bS = 3;
  else if (mixedModeEdgeFlag && verticalEdgeFlag == false &&
           (IS_INTRA_Prediction_Mode(mb_pred_mode_p) ||
            IS_INTRA_Prediction_Mode(mb_pred_mode_q)))
    bS = 3;
  else if (mixedModeEdgeFlag && verticalEdgeFlag == false &&
           (slice_type_p == SLICE_SP || slice_type_p == SLICE_SI ||
            slice_type_q == SLICE_SP || slice_type_q == SLICE_SI))
    bS = 3;
  if (bS == 3) return 0;

  // 6.4.13.1 Derivation process for 4x4 luma block indices
  uint8_t luma4x4BlkIdx_p0 = 0, luma4x4BlkIdx_q0 = 0;
  luma4x4BlkIdx_p0 = 8 * (mb_y_p0 / 8) + 4 * (mb_x_p0 / 8) +
                     2 * ((mb_y_p0 % 8) / 4) + ((mb_x_p0 % 8) / 4);
  luma4x4BlkIdx_q0 = 8 * (mb_y_q0 / 8) + 4 * (mb_x_q0 / 8) +
                     2 * ((mb_y_q0 % 8) / 4) + ((mb_x_q0 % 8) / 4);

  // 6.4.13.3 Derivation process for 8x8 luma block indices
  uint8_t luma8x8BlkIdx_p0 = 0, luma8x8BlkIdx_q0 = 0;
  luma8x8BlkIdx_p0 = 2 * (mb_y_p0 / 8) + (mb_x_p0 / 8);
  luma8x8BlkIdx_q0 = 2 * (mb_y_q0 / 8) + (mb_x_q0 / 8);

  if (mb_p.transform_size_8x8_flag &&
      mb_p.mb_luma_8x8_non_zero_count_coeff[luma8x8BlkIdx_p0] > 0)
    bS = 2;
  else if (mb_p.transform_size_8x8_flag == false &&
           mb_p.mb_luma_4x4_non_zero_count_coeff[luma4x4BlkIdx_p0] > 0)
    bS = 2;
  else if (mb_q.transform_size_8x8_flag &&
           mb_q.mb_luma_8x8_non_zero_count_coeff[luma8x8BlkIdx_q0] > 0)
    bS = 2;
  else if (mb_q.transform_size_8x8_flag == false &&
           mb_q.mb_luma_4x4_non_zero_count_coeff[luma4x4BlkIdx_q0] > 0)
    bS = 2;
  if (bS == 2) return 0;

  int32_t mv_y_diff = 4;
  if (pic->m_picture_coded_type == TOP_FIELD ||
      pic->m_picture_coded_type == BOTTOM_FIELD)
    mv_y_diff = 2;
  else if (MbaffFrameFlag && mb_field_q)
    mv_y_diff = 2;

  int32_t mbPartIdx_p0 = 0, subMbPartIdx_p0 = 0;
  RET(pic->derivation_macroblock_and_sub_macroblock_partition_indices(
      mb_p.m_name_of_mb_type, mb_p.m_name_of_sub_mb_type, mb_x_p0, mb_y_p0,
      mbPartIdx_p0, subMbPartIdx_p0));

  int32_t mbPartIdx_q0 = 0, subMbPartIdx_q0 = 0;
  RET(pic->derivation_macroblock_and_sub_macroblock_partition_indices(
      mb_q.m_name_of_mb_type, mb_q.m_name_of_sub_mb_type, mb_x_q0, mb_y_q0,
      mbPartIdx_q0, subMbPartIdx_q0));

  // -- 别名 --
  const int32_t PredFlagL0_p0 = mb_p.m_PredFlagL0[mbPartIdx_p0];
  const int32_t PredFlagL1_p0 = mb_p.m_PredFlagL1[mbPartIdx_p0];
  const int32_t PredFlagL0_q0 = mb_q.m_PredFlagL0[mbPartIdx_q0];
  const int32_t PredFlagL1_q0 = mb_q.m_PredFlagL1[mbPartIdx_q0];

  const int32_t MvL0_p0_x = mb_p.m_MvL0[mbPartIdx_p0][subMbPartIdx_p0][0];
  const int32_t MvL0_p0_y = mb_p.m_MvL0[mbPartIdx_p0][subMbPartIdx_p0][1];
  const int32_t MvL0_q0_x = mb_q.m_MvL0[mbPartIdx_q0][subMbPartIdx_q0][0];
  const int32_t MvL0_q0_y = mb_q.m_MvL0[mbPartIdx_q0][subMbPartIdx_q0][1];

  const int32_t MvL1_p0_x = mb_p.m_MvL1[mbPartIdx_p0][subMbPartIdx_p0][0];
  const int32_t MvL1_p0_y = mb_p.m_MvL1[mbPartIdx_p0][subMbPartIdx_p0][1];
  const int32_t MvL1_q0_x = mb_q.m_MvL1[mbPartIdx_q0][subMbPartIdx_q0][0];
  const int32_t MvL1_q0_y = mb_q.m_MvL1[mbPartIdx_q0][subMbPartIdx_q0][1];

  if (mixedModeEdgeFlag) {
    bS = 1;
    return 0;
  } else if (mixedModeEdgeFlag == false) {
    Frame *RefPicList0_p0 =
        (mb_p.m_RefIdxL0[mbPartIdx_p0] >= 0)
            ? pic->m_RefPicList0[mb_p.m_RefIdxL0[mbPartIdx_p0]]
            : nullptr;
    Frame *RefPicList1_p0 =
        (mb_p.m_RefIdxL1[mbPartIdx_p0] >= 0)
            ? pic->m_RefPicList1[mb_p.m_RefIdxL1[mbPartIdx_p0]]
            : nullptr;
    Frame *RefPicList0_q0 =
        (mb_q.m_RefIdxL0[mbPartIdx_q0] >= 0)
            ? pic->m_RefPicList0[mb_q.m_RefIdxL0[mbPartIdx_q0]]
            : nullptr;
    Frame *RefPicList1_q0 =
        (mb_q.m_RefIdxL1[mbPartIdx_q0] >= 0)
            ? pic->m_RefPicList1[mb_q.m_RefIdxL1[mbPartIdx_q0]]
            : nullptr;

    // p0和q0有不同的参考图片，或者p0和q0有不同数量的运动向量
    if (((RefPicList0_p0 == RefPicList0_q0 &&
          RefPicList1_p0 == RefPicList1_q0) ||
         (RefPicList0_p0 == RefPicList1_q0 &&
          RefPicList1_p0 == RefPicList0_q0)) &&
        (PredFlagL0_p0 + PredFlagL1_p0) == (PredFlagL0_q0 + PredFlagL1_q0)) {
      // do nothing
    } else {
      bS = 1;
      return 0;
    }

    // 一个运动矢量用于预测p0,一个运动矢量用于预测q0
    if ((PredFlagL0_p0 && PredFlagL1_p0 == 0) &&
        (PredFlagL0_q0 && PredFlagL1_q0 == 0) &&
        (ABS(MvL0_p0_x - MvL0_q0_x) >= 4 ||
         ABS(MvL0_p0_y - MvL0_q0_y) >= mv_y_diff))
      bS = 1;
    else if ((PredFlagL0_p0 && PredFlagL1_p0 == 0) &&
             (PredFlagL0_q0 == 0 && PredFlagL1_q0) &&
             (ABS(MvL0_p0_x - MvL1_q0_x) >= 4 ||
              ABS(MvL0_p0_y - MvL1_q0_y) >= mv_y_diff))
      bS = 1;
    else if ((PredFlagL0_p0 == 0 && PredFlagL1_p0) &&
             (PredFlagL0_q0 && PredFlagL1_q0 == 0) &&
             (ABS(MvL1_p0_x - MvL0_q0_x) >= 4 ||
              ABS(MvL1_p0_y - MvL0_q0_y) >= mv_y_diff))
      bS = 1;
    else if ((PredFlagL0_p0 == 0 && PredFlagL1_p0) &&
             (PredFlagL0_q0 == 0 && PredFlagL1_q0) &&
             (ABS(MvL1_p0_x - MvL1_q0_x) >= 4 ||
              ABS(MvL1_p0_y - MvL1_q0_y) >= mv_y_diff))
      bS = 1;
    if (bS == 1) return 0;

    // p0有两个不同的参考图片，q0也有两个不同的参考图片，并且q0的这两个参考图片和p0的两个参考图片是一样的
    if ((PredFlagL0_p0 && PredFlagL1_p0) &&
        (RefPicList0_p0 != RefPicList1_p0) &&
        (PredFlagL0_q0 && PredFlagL1_q0) &&
        ((RefPicList0_q0 == RefPicList0_p0 &&
          RefPicList1_q0 == RefPicList1_p0) ||
         (RefPicList0_q0 == RefPicList1_p0 &&
          RefPicList1_q0 == RefPicList0_p0))) {
      if (RefPicList0_q0 == RefPicList0_p0 &&
          ((ABS(MvL0_p0_x - MvL0_q0_x) >= 4 ||
            ABS(MvL0_p0_y - MvL0_q0_y) >= mv_y_diff) ||
           (ABS(MvL1_p0_x - MvL1_q0_x) >= 4 ||
            ABS(MvL1_p0_y - MvL1_q0_y) >= mv_y_diff)))
        bS = 1;
      else if (RefPicList0_q0 == RefPicList1_p0 &&
               ((ABS(MvL1_p0_x - MvL0_q0_x) >= 4 ||
                 ABS(MvL1_p0_y - MvL0_q0_y) >= mv_y_diff) ||
                (ABS(MvL0_p0_x - MvL1_q0_x) >= 4 ||
                 ABS(MvL0_p0_y - MvL1_q0_y) >= mv_y_diff)))
        bS = 1;
      if (bS == 1) return 0;
    }

    // p0的两个运动矢量都来自同一张参考图片，q0的两个运动矢量也都来自同一张参考图片，并且q0的这张参考图片和p0的那张参考图片是同一张参考图片
    if ((PredFlagL0_p0 && PredFlagL1_p0) &&
        (RefPicList0_p0 == RefPicList1_p0) &&
        (PredFlagL0_q0 && PredFlagL1_q0) &&
        (RefPicList0_q0 == RefPicList1_q0) &&
        RefPicList0_q0 == RefPicList0_p0) {
      // q0的这张参考图片和p0的那张参考图片是同一张参考图片
      if ((ABS(MvL0_p0_x - MvL0_q0_x) >= 4 ||
           ABS(MvL0_p0_y - MvL0_q0_y) >= mv_y_diff) ||
          ((ABS(MvL1_p0_x - MvL1_q0_x) >= 4 ||
            ABS(MvL1_p0_y - MvL1_q0_y) >= mv_y_diff) &&
           (ABS(MvL0_p0_x - MvL1_q0_x) >= 4 ||
            ABS(MvL0_p0_y - MvL1_q0_y) >= mv_y_diff)) ||
          (ABS(MvL1_p0_x - MvL0_q0_x) >= 4 ||
           ABS(MvL1_p0_y - MvL0_q0_y) >= mv_y_diff)) {
        bS = 1;
        return 0;
      }
    }
  }

  return 0;
}

// 8.7.2.2 Derivation process for the thresholds for each block edge
// NOTE: filterOffsetA,B 是由编码器提供的阈值偏移，用于动态的控制滤波器
int DeblockingFilter::derivation_thresholds_for_each_block_edge(
    int32_t inside0, int32_t outside0, int32_t inside1, int32_t outside1,
    int32_t bS, int32_t filterOffsetA, int32_t filterOffsetB, int32_t qPp,
    int32_t qPq, bool &filterSamplesFlag, int32_t &index_alpha, int32_t &alpha,
    int32_t &beta) {

  // 内侧和外侧宏块量化参数的平均值，用于确定阈值数组中的索引
  int32_t qPav = (qPp + qPq + 1) >> 1;
  index_alpha = CLIP3(0, 51, qPav + filterOffsetA);
  int32_t index_beta = CLIP3(0, 51, qPav + filterOffsetB);
  // 根据亮度、色度位深阈值可能需要按比例调整（当视频序列的位深不是8 bits时）
  if (chromaEdgeFlag == false) {
    alpha = alpha0[index_alpha] * (1 << (BitDepthY - 8));
    beta = beta0[index_beta] * (1 << (BitDepthY - 8));
  } else {
    alpha = alpha0[index_alpha] * (1 << (BitDepthC - 8));
    beta = beta0[index_beta] * (1 << (BitDepthC - 8));
  }

  // 确保有在边缘足够“尖锐”时才进行滤波，以避免过度平滑可能导致的细节丢失。
  filterSamplesFlag =
      (bS != 0 && ABS(inside0 - outside0) < alpha &&
       ABS(inside1 - inside0) < beta && ABS(outside1 - outside0) < beta);
  return 0;
}
