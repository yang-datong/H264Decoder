#include "PictureBase.hpp"

// 6.4.8 Derivation process of the availability for macroblock addresses
int PictureBase::derivation_of_availability_for_macroblock_addresses(
    int32_t mbAddr, int32_t &is_mbAddr_available) {
  is_mbAddr_available = 1;
  if (mbAddr < 0 || mbAddr > CurrMbAddr ||
      m_mbs[mbAddr].slice_number != m_mbs[CurrMbAddr].slice_number)
    is_mbAddr_available = 0;
  return 0;
}

/* 6.4.8 Derivation process of the availability for macroblock addresses */
int PictureBase::derivation_of_availability_macroblock_addresses(
    int32_t _mbAddr, int32_t CurrMbAddr, MB_ADDR_TYPE &mbAddrN_type,
    int32_t &mbAddrN) {

  //宏块被标记为不可用
  if (_mbAddr < 0 || _mbAddr > CurrMbAddr ||
      m_mbs[_mbAddr].slice_number != m_mbs[CurrMbAddr].slice_number)
    mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -1;
  //宏块被标记为可用
  else
    mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = _mbAddr;
  return 0;
}

/* 6.4.9 Derivation process for neighbouring macroblock addresses and their availability */
/* 该过程的输出为：
 * – mbAddrA：当前宏块左侧宏块的地址和可用性状态，
 * – mbAddrB：当前宏块上方宏块的地址和可用性状态，
 * – mbAddrC：地址和可用性状态
 * – mbAddrD：当前宏块左上宏块的地址和可用状态。

 Figure 6-12 – Neighbouring macroblocks for a given macroblock
 +-----+-----+-----+
 |  D  |  B  |  C  |
 +-----+-----+-----+
 |  A  | Addr|     |
 +-----+-----+-----+
 |     |     |     |
 +-----+-----+-----+
 */
int PictureBase::derivation_for_neighbouring_macroblock_addr_availability(
    int32_t xN, int32_t yN, int32_t maxW, int32_t maxH, int32_t CurrMbAddr,
    MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN) {

  mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
  mbAddrN = -1;

  int32_t mbAddrA = CurrMbAddr - 1;
  int32_t mbAddrB = CurrMbAddr - PicWidthInMbs;
  int32_t mbAddrC = CurrMbAddr - PicWidthInMbs + 1;
  int32_t mbAddrD = CurrMbAddr - PicWidthInMbs - 1;

  /* Table 6-3 – Specification of mbAddrN */
  /* 左宏快 */
  if (xN < 0 && (yN >= 0 && yN <= maxH - 1)) {
    /* 第 6.4.8 节中的过程的输入是 mbAddrA = CurrMbAddr − 1，输出是宏块 mbAddrA 是否可用。此外，当 CurrMbAddr % PicWidthInMbs 等于 0 时，mbAddrA 被标记为不可用。*/
    derivation_of_availability_macroblock_addresses(mbAddrA, CurrMbAddr,
                                                    mbAddrN_type, mbAddrN);
    if (CurrMbAddr % PicWidthInMbs == 0)
      mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -1;
    else
      mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA;
  }
  /* 上宏快 */
  else if ((xN >= 0 && xN <= maxW - 1) && yN < 0) {
    /* 第 6.4.8 节中的过程的输入是 mbAddrB = CurrMbAddr - PicWidthInMbs，输出是宏块 mbAddrB 是否可用。 */
    derivation_of_availability_macroblock_addresses(mbAddrB, CurrMbAddr,
                                                    mbAddrN_type, mbAddrN);
  }
  /* 右上宏快 */
  else if (xN > maxW - 1 && yN < 0) {
    /* 第 6.4.8 节中的过程的输入是 mbAddrC = CurrMbAddr − PicWidthInMbs + 1，输出是宏块 mbAddrC 是否可用。此外，当 (CurrMbAddr + 1) % PicWidthInMbs 等于 0 时，mbAddrC 被标记为不可用 */
    derivation_of_availability_macroblock_addresses(mbAddrC, CurrMbAddr,
                                                    mbAddrN_type, mbAddrN);
    if ((CurrMbAddr + 1) % PicWidthInMbs == 0)
      mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -1;
    else
      mbAddrN_type = MB_ADDR_TYPE_mbAddrC, mbAddrN = mbAddrC;
  }
  /* 左上宏快 */
  else if (xN < 0 && yN < 0) {
    /* 第 6.4.8 节中的过程的输入是 mbAddrD = CurrMbAddr − PicWidthInMbs − 1，输出是宏块 mbAddrD 是否可用。此外，当 CurrMbAddr % PicWidthInMbs 等于 0 时，mbAddrD 被标记为不可用 */
    derivation_of_availability_macroblock_addresses(mbAddrD, CurrMbAddr,
                                                    mbAddrN_type, mbAddrN);
    if (CurrMbAddr % PicWidthInMbs == 0)
      mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -1;
    else
      mbAddrN_type = MB_ADDR_TYPE_mbAddrD, mbAddrN = mbAddrD;
  } else if ((xN >= 0 && xN <= maxW - 1) && (yN >= 0 && yN <= maxH - 1))
    /* 当前宏块 */
    mbAddrN_type = MB_ADDR_TYPE_CurrMbAddr, mbAddrN = CurrMbAddr;
  else if ((xN > maxW - 1) && (yN >= 0 && yN <= maxH - 1)) {
    /* not available */
  } else if (yN > maxH - 1) {
    /* not available */
  }
  return 0;
}

/* 6.4.10 Derivation process for neighbouring macroblock addresses and their availability in MBAFF frames */
/* 该过程的输出为：
 * – mbAddrA：当前宏块对左侧宏块对顶部宏块的地址和可用性状态，
 * – mbAddrB：当前宏块对上方宏块对顶部宏块的地址和可用性状态当前宏块对，
 * – mbAddrC：当前宏块对右上方宏块对的顶部宏块的地址和可用性状态，
 * – mbAddrD：当前宏块对左上方宏块对的顶部宏块的地址和可用性状态当前宏块对。

 Figure 6-13 – Neighbouring macroblocks for a given macroblock in MBAFF frames
 +-----+-----+-----+
 |  D  |  B  |  C  |
 +-----+-----+-----+
 |     |     |     |
 +-----+-----+-----+
 |  A  | Addr|     |
 +-----+-----+-----+
 |     | Addr|     |
 +-----+-----+-----+
 */
int PictureBase::
    derivation_for_neighbouring_macroblock_addr_availability_in_MBAFF(
        int32_t &mbAddrA, int32_t &mbAddrB, int32_t &mbAddrC,
        int32_t &mbAddrD) {

  /* 第 6.4.8 节中的过程的输入是 mbAddrA = 2 * ( CurrMbAddr / 2 − 1 )，输出是宏块 mbAddrA 是否可用。此外，当 (CurrMbAddr / 2) % PicWidthInMbs 等于 0 时，mbAddrA 被标记为不可用。 */
  mbAddrA = 2 * (CurrMbAddr / 2 - 1);
  mbAddrB = 2 * (CurrMbAddr / 2 - PicWidthInMbs);
  mbAddrC = 2 * (CurrMbAddr / 2 - PicWidthInMbs + 1);
  mbAddrD = 2 * (CurrMbAddr / 2 - PicWidthInMbs - 1);

  if (mbAddrA < 0 || mbAddrA > CurrMbAddr ||
      m_mbs[mbAddrA].slice_number != m_mbs[CurrMbAddr].slice_number ||
      (CurrMbAddr / 2) % PicWidthInMbs == 0)
    mbAddrA = -2;

  if (mbAddrB < 0 || mbAddrB > CurrMbAddr ||
      m_mbs[mbAddrB].slice_number != m_mbs[CurrMbAddr].slice_number)
    mbAddrB = -2;

  if (mbAddrC < 0 || mbAddrC > CurrMbAddr ||
      m_mbs[mbAddrC].slice_number != m_mbs[CurrMbAddr].slice_number ||
      (CurrMbAddr / 2 + 1) % PicWidthInMbs == 0)
    mbAddrC = -2;

  if (mbAddrD < 0 || mbAddrD > CurrMbAddr ||
      m_mbs[mbAddrD].slice_number != m_mbs[CurrMbAddr].slice_number ||
      (CurrMbAddr / 2) % PicWidthInMbs == 0)
    mbAddrD = -2;

  return 0;
}

// 6.4.11.1 Derivation process for neighbouring macroblocks
/* 该过程的输出为： 
 * – mbAddrA：当前宏块左侧宏块的地址及其可用性状态， 
 * – mbAddrB：当前宏块上方宏块的地址及其可用性状态。*/
int PictureBase::derivation_for_neighbouring_macroblocks(int32_t MbaffFrameFlag,
                                                         int32_t currMbAddr,
                                                         int32_t &mbAddrA,
                                                         int32_t &mbAddrB,
                                                         int32_t isChroma) {

  int32_t xW = 0, yW = 0;

  /* mbAddrA：当前宏块左侧宏块的地址及其可用性状态 */
  MB_ADDR_TYPE mbAddrA_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma4x4BlkIdxA = 0, luma8x8BlkIdxA = 0;
  int32_t xA = -1, yA = 0;
  RET(derivation_for_neighbouring_locations(
      MbaffFrameFlag, xA, yA, currMbAddr, mbAddrA_type, mbAddrA, luma4x4BlkIdxA,
      luma8x8BlkIdxA, xW, yW, isChroma));

  /* mbAddrB：当前宏块上方宏块的地址及其可用性状态 */
  MB_ADDR_TYPE mbAddrB_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma4x4BlkIdxB = 0, luma8x8BlkIdxB = 0;
  int32_t xB = 0, yB = -1;
  RET(derivation_for_neighbouring_locations(
      MbaffFrameFlag, xB, yB, currMbAddr, mbAddrB_type, mbAddrB, luma4x4BlkIdxB,
      luma8x8BlkIdxB, xW, yW, isChroma));

  return 0;
}

// 6.4.11.2 Derivation process for neighbouring 8x8 luma block
int PictureBase::derivation_for_neighbouring_8x8_luma_block(
    int32_t luma8x8BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
    int32_t &luma8x8BlkIdxA, int32_t &luma8x8BlkIdxB, int32_t isChroma) {

  int32_t xW = 0, yW = 0;

  //---------------mbAddrA---------------------
  MB_ADDR_TYPE mbAddrA_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma4x4BlkIdxA = 0;
  int32_t xA = (luma8x8BlkIdx % 2) * 8 - 1;
  int32_t yA = (luma8x8BlkIdx / 2) * 8 + 0;

  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xA, yA, CurrMbAddr, mbAddrA_type,
      mbAddrA, luma4x4BlkIdxA, luma8x8BlkIdxA, xW, yW, isChroma));

  if (mbAddrA < 0)
    luma8x8BlkIdxA = -2;
  else {
    // 6.4.13.3 Derivation process for 8x8 luma block indices
    luma8x8BlkIdxA = 2 * (yW / 8) + (xW / 8);
  }

  //---------------mbAddrB---------------------
  MB_ADDR_TYPE mbAddrB_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma4x4BlkIdxB = 0;
  int32_t xB = (luma8x8BlkIdx % 2) * 8 - 0;
  int32_t yB = (luma8x8BlkIdx / 2) * 8 - 1;

  // 6.4.12 Derivation process for neighbouring locations
  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xB, yB, CurrMbAddr, mbAddrB_type,
      mbAddrB, luma4x4BlkIdxB, luma8x8BlkIdxB, xW, yW, isChroma));

  if (mbAddrB < 0)
    luma8x8BlkIdxB = -2;
  else {
    // 6.4.13.3 Derivation process for 8x8 luma block indices
    luma8x8BlkIdxB = 2 * (yW / 8) + (xW / 8);
  }

  return 0;
}

// 6.4.11.3 Derivation process for neighbouring 8x8 chroma blocks for ChromaArrayType equal to 3
int PictureBase::derivation_for_neighbouring_8x8_chroma_blocks_for_YUV444(
    int32_t chroma8x8BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
    int32_t &chroma8x8BlkIdxA, int32_t &chroma8x8BlkIdxB) {
  return derivation_for_neighbouring_8x8_luma_block(
      chroma8x8BlkIdx, mbAddrA, mbAddrB, chroma8x8BlkIdxA, chroma8x8BlkIdxB, 1);
}

// 6.4.11.4 Derivation process for neighbouring 4x4 luma blocks
int PictureBase::derivation_for_neighbouring_4x4_luma_blocks(
    int32_t luma4x4BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
    int32_t &luma4x4BlkIdxA, int32_t &luma4x4BlkIdxB, int32_t isChroma) {

  int32_t xW = 0, yW = 0;
  int32_t x = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
              InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
  int32_t y = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
              InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

  //---------------mbAddrA---------------------
  MB_ADDR_TYPE mbAddrA_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxA = 0;

  int32_t xA = x - 1, yA = y + 0;

  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xA, yA, CurrMbAddr, mbAddrA_type,
      mbAddrA, luma4x4BlkIdxA, luma8x8BlkIdxA, xW, yW, isChroma));

  if (mbAddrA < 0) {
    luma4x4BlkIdxA = -2;
  } else {
    // 6.4.13.1 Derivation process for 4x4 luma block indices
    luma4x4BlkIdxA =
        8 * (yW / 8) + 4 * (xW / 8) + 2 * ((yW % 8) / 4) + ((xW % 8) / 4);
  }

  //---------------mbAddrB---------------------
  MB_ADDR_TYPE mbAddrB_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxB = 0;
  int32_t xB = x + 0, yB = y - 1;

  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xB, yB, CurrMbAddr, mbAddrB_type,
      mbAddrB, luma4x4BlkIdxB, luma8x8BlkIdxB, xW, yW, isChroma));

  if (mbAddrB < 0)
    luma4x4BlkIdxB = -2;
  else
    luma4x4BlkIdxB =
        8 * (yW / 8) + 4 * (xW / 8) + 2 * ((yW % 8) / 4) + ((xW % 8) / 4);

  return 0;
}

// 6.4.11.5 Derivation process for neighbouring 4x4 chroma blocks
int PictureBase::derivation_for_neighbouring_4x4_chroma_blocks(
    int32_t chroma4x4BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
    int32_t &chroma4x4BlkIdxA, int32_t &chroma4x4BlkIdxB) {

  int32_t xW = 0, yW = 0;
  int32_t x = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 0);
  int32_t y = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 1);

  //---------------mbAddrA---------------------
  MB_ADDR_TYPE mbAddrA_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxA = 0;
  int32_t xA = x - 1, yA = y + 0;

  // 6.4.12 Derivation process for neighbouring locations
  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xA, yA, CurrMbAddr, mbAddrA_type,
      mbAddrA, chroma4x4BlkIdxA, luma8x8BlkIdxA, xW, yW, 1));

  if (mbAddrA < 0)
    chroma4x4BlkIdxA = -2;
  else {
    // 6.4.13.2 Derivation process for 4x4 chroma block indices
    chroma4x4BlkIdxA = 2 * (yW / 4) + (xW / 4);
  }

  //---------------mbAddrB---------------------
  MB_ADDR_TYPE mbAddrB_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxB = 0;
  int32_t xB = x + 0, yB = y - 1;
  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xB, yB, CurrMbAddr, mbAddrB_type,
      mbAddrB, chroma4x4BlkIdxB, luma8x8BlkIdxB, xW, yW, 1));

  if (mbAddrB < 0)
    chroma4x4BlkIdxB = -2;
  else
    // 6.4.13.2 Derivation process for 4x4 chroma block indices
    chroma4x4BlkIdxB = 2 * (yW / 4) + (xW / 4);

  return 0;
}

// 6.4.12 Derivation process for neighbouring locations
/* 输入: 相对于当前宏块左上角表示的亮度或色度位置 ( xN, yN )
 * 输出：
 * – mbAddrN：等于 CurrMbAddr 或等于包含 (xN, yN) 及其可用性状态的相邻宏块的地址，
 * – ( xW, yW )：相对于当前宏块表示的位置 (xN, yN) mbAddrN 宏块的左上角（而不是相对于当前宏块的左上角）。
 * */
int PictureBase::derivation_for_neighbouring_locations(
    int32_t MbaffFrameFlag, int32_t xN, int32_t yN, int32_t currMbAddr,
    MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN, int32_t &b4x4BlkIdxN,
    int32_t &b8x8BlkIdxN, int32_t &xW, int32_t &yW, int32_t isChroma) {
  /* 邻近的亮,色度位置调用此过程 */
  int32_t maxW = (isChroma) ? MbWidthC : 16, maxH = (isChroma) ? MbHeightC : 16;

  if (MbaffFrameFlag == 0) {
    RET(neighbouring_locations_non_MBAFF(xN, yN, maxW, maxH, currMbAddr,
                                         mbAddrN_type, mbAddrN, b4x4BlkIdxN,
                                         b8x8BlkIdxN, xW, yW, isChroma));
  } else
    RET(neighbouring_locations_MBAFF(xN, yN, maxW, maxH, currMbAddr,
                                     mbAddrN_type, mbAddrN, b4x4BlkIdxN,
                                     b8x8BlkIdxN, xW, yW, isChroma));
  return 0;
}

// 6.4.12.2 Specification for neighbouring locations in MBAFF frames
// Table 6-4 – Specification of mbAddrN and yM
int PictureBase::neighbouring_locations_MBAFF(
    int32_t xN, int32_t yN, int32_t maxW, int32_t maxH, int32_t CurrMbAddr,
    MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN, int32_t &b4x4BlkIdxN,
    int32_t &b8x8BlkIdxN, int32_t &xW, int32_t &yW, int32_t isChroma) {

  int32_t yM = 0;
  mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -1;

  /* 第 6.4.10 节中相邻宏块地址及其可用性的推导过程是通过 mbAddrA、mbAddrB、mbAddrC 和 mbAddrD 以及它们的可用性状态作为输出来调用的。 */
  int32_t mbAddrA, mbAddrB, mbAddrC, mbAddrD;
  /* Table 6-4 – Specification of mbAddrN and yM */
  derivation_for_neighbouring_macroblock_addr_availability_in_MBAFF(
      mbAddrA, mbAddrB, mbAddrC, mbAddrD);

  bool currMbFrameFlag = m_mbs[CurrMbAddr].mb_field_decoding_flag == 0;
  bool mbIsTopMbFlag = CurrMbAddr % 2 == 0;

  /* Table 6-4 – Specification of mbAddrN and yM */
  int32_t mbAddrX = -1, mbAddrXFrameFlag = 0;
  if (xN < 0 && yN < 0) {
    if (currMbFrameFlag) {
      if (mbIsTopMbFlag)
        mbAddrX = mbAddrD, mbAddrN_type = MB_ADDR_TYPE_mbAddrD_add_1,
        mbAddrN = mbAddrD + 1, yM = yN;
      else if (mbIsTopMbFlag == false) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA, yM = yN;
          else
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
            yM = (yN + maxH) >> 1;
        }
      }
    } else if (currMbFrameFlag == false) {
      if (mbIsTopMbFlag) {
        mbAddrX = mbAddrD;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrD_add_1, mbAddrN = mbAddrD + 1,
            yM = 2 * yN;
          else
            mbAddrN_type = MB_ADDR_TYPE_mbAddrD, mbAddrN = mbAddrD, yM = yN;
        }
      } else if (mbIsTopMbFlag == false) {
        mbAddrX = mbAddrD, mbAddrN_type = MB_ADDR_TYPE_mbAddrD_add_1,
        mbAddrN = mbAddrD + 1, yM = yN;
      }
    }
  } else if (xN < 0 && (yN >= 0 && yN <= maxH - 1)) {
    if (currMbFrameFlag) {
      if (mbIsTopMbFlag) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA, yM = yN;
          else {
            if (yN % 2 == 0)
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA,
              yM = yN >> 1;
            else if (yN % 2 != 0)
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
              yM = yN >> 1;
          }
        }
      } else if (mbIsTopMbFlag == false) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag == 1) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
            yM = yN;
          } else {
            if (yN % 2 == 0)
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA,
              yM = (yN + maxH) >> 1;
            else if (yN % 2 != 0)
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
              yM = (yN + maxH) >> 1;
          }
        }
      }
    } else if (currMbFrameFlag == false) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag) {
            if (yN < (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA,
              yM = yN << 1;
            } else if (yN >= (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
              yM = (yN << 1) - maxH;
            }
          } else
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA, yM = yN;
        }
      } else if (mbIsTopMbFlag == false) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag) {
            if (yN < (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA,
              yM = (yN << 1) + 1;
            } else if (yN >= (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
              yM = (yN << 1) + 1 - maxH;
            }
          } else
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
            yM = yN;
        }
      }
    }
  } else if ((xN >= 0 && xN <= maxW - 1) && yN < 0) {
    if (currMbFrameFlag) {
      if (mbIsTopMbFlag) {
        mbAddrX = mbAddrB, mbAddrN_type = MB_ADDR_TYPE_mbAddrB_add_1,
        mbAddrN = mbAddrB + 1, yM = yN;
      } else if (mbIsTopMbFlag == false) {
        mbAddrX = CurrMbAddr, mbAddrN_type = MB_ADDR_TYPE_CurrMbAddr_minus_1,
        mbAddrN = CurrMbAddr - 1, yM = yN;
      }
    } else if (currMbFrameFlag == false) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrB;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrB_add_1, mbAddrN = mbAddrB + 1,
            yM = 2 * yN;
          else if (mbAddrXFrameFlag == 0)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrB, mbAddrN = mbAddrB, yM = yN;
        }
      } else
        mbAddrX = mbAddrB, mbAddrN_type = MB_ADDR_TYPE_mbAddrB_add_1,
        mbAddrN = mbAddrB + 1, yM = yN;
    }
  } else if ((xN >= 0 && xN <= maxW - 1) && (yN >= 0 && yN <= maxH - 1)) {
    mbAddrX = CurrMbAddr, mbAddrN_type = MB_ADDR_TYPE_CurrMbAddr,
    mbAddrN = CurrMbAddr, yM = yN;
  } else if (xN > maxW - 1 && yN < 0) {
    if (currMbFrameFlag) {
      if (mbIsTopMbFlag)
        mbAddrX = mbAddrC, mbAddrN_type = MB_ADDR_TYPE_mbAddrC_add_1,
        mbAddrN = mbAddrC + 1, yM = yN;
      else if (mbIsTopMbFlag == 0)
        mbAddrX = -2, mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -2, yM = NA;
    } else {
      if (mbIsTopMbFlag) {
        mbAddrX = mbAddrC;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrC_add_1, mbAddrN = mbAddrC + 1,
            yM = 2 * yN;
          else
            mbAddrN_type = MB_ADDR_TYPE_mbAddrC, mbAddrN = mbAddrC, yM = yN;
        }
      } else if (mbIsTopMbFlag == 0) {
        mbAddrX = mbAddrC, mbAddrN_type = MB_ADDR_TYPE_mbAddrC_add_1,
        mbAddrN = mbAddrC + 1, yM = yN;
      }
    }
  } else {
    // not available
  }
  if (mbAddrN < 0) mbAddrN_type = MB_ADDR_TYPE_UNKOWN;

  if (mbAddrN_type != MB_ADDR_TYPE_UNKOWN) {
    xW = (xN + maxW) % maxW;
    yW = (yM + maxH) % maxH;

    // 6.4.13.2 Derivation process for 4x4 chroma block indices
    if (isChroma == 1) b4x4BlkIdxN = 2 * (yW / 4) + (xW / 4);
    // 6.4.13.1 Derivation process for 4x4 luma block indices
    else
      b4x4BlkIdxN =
          8 * (yW / 8) + 4 * (xW / 8) + 2 * ((yW % 8) / 4) + ((xW % 8) / 4);

    b8x8BlkIdxN = 2 * (yW / 8) + (xW / 8);
  } else
    b4x4BlkIdxN = b8x8BlkIdxN = NA;

  return 0;
}

// 6.4.12.1 Specification for neighbouring locations in fields and non-MBAFF frames
int PictureBase::neighbouring_locations_non_MBAFF(
    int32_t xN, int32_t yN, int32_t maxW, int32_t maxH, int32_t CurrMbAddr,
    MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN, int32_t &b4x4BlkIdx,
    int32_t &b8x8BlkIdxN, int32_t &xW, int32_t &yW, int32_t isChroma) {

  mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
  mbAddrN = -1;

  /* 第 6.4.9 节中相邻宏块地址及其可用性的推导过程是通过 mbAddrA、mbAddrB、mbAddrC 和 mbAddrD 以及它们的可用性状态作为输出来调用的。
   * Table 6-3 specifies mbAddrN depending on ( xN, yN ). */
  derivation_for_neighbouring_macroblock_addr_availability(
      xN, yN, maxW, maxH, CurrMbAddr, mbAddrN_type, mbAddrN);

  if (mbAddrN_type != MB_ADDR_TYPE_UNKOWN) {
    /* 相对于宏块 mbAddrN 左上角的相邻位置 ( xW, yW )  */
    xW = (xN + maxW) % maxW;
    yW = (yN + maxH) % maxH;

    /* For 4x4 Block */
    if (!isChroma)
      // 6.4.13.1 Derivation process for 4x4 luma block indices
      b4x4BlkIdx =
          8 * (yW / 8) + 4 * (xW / 8) + 2 * ((yW % 8) / 4) + ((xW % 8) / 4);
    else
      // 6.4.13.2 Derivation process for 4x4 chroma block indices
      b4x4BlkIdx = 2 * (yW / 4) + (xW / 4);

    /* For 8x8 Block */
    // 6.4.13.3 Derivation process for 8x8 luma block indices
    b8x8BlkIdxN = 2 * (yW / 8) + (xW / 8);
  } else
    b4x4BlkIdx = b8x8BlkIdxN = -1;

  return 0;
}
