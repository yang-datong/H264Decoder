﻿#include "DeblockingFilter.hpp"
#include "MacroBlock.hpp"
#include "PictureBase.hpp"
#include <cstdint>

bool UseMacroblockLeftEdgeFiltering(bool MbaffFrameFlag, int32_t mbAddr,
                                    int32_t mbAddrA, int32_t PicWidthInMbs,
                                    int32_t disable_deblocking_filter_idc) {
  bool filterLeftMbEdgeFlag = true;
  if (MbaffFrameFlag == false && mbAddr % PicWidthInMbs == 0)
    filterLeftMbEdgeFlag = false;
  else if (MbaffFrameFlag && (mbAddr >> 1) % PicWidthInMbs == 0)
    filterLeftMbEdgeFlag = false;
  else if (disable_deblocking_filter_idc == 1)
    filterLeftMbEdgeFlag = false;
  else if (disable_deblocking_filter_idc == 2 && mbAddrA < 0)
    filterLeftMbEdgeFlag = false;
  return filterLeftMbEdgeFlag;
}

bool UseMacroblockTopEdgeFiltering(bool MbaffFrameFlag, int32_t mbAddr,
                                   int32_t mbAddrB, int32_t PicWidthInMbs,
                                   int32_t disable_deblocking_filter_idc,
                                   bool mb_field_decoding_flag) {
  bool filterTopMbEdgeFlag = true;
  if (MbaffFrameFlag == false && mbAddr < PicWidthInMbs)
    filterTopMbEdgeFlag = false;
  else if (MbaffFrameFlag && (mbAddr >> 1) < PicWidthInMbs &&
           mb_field_decoding_flag)
    filterTopMbEdgeFlag = false;
  else if (MbaffFrameFlag && (mbAddr >> 1) < PicWidthInMbs &&
           mb_field_decoding_flag == 0 && (mbAddr % 2) == 0)
    filterTopMbEdgeFlag = false;
  else if (disable_deblocking_filter_idc == 1)
    filterTopMbEdgeFlag = false;
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

int DeblockingFilter::process_filterLeftMbEdge(bool _chromaEdgeFlag,
                                               int32_t _CurrMbAddr,
                                               int32_t mbAddrN,
                                               int32_t (&E)[16][2]) {

  leftMbEdgeFlag = false, verticalEdgeFlag = true,
  chromaEdgeFlag = _chromaEdgeFlag,
  fieldModeInFrameFilteringFlag = fieldMbInFrameFlag;
  if (MbaffFrameFlag && _CurrMbAddr >= 2 && mb_field_decoding_flag == false &&
      pic->m_mbs[(_CurrMbAddr - 2)].mb_field_decoding_flag)
    leftMbEdgeFlag = true;

  int32_t n = chromaEdgeFlag ? pic->MbHeightC : 16;
  for (int32_t k = 0; k < n; k++)
    E[k][0] = 0, E[k][1] = k;

  RET(filtering_for_block_edges(_CurrMbAddr, 0, mbAddrN, E));
  if (chromaEdgeFlag)
    RET(filtering_for_block_edges(_CurrMbAddr, 1, mbAddrN, E));
  return 0;
}

int DeblockingFilter::process_filterTopMbEdge(bool _chromaEdgeFlag,
                                              int32_t _CurrMbAddr,
                                              int32_t mbAddrN,
                                              int32_t (&E)[16][2]) {

  verticalEdgeFlag = false, leftMbEdgeFlag = false,
  chromaEdgeFlag = _chromaEdgeFlag,
  fieldModeInFrameFilteringFlag = fieldMbInFrameFlag;

  int32_t n = chromaEdgeFlag ? pic->MbHeightC : 16;
  if (MbaffFrameFlag && (_CurrMbAddr % 2) == 0 &&
      _CurrMbAddr >= 2 * pic->PicWidthInMbs &&
      mb_field_decoding_flag == false &&
      pic->m_mbs[(_CurrMbAddr - 2 * pic->PicWidthInMbs + 1)]
          .mb_field_decoding_flag) {
    fieldModeInFrameFilteringFlag = true;
    for (int32_t k = 0; k < n - !chromaEdgeFlag; k++)
      E[k][0] = k, E[k][1] = 0;

    RET(filtering_for_block_edges(_CurrMbAddr, 0, mbAddrN - !chromaEdgeFlag,
                                  E));
    if (chromaEdgeFlag)
      RET(filtering_for_block_edges(_CurrMbAddr, 1, mbAddrN, E));

    for (int32_t k = 0; k < n; k++)
      E[k][0] = k, E[k][1] = 1;

  } else {
    for (int32_t k = 0; k < n; k++)
      E[k][0] = k, E[k][1] = 0;
  }

  RET(filtering_for_block_edges(_CurrMbAddr, 0, mbAddrN, E));
  if (chromaEdgeFlag)
    RET(filtering_for_block_edges(_CurrMbAddr, 1, mbAddrN, E));
  return 0;
}

int DeblockingFilter::process_filterInternalEdges(bool _verticalEdgeFlag,
                                                  int32_t _CurrMbAddr,
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

    RET(filtering_for_block_edges(_CurrMbAddr, 0, mbAddrN, E));
  }

  for (int32_t k = 0; k < 16; k++) {
    if (verticalEdgeFlag)
      E[k][0] = 8, E[k][1] = k;
    else
      E[k][0] = k, E[k][1] = 8;
  }

  RET(filtering_for_block_edges(_CurrMbAddr, 0, mbAddrN, E));

  if (transform_size_8x8_flag == false) {
    for (int32_t k = 0; k < 16; k++) {
      if (verticalEdgeFlag)
        E[k][0] = 12, E[k][1] = k;
      else
        E[k][0] = k, E[k][1] = 12;
    }

    RET(filtering_for_block_edges(_CurrMbAddr, 0, mbAddrN, E));
  }
  return 0;
}

int DeblockingFilter::process_filterInternalEdges_chrome(bool _verticalEdgeFlag,
                                                         int32_t _CurrMbAddr,
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

    RET(filtering_for_block_edges(_CurrMbAddr, 0, mbAddrN, E));

    RET(filtering_for_block_edges(_CurrMbAddr, 1, mbAddrN, E));
  }

  if (verticalEdgeFlag) {
    if (ChromaArrayType == 3) {
      for (int32_t k = 0; k < pic->MbHeightC; k++)
        E[k][0] = 8, E[k][1] = k;

      RET(filtering_for_block_edges(_CurrMbAddr, 0, mbAddrN, E));

      RET(filtering_for_block_edges(_CurrMbAddr, 1, mbAddrN, E));
    }
  } else {
    if (ChromaArrayType != 1) {
      for (int32_t k = 0; k < pic->MbWidthC - 1; k++)
        E[k][0] = k, E[k][1] = 8;

      RET(filtering_for_block_edges(_CurrMbAddr, 0, mbAddrN, E));

      RET(filtering_for_block_edges(_CurrMbAddr, 1, mbAddrN, E));
    }

    if (ChromaArrayType == 2) {
      for (int32_t k = 0; k < pic->MbWidthC; k++)
        E[k][0] = k, E[k][1] = 12;

      RET(filtering_for_block_edges(_CurrMbAddr, 0, mbAddrN, E));

      RET(filtering_for_block_edges(_CurrMbAddr, 1, mbAddrN, E));
    }
  }

  // YUV444 水平，垂直方向都需要过滤两次
  if (ChromaArrayType == 3 && transform_size_8x8_flag == false) {
    for (int32_t k = 0; k < pic->MbHeightC; k++)
      if (verticalEdgeFlag)
        E[k][0] = 12, E[k][1] = k;
      else
        E[k][0] = k, E[k][1] = 12;

    RET(filtering_for_block_edges(_CurrMbAddr, 0, mbAddrN, E));
    RET(filtering_for_block_edges(_CurrMbAddr, 1, mbAddrN, E));
  }
  return 0;
}

// 8.7.1 Filtering process for block edges
int DeblockingFilter::filtering_for_block_edges(int32_t _CurrMbAddr,
                                                int32_t iCbCr, int32_t mbAddrN,
                                                int32_t (&E)[16][2]) {

  int32_t PicWidthInSamples = 0;
  uint8_t *pic_buff = nullptr;

  if (chromaEdgeFlag == false)
    pic_buff = pic->m_pic_buff_luma,
    PicWidthInSamples = pic->PicWidthInSamplesL;
  else if (chromaEdgeFlag && iCbCr == 0)
    pic_buff = pic->m_pic_buff_cb, PicWidthInSamples = pic->PicWidthInSamplesC;
  else if (chromaEdgeFlag && iCbCr == 1)
    pic_buff = pic->m_pic_buff_cr, PicWidthInSamples = pic->PicWidthInSamplesC;

  int32_t xI = 0, yI = 0;
  RET(pic->inverse_mb_scanning_process(MbaffFrameFlag, _CurrMbAddr,
                                       mb_field_decoding_flag, xI, yI));
  int32_t xP = xI, yP = yI;
  if (chromaEdgeFlag)
    xP = xI / SubWidthC, yP = (yI + SubHeightC - 1) / SubHeightC;

  int32_t nE = 16;
  if (chromaEdgeFlag != 0)
    nE = (verticalEdgeFlag) ? pic->MbHeightC : pic->MbWidthC;
  int32_t dy = (1 + fieldModeInFrameFilteringFlag);
  for (int32_t k = 0; k < nE; k++) {
    uint8_t p[4] = {0}, q[4] = {0};
    int32_t y = yP + dy * E[k][1];
    int32_t x = xP + E[k][0];
    for (int32_t i = 0; i < 4; i++) {
      if (verticalEdgeFlag) {
        q[i] = pic_buff[y * PicWidthInSamples + x + i];
        p[i] = pic_buff[y * PicWidthInSamples + x - i - 1];
      } else {
        y = yP - (E[k][1] % 2);
        q[i] = pic_buff[(y + dy * (E[k][1] + i)) * PicWidthInSamples + x];
        p[i] = pic_buff[(y + dy * (E[k][1] - i - 1)) * PicWidthInSamples + x];
      }
    }

    //-----------用于查找4x4或8x8非零系数数目------------------
    int32_t mbAddr_p0 = _CurrMbAddr;
    int8_t mb_x_p0 = 0, mb_y_p0 = 0, mb_x_q0 = 0, mb_y_q0 = 0;

    if (verticalEdgeFlag) {
      mb_x_p0 = E[k][0] - 1, mb_y_p0 = E[k][1];
      mb_x_q0 = E[k][0], mb_y_q0 = E[k][1];
      if (mb_x_p0 < 0) {
        if (mbAddrN >= 0)
          mbAddr_p0 = (leftMbEdgeFlag) ? mbAddrN + (E[k][1] % 2) : mbAddrN;
        mb_x_p0 += (chromaEdgeFlag) ? 8 : 16;
      }

    } else {
      mb_x_p0 = E[k][0], mb_y_p0 = E[k][1] - 1 - (E[k][1] % 2),
      mb_x_q0 = E[k][0], mb_y_q0 = E[k][1] - (E[k][1] % 2);
      if (mb_y_p0 < 0) {
        if (mbAddrN >= 0) mbAddr_p0 = mbAddrN;
        mb_y_p0 += (chromaEdgeFlag) ? 8 : 16;
      }
    }

    uint8_t pp[3] = {0}, qq[3] = {0};
    RET(filtering_for_a_set_of_samples_across_a_horizontal_or_vertical_block_edge(
        _CurrMbAddr, iCbCr, mb_x_p0, mb_y_p0, mb_x_q0, mb_y_q0, mbAddr_p0, p, q,
        pp, qq));

    for (int32_t i = 0; i < 3; i++) {
      if (verticalEdgeFlag) {
        pic_buff[y * PicWidthInSamples + x + i] = qq[i];
        pic_buff[y * PicWidthInSamples + x - i - 1] = pp[i];
      } else {
        y = yP - (E[k][1] % 2);
        pic_buff[(y + dy * (E[k][1] + i)) * PicWidthInSamples + x] = qq[i];
        pic_buff[(y + dy * (E[k][1] - i - 1)) * PicWidthInSamples + x] = pp[i];
      }
    }
  }

  return 0;
}

// 8.7.2 Filtering process for a set of samples across a horizontal or vertical block edge
int DeblockingFilter::
    filtering_for_a_set_of_samples_across_a_horizontal_or_vertical_block_edge(
        int32_t _CurrMbAddr, int32_t isChromaCb, uint8_t mb_x_p0,
        uint8_t mb_y_p0, uint8_t mb_x_q0, uint8_t mb_y_q0, int32_t mbAddrN,
        const uint8_t (&p)[4], const uint8_t (&q)[4], uint8_t (&pp)[3],
        uint8_t (&qq)[3]) {

  int32_t bS = 0;
  int32_t mbAddr_p0 = mbAddrN, mbAddr_q0 = _CurrMbAddr;
  if (chromaEdgeFlag == false) {
    RET(derivation_the_luma_content_dependent_boundary_filtering_strength(
        p[0], q[0], mb_x_p0, mb_y_p0, mb_x_q0, mb_y_q0, mbAddr_p0, mbAddr_q0,
        bS));
  } else {
    uint8_t mb_x_p0_chroma = SubWidthC * mb_x_p0;
    uint8_t mb_y_p0_chroma = SubHeightC * mb_y_p0;
    uint8_t mb_x_q0_chroma = SubWidthC * mb_x_q0;
    uint8_t mb_y_q0_chroma = SubHeightC * mb_y_q0;

    RET(derivation_the_luma_content_dependent_boundary_filtering_strength(
        p[0], q[0], mb_x_p0_chroma, mb_y_p0_chroma, mb_x_q0_chroma,
        mb_y_q0_chroma, mbAddr_p0, mbAddr_q0, bS));
  }

  int32_t filterOffsetA = pic->m_mbs[mbAddr_q0].FilterOffsetA;
  int32_t filterOffsetB = pic->m_mbs[mbAddr_q0].FilterOffsetB;
  int32_t qPp = 0, qPq = 0;
  if (chromaEdgeFlag == false) {
    qPp = (pic->m_mbs[mbAddr_p0].m_name_of_mb_type == I_PCM)
              ? 0
              : pic->m_mbs[mbAddr_p0].QPY;
    qPq = (pic->m_mbs[mbAddr_q0].m_name_of_mb_type == I_PCM)
              ? 0
              : pic->m_mbs[mbAddr_q0].QPY;
  } else {
    int32_t QPY = 0, QPC = 0;
    if (pic->m_mbs[mbAddr_p0].m_name_of_mb_type == I_PCM) {
      QPY = 0;
      RET(pic->get_chroma_quantisation_parameters2(QPY, isChromaCb, QPC));
      qPp = QPC;
    } else {
      QPY = pic->m_mbs[mbAddr_p0].QPY;
      RET(pic->get_chroma_quantisation_parameters2(QPY, isChromaCb, QPC));
      qPp = QPC;
    }

    if (pic->m_mbs[mbAddr_q0].m_name_of_mb_type == I_PCM) {
      QPY = 0;
      RET(pic->get_chroma_quantisation_parameters2(QPY, isChromaCb, QPC));
      qPq = QPC;
    } else {
      QPY = pic->m_mbs[mbAddr_q0].QPY;
      RET(pic->get_chroma_quantisation_parameters2(QPY, isChromaCb, QPC));
      qPq = QPC;
    }
  }

  int32_t filterSamplesFlag = 0;
  int32_t indexA = 0;
  int32_t alpha = 0, beta = 0;

  RET(derivation_for_the_thresholds_for_each_block_edge(
      p[0], q[0], p[1], q[1], bS, filterOffsetA, filterOffsetB, qPp, qPq,
      filterSamplesFlag, indexA, alpha, beta));

  int32_t chromaStyleFilteringFlag = chromaEdgeFlag && (ChromaArrayType != 3);

  if (filterSamplesFlag) {
    if (bS < 4) {
      RET(filtering_for_edges_with_bS_less_than_4(
          p, q, chromaStyleFilteringFlag, bS, beta, indexA, pp, qq));
    } else
      RET(filtering_for_edges_for_bS_equal_to_4(p, q, chromaStyleFilteringFlag,
                                                alpha, beta, pp, qq));
  } else {
    pp[0] = p[0], pp[1] = p[1], pp[2] = p[2];
    qq[0] = q[0], qq[1] = q[1], qq[2] = q[2];
  }

  return 0;
}

// 8.7.2.3 Filtering process for edges with bS less than 4
int DeblockingFilter::filtering_for_edges_with_bS_less_than_4(
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

  int32_t tC0 = 0;
  if (chromaEdgeFlag == false)
    tC0 = ttC0[bS - 1][indexA] * (1 << (BitDepthY - 8));
  else
    tC0 = ttC0[bS - 1][indexA] * (1 << (BitDepthC - 8));

  int32_t ap = ABS(p[2] - p[0]), aq = ABS(q[2] - q[0]);

  int32_t tC = tC0 + 1;
  if (chromaStyleFilteringFlag == false)
    tC = tC0 + ((ap < beta) ? 1 : 0) + ((aq < beta) ? 1 : 0);

  int32_t delta =
      CLIP3(-tC, tC, ((((q[0] - p[0]) << 2) + (p[1] - q[1]) + 4) >> 3));

  if (chromaEdgeFlag == false) {
    pp[0] = CLIP3(0, (1 << BitDepthY) - 1, p[0] + delta);
    qq[0] = CLIP3(0, (1 << BitDepthY) - 1, q[0] - delta);
  } else {
    pp[0] = CLIP3(0, (1 << BitDepthC) - 1, p[0] + delta);
    qq[0] = CLIP3(0, (1 << BitDepthC) - 1, q[0] - delta);
  }

  pp[1] = p[1];
  if (chromaStyleFilteringFlag == false && ap < beta)
    pp[1] = p[1] + CLIP3(-tC0, tC0,
                         (p[2] + ((p[0] + q[0] + 1) >> 1) - (p[1] << 1)) >> 1);

  qq[1] = q[1];
  if (chromaStyleFilteringFlag == false && aq < beta)
    qq[1] = q[1] + CLIP3(-tC0, tC0,
                         (q[2] + ((p[0] + q[0] + 1) >> 1) - (q[1] << 1)) >> 1);

  pp[2] = p[2], qq[2] = q[2];

  return 0;
}

// 8.7.2.4 Filtering process for edges for bS equal to 4
int DeblockingFilter::filtering_for_edges_for_bS_equal_to_4(
    const uint8_t (&p)[4], const uint8_t (&q)[4],
    int32_t chromaStyleFilteringFlag, int32_t alpha, int32_t beta,
    uint8_t (&pp)[3], uint8_t (&qq)[3]) {

  int32_t ap = ABS(p[2] - p[0]), aq = ABS(q[2] - q[0]);
  if (chromaStyleFilteringFlag == false &&
      (ap < beta && ABS(p[0] - q[0]) < ((alpha >> 2) + 2))) {
    // 抽头滤波器
    pp[0] = (p[2] + 2 * p[1] + 2 * p[0] + 2 * q[0] + q[1] + 4) >> 3;
    pp[1] = (p[2] + p[1] + p[0] + q[0] + 2) >> 2;
    pp[2] = (2 * p[3] + 3 * p[2] + p[1] + p[0] + q[0] + 4) >> 3;
  } else {
    pp[0] = (2 * p[1] + p[0] + q[1] + 2) >> 2;
    pp[1] = p[1];
    pp[2] = p[2];
  }

  if (chromaStyleFilteringFlag == false &&
      (aq < beta && ABS(p[0] - q[0]) < ((alpha >> 2) + 2))) {
    // 抽头滤波器
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
    derivation_the_luma_content_dependent_boundary_filtering_strength(
        int32_t p0, int32_t q0, uint8_t mb_x_p0, uint8_t mb_y_p0,
        uint8_t mb_x_q0, uint8_t mb_y_q0, int32_t mbAddr_p0, int32_t mbAddr_q0,
        int32_t &bS) {

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
         IS_INTRA_Prediction_Mode(mb_pred_mode_q))) {
      bS = 4;
      return 0;
    } else if (mb_field_p == false && mb_field_q == false &&
               (slice_type_p == SLICE_SP || slice_type_p == SLICE_SI ||
                slice_type_q == SLICE_SP || slice_type_q == SLICE_SI)) {
      bS = 4;
      return 0;
    } else if ((MbaffFrameFlag || mb_q.field_pic_flag) && verticalEdgeFlag &&
               (IS_INTRA_Prediction_Mode(mb_pred_mode_p) ||
                IS_INTRA_Prediction_Mode(mb_pred_mode_q))) {
      bS = 4;
      return 0;
    } else if ((MbaffFrameFlag || mb_q.field_pic_flag) && verticalEdgeFlag &&
               (slice_type_p == SLICE_SP || slice_type_p == SLICE_SI ||
                slice_type_q == SLICE_SP || slice_type_q == SLICE_SI)) {
      bS = 4;
      return 0;
    }
  }

  if (mixedModeEdgeFlag == false &&
      (IS_INTRA_Prediction_Mode(mb_pred_mode_p) ||
       IS_INTRA_Prediction_Mode(mb_pred_mode_q))) {
    bS = 3;
    return 0;
  } else if (mixedModeEdgeFlag == false &&
             (slice_type_p == SLICE_SP || slice_type_p == SLICE_SI ||
              slice_type_q == SLICE_SP || slice_type_q == SLICE_SI)) {
    bS = 3;
    return 0;
  } else if (mixedModeEdgeFlag && verticalEdgeFlag == false &&
             (IS_INTRA_Prediction_Mode(mb_pred_mode_p) ||
              IS_INTRA_Prediction_Mode(mb_pred_mode_q))) {
    bS = 3;
    return 0;
  } else if (mixedModeEdgeFlag && verticalEdgeFlag == false &&
             (slice_type_p == SLICE_SP || slice_type_p == SLICE_SI ||
              slice_type_q == SLICE_SP || slice_type_q == SLICE_SI)) {
    bS = 3;
    return 0;
  }

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
      mb_p.mb_luma_8x8_non_zero_count_coeff[luma8x8BlkIdx_p0] > 0) {
    bS = 2;
    return 0;
  } else if (mb_p.transform_size_8x8_flag == false &&
             mb_p.mb_luma_4x4_non_zero_count_coeff[luma4x4BlkIdx_p0] > 0) {
    bS = 2;
    return 0;
  } else if (mb_q.transform_size_8x8_flag &&
             mb_q.mb_luma_8x8_non_zero_count_coeff[luma8x8BlkIdx_q0] > 0) {
    bS = 2;
    return 0;
  } else if (mb_q.transform_size_8x8_flag == false &&
             mb_q.mb_luma_4x4_non_zero_count_coeff[luma4x4BlkIdx_q0] > 0) {
    bS = 2;
    return 0;
  }

  int32_t mv_y_diff = 4;
  if (pic->m_picture_coded_type == PICTURE_CODED_TYPE_TOP_FIELD ||
      pic->m_picture_coded_type == PICTURE_CODED_TYPE_BOTTOM_FIELD)
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
         ABS(MvL0_p0_y - MvL0_q0_y) >= mv_y_diff)) {
      bS = 1;
      return 0;
    } else if ((PredFlagL0_p0 && PredFlagL1_p0 == 0) &&
               (PredFlagL0_q0 == 0 && PredFlagL1_q0) &&
               (ABS(MvL0_p0_x - MvL1_q0_x) >= 4 ||
                ABS(MvL0_p0_y - MvL1_q0_y) >= mv_y_diff)) {
      bS = 1;
      return 0;
    } else if ((PredFlagL0_p0 == 0 && PredFlagL1_p0) &&
               (PredFlagL0_q0 && PredFlagL1_q0 == 0) &&
               (ABS(MvL1_p0_x - MvL0_q0_x) >= 4 ||
                ABS(MvL1_p0_y - MvL0_q0_y) >= mv_y_diff)) {
      bS = 1;
      return 0;
    } else if ((PredFlagL0_p0 == 0 && PredFlagL1_p0) &&
               (PredFlagL0_q0 == 0 && PredFlagL1_q0) &&
               (ABS(MvL1_p0_x - MvL1_q0_x) >= 4 ||
                ABS(MvL1_p0_y - MvL1_q0_y) >= mv_y_diff)) {
      bS = 1;
      return 0;
    }

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
            ABS(MvL1_p0_y - MvL1_q0_y) >= mv_y_diff))) {
        bS = 1;
        return 0;
      } else if (RefPicList0_q0 == RefPicList1_p0 &&
                 ((ABS(MvL1_p0_x - MvL0_q0_x) >= 4 ||
                   ABS(MvL1_p0_y - MvL0_q0_y) >= mv_y_diff) ||
                  (ABS(MvL0_p0_x - MvL1_q0_x) >= 4 ||
                   ABS(MvL0_p0_y - MvL1_q0_y) >= mv_y_diff))) {
        bS = 1;
        return 0;
      }
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

  bS = 0;
  return 0;
}

// 8.7.2.2 Derivation process for the thresholds for each block edge
int DeblockingFilter::derivation_for_the_thresholds_for_each_block_edge(
    int32_t p0, int32_t q0, int32_t p1, int32_t q1, int32_t bS,
    int32_t filterOffsetA, int32_t filterOffsetB, int32_t qPp, int32_t qPq,
    int32_t &filterSamplesFlag, int32_t &indexA, int32_t &alpha,
    int32_t &beta) {

  // Table 8-16 – Derivation of offset dependent threshold variables α´ and β´ from indexA and indexB indexA (for α′) or indexB (for β′)
  const int32_t alpha2[52] = {
      0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,  0,  0,  4,   4,   5,   6,   7,   8,   9,   10,  12,  13,
      15, 17, 20, 22,  25,  28,  32,  36,  40,  45,  50,  56,  63,
      71, 80, 90, 101, 113, 127, 144, 162, 182, 203, 226, 255, 255};

  const int32_t beta2[52] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 2,  2,
      2,  3,  3,  3,  3,  4,  4,  4,  6,  6,  7,  7,  8,  8,  9,  9, 10, 10,
      11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18};

  int32_t qPav = (qPp + qPq + 1) >> 1;
  indexA = CLIP3(0, 51, qPav + filterOffsetA);
  int32_t indexB = CLIP3(0, 51, qPav + filterOffsetB);
  if (chromaEdgeFlag == false) {
    alpha = alpha2[indexA] * (1 << (BitDepthY - 8));
    beta = beta2[indexB] * (1 << (BitDepthY - 8));
  } else {
    alpha = alpha2[indexA] * (1 << (BitDepthC - 8));
    beta = beta2[indexB] * (1 << (BitDepthC - 8));
  }

  filterSamplesFlag = (bS != 0 && ABS(p0 - q0) < alpha && ABS(p1 - p0) < beta &&
                       ABS(q1 - q0) < beta);

  return 0;
}
