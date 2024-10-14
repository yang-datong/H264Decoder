#include "Cavlc.hpp"

//7.3.5.3.2 Residual block CAVLC syntax
int Cavlc::residual_block_cavlc(int32_t *coeffLevel, int32_t startIdx,
                                int32_t endIdx, int32_t maxNumCoeff,
                                MB_RESIDUAL_LEVEL mb_residual_level,
                                int32_t MbPartPredMode, int32_t BlkIdx,
                                int32_t &TotalCoeff) {
  TotalCoeff = 0;
  std::fill_n(coeffLevel, maxNumCoeff, 0);

  // 先尝试获取16bit数据（并不是读取，比特流并没有产生位移）
  int32_t coeff_token = _bs->getUn(16), coeff_token_bit_length = 0;
  int32_t TrailingOnes = 0;
  // 查表找到对应的TrailingOnes, TotalCoeff值
  RET(process_coeff_token(mb_residual_level, MbPartPredMode, BlkIdx,
                          coeff_token, coeff_token_bit_length, TrailingOnes,
                          TotalCoeff));

  int32_t suffixLength = 0;
  if (TotalCoeff > 0) {
    suffixLength = (TotalCoeff > 10 && TrailingOnes < 3) ? 1 : 0;
    for (int32_t i = 0; i < TotalCoeff; i++) {
      if (i < TrailingOnes) {
        trailing_ones_sign_flag = _bs->readUn(1);
        levelVal[i] = 1 - 2 * trailing_ones_sign_flag;
      } else {
        int32_t leadingZeroBits = -1;
        for (int32_t b = 0; !b; leadingZeroBits++)
          b = _bs->readUn(1);
        level_prefix = leadingZeroBits;

        //---------------levelSuffixSize-----------------------------------
        int32_t levelSuffixSize = suffixLength;
        if (level_prefix == 14 && suffixLength == 0)
          levelSuffixSize = 4;
        else if (level_prefix >= 15)
          levelSuffixSize = level_prefix - 3;
        //-----------------------------------------------------------------

        int32_t levelCode = (MIN(15, level_prefix) << suffixLength);
        if (suffixLength > 0 || level_prefix >= 14) {
          level_suffix =
              (levelSuffixSize > 0) ? _bs->readUn(levelSuffixSize) : 0;
          levelCode += level_suffix;
        }
        if (level_prefix >= 15 && suffixLength == 0) levelCode += 15;
        if (level_prefix >= 16) levelCode += (1 << (level_prefix - 3)) - 4096;
        if (i == TrailingOnes && TrailingOnes < 3) levelCode += 2;
        if (levelCode % 2 == 0)
          levelVal[i] = (levelCode + 2) >> 1;
        else
          levelVal[i] = (-levelCode - 1) >> 1;
        if (suffixLength == 0) suffixLength = 1;
        if (ABS(levelVal[i]) > (3 << (suffixLength - 1)) && suffixLength < 6)
          suffixLength++;
      }
    }

    int32_t zerosLeft = 0;
    if (TotalCoeff < endIdx - startIdx + 1) {
      int32_t tzVlcIndex = TotalCoeff;
      RET(process_total_zeros(maxNumCoeff, tzVlcIndex, total_zeros));
      zerosLeft = total_zeros;
    } else
      zerosLeft = 0;

    for (int32_t i = 0; i < TotalCoeff - 1; i++) {
      if (zerosLeft > 0) {
        RET(process_run_before(zerosLeft, run_before));
        runVal[i] = run_before;
      } else
        runVal[i] = 0;
      zerosLeft = zerosLeft - runVal[i];
    }
    runVal[TotalCoeff - 1] = zerosLeft;

    int32_t coeffNum = -1;
    for (int32_t i = TotalCoeff - 1; i >= 0; i--) {
      coeffNum += runVal[i] + 1;
      coeffLevel[startIdx + coeffNum] = levelVal[i];
    }
  }

  return 0;
}

/*
 *      |               |
 *    D |       B       |    C
 *  ----+---------------+----------
 *    A | Current       |
 *      | Macroblock    |
 *      | or Partition  |
 *      | or Block      |
 */
// 9.2.1 Parsing process for total number of non-zero transform coefficient levels and number of trailing ones
int Cavlc::process_nC(MB_RESIDUAL_LEVEL mb_residual_level,
                      int32_t MbPartPredMode, int32_t BlkIdx, int32_t &nC) {
  const SliceHeader *header = _picture->m_slice->slice_header;
  const uint32_t ChromaArrayType = header->m_sps->ChromaArrayType;

  int32_t BlkIdxA = BlkIdx, BlkIdxB = BlkIdx;
  int32_t luma4x4BlkIdx = BlkIdx, chroma4x4BlkIdx = BlkIdx,
          cb4x4BlkIdx = BlkIdx, cr4x4BlkIdx = BlkIdx;
  int32_t luma4x4BlkIdxN_A = 0, cb4x4BlkIdxN_A = 0, cr4x4BlkIdxN_A = 0,
          luma4x4BlkIdxN_B = 0, cb4x4BlkIdxN_B = 0, cr4x4BlkIdxN_B = 0;
  int32_t luma8x8BlkIdxN_A = 0, luma8x8BlkIdxN_B = 0, chroma4x4BlkIdxN_A = 0,
          chroma4x4BlkIdxN_B = 0;

  MB_ADDR_TYPE mbAddrN_A_type = MB_ADDR_TYPE_UNKOWN,
               mbAddrN_B_type = MB_ADDR_TYPE_UNKOWN;
  int32_t mbAddrN_A = -1, mbAddrN_B = -1;
  int32_t CurrMbAddr = _picture->CurrMbAddr;

  // Table 6-2 – Specification of input and output assignments for clauses 6.4.11.1 to 6.4.11.7
  int32_t x = 0, y = 0, maxW = 0, maxH = 0, xW = 0, yW = 0;
  int32_t isChroma = 0;

  if (mb_residual_level == MB_RESIDUAL_ChromaDCLevelCb ||
      mb_residual_level == MB_RESIDUAL_ChromaDCLevelCr) {
    nC = (ChromaArrayType == 1) ? -1 : -2;
  } else {
    if (mb_residual_level == MB_RESIDUAL_Intra16x16DCLevel) luma4x4BlkIdx = 0;
    if (mb_residual_level == MB_RESIDUAL_CbIntra16x16DCLevel) cb4x4BlkIdx = 0;
    if (mb_residual_level == MB_RESIDUAL_CrIntra16x16DCLevel) cr4x4BlkIdx = 0;

    if (mb_residual_level == MB_RESIDUAL_Intra16x16DCLevel ||
        mb_residual_level == MB_RESIDUAL_Intra16x16ACLevel ||
        mb_residual_level == MB_RESIDUAL_LumaLevel4x4) {
      x = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
          InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
      y = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
          InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

      // 6.4.12 Derivation process for neighbouring locations
      maxW = 16, maxH = 16, isChroma = 0;
      if (header->MbaffFrameFlag == 0) {
        RET(_picture->neighbouring_locations_non_MBAFF(
            x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_A_type, mbAddrN_A,
            luma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));

        RET(_picture->neighbouring_locations_non_MBAFF(
            x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_B_type, mbAddrN_B,
            luma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
      } else {
        RET(_picture->neighbouring_locations_MBAFF(
            x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_A_type, mbAddrN_A,
            luma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));

        RET(_picture->neighbouring_locations_MBAFF(
            x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_B_type, mbAddrN_B,
            luma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
      }

      BlkIdxA = luma4x4BlkIdxN_A, BlkIdxB = luma4x4BlkIdxN_B;
    } else if (mb_residual_level == MB_RESIDUAL_CbIntra16x16DCLevel ||
               mb_residual_level == MB_RESIDUAL_CbIntra16x16DCLevel ||
               mb_residual_level == MB_RESIDUAL_CbLevel4x4) {
      RET(ChromaArrayType != 3);

      x = InverseRasterScan(cb4x4BlkIdx / 4, 8, 8, 16, 0) +
          InverseRasterScan(cb4x4BlkIdx % 4, 4, 4, 8, 0);
      y = InverseRasterScan(cb4x4BlkIdx / 4, 8, 8, 16, 1) +
          InverseRasterScan(cb4x4BlkIdx % 4, 4, 4, 8, 1);

      // 6.4.12 Derivation process for neighbouring locations
      maxW = _picture->m_slice->slice_header->m_sps->MbWidthC;
      maxH = _picture->m_slice->slice_header->m_sps->MbHeightC;
      isChroma = 0;

      if (header->MbaffFrameFlag == 0) {
        RET(_picture->neighbouring_locations_non_MBAFF(
            x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_A_type, mbAddrN_A,
            cb4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));

        RET(_picture->neighbouring_locations_non_MBAFF(
            x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_B_type, mbAddrN_B,
            cb4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
      } else {
        RET(_picture->neighbouring_locations_MBAFF(
            x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_A_type, mbAddrN_A,
            cb4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));

        RET(_picture->neighbouring_locations_MBAFF(
            x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_B_type, mbAddrN_B,
            cb4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
      }

      BlkIdxA = cb4x4BlkIdxN_A, BlkIdxB = cb4x4BlkIdxN_B;
    } else if (mb_residual_level == MB_RESIDUAL_CrIntra16x16DCLevel ||
               mb_residual_level == MB_RESIDUAL_CrIntra16x16ACLevel ||
               mb_residual_level == MB_RESIDUAL_CrLevel4x4) {
      // 6.4.11.6 Derivation process for neighbouring 4x4 chroma blocks for ChromaArrayType equal to 3
      RET(ChromaArrayType != 3);

      x = InverseRasterScan(cr4x4BlkIdx / 4, 8, 8, 16, 0) +
          InverseRasterScan(cr4x4BlkIdx % 4, 4, 4, 8, 0);
      y = InverseRasterScan(cr4x4BlkIdx / 4, 8, 8, 16, 1) +
          InverseRasterScan(cr4x4BlkIdx % 4, 4, 4, 8, 1);

      // 6.4.12 Derivation process for neighbouring locations
      maxW = _picture->m_slice->slice_header->m_sps->MbWidthC;
      maxH = _picture->m_slice->slice_header->m_sps->MbHeightC;
      isChroma = 0;

      if (header->MbaffFrameFlag == 0) {
        RET(_picture->neighbouring_locations_non_MBAFF(
            x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_A_type, mbAddrN_A,
            cr4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));

        RET(_picture->neighbouring_locations_non_MBAFF(
            x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_B_type, mbAddrN_B,
            cr4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
      } else {
        RET(_picture->neighbouring_locations_MBAFF(
            x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_A_type, mbAddrN_A,
            cr4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));

        RET(_picture->neighbouring_locations_MBAFF(
            x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_B_type, mbAddrN_B,
            cr4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
      }

      BlkIdxA = cr4x4BlkIdxN_A, BlkIdxB = cr4x4BlkIdxN_B;
    } else if (mb_residual_level == MB_RESIDUAL_ChromaACLevelCb ||
               mb_residual_level == MB_RESIDUAL_ChromaACLevelCr) {
      // 6.4.11.5 Derivation process for neighbouring 4x4 chroma blocks
      RET(ChromaArrayType != 1 && ChromaArrayType != 2);

      // 6.4.7 Inverse 4x4 chroma block scanning process
      x = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 0);
      y = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 1);

      maxW = _picture->m_slice->slice_header->m_sps->MbWidthC;
      maxH = _picture->m_slice->slice_header->m_sps->MbHeightC;
      isChroma = 1;

      if (header->MbaffFrameFlag == 0) {
        RET(_picture->neighbouring_locations_non_MBAFF(
            x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_A_type, mbAddrN_A,
            chroma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));

        RET(_picture->neighbouring_locations_non_MBAFF(
            x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_B_type, mbAddrN_B,
            chroma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
      } else {
        RET(_picture->neighbouring_locations_MBAFF(
            x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_A_type, mbAddrN_A,
            chroma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma));

        RET(_picture->neighbouring_locations_MBAFF(
            x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_B_type, mbAddrN_B,
            chroma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma));
      }

      BlkIdxA = chroma4x4BlkIdxN_A, BlkIdxB = chroma4x4BlkIdxN_B;
    }

    int32_t availableFlagN_A = 1, availableFlagN_B = 1;

    if (mbAddrN_A < 0 ||
        (IS_INTRA_Prediction_Mode(MbPartPredMode) &&
         _picture->m_slice->slice_header->m_pps->constrained_intra_pred_flag ==
             1 &&
         !IS_INTRA_Prediction_Mode(_picture->m_mbs[mbAddrN_A].m_mb_pred_mode) &&
         header->nal_unit_type >= 2 && header->nal_unit_type <= 4))
      availableFlagN_A = 0;

    if (mbAddrN_B < 0 ||
        (IS_INTRA_Prediction_Mode(MbPartPredMode) &&
         _picture->m_slice->slice_header->m_pps->constrained_intra_pred_flag ==
             1 &&
         !IS_INTRA_Prediction_Mode(_picture->m_mbs[mbAddrN_B].m_mb_pred_mode) &&
         header->nal_unit_type >= 2 && header->nal_unit_type <= 4))
      availableFlagN_B = 0;

    int32_t nA = 0, nB = 0;

    if (availableFlagN_A == 1) {
      if (_picture->m_mbs[mbAddrN_A].m_mb_type_fixed == P_Skip ||
          _picture->m_mbs[mbAddrN_A].m_mb_type_fixed == B_Skip)
        nA = 0;
      else if (_picture->m_mbs[mbAddrN_A].m_mb_type_fixed == I_PCM)
        nA = 16;
      else {
        if (mb_residual_level == MB_RESIDUAL_Intra16x16DCLevel ||
            mb_residual_level == MB_RESIDUAL_Intra16x16ACLevel ||
            mb_residual_level == MB_RESIDUAL_LumaLevel4x4)
          nA = _picture->m_mbs[mbAddrN_A]
                   .mb_luma_4x4_non_zero_count_coeff[BlkIdxA];
        else if (mb_residual_level == MB_RESIDUAL_CbIntra16x16DCLevel ||
                 mb_residual_level == MB_RESIDUAL_CbIntra16x16DCLevel ||
                 mb_residual_level == MB_RESIDUAL_CbLevel4x4)
          nA = _picture->m_mbs[mbAddrN_A]
                   .mb_chroma_4x4_non_zero_count_coeff[0][BlkIdxA];
        else if (mb_residual_level == MB_RESIDUAL_CrIntra16x16DCLevel ||
                 mb_residual_level == MB_RESIDUAL_CrIntra16x16ACLevel ||
                 mb_residual_level == MB_RESIDUAL_CrLevel4x4)
          nA = _picture->m_mbs[mbAddrN_A]
                   .mb_chroma_4x4_non_zero_count_coeff[1][BlkIdxA];
        else if (mb_residual_level == MB_RESIDUAL_ChromaACLevelCb)
          nA = _picture->m_mbs[mbAddrN_A]
                   .mb_chroma_4x4_non_zero_count_coeff[0][BlkIdxA];
        else if (mb_residual_level == MB_RESIDUAL_ChromaACLevelCr)
          nA = _picture->m_mbs[mbAddrN_A]
                   .mb_chroma_4x4_non_zero_count_coeff[1][BlkIdxA];
      }
    }

    if (availableFlagN_B == 1) {
      if (_picture->m_mbs[mbAddrN_B].m_mb_type_fixed == P_Skip ||
          _picture->m_mbs[mbAddrN_B].m_mb_type_fixed == B_Skip)
        nB = 0;
      else if (_picture->m_mbs[mbAddrN_B].m_mb_type_fixed == I_PCM)
        nB = 16;
      else {
        if (mb_residual_level == MB_RESIDUAL_Intra16x16DCLevel ||
            mb_residual_level == MB_RESIDUAL_Intra16x16ACLevel ||
            mb_residual_level == MB_RESIDUAL_LumaLevel4x4)
          nB = _picture->m_mbs[mbAddrN_B]
                   .mb_luma_4x4_non_zero_count_coeff[BlkIdxB];
        else if (mb_residual_level == MB_RESIDUAL_CbIntra16x16DCLevel ||
                 mb_residual_level == MB_RESIDUAL_CbIntra16x16DCLevel ||
                 mb_residual_level == MB_RESIDUAL_CbLevel4x4)
          nB = _picture->m_mbs[mbAddrN_B]
                   .mb_chroma_4x4_non_zero_count_coeff[0][BlkIdxB];
        else if (mb_residual_level == MB_RESIDUAL_CrIntra16x16DCLevel ||
                 mb_residual_level == MB_RESIDUAL_CrIntra16x16ACLevel ||
                 mb_residual_level == MB_RESIDUAL_CrLevel4x4)
          nB = _picture->m_mbs[mbAddrN_B]
                   .mb_chroma_4x4_non_zero_count_coeff[1][BlkIdxB];
        else if (mb_residual_level == MB_RESIDUAL_ChromaACLevelCb)
          nB = _picture->m_mbs[mbAddrN_B]
                   .mb_chroma_4x4_non_zero_count_coeff[0][BlkIdxB];
        else if (mb_residual_level == MB_RESIDUAL_ChromaACLevelCr)
          nB = _picture->m_mbs[mbAddrN_B]
                   .mb_chroma_4x4_non_zero_count_coeff[1][BlkIdxB];
      }
    }

    if (availableFlagN_A == 1 && availableFlagN_B == 1)
      nC = (nA + nB + 1) >> 1;
    else if (availableFlagN_A == 1 && availableFlagN_B == 0)
      nC = nA;
    else if (availableFlagN_A == 0 && availableFlagN_B == 1)
      nC = nB;
    else
      nC = 0;
  }

  return 0;
}

// Table 9-5 – coeff_token mapping to TotalCoeff( coeff_token ) and TrailingOnes( coeff_token )
int Cavlc::process_coeff_token(MB_RESIDUAL_LEVEL mb_residual_level,
                               int32_t MbPartPredMode, int32_t BlkIdx,
                               uint16_t coeff_token,
                               int32_t &coeff_token_bit_length,
                               int32_t &TrailingOnes, int32_t &TotalCoeff) {

  int32_t nC = 0;
  RET(process_nC(mb_residual_level, MbPartPredMode, BlkIdx, nC));
  if (nC >= 0 && nC < 2) {
    if ((coeff_token >> 15) == 0x0001)
      coeff_token_bit_length = 1, TrailingOnes = 0, TotalCoeff = 0;
    else if ((coeff_token >> 10) == 0x0005)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 1;
    else if ((coeff_token >> 14) == 0x0001)
      coeff_token_bit_length = 2, TrailingOnes = 1, TotalCoeff = 1;
    else if ((coeff_token >> 8) == 0x0007)
      coeff_token_bit_length = 8, TrailingOnes = 0, TotalCoeff = 2;
    else if ((coeff_token >> 10) == 0x0004)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 2;
    else if ((coeff_token >> 13) == 0x0001)
      coeff_token_bit_length = 3, TrailingOnes = 2, TotalCoeff = 2;
    else if ((coeff_token >> 7) == 0x0007)
      coeff_token_bit_length = 9, TrailingOnes = 0, TotalCoeff = 3;
    else if ((coeff_token >> 8) == 0x0006)
      coeff_token_bit_length = 8, TrailingOnes = 1, TotalCoeff = 3;
    else if ((coeff_token >> 9) == 0x0005)
      coeff_token_bit_length = 7, TrailingOnes = 2, TotalCoeff = 3;
    else if ((coeff_token >> 11) == 0x0003)
      coeff_token_bit_length = 5, TrailingOnes = 3, TotalCoeff = 3;
    else if ((coeff_token >> 6) == 0x0007)
      coeff_token_bit_length = 10, TrailingOnes = 0, TotalCoeff = 4;
    else if ((coeff_token >> 7) == 0x0006)
      coeff_token_bit_length = 9, TrailingOnes = 1, TotalCoeff = 4;
    else if ((coeff_token >> 8) == 0x0005)
      coeff_token_bit_length = 8, TrailingOnes = 2, TotalCoeff = 4;
    else if ((coeff_token >> 10) == 0x0003)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 4;
    else if ((coeff_token >> 5) == 0x0007)
      coeff_token_bit_length = 11, TrailingOnes = 0, TotalCoeff = 5;
    else if ((coeff_token >> 6) == 0x0006)
      coeff_token_bit_length = 10, TrailingOnes = 1, TotalCoeff = 5;
    else if ((coeff_token >> 7) == 0x0005)
      coeff_token_bit_length = 9, TrailingOnes = 2, TotalCoeff = 5;
    else if ((coeff_token >> 9) == 0x0004)
      coeff_token_bit_length = 7, TrailingOnes = 3, TotalCoeff = 5;
    else if ((coeff_token >> 3) == 0x000F)
      coeff_token_bit_length = 13, TrailingOnes = 0, TotalCoeff = 6;
    else if ((coeff_token >> 5) == 0x0006)
      coeff_token_bit_length = 11, TrailingOnes = 1, TotalCoeff = 6;
    else if ((coeff_token >> 6) == 0x0005)
      coeff_token_bit_length = 10, TrailingOnes = 2, TotalCoeff = 6;
    else if ((coeff_token >> 8) == 0x0004)
      coeff_token_bit_length = 8, TrailingOnes = 3, TotalCoeff = 6;
    else if ((coeff_token >> 3) == 0x000B)
      coeff_token_bit_length = 13, TrailingOnes = 0, TotalCoeff = 7;
    else if ((coeff_token >> 3) == 0x000E)
      coeff_token_bit_length = 13, TrailingOnes = 1, TotalCoeff = 7;
    else if ((coeff_token >> 5) == 0x0005)
      coeff_token_bit_length = 11, TrailingOnes = 2, TotalCoeff = 7;
    else if ((coeff_token >> 7) == 0x0004)
      coeff_token_bit_length = 9, TrailingOnes = 3, TotalCoeff = 7;
    else if ((coeff_token >> 3) == 0x0008)
      coeff_token_bit_length = 13, TrailingOnes = 0, TotalCoeff = 8;
    else if ((coeff_token >> 3) == 0x000A)
      coeff_token_bit_length = 13, TrailingOnes = 1, TotalCoeff = 8;
    else if ((coeff_token >> 3) == 0x000D)
      coeff_token_bit_length = 13, TrailingOnes = 2, TotalCoeff = 8;
    else if ((coeff_token >> 6) == 0x0004)
      coeff_token_bit_length = 10, TrailingOnes = 3, TotalCoeff = 8;
    else if ((coeff_token >> 2) == 0x000F)
      coeff_token_bit_length = 14, TrailingOnes = 0, TotalCoeff = 9;
    else if ((coeff_token >> 2) == 0x000E)
      coeff_token_bit_length = 14, TrailingOnes = 1, TotalCoeff = 9;
    else if ((coeff_token >> 3) == 0x0009)
      coeff_token_bit_length = 13, TrailingOnes = 2, TotalCoeff = 9;
    else if ((coeff_token >> 5) == 0x0004)
      coeff_token_bit_length = 11, TrailingOnes = 3, TotalCoeff = 9;
    else if ((coeff_token >> 2) == 0x000B)
      coeff_token_bit_length = 14, TrailingOnes = 0, TotalCoeff = 10;
    else if ((coeff_token >> 2) == 0x000A)
      coeff_token_bit_length = 14, TrailingOnes = 1, TotalCoeff = 10;
    else if ((coeff_token >> 2) == 0x000D)
      coeff_token_bit_length = 14, TrailingOnes = 2, TotalCoeff = 10;
    else if ((coeff_token >> 3) == 0x000C)
      coeff_token_bit_length = 13, TrailingOnes = 3, TotalCoeff = 10;
    else if ((coeff_token >> 1) == 0x000F)
      coeff_token_bit_length = 15, TrailingOnes = 0, TotalCoeff = 11;
    else if ((coeff_token >> 1) == 0x000E)
      coeff_token_bit_length = 15, TrailingOnes = 1, TotalCoeff = 11;
    else if ((coeff_token >> 2) == 0x0009)
      coeff_token_bit_length = 14, TrailingOnes = 2, TotalCoeff = 11;
    else if ((coeff_token >> 2) == 0x000C)
      coeff_token_bit_length = 14, TrailingOnes = 3, TotalCoeff = 11;
    else if ((coeff_token >> 1) == 0x000B)
      coeff_token_bit_length = 15, TrailingOnes = 0, TotalCoeff = 12;
    else if ((coeff_token >> 1) == 0x000A)
      coeff_token_bit_length = 15, TrailingOnes = 1, TotalCoeff = 12;
    else if ((coeff_token >> 1) == 0x000D)
      coeff_token_bit_length = 15, TrailingOnes = 2, TotalCoeff = 12;
    else if ((coeff_token >> 2) == 0x0008)
      coeff_token_bit_length = 14, TrailingOnes = 3, TotalCoeff = 12;
    else if ((coeff_token >> 0) == 0x000F)
      coeff_token_bit_length = 16, TrailingOnes = 0, TotalCoeff = 13;
    else if ((coeff_token >> 1) == 0x0001)
      coeff_token_bit_length = 15, TrailingOnes = 1, TotalCoeff = 13;
    else if ((coeff_token >> 1) == 0x0009)
      coeff_token_bit_length = 15, TrailingOnes = 2, TotalCoeff = 13;
    else if ((coeff_token >> 1) == 0x000C)
      coeff_token_bit_length = 15, TrailingOnes = 3, TotalCoeff = 13;
    else if ((coeff_token >> 0) == 0x000B)
      coeff_token_bit_length = 16, TrailingOnes = 0, TotalCoeff = 14;
    else if ((coeff_token >> 0) == 0x000E)
      coeff_token_bit_length = 16, TrailingOnes = 1, TotalCoeff = 14;
    else if ((coeff_token >> 0) == 0x000D)
      coeff_token_bit_length = 16, TrailingOnes = 2, TotalCoeff = 14;
    else if ((coeff_token >> 1) == 0x0008)
      coeff_token_bit_length = 15, TrailingOnes = 3, TotalCoeff = 14;
    else if ((coeff_token >> 0) == 0x0007)
      coeff_token_bit_length = 16, TrailingOnes = 0, TotalCoeff = 15;
    else if ((coeff_token >> 0) == 0x000A)
      coeff_token_bit_length = 16, TrailingOnes = 1, TotalCoeff = 15;
    else if ((coeff_token >> 0) == 0x0009)
      coeff_token_bit_length = 16, TrailingOnes = 2, TotalCoeff = 15;
    else if ((coeff_token >> 0) == 0x000C)
      coeff_token_bit_length = 16, TrailingOnes = 3, TotalCoeff = 15;
    else if ((coeff_token >> 0) == 0x0004)
      coeff_token_bit_length = 16, TrailingOnes = 0, TotalCoeff = 16;
    else if ((coeff_token >> 0) == 0x0006)
      coeff_token_bit_length = 16, TrailingOnes = 1, TotalCoeff = 16;
    else if ((coeff_token >> 0) == 0x0005)
      coeff_token_bit_length = 16, TrailingOnes = 2, TotalCoeff = 16;
    else if ((coeff_token >> 0) == 0x0008)
      coeff_token_bit_length = 16, TrailingOnes = 3, TotalCoeff = 16;
    else
      RET(-1);
  } else if (nC >= 2 && nC < 4) {
    if ((coeff_token >> 14) == 0x0003)
      coeff_token_bit_length = 2, TrailingOnes = 0, TotalCoeff = 0;
    else if ((coeff_token >> 10) == 0x000B)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 1;
    else if ((coeff_token >> 14) == 0x0002)
      coeff_token_bit_length = 2, TrailingOnes = 1, TotalCoeff = 1;
    else if ((coeff_token >> 10) == 0x0007)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 2;
    else if ((coeff_token >> 11) == 0x0007)
      coeff_token_bit_length = 5, TrailingOnes = 1, TotalCoeff = 2;
    else if ((coeff_token >> 13) == 0x0003)
      coeff_token_bit_length = 3, TrailingOnes = 2, TotalCoeff = 2;
    else if ((coeff_token >> 9) == 0x0007)
      coeff_token_bit_length = 7, TrailingOnes = 0, TotalCoeff = 3;
    else if ((coeff_token >> 10) == 0x000A)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 3;
    else if ((coeff_token >> 10) == 0x0009)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 3;
    else if ((coeff_token >> 12) == 0x0005)
      coeff_token_bit_length = 4, TrailingOnes = 3, TotalCoeff = 3;
    else if ((coeff_token >> 8) == 0x0007)
      coeff_token_bit_length = 8, TrailingOnes = 0, TotalCoeff = 4;
    else if ((coeff_token >> 10) == 0x0006)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 4;
    else if ((coeff_token >> 10) == 0x0005)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 4;
    else if ((coeff_token >> 12) == 0x0004)
      coeff_token_bit_length = 4, TrailingOnes = 3, TotalCoeff = 4;
    else if ((coeff_token >> 8) == 0x0004)
      coeff_token_bit_length = 8, TrailingOnes = 0, TotalCoeff = 5;
    else if ((coeff_token >> 9) == 0x0006)
      coeff_token_bit_length = 7, TrailingOnes = 1, TotalCoeff = 5;
    else if ((coeff_token >> 9) == 0x0005)
      coeff_token_bit_length = 7, TrailingOnes = 2, TotalCoeff = 5;
    else if ((coeff_token >> 11) == 0x0006)
      coeff_token_bit_length = 5, TrailingOnes = 3, TotalCoeff = 5;
    else if ((coeff_token >> 7) == 0x0007)
      coeff_token_bit_length = 9, TrailingOnes = 0, TotalCoeff = 6;
    else if ((coeff_token >> 8) == 0x0006)
      coeff_token_bit_length = 8, TrailingOnes = 1, TotalCoeff = 6;
    else if ((coeff_token >> 8) == 0x0005)
      coeff_token_bit_length = 8, TrailingOnes = 2, TotalCoeff = 6;
    else if ((coeff_token >> 10) == 0x0008)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 6;
    else if ((coeff_token >> 5) == 0x000F)
      coeff_token_bit_length = 11, TrailingOnes = 0, TotalCoeff = 7;
    else if ((coeff_token >> 7) == 0x0006)
      coeff_token_bit_length = 9, TrailingOnes = 1, TotalCoeff = 7;
    else if ((coeff_token >> 7) == 0x0005)
      coeff_token_bit_length = 9, TrailingOnes = 2, TotalCoeff = 7;
    else if ((coeff_token >> 10) == 0x0004)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 7;
    else if ((coeff_token >> 5) == 0x000B)
      coeff_token_bit_length = 11, TrailingOnes = 0, TotalCoeff = 8;
    else if ((coeff_token >> 5) == 0x000E)
      coeff_token_bit_length = 11, TrailingOnes = 1, TotalCoeff = 8;
    else if ((coeff_token >> 5) == 0x000D)
      coeff_token_bit_length = 11, TrailingOnes = 2, TotalCoeff = 8;
    else if ((coeff_token >> 9) == 0x0004)
      coeff_token_bit_length = 7, TrailingOnes = 3, TotalCoeff = 8;
    else if ((coeff_token >> 4) == 0x000F)
      coeff_token_bit_length = 12, TrailingOnes = 0, TotalCoeff = 9;
    else if ((coeff_token >> 5) == 0x000A)
      coeff_token_bit_length = 11, TrailingOnes = 1, TotalCoeff = 9;
    else if ((coeff_token >> 5) == 0x0009)
      coeff_token_bit_length = 11, TrailingOnes = 2, TotalCoeff = 9;
    else if ((coeff_token >> 7) == 0x0004)
      coeff_token_bit_length = 9, TrailingOnes = 3, TotalCoeff = 9;
    else if ((coeff_token >> 4) == 0x000B)
      coeff_token_bit_length = 12, TrailingOnes = 0, TotalCoeff = 10;
    else if ((coeff_token >> 4) == 0x000E)
      coeff_token_bit_length = 12, TrailingOnes = 1, TotalCoeff = 10;
    else if ((coeff_token >> 4) == 0x000D)
      coeff_token_bit_length = 12, TrailingOnes = 2, TotalCoeff = 10;
    else if ((coeff_token >> 5) == 0x000C)
      coeff_token_bit_length = 11, TrailingOnes = 3, TotalCoeff = 10;
    else if ((coeff_token >> 4) == 0x0008)
      coeff_token_bit_length = 12, TrailingOnes = 0, TotalCoeff = 11;
    else if ((coeff_token >> 4) == 0x000A)
      coeff_token_bit_length = 12, TrailingOnes = 1, TotalCoeff = 11;
    else if ((coeff_token >> 4) == 0x0009)
      coeff_token_bit_length = 12, TrailingOnes = 2, TotalCoeff = 11;
    else if ((coeff_token >> 5) == 0x0008)
      coeff_token_bit_length = 11, TrailingOnes = 3, TotalCoeff = 11;
    else if ((coeff_token >> 3) == 0x000F)
      coeff_token_bit_length = 13, TrailingOnes = 0, TotalCoeff = 12;
    else if ((coeff_token >> 3) == 0x000E)
      coeff_token_bit_length = 13, TrailingOnes = 1, TotalCoeff = 12;
    else if ((coeff_token >> 3) == 0x000D)
      coeff_token_bit_length = 13, TrailingOnes = 2, TotalCoeff = 12;
    else if ((coeff_token >> 4) == 0x000C)
      coeff_token_bit_length = 12, TrailingOnes = 3, TotalCoeff = 12;
    else if ((coeff_token >> 3) == 0x000B)
      coeff_token_bit_length = 13, TrailingOnes = 0, TotalCoeff = 13;
    else if ((coeff_token >> 3) == 0x000A)
      coeff_token_bit_length = 13, TrailingOnes = 1, TotalCoeff = 13;
    else if ((coeff_token >> 3) == 0x0009)
      coeff_token_bit_length = 13, TrailingOnes = 2, TotalCoeff = 13;
    else if ((coeff_token >> 3) == 0x000C)
      coeff_token_bit_length = 13, TrailingOnes = 3, TotalCoeff = 13;
    else if ((coeff_token >> 3) == 0x0007)
      coeff_token_bit_length = 13, TrailingOnes = 0, TotalCoeff = 14;
    else if ((coeff_token >> 2) == 0x000B)
      coeff_token_bit_length = 14, TrailingOnes = 1, TotalCoeff = 14;
    else if ((coeff_token >> 3) == 0x0006)
      coeff_token_bit_length = 13, TrailingOnes = 2, TotalCoeff = 14;
    else if ((coeff_token >> 3) == 0x0008)
      coeff_token_bit_length = 13, TrailingOnes = 3, TotalCoeff = 14;
    else if ((coeff_token >> 2) == 0x0009)
      coeff_token_bit_length = 14, TrailingOnes = 0, TotalCoeff = 15;
    else if ((coeff_token >> 2) == 0x0008)
      coeff_token_bit_length = 14, TrailingOnes = 1, TotalCoeff = 15;
    else if ((coeff_token >> 2) == 0x000A)
      coeff_token_bit_length = 14, TrailingOnes = 2, TotalCoeff = 15;
    else if ((coeff_token >> 3) == 0x0001)
      coeff_token_bit_length = 13, TrailingOnes = 3, TotalCoeff = 15;
    else if ((coeff_token >> 2) == 0x0007)
      coeff_token_bit_length = 14, TrailingOnes = 0, TotalCoeff = 16;
    else if ((coeff_token >> 2) == 0x0006)
      coeff_token_bit_length = 14, TrailingOnes = 1, TotalCoeff = 16;
    else if ((coeff_token >> 2) == 0x0005)
      coeff_token_bit_length = 14, TrailingOnes = 2, TotalCoeff = 16;
    else if ((coeff_token >> 2) == 0x0004)
      coeff_token_bit_length = 14, TrailingOnes = 3, TotalCoeff = 16;
    else
      RET(-1);
  } else if (nC >= 4 && nC < 8) {
    if ((coeff_token >> 12) == 0x000F)
      coeff_token_bit_length = 4, TrailingOnes = 0, TotalCoeff = 0;
    else if ((coeff_token >> 10) == 0x000F)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 1;
    else if ((coeff_token >> 12) == 0x000E)
      coeff_token_bit_length = 4, TrailingOnes = 1, TotalCoeff = 1;
    else if ((coeff_token >> 10) == 0x000B)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 2;
    else if ((coeff_token >> 11) == 0x000F)
      coeff_token_bit_length = 5, TrailingOnes = 1, TotalCoeff = 2;
    else if ((coeff_token >> 12) == 0x000D)
      coeff_token_bit_length = 4, TrailingOnes = 2, TotalCoeff = 2;
    else if ((coeff_token >> 10) == 0x0008)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 3;
    else if ((coeff_token >> 11) == 0x000C)
      coeff_token_bit_length = 5, TrailingOnes = 1, TotalCoeff = 3;
    else if ((coeff_token >> 11) == 0x000E)
      coeff_token_bit_length = 5, TrailingOnes = 2, TotalCoeff = 3;
    else if ((coeff_token >> 12) == 0x000C)
      coeff_token_bit_length = 4, TrailingOnes = 3, TotalCoeff = 3;
    else if ((coeff_token >> 9) == 0x000F)
      coeff_token_bit_length = 7, TrailingOnes = 0, TotalCoeff = 4;
    else if ((coeff_token >> 11) == 0x000A)
      coeff_token_bit_length = 5, TrailingOnes = 1, TotalCoeff = 4;
    else if ((coeff_token >> 11) == 0x000B)
      coeff_token_bit_length = 5, TrailingOnes = 2, TotalCoeff = 4;
    else if ((coeff_token >> 12) == 0x000B)
      coeff_token_bit_length = 4, TrailingOnes = 3, TotalCoeff = 4;
    else if ((coeff_token >> 9) == 0x000B)
      coeff_token_bit_length = 7, TrailingOnes = 0, TotalCoeff = 5;
    else if ((coeff_token >> 11) == 0x0008)
      coeff_token_bit_length = 5, TrailingOnes = 1, TotalCoeff = 5;
    else if ((coeff_token >> 11) == 0x0009)
      coeff_token_bit_length = 5, TrailingOnes = 2, TotalCoeff = 5;
    else if ((coeff_token >> 12) == 0x000A)
      coeff_token_bit_length = 4, TrailingOnes = 3, TotalCoeff = 5;
    else if ((coeff_token >> 9) == 0x0009)
      coeff_token_bit_length = 7, TrailingOnes = 0, TotalCoeff = 6;
    else if ((coeff_token >> 10) == 0x000E)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 6;
    else if ((coeff_token >> 10) == 0x000D)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 6;
    else if ((coeff_token >> 12) == 0x0009)
      coeff_token_bit_length = 4, TrailingOnes = 3, TotalCoeff = 6;
    else if ((coeff_token >> 9) == 0x0008)
      coeff_token_bit_length = 7, TrailingOnes = 0, TotalCoeff = 7;
    else if ((coeff_token >> 10) == 0x000A)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 7;
    else if ((coeff_token >> 10) == 0x0009)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 7;
    else if ((coeff_token >> 12) == 0x0008)
      coeff_token_bit_length = 4, TrailingOnes = 3, TotalCoeff = 7;
    else if ((coeff_token >> 8) == 0x000F)
      coeff_token_bit_length = 8, TrailingOnes = 0, TotalCoeff = 8;
    else if ((coeff_token >> 9) == 0x000E)
      coeff_token_bit_length = 7, TrailingOnes = 1, TotalCoeff = 8;
    else if ((coeff_token >> 9) == 0x000D)
      coeff_token_bit_length = 7, TrailingOnes = 2, TotalCoeff = 8;
    else if ((coeff_token >> 11) == 0x000D)
      coeff_token_bit_length = 5, TrailingOnes = 3, TotalCoeff = 8;
    else if ((coeff_token >> 8) == 0x000B)
      coeff_token_bit_length = 8, TrailingOnes = 0, TotalCoeff = 9;
    else if ((coeff_token >> 8) == 0x000E)
      coeff_token_bit_length = 8, TrailingOnes = 1, TotalCoeff = 9;
    else if ((coeff_token >> 9) == 0x000A)
      coeff_token_bit_length = 7, TrailingOnes = 2, TotalCoeff = 9;
    else if ((coeff_token >> 10) == 0x000C)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 9;
    else if ((coeff_token >> 7) == 0x000F)
      coeff_token_bit_length = 9, TrailingOnes = 0, TotalCoeff = 10;
    else if ((coeff_token >> 8) == 0x000A)
      coeff_token_bit_length = 8, TrailingOnes = 1, TotalCoeff = 10;
    else if ((coeff_token >> 8) == 0x000D)
      coeff_token_bit_length = 8, TrailingOnes = 2, TotalCoeff = 10;
    else if ((coeff_token >> 9) == 0x000C)
      coeff_token_bit_length = 7, TrailingOnes = 3, TotalCoeff = 10;
    else if ((coeff_token >> 7) == 0x000B)
      coeff_token_bit_length = 9, TrailingOnes = 0, TotalCoeff = 11;
    else if ((coeff_token >> 7) == 0x000E)
      coeff_token_bit_length = 9, TrailingOnes = 1, TotalCoeff = 11;
    else if ((coeff_token >> 8) == 0x0009)
      coeff_token_bit_length = 8, TrailingOnes = 2, TotalCoeff = 11;
    else if ((coeff_token >> 8) == 0x000C)
      coeff_token_bit_length = 8, TrailingOnes = 3, TotalCoeff = 11;
    else if ((coeff_token >> 7) == 0x0008)
      coeff_token_bit_length = 9, TrailingOnes = 0, TotalCoeff = 12;
    else if ((coeff_token >> 7) == 0x000A)
      coeff_token_bit_length = 9, TrailingOnes = 1, TotalCoeff = 12;
    else if ((coeff_token >> 7) == 0x000D)
      coeff_token_bit_length = 9, TrailingOnes = 2, TotalCoeff = 12;
    else if ((coeff_token >> 8) == 0x0008)
      coeff_token_bit_length = 8, TrailingOnes = 3, TotalCoeff = 12;
    else if ((coeff_token >> 6) == 0x000D)
      coeff_token_bit_length = 10, TrailingOnes = 0, TotalCoeff = 13;
    else if ((coeff_token >> 7) == 0x0007)
      coeff_token_bit_length = 9, TrailingOnes = 1, TotalCoeff = 13;
    else if ((coeff_token >> 7) == 0x0009)
      coeff_token_bit_length = 9, TrailingOnes = 2, TotalCoeff = 13;
    else if ((coeff_token >> 7) == 0x000C)
      coeff_token_bit_length = 9, TrailingOnes = 3, TotalCoeff = 13;
    else if ((coeff_token >> 6) == 0x0009)
      coeff_token_bit_length = 10, TrailingOnes = 0, TotalCoeff = 14;
    else if ((coeff_token >> 6) == 0x000C)
      coeff_token_bit_length = 10, TrailingOnes = 1, TotalCoeff = 14;
    else if ((coeff_token >> 6) == 0x000B)
      coeff_token_bit_length = 10, TrailingOnes = 2, TotalCoeff = 14;
    else if ((coeff_token >> 6) == 0x000A)
      coeff_token_bit_length = 10, TrailingOnes = 3, TotalCoeff = 14;
    else if ((coeff_token >> 6) == 0x0005)
      coeff_token_bit_length = 10, TrailingOnes = 0, TotalCoeff = 15;
    else if ((coeff_token >> 6) == 0x0008)
      coeff_token_bit_length = 10, TrailingOnes = 1, TotalCoeff = 15;
    else if ((coeff_token >> 6) == 0x0007)
      coeff_token_bit_length = 10, TrailingOnes = 2, TotalCoeff = 15;
    else if ((coeff_token >> 6) == 0x0006)
      coeff_token_bit_length = 10, TrailingOnes = 3, TotalCoeff = 15;
    else if ((coeff_token >> 6) == 0x0001)
      coeff_token_bit_length = 10, TrailingOnes = 0, TotalCoeff = 16;
    else if ((coeff_token >> 6) == 0x0004)
      coeff_token_bit_length = 10, TrailingOnes = 1, TotalCoeff = 16;
    else if ((coeff_token >> 6) == 0x0003)
      coeff_token_bit_length = 10, TrailingOnes = 2, TotalCoeff = 16;
    else if ((coeff_token >> 6) == 0x0002)
      coeff_token_bit_length = 10, TrailingOnes = 3, TotalCoeff = 16;
    else
      RET(-1);
  } else if (nC >= 8) {
    if ((coeff_token >> 10) == 0x0003)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 0;
    else if ((coeff_token >> 10) == 0x0000)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 1;
    else if ((coeff_token >> 10) == 0x0001)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 1;
    else if ((coeff_token >> 10) == 0x0004)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 2;
    else if ((coeff_token >> 10) == 0x0005)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 2;
    else if ((coeff_token >> 10) == 0x0006)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 2;
    else if ((coeff_token >> 10) == 0x0008)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 3;
    else if ((coeff_token >> 10) == 0x0009)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 3;
    else if ((coeff_token >> 10) == 0x000A)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 3;
    else if ((coeff_token >> 10) == 0x000B)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 3;
    else if ((coeff_token >> 10) == 0x000C)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 4;
    else if ((coeff_token >> 10) == 0x000D)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 4;
    else if ((coeff_token >> 10) == 0x000E)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 4;
    else if ((coeff_token >> 10) == 0x000F)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 4;
    else if ((coeff_token >> 10) == 0x0010)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 5;
    else if ((coeff_token >> 10) == 0x0011)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 5;
    else if ((coeff_token >> 10) == 0x0012)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 5;
    else if ((coeff_token >> 10) == 0x0013)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 5;
    else if ((coeff_token >> 10) == 0x0014)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 6;
    else if ((coeff_token >> 10) == 0x0015)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 6;
    else if ((coeff_token >> 10) == 0x0016)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 6;
    else if ((coeff_token >> 10) == 0x0017)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 6;
    else if ((coeff_token >> 10) == 0x0018)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 7;
    else if ((coeff_token >> 10) == 0x0019)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 7;
    else if ((coeff_token >> 10) == 0x001A)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 7;
    else if ((coeff_token >> 10) == 0x001B)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 7;
    else if ((coeff_token >> 10) == 0x001C)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 8;
    else if ((coeff_token >> 10) == 0x001D)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 8;
    else if ((coeff_token >> 10) == 0x001E)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 8;
    else if ((coeff_token >> 10) == 0x001F)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 8;
    else if ((coeff_token >> 10) == 0x0020)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 9;
    else if ((coeff_token >> 10) == 0x0021)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 9;
    else if ((coeff_token >> 10) == 0x0022)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 9;
    else if ((coeff_token >> 10) == 0x0023)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 9;
    else if ((coeff_token >> 10) == 0x0024)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 10;
    else if ((coeff_token >> 10) == 0x0025)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 10;
    else if ((coeff_token >> 10) == 0x0026)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 10;
    else if ((coeff_token >> 10) == 0x0027)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 10;
    else if ((coeff_token >> 10) == 0x0028)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 11;
    else if ((coeff_token >> 10) == 0x0029)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 11;
    else if ((coeff_token >> 10) == 0x002A)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 11;
    else if ((coeff_token >> 10) == 0x002B)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 11;
    else if ((coeff_token >> 10) == 0x002C)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 12;
    else if ((coeff_token >> 10) == 0x002D)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 12;
    else if ((coeff_token >> 10) == 0x002E)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 12;
    else if ((coeff_token >> 10) == 0x002F)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 12;
    else if ((coeff_token >> 10) == 0x0030)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 13;
    else if ((coeff_token >> 10) == 0x0031)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 13;
    else if ((coeff_token >> 10) == 0x0032)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 13;
    else if ((coeff_token >> 10) == 0x0033)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 13;
    else if ((coeff_token >> 10) == 0x0034)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 14;
    else if ((coeff_token >> 10) == 0x0035)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 14;
    else if ((coeff_token >> 10) == 0x0036)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 14;
    else if ((coeff_token >> 10) == 0x0037)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 14;
    else if ((coeff_token >> 10) == 0x0038)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 15;
    else if ((coeff_token >> 10) == 0x0039)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 15;
    else if ((coeff_token >> 10) == 0x003A)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 15;
    else if ((coeff_token >> 10) == 0x003B)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 15;
    else if ((coeff_token >> 10) == 0x003C)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 16;
    else if ((coeff_token >> 10) == 0x003D)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 16;
    else if ((coeff_token >> 10) == 0x003E)
      coeff_token_bit_length = 6, TrailingOnes = 2, TotalCoeff = 16;
    else if ((coeff_token >> 10) == 0x003F)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 16;
    else
      RET(-1);
  } else if (nC == -1) {
    if ((coeff_token >> 14) == 0x0001)
      coeff_token_bit_length = 2, TrailingOnes = 0, TotalCoeff = 0;
    else if ((coeff_token >> 10) == 0x0007)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 1;
    else if ((coeff_token >> 15) == 0x0001)
      coeff_token_bit_length = 1, TrailingOnes = 1, TotalCoeff = 1;
    else if ((coeff_token >> 10) == 0x0004)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 2;
    else if ((coeff_token >> 10) == 0x0006)
      coeff_token_bit_length = 6, TrailingOnes = 1, TotalCoeff = 2;
    else if ((coeff_token >> 13) == 0x0001)
      coeff_token_bit_length = 3, TrailingOnes = 2, TotalCoeff = 2;
    else if ((coeff_token >> 10) == 0x0003)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 3;
    else if ((coeff_token >> 9) == 0x0003)
      coeff_token_bit_length = 7, TrailingOnes = 1, TotalCoeff = 3;
    else if ((coeff_token >> 9) == 0x0002)
      coeff_token_bit_length = 7, TrailingOnes = 2, TotalCoeff = 3;
    else if ((coeff_token >> 10) == 0x0005)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 3;
    else if ((coeff_token >> 10) == 0x0002)
      coeff_token_bit_length = 6, TrailingOnes = 0, TotalCoeff = 4;
    else if ((coeff_token >> 8) == 0x0003)
      coeff_token_bit_length = 8, TrailingOnes = 1, TotalCoeff = 4;
    else if ((coeff_token >> 8) == 0x0002)
      coeff_token_bit_length = 8, TrailingOnes = 2, TotalCoeff = 4;
    else if ((coeff_token >> 9) == 0x0000)
      coeff_token_bit_length = 7, TrailingOnes = 3, TotalCoeff = 4;
    else
      RET(-1);
  } else if (nC == -2) {
    if ((coeff_token >> 15) == 0x0001)
      coeff_token_bit_length = 1, TrailingOnes = 0, TotalCoeff = 0;
    else if ((coeff_token >> 9) == 0x000F)
      coeff_token_bit_length = 7, TrailingOnes = 0, TotalCoeff = 1;
    else if ((coeff_token >> 14) == 0x0001)
      coeff_token_bit_length = 2, TrailingOnes = 1, TotalCoeff = 1;
    else if ((coeff_token >> 9) == 0x000E)
      coeff_token_bit_length = 7, TrailingOnes = 0, TotalCoeff = 2;
    else if ((coeff_token >> 9) == 0x000D)
      coeff_token_bit_length = 7, TrailingOnes = 1, TotalCoeff = 2;
    else if ((coeff_token >> 13) == 0x0001)
      coeff_token_bit_length = 3, TrailingOnes = 2, TotalCoeff = 2;
    else if ((coeff_token >> 7) == 0x0007)
      coeff_token_bit_length = 9, TrailingOnes = 0, TotalCoeff = 3;
    else if ((coeff_token >> 9) == 0x000C)
      coeff_token_bit_length = 7, TrailingOnes = 1, TotalCoeff = 3;
    else if ((coeff_token >> 9) == 0x000B)
      coeff_token_bit_length = 7, TrailingOnes = 2, TotalCoeff = 3;
    else if ((coeff_token >> 11) == 0x0001)
      coeff_token_bit_length = 5, TrailingOnes = 3, TotalCoeff = 3;
    else if ((coeff_token >> 7) == 0x0006)
      coeff_token_bit_length = 9, TrailingOnes = 0, TotalCoeff = 4;
    else if ((coeff_token >> 7) == 0x0005)
      coeff_token_bit_length = 9, TrailingOnes = 1, TotalCoeff = 4;
    else if ((coeff_token >> 9) == 0x000A)
      coeff_token_bit_length = 7, TrailingOnes = 2, TotalCoeff = 4;
    else if ((coeff_token >> 10) == 0x0001)
      coeff_token_bit_length = 6, TrailingOnes = 3, TotalCoeff = 4;
    else if ((coeff_token >> 6) == 0x0007)
      coeff_token_bit_length = 10, TrailingOnes = 0, TotalCoeff = 5;
    else if ((coeff_token >> 6) == 0x0006)
      coeff_token_bit_length = 10, TrailingOnes = 1, TotalCoeff = 5;
    else if ((coeff_token >> 7) == 0x0004)
      coeff_token_bit_length = 9, TrailingOnes = 2, TotalCoeff = 5;
    else if ((coeff_token >> 9) == 0x0009)
      coeff_token_bit_length = 7, TrailingOnes = 3, TotalCoeff = 5;
    else if ((coeff_token >> 5) == 0x0007)
      coeff_token_bit_length = 11, TrailingOnes = 0, TotalCoeff = 6;
    else if ((coeff_token >> 5) == 0x0006)
      coeff_token_bit_length = 11, TrailingOnes = 1, TotalCoeff = 6;
    else if ((coeff_token >> 6) == 0x0005)
      coeff_token_bit_length = 10, TrailingOnes = 2, TotalCoeff = 6;
    else if ((coeff_token >> 9) == 0x0008)
      coeff_token_bit_length = 7, TrailingOnes = 3, TotalCoeff = 6;
    else if ((coeff_token >> 4) == 0x0007)
      coeff_token_bit_length = 12, TrailingOnes = 0, TotalCoeff = 7;
    else if ((coeff_token >> 4) == 0x0006)
      coeff_token_bit_length = 12, TrailingOnes = 1, TotalCoeff = 7;
    else if ((coeff_token >> 5) == 0x0005)
      coeff_token_bit_length = 11, TrailingOnes = 2, TotalCoeff = 7;
    else if ((coeff_token >> 6) == 0x0004)
      coeff_token_bit_length = 10, TrailingOnes = 3, TotalCoeff = 7;
    else if ((coeff_token >> 3) == 0x0007)
      coeff_token_bit_length = 13, TrailingOnes = 0, TotalCoeff = 8;
    else if ((coeff_token >> 4) == 0x0005)
      coeff_token_bit_length = 12, TrailingOnes = 1, TotalCoeff = 8;
    else if ((coeff_token >> 4) == 0x0004)
      coeff_token_bit_length = 12, TrailingOnes = 2, TotalCoeff = 8;
    else if ((coeff_token >> 5) == 0x0004)
      coeff_token_bit_length = 11, TrailingOnes = 3, TotalCoeff = 8;
    else
      RET(-1);
  } else
    RET(-1);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
  uint16_t coeff_token2 = _bs->readUn(coeff_token_bit_length);
#pragma GCC diagnostic pop

  return 0;
}

int Cavlc::process_total_zeros(int32_t maxNumCoeff, int32_t tzVlcIndex,
                               int32_t &total_zeros) {
  int32_t token = 0, token_length = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  int32_t token2 = 0;
#pragma GCC diagnostic pop

  // Table 9-9 – total_zeros tables for chroma DC 2x2 and 2x4 blocks (a) Chroma DC 2x2 block (4:2:0 chroma sampling)
  if (maxNumCoeff == 4) {
    if (tzVlcIndex == 1) {
      token = _bs->getUn(3), token_length = 0;
      if ((token >> 2) == 0x01)
        token_length = 1, total_zeros = 0;
      else if ((token >> 1) == 0x01)
        token_length = 2, total_zeros = 1;
      else if ((token >> 0) == 0x01)
        token_length = 3, total_zeros = 2;
      else if ((token >> 0) == 0x00)
        token_length = 3, total_zeros = 3;
      else
        RET(-1);
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 2) {
      token_length = 0;
      token = _bs->getUn(2);
      if ((token >> 1) == 0x01)
        token_length = 1, total_zeros = 0;
      else if ((token >> 0) == 0x01)
        token_length = 2, total_zeros = 1;
      else if ((token >> 0) == 0x00)
        token_length = 2, total_zeros = 2;
      else
        RET(-1);
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 3) {
      token = _bs->getUn(1);
      token_length = 0;
      if ((token >> 0) == 0x01)
        token_length = 1, total_zeros = 0;
      else if ((token >> 0) == 0x00)
        token_length = 1, total_zeros = 1;
      else
        RET(-1);
      token2 = _bs->readUn(token_length);
    } else
      RET(-1);
  }

  // Table 9-9 – total_zeros tables for chroma DC 2x2 and 2x4 blocks (b) Chroma DC 2x4 block (4:2:2 chroma sampling)
  else if (maxNumCoeff == 8) {
    if (tzVlcIndex == 1) {
      token_length = 0, token = _bs->getUn(5);
      if ((token >> 4) == 0x01)
        token_length = 1, total_zeros = 0;
      else if ((token >> 2) == 0x02)
        token_length = 3, total_zeros = 1;
      else if ((token >> 2) == 0x03)
        token_length = 3, total_zeros = 2;
      else if ((token >> 1) == 0x02)
        token_length = 4, total_zeros = 3;
      else if ((token >> 1) == 0x03)
        token_length = 4, total_zeros = 4;
      else if ((token >> 1) == 0x01)
        token_length = 4, total_zeros = 5;
      else if ((token >> 0) == 0x01)
        token_length = 5, total_zeros = 6;
      else if ((token >> 0) == 0x00)
        token_length = 5, total_zeros = 7;
      else
        RET(-1);
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 2) {
      token_length = 0, token = _bs->getUn(3);
      if ((token >> 0) == 0x00)
        token_length = 3, total_zeros = 0;
      else if ((token >> 1) == 0x01)
        token_length = 2, total_zeros = 1;
      else if ((token >> 0) == 0x01)
        token_length = 3, total_zeros = 2;
      else if ((token >> 0) == 0x04)
        token_length = 3, total_zeros = 3;
      else if ((token >> 0) == 0x05)
        token_length = 3, total_zeros = 4;
      else if ((token >> 0) == 0x06)
        token_length = 3, total_zeros = 5;
      else if ((token >> 0) == 0x07)
        token_length = 3, total_zeros = 6;
      else
        RET(-1);
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 3) {
      token_length = 0, token = _bs->getUn(3);
      if ((token >> 0) == 0x00)
        token_length = 3, total_zeros = 0;
      else if ((token >> 0) == 0x01)
        token_length = 3, total_zeros = 1;
      else if ((token >> 1) == 0x01)
        token_length = 2, total_zeros = 2;
      else if ((token >> 1) == 0x02)
        token_length = 2, total_zeros = 3;
      else if ((token >> 0) == 0x06)
        token_length = 3, total_zeros = 4;
      else if ((token >> 0) == 0x07)
        token_length = 3, total_zeros = 5;
      else
        RET(-1);
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 4) {
      token_length = 0, token = _bs->getUn(3);
      if ((token >> 0) == 0x06)
        token_length = 3, total_zeros = 0;
      else if ((token >> 1) == 0x00)
        token_length = 2, total_zeros = 1;
      else if ((token >> 1) == 0x01)
        token_length = 2, total_zeros = 2;
      else if ((token >> 1) == 0x02)
        token_length = 2, total_zeros = 3;
      else if ((token >> 0) == 0x07)
        token_length = 3, total_zeros = 4;
      else
        RET(-1);
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 5) {
      token_length = 0, token = _bs->getUn(2);
      if ((token >> 0) == 0x00)
        token_length = 2, total_zeros = 0;
      else if ((token >> 0) == 0x01)
        token_length = 2, total_zeros = 1;
      else if ((token >> 0) == 0x02)
        token_length = 2, total_zeros = 2;
      else if ((token >> 0) == 0x03)
        token_length = 2, total_zeros = 3;
      else
        RET(-1);
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 6) {
      token_length = 0, token = _bs->getUn(2);
      if ((token >> 0) == 0x00)
        token_length = 2, total_zeros = 0;
      else if ((token >> 0) == 0x01)
        token_length = 2, total_zeros = 1;
      else if ((token >> 1) == 0x01)
        token_length = 1, total_zeros = 2;
      else
        RET(-1);
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 7) {
      token_length = 0, token = _bs->getUn(1);
      if ((token >> 0) == 0x00)
        token_length = 1, total_zeros = 0;
      else if ((token >> 0) == 0x01)
        token_length = 1, total_zeros = 1;
      else
        RET(-1);
      token2 = _bs->readUn(token_length);
    } else
      RET(-1);
  }

  // Table 9-7 – total_zeros tables for 4x4 blocks with tzVlcIndex 1 to 7
  else {
    if (tzVlcIndex == 1) {
      token_length = 0, token = _bs->getUn(9);
      if ((token >> 8) == 0x01)
        token_length = 1, total_zeros = 0;
      else if ((token >> 6) == 0x03)
        token_length = 3, total_zeros = 1;
      else if ((token >> 6) == 0x02)
        token_length = 3, total_zeros = 2;
      else if ((token >> 5) == 0x03)
        token_length = 4, total_zeros = 3;
      else if ((token >> 5) == 0x02)
        token_length = 4, total_zeros = 4;
      else if ((token >> 4) == 0x03)
        token_length = 5, total_zeros = 5;
      else if ((token >> 4) == 0x02)
        token_length = 5, total_zeros = 6;
      else if ((token >> 3) == 0x03)
        token_length = 6, total_zeros = 7;
      else if ((token >> 3) == 0x02)
        token_length = 6, total_zeros = 8;
      else if ((token >> 2) == 0x03)
        token_length = 7, total_zeros = 9;
      else if ((token >> 2) == 0x02)
        token_length = 7, total_zeros = 10;
      else if ((token >> 1) == 0x03)
        token_length = 8, total_zeros = 11;
      else if ((token >> 1) == 0x02)
        token_length = 8, total_zeros = 12;
      else if ((token >> 0) == 0x03)
        token_length = 9, total_zeros = 13;
      else if ((token >> 0) == 0x02)
        token_length = 9, total_zeros = 14;
      else if ((token >> 0) == 0x01)
        token_length = 9, total_zeros = 15;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 2) {
      token_length = 0, token = _bs->getUn(6);
      if ((token >> 3) == 0x07)
        token_length = 3, total_zeros = 0;
      else if ((token >> 3) == 0x06)
        token_length = 3, total_zeros = 1;
      else if ((token >> 3) == 0x05)
        token_length = 3, total_zeros = 2;
      else if ((token >> 3) == 0x04)
        token_length = 3, total_zeros = 3;
      else if ((token >> 3) == 0x03)
        token_length = 3, total_zeros = 4;
      else if ((token >> 2) == 0x05)
        token_length = 4, total_zeros = 5;
      else if ((token >> 2) == 0x04)
        token_length = 4, total_zeros = 6;
      else if ((token >> 2) == 0x03)
        token_length = 4, total_zeros = 7;
      else if ((token >> 2) == 0x02)
        token_length = 4, total_zeros = 8;
      else if ((token >> 1) == 0x03)
        token_length = 5, total_zeros = 9;
      else if ((token >> 1) == 0x02)
        token_length = 5, total_zeros = 10;
      else if ((token >> 0) == 0x03)
        token_length = 6, total_zeros = 11;
      else if ((token >> 0) == 0x02)
        token_length = 6, total_zeros = 12;
      else if ((token >> 0) == 0x01)
        token_length = 6, total_zeros = 13;
      else if ((token >> 0) == 0x00)
        token_length = 6, total_zeros = 14;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 3) {
      token_length = 0, token = _bs->getUn(6);
      if ((token >> 2) == 0x05)
        token_length = 4, total_zeros = 0;
      else if ((token >> 3) == 0x07)
        token_length = 3, total_zeros = 1;
      else if ((token >> 3) == 0x06)
        token_length = 3, total_zeros = 2;
      else if ((token >> 3) == 0x05)
        token_length = 3, total_zeros = 3;
      else if ((token >> 2) == 0x04)
        token_length = 4, total_zeros = 4;
      else if ((token >> 2) == 0x03)
        token_length = 4, total_zeros = 5;
      else if ((token >> 3) == 0x04)
        token_length = 3, total_zeros = 6;
      else if ((token >> 3) == 0x03)
        token_length = 3, total_zeros = 7;
      else if ((token >> 2) == 0x02)
        token_length = 4, total_zeros = 8;
      else if ((token >> 1) == 0x03)
        token_length = 5, total_zeros = 9;
      else if ((token >> 1) == 0x02)
        token_length = 5, total_zeros = 10;
      else if ((token >> 0) == 0x01)
        token_length = 6, total_zeros = 11;
      else if ((token >> 1) == 0x01)
        token_length = 5, total_zeros = 12;
      else if ((token >> 0) == 0x00)
        token_length = 6, total_zeros = 13;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 4) {
      token_length = 0, token = _bs->getUn(5);
      if ((token >> 0) == 0x03)
        token_length = 5, total_zeros = 0;
      else if ((token >> 2) == 0x07)
        token_length = 3, total_zeros = 1;
      else if ((token >> 1) == 0x05)
        token_length = 4, total_zeros = 2;
      else if ((token >> 1) == 0x04)
        token_length = 4, total_zeros = 3;
      else if ((token >> 2) == 0x06)
        token_length = 3, total_zeros = 4;
      else if ((token >> 2) == 0x05)
        token_length = 3, total_zeros = 5;
      else if ((token >> 2) == 0x04)
        token_length = 3, total_zeros = 6;
      else if ((token >> 1) == 0x03)
        token_length = 4, total_zeros = 7;
      else if ((token >> 2) == 0x03)
        token_length = 3, total_zeros = 8;
      else if ((token >> 1) == 0x02)
        token_length = 4, total_zeros = 9;
      else if ((token >> 0) == 0x02)
        token_length = 5, total_zeros = 10;
      else if ((token >> 0) == 0x01)
        token_length = 5, total_zeros = 11;
      else if ((token >> 0) == 0x00)
        token_length = 5, total_zeros = 12;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 5) {
      token_length = 0, token = _bs->getUn(5);
      if ((token >> 1) == 0x05)
        token_length = 4, total_zeros = 0;
      else if ((token >> 1) == 0x04)
        token_length = 4, total_zeros = 1;
      else if ((token >> 1) == 0x03)
        token_length = 4, total_zeros = 2;
      else if ((token >> 2) == 0x07)
        token_length = 3, total_zeros = 3;
      else if ((token >> 2) == 0x06)
        token_length = 3, total_zeros = 4;
      else if ((token >> 2) == 0x05)
        token_length = 3, total_zeros = 5;
      else if ((token >> 2) == 0x04)
        token_length = 3, total_zeros = 6;
      else if ((token >> 2) == 0x03)
        token_length = 3, total_zeros = 7;
      else if ((token >> 1) == 0x02)
        token_length = 4, total_zeros = 8;
      else if ((token >> 0) == 0x01)
        token_length = 5, total_zeros = 9;
      else if ((token >> 1) == 0x01)
        token_length = 4, total_zeros = 10;
      else if ((token >> 0) == 0x00)
        token_length = 5, total_zeros = 11;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 6) {
      token_length = 0, token = _bs->getUn(6);
      if ((token >> 0) == 0x01)
        token_length = 6, total_zeros = 0;
      else if ((token >> 1) == 0x01)
        token_length = 5, total_zeros = 1;
      else if ((token >> 3) == 0x07)
        token_length = 3, total_zeros = 2;
      else if ((token >> 3) == 0x06)
        token_length = 3, total_zeros = 3;
      else if ((token >> 3) == 0x05)
        token_length = 3, total_zeros = 4;
      else if ((token >> 3) == 0x04)
        token_length = 3, total_zeros = 5;
      else if ((token >> 3) == 0x03)
        token_length = 3, total_zeros = 6;
      else if ((token >> 3) == 0x02)
        token_length = 3, total_zeros = 7;
      else if ((token >> 2) == 0x01)
        token_length = 4, total_zeros = 8;
      else if ((token >> 3) == 0x01)
        token_length = 3, total_zeros = 9;
      else if ((token >> 0) == 0x00)
        token_length = 6, total_zeros = 10;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 7) {
      token_length = 0, token = _bs->getUn(6);
      if ((token >> 0) == 0x01)
        token_length = 6, total_zeros = 0;
      else if ((token >> 1) == 0x01)
        token_length = 5, total_zeros = 1;
      else if ((token >> 3) == 0x05)
        token_length = 3, total_zeros = 2;
      else if ((token >> 3) == 0x04)
        token_length = 3, total_zeros = 3;
      else if ((token >> 3) == 0x03)
        token_length = 3, total_zeros = 4;
      else if ((token >> 4) == 0x03)
        token_length = 2, total_zeros = 5;
      else if ((token >> 3) == 0x02)
        token_length = 3, total_zeros = 6;
      else if ((token >> 2) == 0x01)
        token_length = 4, total_zeros = 7;
      else if ((token >> 3) == 0x01)
        token_length = 3, total_zeros = 8;
      else if ((token >> 0) == 0x00)
        token_length = 6, total_zeros = 9;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 8) {
      token_length = 0, token = _bs->getUn(6);
      if ((token >> 0) == 0x01)
        token_length = 6, total_zeros = 0;
      else if ((token >> 2) == 0x01)
        token_length = 4, total_zeros = 1;
      else if ((token >> 1) == 0x01)
        token_length = 5, total_zeros = 2;
      else if ((token >> 3) == 0x03)
        token_length = 3, total_zeros = 3;
      else if ((token >> 4) == 0x03)
        token_length = 2, total_zeros = 4;
      else if ((token >> 4) == 0x02)
        token_length = 2, total_zeros = 5;
      else if ((token >> 3) == 0x02)
        token_length = 3, total_zeros = 6;
      else if ((token >> 3) == 0x01)
        token_length = 3, total_zeros = 7;
      else if ((token >> 0) == 0x00)
        token_length = 6, total_zeros = 8;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 9) {
      token_length = 0, token = _bs->getUn(6);
      if ((token >> 0) == 0x01)
        token_length = 6, total_zeros = 0;
      else if ((token >> 0) == 0x00)
        token_length = 6, total_zeros = 1;
      else if ((token >> 2) == 0x01)
        token_length = 4, total_zeros = 2;
      else if ((token >> 4) == 0x03)
        token_length = 2, total_zeros = 3;
      else if ((token >> 4) == 0x02)
        token_length = 2, total_zeros = 4;
      else if ((token >> 3) == 0x01)
        token_length = 3, total_zeros = 5;
      else if ((token >> 4) == 0x01)
        token_length = 2, total_zeros = 6;
      else if ((token >> 1) == 0x01)
        token_length = 5, total_zeros = 7;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 10) {
      token_length = 0, token = _bs->getUn(5);
      if ((token >> 0) == 0x01)
        token_length = 5, total_zeros = 0;
      else if ((token >> 0) == 0x00)
        token_length = 5, total_zeros = 1;
      else if ((token >> 2) == 0x01)
        token_length = 3, total_zeros = 2;
      else if ((token >> 3) == 0x03)
        token_length = 2, total_zeros = 3;
      else if ((token >> 3) == 0x02)
        token_length = 2, total_zeros = 4;
      else if ((token >> 3) == 0x01)
        token_length = 2, total_zeros = 5;
      else if ((token >> 1) == 0x01)
        token_length = 4, total_zeros = 6;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 11) {
      token_length = 0, token = _bs->getUn(4);
      if ((token >> 0) == 0x00)
        token_length = 4, total_zeros = 0;
      else if ((token >> 0) == 0x01)
        token_length = 4, total_zeros = 1;
      else if ((token >> 1) == 0x01)
        token_length = 3, total_zeros = 2;
      else if ((token >> 1) == 0x02)
        token_length = 3, total_zeros = 3;
      else if ((token >> 3) == 0x01)
        token_length = 1, total_zeros = 4;
      else if ((token >> 1) == 0x03)
        token_length = 3, total_zeros = 5;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 12) {
      token_length = 0, token = _bs->getUn(4);
      if ((token >> 0) == 0x00)
        token_length = 4, total_zeros = 0;
      else if ((token >> 0) == 0x01)
        token_length = 4, total_zeros = 1;
      else if ((token >> 2) == 0x01)
        token_length = 2, total_zeros = 2;
      else if ((token >> 3) == 0x01)
        token_length = 1, total_zeros = 3;
      else if ((token >> 1) == 0x01)
        token_length = 3, total_zeros = 4;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 13) {
      token_length = 0, token = _bs->getUn(4);
      if ((token >> 1) == 0x00)
        token_length = 3, total_zeros = 0;
      else if ((token >> 1) == 0x01)
        token_length = 3, total_zeros = 1;
      else if ((token >> 3) == 0x01)
        token_length = 1, total_zeros = 2;
      else if ((token >> 2) == 0x01)
        token_length = 2, total_zeros = 3;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 14) {
      token_length = 0, token = _bs->getUn(2);
      if ((token >> 0) == 0x00)
        token_length = 2, total_zeros = 0;
      else if ((token >> 0) == 0x01)
        token_length = 2, total_zeros = 1;
      else if ((token >> 1) == 0x01)
        token_length = 1, total_zeros = 2;
      token2 = _bs->readUn(token_length);
    } else if (tzVlcIndex == 15) {
      token_length = 0, token = _bs->getUn(1);
      if ((token >> 0) == 0x00)
        token_length = 1, total_zeros = 0;
      else if ((token >> 0) == 0x01)
        token_length = 1, total_zeros = 1;
      token2 = _bs->readUn(token_length);
    }
  }

  return 0;
}

int Cavlc::process_run_before(int32_t zerosLeft, int32_t &run_before) {
  int32_t token = 0, token_length = 0;
  if (zerosLeft == 1) {
    token_length = 0, token = _bs->getUn(1);
    if ((token >> 0) == 0x01)
      token_length = 1, run_before = 0;
    else if ((token >> 0) == 0x00)
      token_length = 1, run_before = 1;

    _bs->readUn(token_length);
  } else if (zerosLeft == 2) {
    token_length = 0, token = _bs->getUn(2);
    if ((token >> 1) == 0x01)
      token_length = 1, run_before = 0;
    else if ((token >> 0) == 0x01)
      token_length = 2, run_before = 1;
    else if ((token >> 0) == 0x00)
      token_length = 2, run_before = 2;

    _bs->readUn(token_length);
  } else if (zerosLeft == 3) {
    token_length = 0, token = _bs->getUn(2);
    if ((token >> 0) == 0x03)
      token_length = 2, run_before = 0;
    else if ((token >> 0) == 0x02)
      token_length = 2, run_before = 1;
    else if ((token >> 0) == 0x01)
      token_length = 2, run_before = 2;
    else if ((token >> 0) == 0x00)
      token_length = 2, run_before = 3;

    _bs->readUn(token_length);
  } else if (zerosLeft == 4) {
    token_length = 0, token = _bs->getUn(3);
    if ((token >> 1) == 0x03)
      token_length = 2, run_before = 0;
    else if ((token >> 1) == 0x02)
      token_length = 2, run_before = 1;
    else if ((token >> 1) == 0x01)
      token_length = 2, run_before = 2;
    else if ((token >> 0) == 0x01)
      token_length = 3, run_before = 3;
    else if ((token >> 0) == 0x00)
      token_length = 3, run_before = 4;

    _bs->readUn(token_length);
  } else if (zerosLeft == 5) {
    token_length = 0, token = _bs->getUn(3);
    if ((token >> 1) == 0x03)
      token_length = 2, run_before = 0;
    else if ((token >> 1) == 0x02)
      token_length = 2, run_before = 1;
    else if ((token >> 0) == 0x03)
      token_length = 3, run_before = 2;
    else if ((token >> 0) == 0x02)
      token_length = 3, run_before = 3;
    else if ((token >> 0) == 0x01)
      token_length = 3, run_before = 4;
    else if ((token >> 0) == 0x00)
      token_length = 3, run_before = 5;

    _bs->readUn(token_length);
  } else if (zerosLeft == 6) {
    token_length = 0, token = _bs->getUn(3);
    if ((token >> 1) == 0x03)
      token_length = 2, run_before = 0;
    else if ((token >> 0) == 0x00)
      token_length = 3, run_before = 1;
    else if ((token >> 0) == 0x01)
      token_length = 3, run_before = 2;
    else if ((token >> 0) == 0x03)
      token_length = 3, run_before = 3;
    else if ((token >> 0) == 0x02)
      token_length = 3, run_before = 4;
    else if ((token >> 0) == 0x05)
      token_length = 3, run_before = 5;
    else if ((token >> 0) == 0x04)
      token_length = 3, run_before = 6;

    _bs->readUn(token_length);
  } else if (zerosLeft > 6) {
    token_length = 0, token = _bs->getUn(11);
    if ((token >> 8) == 0x07)
      token_length = 3, run_before = 0;
    else if ((token >> 8) == 0x06)
      token_length = 3, run_before = 1;
    else if ((token >> 8) == 0x05)
      token_length = 3, run_before = 2;
    else if ((token >> 8) == 0x04)
      token_length = 3, run_before = 3;
    else if ((token >> 8) == 0x03)
      token_length = 3, run_before = 4;
    else if ((token >> 8) == 0x02)
      token_length = 3, run_before = 5;
    else if ((token >> 8) == 0x01)
      token_length = 3, run_before = 6;
    else if ((token >> 7) == 0x01)
      token_length = 4, run_before = 7;
    else if ((token >> 6) == 0x01)
      token_length = 5, run_before = 8;
    else if ((token >> 5) == 0x01)
      token_length = 6, run_before = 9;
    else if ((token >> 4) == 0x01)
      token_length = 7, run_before = 10;
    else if ((token >> 3) == 0x01)
      token_length = 8, run_before = 11;
    else if ((token >> 2) == 0x01)
      token_length = 9, run_before = 12;
    else if ((token >> 1) == 0x01)
      token_length = 10, run_before = 13;
    else if ((token >> 0) == 0x01)
      token_length = 11, run_before = 14;

    _bs->readUn(token_length);
  }

  return 0;
}
