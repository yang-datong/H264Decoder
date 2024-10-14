#include "Cabac.hpp"
#include "Constants.hpp"
#include "MacroBlock.hpp"
#include "PictureBase.hpp"
#include "SliceHeader.hpp"
#include "Type.hpp"
#include <cstdint>

/* 对于P、SP和B Slice 类型，初始化还取决于cabac_init_idc语法元素的值 */
int Cabac::init_m_n(int32_t ctxIdx, H264_SLICE_TYPE slice_type,
                    int32_t cabac_init_idc, int32_t &m, int32_t &n) {
  RET(ctxIdx < 0 || ctxIdx > 1024 || cabac_init_idc < 0 || cabac_init_idc > 2);

  if (ctxIdx >= 0 && ctxIdx <= 10) {
    // Table 9-12 – Values of variables m and n for ctxIdx from 0 to 10
    m = mn_0_10[ctxIdx][0];
    n = mn_0_10[ctxIdx][1];
  } else if (ctxIdx >= 11 && ctxIdx <= 23) {
    // Table 9-13 – Values of variables m and n for ctxIdx from 11 to 23
    m = mn_11_23[cabac_init_idc][ctxIdx - 11][0];
    n = mn_11_23[cabac_init_idc][ctxIdx - 11][1];
  } else if (ctxIdx >= 24 && ctxIdx <= 39) {
    // Table 9-14 – Values of variables m and n for ctxIdx from 24 to 39
    m = mn_24_39[cabac_init_idc][ctxIdx - 24][0];
    n = mn_24_39[cabac_init_idc][ctxIdx - 24][1];
  } else if (ctxIdx >= 40 && ctxIdx <= 53) {
    // Table 9-15 – Values of variables m and n for ctxIdx from 40 to 53
    m = mn_40_53[cabac_init_idc][ctxIdx - 40][0];
    n = mn_40_53[cabac_init_idc][ctxIdx - 40][1];
  } else if (ctxIdx >= 54 && ctxIdx <= 59) {
    // Table 9-16 – Values of variables m and n for ctxIdx from 54 to 59, and 399 to 401
    m = mn_54_59[cabac_init_idc][ctxIdx - 54][0];
    n = mn_54_59[cabac_init_idc][ctxIdx - 54][1];
  } else if (ctxIdx >= 60 && ctxIdx <= 69) {
    // Table 9-17 – Values of variables m and n for ctxIdx from 60 to 69
    m = mn_60_69[ctxIdx - 60][0];
    n = mn_60_69[ctxIdx - 60][1];
  } else if (ctxIdx >= 70 && ctxIdx <= 104) {
    // Table 9-18 – Values of variables m and n for ctxIdx from 70 to 104
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_70_104[0][ctxIdx - 70][0];
      n = mn_70_104[0][ctxIdx - 70][1];
    } else {
      m = mn_70_104[cabac_init_idc + 1][ctxIdx - 70][0];
      n = mn_70_104[cabac_init_idc + 1][ctxIdx - 70][1];
    }
  } else if (ctxIdx >= 105 && ctxIdx <= 165) {
    // Table 9-19 – Values of variables m and n for ctxIdx from 105 to 165
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_105_165[0][ctxIdx - 105][0];
      n = mn_105_165[0][ctxIdx - 105][1];
    } else {
      m = mn_105_165[cabac_init_idc + 1][ctxIdx - 105][0];
      n = mn_105_165[cabac_init_idc + 1][ctxIdx - 105][1];
    }
  } else if (ctxIdx >= 166 && ctxIdx <= 226) {
    // Table 9-20 – Values of variables m and n for ctxIdx from 166 to 226
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_166_226[0][ctxIdx - 166][0];
      n = mn_166_226[0][ctxIdx - 166][1];
    } else {
      m = mn_166_226[cabac_init_idc + 1][ctxIdx - 166][0];
      n = mn_166_226[cabac_init_idc + 1][ctxIdx - 166][1];
    }
  } else if (ctxIdx >= 227 && ctxIdx <= 275) {
    // Table 9-21 – Values of variables m and n for ctxIdx from 227 to 275
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_227_275[0][ctxIdx - 227][0];
      n = mn_227_275[0][ctxIdx - 227][1];
    } else {
      m = mn_227_275[cabac_init_idc + 1][ctxIdx - 227][0];
      n = mn_227_275[cabac_init_idc + 1][ctxIdx - 227][1];
    }
  } else if (ctxIdx == 276) {
    /* NOTE: ctxIdx = 276与end_of_slice_flag和mb_type的bin相关联，mb_type指定I_PCM macroblock类型。9.3.3.2.4中规定的解码过程适用于等于276的ctxIdx。这个解码过程，然而，也可以通过使用第9.3.3.2.1小节中指定的解码过程来实现。在本例中，与ctxIdx = 276相关联的初始值被指定为pStateIdx = 63和valMPS = 0，其中pStateIdx = 63表示非自适应概率状态。 */
    return -1;
  } else if (ctxIdx >= 277 && ctxIdx <= 337) {
    // Table 9-22 – Values of variables m and n for ctxIdx from 277 to 337
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_277_337[0][ctxIdx - 277][0];
      n = mn_277_337[0][ctxIdx - 277][1];
    } else {
      m = mn_277_337[cabac_init_idc + 1][ctxIdx - 277][0];
      n = mn_277_337[cabac_init_idc + 1][ctxIdx - 277][1];
    }
  } else if (ctxIdx >= 338 && ctxIdx <= 398) {
    // Table 9-23 – Values of variables m and n for ctxIdx from 338 to 398
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_338_398[0][ctxIdx - 338][0];
      n = mn_338_398[0][ctxIdx - 338][1];
    } else {
      m = mn_338_398[cabac_init_idc + 1][ctxIdx - 338][0];
      n = mn_338_398[cabac_init_idc + 1][ctxIdx - 338][1];
    }
  } else if (ctxIdx >= 399 && ctxIdx <= 401) {
    if (slice_type == SLICE_I) {
      m = mn_399_401[0][ctxIdx - 399][0];
      n = mn_399_401[0][ctxIdx - 399][1];
    } else {
      m = mn_399_401[cabac_init_idc + 1][ctxIdx - 399][0];
      n = mn_399_401[cabac_init_idc + 1][ctxIdx - 399][1];
    }
  } else if (ctxIdx >= 402 && ctxIdx <= 459) {
    // Table 9-24 – Values of variables m and n for ctxIdx from 402 to 459
    if (slice_type == SLICE_I) {
      m = mn_402_459[0][ctxIdx - 402][0];
      n = mn_402_459[0][ctxIdx - 402][1];
    } else {
      m = mn_402_459[cabac_init_idc + 1][ctxIdx - 402][0];
      n = mn_402_459[cabac_init_idc + 1][ctxIdx - 402][1];
    }
  } else if (ctxIdx >= 460 && ctxIdx <= 483) {
    // Table 9-25 – Values of variables m and n for ctxIdx from 460 to 483
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_460_483[0][ctxIdx - 460][0];
      n = mn_460_483[0][ctxIdx - 460][1];
    } else {
      m = mn_460_483[cabac_init_idc + 1][ctxIdx - 460][0];
      n = mn_460_483[cabac_init_idc + 1][ctxIdx - 460][1];
    }
  } else if (ctxIdx >= 484 && ctxIdx <= 571) {
    // Table 9-26 – Values of variables m and n for ctxIdx from 484 to 571
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_484_571[0][ctxIdx - 484][0];
      n = mn_484_571[0][ctxIdx - 484][1];
    } else {
      m = mn_484_571[cabac_init_idc + 1][ctxIdx - 484][0];
      n = mn_484_571[cabac_init_idc + 1][ctxIdx - 484][1];
    }
  } else if (ctxIdx >= 572 && ctxIdx <= 659) {
    // Table 9-27 – Values of variables m and n for ctxIdx from 572 to 659
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_572_659[0][ctxIdx - 572][0];
      n = mn_572_659[0][ctxIdx - 572][1];
    } else {
      m = mn_572_659[cabac_init_idc + 1][ctxIdx - 572][0];
      n = mn_572_659[cabac_init_idc + 1][ctxIdx - 572][1];
    }
  } else if (ctxIdx >= 660 && ctxIdx <= 717) {
    // Table 9-28 – Values of variables m and n for ctxIdx from 660 to 717
    if (slice_type == SLICE_I) {
      m = mn_660_717[0][ctxIdx - 660][0];
      n = mn_660_717[0][ctxIdx - 660][1];
    } else {
      m = mn_660_717[cabac_init_idc + 1][ctxIdx - 660][0];
      n = mn_660_717[cabac_init_idc + 1][ctxIdx - 660][1];
    }
  } else if (ctxIdx >= 718 && ctxIdx <= 775) {
    // Table 9-29 – Values of variables m and n for ctxIdx from 718 to 775
    if (slice_type == SLICE_I) {
      m = mn_718_775[0][ctxIdx - 718][0];
      n = mn_718_775[0][ctxIdx - 718][1];
    } else {
      m = mn_718_775[cabac_init_idc + 1][ctxIdx - 718][0];
      n = mn_718_775[cabac_init_idc + 1][ctxIdx - 718][1];
    }
  } else if (ctxIdx >= 776 && ctxIdx <= 863) {
    // Table 9-30 – Values of variables m and n for ctxIdx from 776 to 863
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_776_863[0][ctxIdx - 776][0];
      n = mn_776_863[0][ctxIdx - 776][1];
    } else {
      m = mn_776_863[cabac_init_idc + 1][ctxIdx - 776][0];
      n = mn_776_863[cabac_init_idc + 1][ctxIdx - 776][1];
    }
  } else if (ctxIdx >= 864 && ctxIdx <= 951) {
    // Table 9-31 – Values of variables m and n for ctxIdx from 864 to 951
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_864_951[0][ctxIdx - 864][0];
      n = mn_864_951[0][ctxIdx - 864][1];
    } else {
      m = mn_864_951[cabac_init_idc + 1][ctxIdx - 864][0];
      n = mn_864_951[cabac_init_idc + 1][ctxIdx - 864][1];
    }
  } else if (ctxIdx >= 952 && ctxIdx <= 1011) {
    // Table 9-32 – Values of variables m and n for ctxIdx from 952 to 1011
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_952_1011[0][ctxIdx - 952][0];
      n = mn_952_1011[0][ctxIdx - 952][1];
    } else {
      m = mn_952_1011[cabac_init_idc + 1][ctxIdx - 952][0];
      n = mn_952_1011[cabac_init_idc + 1][ctxIdx - 952][1];
    }
  } else if (ctxIdx >= 1012 && ctxIdx <= 1023) {
    // Table 9-33 – Values of variables m and n for ctxIdx from 1012 to 1023
    if (slice_type == SLICE_I || slice_type == SLICE_SI) {
      m = mn_1012_1023[0][ctxIdx - 1012][0];
      n = mn_1012_1023[0][ctxIdx - 1012][1];
    } else {
      m = mn_1012_1023[cabac_init_idc + 1][ctxIdx - 1012][0];
      n = mn_1012_1023[cabac_init_idc + 1][ctxIdx - 1012][1];
    }
  } else
    RET(-1);
  return 0;
}

// 9.3.1.1 Initialisation process for context variables
int Cabac::init_of_context_variables(H264_SLICE_TYPE slice_type,
                                     int32_t cabac_init_idc, int32_t SliceQPY) {
  int m = 0, n = 0;
  // 遍历所有的 CABAC 上下文索引
  for (int ctxIdx = 0; ctxIdx < 1024; ++ctxIdx) {
    // 不同的片类型（I、P、B）使用不同的上下文模型，因此 m 和 n 是根据片类型来选择的，表9-12到表9-23包含了上下文变量初始化时使用的变量n和m的值
    if (init_m_n(ctxIdx, slice_type, cabac_init_idc, m, n) != 0) continue;
    int preCtxState = CLIP3(1, 126, ((m * CLIP3(0, 51, SliceQPY)) >> 4) + n);
    // preCtxState: 判断 CABAC 的起始状态属于 MPS（Most Probable Symbol，最可能符号）还是 LPS（Less Probable Symbol，较少可能符号）
    if (preCtxState <= 63) // 初始化为 LPS
      pStateIdxs[ctxIdx] = 63 - preCtxState, valMPSs[ctxIdx] = 0;
    else // 初始化为 MPS
      pStateIdxs[ctxIdx] = preCtxState - 64, valMPSs[ctxIdx] = 1;
  }
  return 0;
}

// 9.3.1.2 Initialisation process for the arithmetic decoding engine
int Cabac::init_of_decoding_engine() {
  // 编码间隔：较大的值代表了最大的概率区间, 编码位移
  codIRange = 510, codIOffset = bs.readUn(9);
  RET(codIOffset == 510 || codIOffset == 511);
  return 0;
}

//NOTE: 首次调用的CABAC解码
/* 9.3.3.1.1.1 Derivation process of ctxIdxInc for the syntax element mb_skip_flag */
int Cabac::decode_mb_skip_flag(int32_t currMbAddr, int32_t &synElVal) {
  const int slice_type = picture.m_slice->slice_header->slice_type % 5;
  // Table 9-34 : slice_type == SLICE_P,SLICE_SP, SLICE_B)
  int32_t ctxIdxOffset = (slice_type == SLICE_B) ? 24 : 11;

  // 获取ctxIdxInc后才能进行算术解码
  int32_t ctxIdxInc;
  RET(derivation_ctxIdxInc_for_mb_skip_flag(currMbAddr, ctxIdxInc));

  //NOTE:如果没有为表 9-34 中标记的相应二值化或二值化部分的 ctxIdxOffset 分配值作为“na”，相应二值化或二值化前缀/后缀部分的比特串的所有bin通过调用第9.3.3.2.3节中指定的DecodeBypass过程来解码。在这种情况下，bypassFlag被设置为等于1，其中bypassFlag用于指示为了从比特流解析bin的值，应用DecodeBypass过程。

  int bypassFlag = (ctxIdxOffset == NA);
  int ctxIdx = ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(bypassFlag, ctxIdx, synElVal));
  return 0;
}

/* 9.3.3.1.1.1 Derivation process of ctxIdxInc for the syntax element mb_skip_flag */
/* 输出: ctxIdxInc */
int Cabac::derivation_ctxIdxInc_for_mb_skip_flag(int32_t currMbAddr,
                                                 int32_t &ctxIdxInc) {
  const bool MbaffFrameFlag = picture.m_slice->slice_header->MbaffFrameFlag;
  int32_t mbAddrA = 0, mbAddrB = 0;
  RET(picture.derivation_for_neighbouring_macroblocks(
      MbaffFrameFlag, currMbAddr, mbAddrA, mbAddrB, 0));

  int32_t condTermFlagA =
      (mbAddrA < 0 || picture.m_mbs[mbAddrA].mb_skip_flag) ? 0 : 1;
  int32_t condTermFlagB =
      (mbAddrB < 0 || picture.m_mbs[mbAddrB].mb_skip_flag) ? 0 : 1;

  // ctxIdxInc = [0,2]
  ctxIdxInc = condTermFlagA + condTermFlagB;

  return 0;
}

// 9.3.3.1.1.2 Derivation process of ctxIdxInc for the syntax element mb_field_decoding_flag
int Cabac::decode_mb_field_decoding_flag(int32_t &synElVal) {
  int ctxIdxInc = 0;
  RET(derivation_ctxIdxInc_for_mb_field_decoding_flag(ctxIdxInc));

  int ctxIdxOffset = 70;
  int bypassFlag = (ctxIdxOffset == -1) ? 1 : 0;
  int ctxIdx = ctxIdxOffset + ctxIdxInc;

  int &bin = synElVal;
  RET(decodeBin(bypassFlag, ctxIdx, bin)); // binIdx = 0;
  return 0;
}

// 9.3.3.1.1.2 Derivation process of ctxIdxInc for the syntax element mb_field_decoding_flag
int Cabac::derivation_ctxIdxInc_for_mb_field_decoding_flag(int32_t &ctxIdxInc) {

  /* Table 6-4 – Specification of mbAddrN and yM */
  int32_t mbAddrA, mbAddrB, mbAddrC, mbAddrD;
  /* 调用第 6.4.10 节中指定的相邻宏块地址及其在 MBAFF 帧中的可用性的导出过程，并将输出分配给 mbAddrA 和 mbAddrB。  */
  picture.derivation_for_neighbouring_macroblock_addr_availability_in_MBAFF(
      mbAddrA, mbAddrB, mbAddrC, mbAddrD);

  /* 当宏块mbAddrN和mbAddrN+1两者都具有等于P_Skip或B_Skip的mb_type时，将如第7.4.4节中指定的语法元素mb_field_decoding_flag的推断规则应用于宏块mbAddrN。  
   * 让变量 condTermFlagN（N 为 A 或 B）按如下方式导出： 
   * — 如果以下任一条件为真，则 condTermFlagN 设置为等于 0： 
      * — mbAddrN 不可用， 
      * — 宏块 mbAddrN 是帧宏块。  
   * – 否则，condTermFlagN 设置为等于 1。*/

  int32_t condTermFlagA = 1, condTermFlagB = 1;
  if (mbAddrA < 0 || picture.m_mbs[mbAddrA].mb_field_decoding_flag == 0)
    condTermFlagA = 0;

  if (mbAddrB < 0 || picture.m_mbs[mbAddrB].mb_field_decoding_flag == 0)
    condTermFlagB = 0;

  // 变量 ctxIdxInc 由以下公式得出： ctxIdxInc = condTermFlagA + condTermFlagB
  ctxIdxInc = condTermFlagA + condTermFlagB;

  return 0;
}

// 9.3.3.1.1.3 Derivation process of ctxIdxInc for the syntax element mb_type
int Cabac::decode_mb_type(int32_t &synElVal) {
  H264_SLICE_TYPE slice_type =
      (H264_SLICE_TYPE)picture.m_slice->slice_header->slice_type;
  int ret = 0;
  if ((slice_type % 5) == SLICE_SI)
    ret = decode_mb_type_in_SI_slices(synElVal);
  else if ((slice_type % 5) == SLICE_I)
    ret = decode_mb_type_in_I_slices(3, synElVal);
  else if ((slice_type % 5) == SLICE_P || (slice_type % 5) == SLICE_SP)
    ret = decode_mb_type_in_P_SP_slices(synElVal);
  else if ((slice_type % 5) == SLICE_B)
    ret = decode_mb_type_in_B_slices(synElVal);
  else
    RET(-1);
  RET(ret);
  return 0;
}

int Cabac::decode_mb_type_in_SI_slices(int32_t &synElVal) {
  H264_SLICE_TYPE slice_type =
      (H264_SLICE_TYPE)picture.m_slice->slice_header->slice_type;
  RET((slice_type % 5) != SLICE_SI);

  int32_t ctxIdxOffset = 0, ctxIdxInc = 0;
  RET(derivation_ctxIdxInc_for_mb_type(ctxIdxOffset, ctxIdxInc));
  int32_t binVal = 0;
  RET(decodeBin((ctxIdxOffset == NA), ctxIdxOffset + ctxIdxInc, binVal));

  if (binVal == 0)
    synElVal = 0;
  else {
    RET(decode_mb_type_in_I_slices(3, synElVal));
    synElVal += 1;
  }

  return 0;
}

int Cabac::decode_mb_type_in_I_slices(int32_t ctxIdxOffset, int32_t &synElVal) {
  int32_t ctxIdxInc = 0;
  if (ctxIdxOffset == 3) {
    RET(derivation_ctxIdxInc_for_mb_type(ctxIdxOffset, ctxIdxInc));
  }

  int32_t binVal = 0;
  int32_t ctxIdx = ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(false, ctxIdx, binVal));

  if (binVal == 0)
    synElVal = 0;
  else {
    ctxIdx = 276;
    RET(decodeBin(false, ctxIdx, binVal));

    if (binVal == 0) //(10)b
    {
      ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 3 : 1);
      RET(decodeBin(false, ctxIdx, binVal));

      if (binVal == 0) //(100)b
      {
        ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 4 : 2);
        RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 3; b3 = binVal;

        if (binVal == 0) //(1000)b //b3=0
        {
          if (ctxIdxOffset == 3) // I slice
          {
            ctxIdx = ctxIdxOffset + 6; //(b3 != 0) ? 5: 6; b3=0; //Table 9-41
          } else                       // P/SP/B slice
          {
            ctxIdx = ctxIdxOffset + 3; //(b3 != 0) ? 2: 3; b3=0; //Table 9-41
          }

          RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(10000)b
          {
            //(b3 != 0) ? 6: 7; b3=0; //Table 9-41
            ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
            RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(100000)b
            {
              synElVal = 1; // 1 (I_16x16_0_0_0)
            } else          // if (binVal == 1) //(100001)b
            {
              synElVal = 2; // 2 (I_16x16_1_0_0)
            }
          } else if (binVal == 1) //(10001)b
          {
            //(b3 != 0) ? 6: 7; b3=0; //Table 9-41
            ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
            RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(100010)b
            {
              synElVal = 3; // 3 (I_16x16_2_0_0)
            } else          // if (binVal == 1) //(100011)b
            {
              synElVal = 4; // 4 (I_16x16_3_0_0)
            }
          }
        } else if (binVal == 1) //(1001)b //b3=1
        {
          if (ctxIdxOffset == 3) // I slice
          {
            ctxIdx = ctxIdxOffset + 5; //(b3 != 0) ? 5: 6; b3=1; //Table 9-41
          } else                       // P/SP/B slice
          {
            ctxIdx = ctxIdxOffset + 2; //(b3 != 0) ? 2: 3; b3=1; //Table 9-41
          }

          RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(10010)b
          {
            //(b3 != 0) ? 6: 7; b3=1; //Table 9-41
            ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 6 : 3);
            RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(100100)b
            {
              ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
              RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1001000)b
              {
                synElVal = 5; // 5 (I_16x16_0_1_0)
              } else          // if (binVal == 1) //(1001001)b
              {
                synElVal = 6; // 6 (I_16x16_1_1_0)
              }
            } else if (binVal == 1) //(100101)b
            {
              ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
              RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1001010)b
              {
                synElVal = 7; // 7 (I_16x16_2_1_0)
              } else          // if (binVal == 1) //(1001011)b
              {
                synElVal = 8; // 8 (I_16x16_3_1_0)
              }
            }
          } else if (binVal == 1) //(10011)b
          {
            //(b3 != 0) ? 6: 7; b3=1; //Table 9-41
            ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 6 : 3);
            RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(100110)b
            {
              ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
              RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1001100)b
              {
                synElVal = 9; // 9 (I_16x16_0_2_0)
              } else          // if (binVal == 1) //(1001101)b
              {
                synElVal = 10; // 10 (I_16x16_1_2_0)
              }
            } else if (binVal == 1) //(100111)b
            {
              ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
              RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1001110)b
              {
                synElVal = 11; // 11 (I_16x16_2_2_0)
              } else           // if (binVal == 1) //(1001111)b
              {
                synElVal = 12; // 12 (I_16x16_3_2_0)
              }
            }
          }
        }
      } else if (binVal == 1) //(101)b
      {
        ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 4 : 2);
        RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 3; b3=binVal;

        if (binVal == 0) //(1010)b //b3=0
        {
          if (ctxIdxOffset == 3) // I slice
          {
            ctxIdx = ctxIdxOffset + 6; //(b3 != 0) ? 5: 6; b3=0; //Table 9-41
          } else                       // P/SP/B slice
          {
            ctxIdx = ctxIdxOffset + 3; //(b3 != 0) ? 2: 3; b3=0; //Table 9-41
          }

          RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(10100)b
          {
            //(b3 != 0) ? 6: 7; b3=0; //Table 9-41
            ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
            RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(101000)b
            {
              synElVal = 13; // 13 (I_16x16_0_0_1)
            } else           // if (binVal == 1) //(101001)b
            {
              synElVal = 14; // 14 (I_16x16_1_0_1)
            }
          } else if (binVal == 1) //(10101)b
          {
            //(b3 != 0) ? 6: 7; b3=0; //Table 9-41
            ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
            RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(101010)b
            {
              synElVal = 15; // 15 (I_16x16_2_0_1)
            } else           // if (binVal == 1) //(101011)b
            {
              synElVal = 16; // 16 (I_16x16_3_0_1)
            }
          }
        } else if (binVal == 1) //(1011)b //b3=1
        {
          if (ctxIdxOffset == 3) // I slice
          {
            ctxIdx = ctxIdxOffset + 5; //(b3 != 0) ? 5: 6; b3=1; //Table 9-41
          } else                       // P/SP/B slice
          {
            ctxIdx = ctxIdxOffset + 2; //(b3 != 0) ? 2: 3; b3=1; //Table 9-41
          }

          RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(10110)b
          {
            //(b3 != 0) ? 6: 7; b3=1; //Table 9-41
            ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 6 : 3);
            RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(101100)b
            {
              ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
              RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1011000)b
              {
                synElVal = 17; // 17 (I_16x16_0_1_1)
              } else           // if (binVal == 1) //(1011001)b
              {
                synElVal = 18; // 18 (I_16x16_1_1_1)
              }
            } else if (binVal == 1) //(101101)b
            {
              ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
              RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1011010)b
              {
                synElVal = 19; // 19 (I_16x16_2_1_1)
              } else           // if (binVal == 1) //(1011011)b
              {
                synElVal = 20; // 20 (I_16x16_3_1_1)
              }
            }
          } else if (binVal == 1) //(10111)b
          {
            //(b3 != 0) ? 6: 7; b3=1; //Table 9-41
            ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 6 : 3);
            RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(101110)b
            {
              ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
              RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1011100)b
              {
                synElVal = 21; // 21 (I_16x16_0_2_1)
              } else           // if (binVal == 1) //(1011101)b
              {
                synElVal = 22; // 22 (I_16x16_1_2_1)
              }
            } else if (binVal == 1) //(101111)b
            {
              ctxIdx = ctxIdxOffset + ((ctxIdxOffset == 3) ? 7 : 3);
              RET(decodeBin(false, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1011110)b
              {
                synElVal = 23; // 23 (I_16x16_2_2_1)
              } else           // if (binVal == 1) //(1011111)b
              {
                synElVal = 24; // 24 (I_16x16_3_2_1)
              }
            }
          }
        }
      }
    } else           // if (binVal == 1) //(11)b
      synElVal = 25; // 25 (I_PCM)
  }

  if (synElVal == 25) //&& value(b0,b1,...,bbinIdx) == I_PCM = 25
    init_of_decoding_engine();

  return 0;
}

int Cabac::decode_mb_type_in_P_SP_slices(int32_t &synElVal) {

  H264_SLICE_TYPE slice_type =
      (H264_SLICE_TYPE)picture.m_slice->slice_header->slice_type;
  int32_t ctxIdxOffset = 0, ctxIdx = 0;
  int32_t binVal = 0;
  int32_t bypassFlag = 0;

  RET((slice_type % 5) != SLICE_P && (slice_type % 5) != SLICE_SP);

  //------Table 9-34: ctxIdxOffset-prefix: 14--------
  ctxIdxOffset = 14;

  // Table 9-39 – Assignment of ctxIdxInc to binIdx for all ctxIdxOffset values except those related to the syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1

  ctxIdx = ctxIdxOffset + 0; // ctxIdxOffset + ctxIdxInc = 14 + 0;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

  if (binVal == 0) //(0)b
  {
    ctxIdx = ctxIdxOffset + 1;
    RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1; b1=binVal;

    if (binVal == 0) //(00)b //b1=0
    {
      //(b1 != 1) ? 2: 3; b1=0; //Table 9-41 //2,3 (clause 9.3.3.1.2)
      ctxIdx = ctxIdxOffset + 2;
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2;

      if (binVal == 0) //(000)b
      {
        synElVal = 0; // 0 (P_L0_16x16)
      } else          // if (binVal == 1) //(001)b
      {
        synElVal = 3; // 3 (P_8x8)
      }
    } else // if (binVal == 1) //(01)b //b1=1
    {
      //(b1 != 1) ? 2: 3; b1=0; //Table 9-41 //2,3 (clause 9.3.3.1.2)
      ctxIdx = ctxIdxOffset + 3;
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2;

      if (binVal == 0) //(010)b
      {
        synElVal = 2; // 2 (P_L0_L0_8x16)
      } else          // if (binVal == 1) //(011)b
      {
        synElVal = 1; // 1 (P_L0_L0_16x8)
      }
    }
  } else {
    //------Table 9-34: ctxIdxOffset-suffix: 17--------
    RET(decode_mb_type_in_I_slices(17, synElVal));
    synElVal += 5;
  }

  return 0;
}

int Cabac::decode_mb_type_in_B_slices(int32_t &synElVal) {
  H264_SLICE_TYPE slice_type =
      (H264_SLICE_TYPE)picture.m_slice->slice_header->slice_type;
  int32_t ctxIdxOffset = 0, ctxIdxInc = 0;
  int32_t binVal = 0;
  int32_t ctxIdx = 0;
  int32_t bypassFlag = 0;

  RET((slice_type % 5) != SLICE_B);

  //------Table 9-34: ctxIdxOffset-prefix: 27--------
  ctxIdxOffset = 27;

  // Table 9-39 – Assignment of ctxIdxInc to binIdx for all ctxIdxOffset values except those related to the syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1

  // 0,1,2 (clause 9.3.3.1.1.3)
  RET(derivation_ctxIdxInc_for_mb_type(ctxIdxOffset, ctxIdxInc));
  ctxIdx = ctxIdxOffset + ctxIdxInc;

  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

  if (binVal == 0) //(0)b
    synElVal = 0;  // 0 (B_Direct_16x16)
  else             // if (binVal == 1) //(1)b
  {
    ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
    RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1; b1=binVal;

    if (binVal == 0) //(10)b //b1=0
    {
      //(b1 != 0) ? 4: 5; b1=0; //Table 9-41 //2,3 (clause 9.3.3.1.2)
      ctxIdx = ctxIdxOffset + 5;
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2;

      if (binVal == 0) //(100)b
        synElVal = 1;  // 1 (B_L0_16x16)
      else             // if (binVal == 1) //(101)b
        synElVal = 2;  // 2 (B_L1_16x16)
    } else {
      //(b1 != 0) ? 4: 5; b1=1; //Table 9-41 //2,3 (clause 9.3.3.1.2)
      ctxIdx = ctxIdxOffset + 4;
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2;

      if (binVal == 0) //(110)b
      {
        ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
        RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 3;

        if (binVal == 0) //(1100)b
        {
          ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
          RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(11000)b
          {
            ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
            RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(110000)b
            {
              synElVal = 3; // 3 (B_Bi_16x16)
            } else          // if (binVal == 1) //(110001)b
            {
              synElVal = 4; // 4 (B_L0_L0_16x8)
            }
          } else // if (binVal == 1) //(11001)b
          {
            ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
            RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(110010)b
            {
              synElVal = 5; // 5 (B_L0_L0_8x16)
            } else          // if (binVal == 1) //(110011)b
            {
              synElVal = 6; // 6 (B_L1_L1_16x8)
            }
          }
        } else // if (binVal == 1) //(1101)b
        {
          ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
          RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(11010)b
          {
            ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
            RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(110100)b
            {
              synElVal = 7; // 7 (B_L1_L1_8x16)
            } else          // if (binVal == 1) //(110101)b
            {
              synElVal = 8; // 8 (B_L0_L1_16x8)
            }
          } else // if (binVal == 1) //(11011)b
          {
            ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
            RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(110110)b
            {
              synElVal = 9; // 9 (B_L0_L1_8x16)
            } else          // if (binVal == 1) //(110111)b
            {
              synElVal = 10; // 10 (B_L1_L0_16x8)
            }
          }
        }
      } else // if (binVal == 1) //(111)b
      {
        ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
        RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 3;

        if (binVal == 0) //(1110)b
        {
          ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
          RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(11100)b
          {
            ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
            RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(111000)b
            {
              ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
              RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1110000)b
              {
                synElVal = 12; // 12 (B_L0_Bi_16x8)
              } else           // if (binVal == 1) //(1110001)b
              {
                synElVal = 13; // 13 (B_L0_Bi_8x16)
              }
            } else // if (binVal == 1) //(111001)b
            {
              ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
              RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1110010)b
              {
                synElVal = 14; // 14 (B_L1_Bi_16x8)
              } else           // if (binVal == 1) //(1110011)b
              {
                synElVal = 15; // 15 (B_L1_Bi_8x16)
              }
            }
          } else // if (binVal == 1) //(11101)b
          {
            ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
            RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(111010)b
            {
              ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
              RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1110100)b
              {
                synElVal = 16; // 16 (B_Bi_L0_16x8)
              } else           // if (binVal == 1) //(1110101)b
              {
                synElVal = 17; // 17 (B_Bi_L0_8x16)
              }
            } else // if (binVal == 1) //(111011)b
            {
              ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
              RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1110110)b
              {
                synElVal = 18; // 18 (B_Bi_L1_16x8)
              } else           // if (binVal == 1) //(1110111)b
              {
                synElVal = 19; // 19 (B_Bi_L1_8x16)
              }
            }
          }
        } else // if (binVal == 1) //(1111)b
        {
          ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
          RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(11110)b
          {
            ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
            RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(111100)b
            {
              ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
              RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 6;

              if (binVal == 0) //(1111000)b
              {
                synElVal = 20; // 20 (B_Bi_Bi_16x8)
              } else           // if (binVal == 1) //(1111001)b
              {
                synElVal = 21; // 21 (B_Bi_Bi_8x16)
              }
            } else // if (binVal == 1) //(111101)b
            {
              // 23 to 48 (Intra, prefix only)

              //------Table 9-34: ctxIdxOffset-suffix: 32--------
              // maxBinIdxCtx = 5;
              ctxIdxOffset = 32;

              RET(decode_mb_type_in_I_slices(ctxIdxOffset, synElVal));

              synElVal += 23;
            }
          } else // if (binVal == 1) //(11111)b
          {
            ctxIdx = ctxIdxOffset + 5;                  // Table 9-39
            RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(111110)b
              synElVal = 11; // 11 (B_L1_L0_8x16)
            else
              synElVal = 22; // 22 (B_8x8)
          }
        }
      }
    }
  }

  return 0;
}

// 9.3.3.1.1.3 Derivation process of ctxIdxInc for the syntax element mb_type
int Cabac::derivation_ctxIdxInc_for_mb_type(int32_t ctxIdxOffset,
                                            int32_t &ctxIdxInc) {
  const bool MbaffFrameFlag = picture.m_slice->slice_header->MbaffFrameFlag;

  int32_t mbAddrA = 0, mbAddrB = 0;

  // 6.4.11.1 Derivation process for neighbouring macroblocks
  RET(picture.derivation_for_neighbouring_macroblocks(
      MbaffFrameFlag, picture.CurrMbAddr, mbAddrA, mbAddrB, 0));

  int32_t condTermFlagA = 1, condTermFlagB = 1;

  if (mbAddrA < 0 ||
      (ctxIdxOffset == 0 && picture.m_mbs[mbAddrA].m_name_of_mb_type == SI) ||
      (ctxIdxOffset == 3 &&
       picture.m_mbs[mbAddrA].m_name_of_mb_type == I_NxN) ||
      (ctxIdxOffset == 27 &&
       (picture.m_mbs[mbAddrA].m_name_of_mb_type == B_Skip ||
        picture.m_mbs[mbAddrA].m_name_of_mb_type == B_Direct_16x16)))
    condTermFlagA = 0;

  if (mbAddrB < 0 ||
      (ctxIdxOffset == 0 && picture.m_mbs[mbAddrB].m_name_of_mb_type == SI) ||
      (ctxIdxOffset == 3 &&
       picture.m_mbs[mbAddrB].m_name_of_mb_type == I_NxN) ||
      (ctxIdxOffset == 27 &&
       (picture.m_mbs[mbAddrB].m_name_of_mb_type == B_Skip ||
        picture.m_mbs[mbAddrB].m_name_of_mb_type == B_Direct_16x16)))
    condTermFlagB = 0;

  ctxIdxInc = condTermFlagA + condTermFlagB;

  return 0;
}

int Cabac::decode_sub_mb_type(int32_t &synElVal) {
  H264_SLICE_TYPE slice_type =
      (H264_SLICE_TYPE)picture.m_slice->slice_header->slice_type;
  int ret = 0;
  if ((slice_type % 5) == SLICE_P || (slice_type % 5) == SLICE_SP)
    ret = decode_sub_mb_type_in_P_SP_slices(synElVal);
  else if ((slice_type % 5) == SLICE_B)
    ret = decode_sub_mb_type_in_B_slices(synElVal);
  else
    RET(-1);
  RET(ret);
  return 0;
}

int Cabac::decode_sub_mb_type_in_P_SP_slices(int32_t &synElVal) {

  H264_SLICE_TYPE slice_type =
      (H264_SLICE_TYPE)picture.m_slice->slice_header->slice_type;
  int32_t ctxIdxOffset = 0, ctxIdx = 0;
  int32_t binVal = 0;
  int32_t bypassFlag = 0;

  RET((slice_type % 5) != SLICE_P && (slice_type % 5) != SLICE_SP);

  //------Table 9-34: ctxIdxOffset: 21--------
  ctxIdxOffset = 21;

  // Table 9-39 – Assignment of ctxIdxInc to binIdx for all ctxIdxOffset values except those related to the syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1
  ctxIdx = ctxIdxOffset + 0;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

  if (binVal == 1)      //(1)b
    synElVal = 0;       // 0 (P_L0_8x8)
  else if (binVal == 0) //(0)b
  {
    ctxIdx = ctxIdxOffset + 1;                  // Table 9-39
    RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1;

    if (binVal == 0) //(00)b //b1=0
      synElVal = 1;  // 1 (P_L0_8x4)
    else             // if (binVal == 1) //(01)b //b1=1
    {
      ctxIdx = ctxIdxOffset + 2;                  // Table 9-39
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2;

      if (binVal == 1) //(011)b
        synElVal = 2;  // 2 (P_L0_4x8)
      else             // if (binVal == 0) //(010)b
        synElVal = 3;  // 3 (P_L0_4x4)
    }
  }

  return 0;
}

int Cabac::decode_sub_mb_type_in_B_slices(int32_t &synElVal) {

  H264_SLICE_TYPE slice_type =
      (H264_SLICE_TYPE)picture.m_slice->slice_header->slice_type;
  int32_t ctxIdxOffset = 0, ctxIdx = 0;
  int32_t binVal = 0;
  int32_t bypassFlag = 0;

  RET((slice_type % 5) != SLICE_B);

  //------Table 9-34: ctxIdxOffset: 36--------
  ctxIdxOffset = 36;

  // Table 9-39 – Assignment of ctxIdxInc to binIdx for all ctxIdxOffset values except those related to the syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1

  ctxIdx = ctxIdxOffset + 0; // ctxIdxOffset + ctxIdxInc = 36 + 0;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

  if (binVal == 0) //(0)b
    synElVal = 0;  // 0 (B_Direct_8x8)
  else             // if (binVal == 1) //(1)b
  {
    ctxIdx = ctxIdxOffset + 1;                  // Table 9-39
    RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1; b1=binVal;

    if (binVal == 0) //(10)b //b1=0
    {
      //(b1 != 0) ? 2: 3; b1=0; //Table 9-41 //2,3 (clause 9.3.3.1.2)
      ctxIdx = ctxIdxOffset + 3;
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2;

      if (binVal == 0) //(100)b
        synElVal = 1;  // 1 (B_L0_8x8)
      else             // if (binVal == 1) //(101)b
        synElVal = 2;  // 2 (B_L1_8x8)
    } else             // if (binVal == 1) //(11)b //b1=1
    {
      //(b1 != 0) ? 2: 3; b1=0; //Table 9-41 //2,3 (clause 9.3.3.1.2)
      ctxIdx = ctxIdxOffset + 2;
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2;

      if (binVal == 0) //(110)b
      {
        ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
        RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 3;

        if (binVal == 0) //(1100)b
        {
          ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
          RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(11000)b
            synElVal = 3;  // 3 (B_Bi_8x8)
          else             // if (binVal == 1) //(11001)b
            synElVal = 4;  // 4 (B_L0_8x4)
        } else             // if (binVal == 1) //(1101)b
        {
          ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
          RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(11010)b
            synElVal = 5;  // 5 (B_L0_4x8)
          else             // if (binVal == 1) //(11011)b
            synElVal = 6;  // 6 (B_L1_8x4)
        }
      } else // if (binVal == 1) //(111)b
      {
        ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
        RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 3;

        if (binVal == 0) //(1110)b
        {
          ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
          RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(11100)b
          {
            ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
            RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(111000)b
              synElVal = 7;  // 7 (B_L1_4x8)
            else             // if (binVal == 1) //(111001)b
              synElVal = 8;  // 8 (B_Bi_8x4)
          } else             // if (binVal == 1) //(11101)b
          {
            ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
            RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 5;

            if (binVal == 0) //(111010)b
              synElVal = 9;  // 9 (B_Bi_4x8)
            else             // if (binVal == 1) //(111011)b
              synElVal = 10; // 10 (B_L0_4x4)
          }
        } else // if (binVal == 1) //(1111)b
        {
          ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
          RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 4;

          if (binVal == 0) //(11110)b
            synElVal = 11; // 11 (B_L1_4x4)
          else
            synElVal = 12; // 12 (B_Bi_4x4)
        }
      }
    }
  }

  return 0;
}

// 9.3.3.1.1.4 Derivation process of ctxIdxInc for the syntax element coded_block_pattern
int Cabac::decode_coded_block_pattern(int32_t &synElVal) {
  int32_t ChromaArrayType =
      picture.m_slice->slice_header->m_sps->ChromaArrayType;
  int32_t ctxIdxOffset = 0, ctxIdxInc = 0, ctxIdx = 0;
  int32_t binIdx = -1, binVal = 0, binValues = 0;
  int32_t bypassFlag = 0;

  //------Table 9-34: ctxIdxOffset-prefix: 73--------
  ctxIdxOffset = 73;

  //------b0--------
  binIdx = 0;
  RET(derivation_ctxIdxInc_for_coded_block_pattern(binIdx, binValues,
                                                   ctxIdxOffset, ctxIdxInc));

  ctxIdx = ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;
  binValues = binVal;

  //------b1--------
  binIdx = 1;
  RET(derivation_ctxIdxInc_for_coded_block_pattern(binIdx, binValues,
                                                   ctxIdxOffset, ctxIdxInc));

  ctxIdx = ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1;
  binValues += binVal << 1;

  //------b2--------
  binIdx = 2;
  RET(derivation_ctxIdxInc_for_coded_block_pattern(binIdx, binValues,
                                                   ctxIdxOffset, ctxIdxInc));

  ctxIdx = ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2;
  binValues += binVal << 2;

  //------b3--------
  binIdx = 3;
  RET(derivation_ctxIdxInc_for_coded_block_pattern(binIdx, binValues,
                                                   ctxIdxOffset, ctxIdxInc));

  ctxIdx = ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 3;
  binValues += binVal << 3;

  int32_t CodedBlockPatternLuma = binValues; //[0,15]

  //------Table 9-34: ctxIdxOffset-suffix: 77--------
  // 后缀部分CodedBlockPatternChroma：TU, cMax=2

  int32_t CodedBlockPatternChroma = 0; // 0,1,2
  if (ChromaArrayType != 0 && ChromaArrayType != 3) {
    ctxIdxOffset = 77;
    binValues = 0;
    binIdx = 0;
    RET(derivation_ctxIdxInc_for_coded_block_pattern(binIdx, binValues,
                                                     ctxIdxOffset, ctxIdxInc));

    ctxIdx = ctxIdxOffset + ctxIdxInc;
    RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

    if (binVal == 0) //(0)b
      CodedBlockPatternChroma = 0;
    else {
      CodedBlockPatternChroma = 1;
      binIdx = 1;
      RET(derivation_ctxIdxInc_for_coded_block_pattern(
          binIdx, binValues, ctxIdxOffset, ctxIdxInc));

      ctxIdx = ctxIdxOffset + ctxIdxInc;
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1;

      if (binVal == 1)               //(11)b
        CodedBlockPatternChroma = 2; // cMax = 2
    }
  }

  synElVal = CodedBlockPatternLuma + CodedBlockPatternChroma * 16;

  return 0;
}

// 9.3.3.1.1.4 Derivation process of ctxIdxInc for the syntax element coded_block_pattern
int Cabac::derivation_ctxIdxInc_for_coded_block_pattern(int32_t binIdx,
                                                        int32_t binValues,
                                                        int32_t ctxIdxOffset,
                                                        int32_t &ctxIdxInc) {
  const bool MbaffFrameFlag = picture.m_slice->slice_header->MbaffFrameFlag;

  if (ctxIdxOffset == 73) {
    int32_t luma8x8BlkIdx = binIdx;
    int32_t mbAddrA = 0, mbAddrB = 0;
    int32_t luma8x8BlkIdxA = 0, luma8x8BlkIdxB = 0;
    int32_t isChroma = 0;

    // 6.4.11.2 Derivation process for neighbouring 8x8 luma block
    RET(picture.derivation_for_neighbouring_8x8_luma_block(
        luma8x8BlkIdx, mbAddrA, mbAddrB, luma8x8BlkIdxA, luma8x8BlkIdxB,
        isChroma));

    int32_t condTermFlagA = 1, condTermFlagB = 1;

    //-------A-----------
    if (mbAddrA < 0 || picture.m_mbs[mbAddrA].m_name_of_mb_type == I_PCM ||
        (mbAddrA != picture.CurrMbAddr &&
         (picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
          picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip) &&
         ((picture.m_mbs[mbAddrA].CodedBlockPatternLuma >> luma8x8BlkIdxA) &
          1) != 0) ||
        (mbAddrA == picture.CurrMbAddr &&
         ((binValues >> luma8x8BlkIdxA) & 0x01) != 0))
      condTermFlagA = 0;

    //-------B-----------
    if (mbAddrB < 0 || picture.m_mbs[mbAddrB].m_name_of_mb_type == I_PCM ||
        (mbAddrB != picture.CurrMbAddr &&
         (picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
          picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip) &&
         ((picture.m_mbs[mbAddrB].CodedBlockPatternLuma >> luma8x8BlkIdxB) &
          1) != 0) ||
        (mbAddrB == picture.CurrMbAddr &&
         ((binValues >> luma8x8BlkIdxB) & 0x01) != 0))
      condTermFlagB = 0;

    ctxIdxInc = condTermFlagA + 2 * condTermFlagB;
  } else {
    int32_t mbAddrA = 0, mbAddrB = 0;
    const int32_t isChroma = 0;

    // 6.4.11.1 Derivation process for neighbouring macroblocks
    RET(picture.derivation_for_neighbouring_macroblocks(
        MbaffFrameFlag, picture.CurrMbAddr, mbAddrA, mbAddrB, isChroma));

    int32_t condTermFlagA = 1, condTermFlagB = 1;
    //-----------A----------------
    if (mbAddrA >= 0 && picture.m_mbs[mbAddrA].m_name_of_mb_type == I_PCM)
      condTermFlagA = 1;
    else if (mbAddrA < 0 ||
             picture.m_mbs[mbAddrA].m_name_of_mb_type == P_Skip ||
             picture.m_mbs[mbAddrA].m_name_of_mb_type == B_Skip ||
             (binIdx == 0 &&
              picture.m_mbs[mbAddrA].CodedBlockPatternChroma == 0) ||
             (binIdx == 1 &&
              picture.m_mbs[mbAddrA].CodedBlockPatternChroma != 2))
      condTermFlagA = 0;

    //-----------B----------------
    if (mbAddrB >= 0 && picture.m_mbs[mbAddrB].m_name_of_mb_type == I_PCM)
      condTermFlagB = 1;
    else if (mbAddrB < 0 ||
             picture.m_mbs[mbAddrB].m_name_of_mb_type == P_Skip ||
             picture.m_mbs[mbAddrB].m_name_of_mb_type == B_Skip ||
             (binIdx == 0 &&
              picture.m_mbs[mbAddrB].CodedBlockPatternChroma == 0) ||
             (binIdx == 1 &&
              picture.m_mbs[mbAddrB].CodedBlockPatternChroma != 2))
      condTermFlagB = 0;

    ctxIdxInc = condTermFlagA + 2 * condTermFlagB + ((binIdx == 1) ? 4 : 0);
  }

  return 0;
}

//9.3.3.1.1.5 Derivation process of ctxIdxInc for the syntax element mb_qp_delta
int Cabac::decode_mb_qp_delta(int32_t &synElVal) {
  int32_t ctxIdxOffset = 0, ctxIdxInc = 0, ctxIdx = 0;
  int32_t binIdx = -1, binVal = 0;
  int32_t bypassFlag = 0;
  int32_t bit_depth_luma = 8;

  //------Table 9-34: ctxIdxOffset: 60--------
  ctxIdxOffset = 60;

  // 9.3.2.7 Binarization process for mb_qp_delta

  // 9.3.3.1.1.5 Derivation process of ctxIdxInc for the syntax element
  RET(derivation_ctxIdxInc_for_mb_qp_delta(ctxIdxInc));

  //---------------注意是：U binarization------------------------
  ctxIdx = ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

  // unary (U) binarization即一元二值化，就是类似 (111...1110)b这样的二进制字符串，最后一个二进制值为0，其他都是1，其中1的个数就是对应的语法元素的值
  if (binVal == 0) //(0)b
    synElVal = 0;
  else {
    ctxIdx = ctxIdxOffset + 2;                  // Table 9-39
    RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1;

    int mb_qp_max = 51 + 6 * (bit_depth_luma - 8);
    binIdx = 1;

    while (binVal == 1) //(11...1)b
    {
      ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2,3,4.,..,k;

      binIdx++;

      if (binIdx > 2 * mb_qp_max) {
        // mb_qp_delta值太大了
        std::cerr << "An error occurred on binIdx:" << binIdx << " > "
                  << 2 * mb_qp_max << " ," << __FUNCTION__ << "():" << __LINE__
                  << std::endl;
        binIdx = 2 * mb_qp_max;
        break;
      }
    }

    //----------Table 9-3 se(v)-------------
    if (binIdx & 0x01)               // 奇数
      binIdx = (binIdx + 1) >> 1;    //(−1)^(k+1) * Ceil(k÷2)
    else                             // 偶数
      binIdx = -((binIdx + 1) >> 1); //(−1)^(k+1) * Ceil(k÷2)

    synElVal = binIdx;
  }

  return 0;
}

// 9.3.3.1.1.5 Derivation process of ctxIdxInc for the syntax element
int Cabac::derivation_ctxIdxInc_for_mb_qp_delta(int32_t &ctxIdxInc) {
  const SliceHeader *header = picture.m_slice->slice_header;

  /* 令 prevMbAddr 为按解码顺序位于当前宏块之前的宏块的宏块地址。当当前宏块是切片的第一个宏块时，prevMbAddr被标记为不可用。 */
  int32_t prevMbAddr = picture.CurrMbAddr - 1;
  const MacroBlock &pre_mb = picture.m_mbs[prevMbAddr];

  /* Slice的第一个宏块地址推导如下：
– 如果MbaffFrameFlag等于0，则first_mb_in_slice是切片中第一个宏块的宏块地址，并且first_mb_in_slice应在0到PicSizeInMbs - 1的范围内（包括0和PicSizeInMbs - 1）。
– 否则（MbaffFrameFlag等于1），first_mb_in_slice * 2是第一个宏块的宏块地址*/
  int32_t FirstMbAddrOfSlice =
      header->first_mb_in_slice * (1 + header->MbaffFrameFlag);

  if (picture.CurrMbAddr == FirstMbAddrOfSlice) prevMbAddr = -1;

  /* 变量 ctxIdxInc 的推导如下： 
   * – 如果以下任一条件成立，则 ctxIdxInc 设置为等于 0： 
     * – prevMbAddr 不可用或宏块 prevMbAddr 的 mb_type 等于 P_Skip 或 B_Skip， 
     * – 宏块 prevMbAddr 的 mb_type 为等于 I_PCM， 
     * – 宏块 prevMbAddr 未在 Intra_16x16 宏块预测模式下编码，且宏块 prevMbAddr 的 CodedBlockPatternLuma 和 CodedBlockPatternChroma 均等于 0， 
     * – 宏块 prevMbAddr 的 mb_qp_delta 等于 0。 
   * – 否则，将 ctxIdxInc 设置为等于 1 。 */
  ctxIdxInc = 1;
  if (prevMbAddr < 0 || pre_mb.m_name_of_mb_type == P_Skip ||
      pre_mb.m_name_of_mb_type == B_Skip || pre_mb.m_name_of_mb_type == I_PCM ||
      (pre_mb.m_mb_pred_mode != Intra_16x16 &&
       pre_mb.CodedBlockPatternLuma == 0 &&
       pre_mb.CodedBlockPatternChroma == 0) ||
      pre_mb.mb_qp_delta == 0)
    ctxIdxInc = 0;

  return 0;
}

int Cabac::decode_ref_idx_lX(int32_t ref_idx_flag, int32_t mbPartIdx,
                             int32_t &synElVal) {
  int32_t ctxIdxOffset = 0, ctxIdxInc = 0, ctxIdx = 0;
  int32_t binIdx = -1, binVal = 0;
  int32_t bypassFlag = 0;

  // Table 9-34 – Syntax elements and associated types of binarization, maxBinIdxCtx, and ctxIdxOffset
  //------Table 9-34: ctxIdxOffset: 54--------
  ctxIdxOffset = 54;

  // 0,1,2,3 (clause 9.3.3.1.1.6)
  int32_t is_ref_idx_10 = (ref_idx_flag == 0) ? 1 : 0;
  RET(derivation_ctxIdxInc_for_ref_idx_lX(is_ref_idx_10, mbPartIdx, ctxIdxInc));

  //---------------注意是：U binarization------------------------
  ctxIdx = ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

  // unary (U) binarization即一元二值化，就是类似 (111...1110)b这样的二进制字符串，最后一个二进制值为0，其他都是1，其中1的个数就是对应的语法元素的值

  // binIdx=[0,1,2,3,4,5,6,...,k] --> ctxIdx = ctxIdxOffset + ctxIdxInc = 54 + [?,4,5,5,5,5,5,...,5]

  if (binVal == 0) //(0)b
    synElVal = 0;
  else {
    ctxIdx = ctxIdxOffset + 4;                  // Table 9-39
    RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1;
    binIdx = 1;
    ctxIdx = ctxIdxOffset + 5; // Table 9-39
    while (binVal == 1)        //(11...1)b
    {
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2,3,4.,..,k;
      binIdx++;
      RET(binIdx > 32); // error: ref_idx 值太大了
    }
    synElVal = binIdx;
  }

  return 0;
}

// 9.3.3.1.1.6 Derivation process of ctxIdxInc for the syntax elements ref_idx_l0 and ref_idx_l1
int Cabac::derivation_ctxIdxInc_for_ref_idx_lX(int32_t is_ref_idx_10,
                                               int32_t mbPartIdx,
                                               int32_t &ctxIdxInc) {

  const MacroBlock &mb = picture.m_mbs[picture.CurrMbAddr];
  const int32_t isChroma = 0;

  int32_t mbAddrN_A = 0, mbPartIdxN_A = 0, subMbPartIdxN_A = 0;
  int32_t mbAddrN_B = 0, mbPartIdxN_B = 0, subMbPartIdxN_B = 0;
  int32_t subMbPartIdx = 0;
  H264_MB_TYPE currSubMbType = mb.m_name_of_sub_mb_type[mbPartIdx];

  //--------------------------------
  // 6.4.2.1 Inverse macroblock partition scanning process
  int32_t MbPartWidth = mb.MbPartWidth;
  int32_t MbPartHeight = mb.MbPartHeight;
  int32_t SubMbPartWidth = mb.SubMbPartWidth[mbPartIdx];
  int32_t SubMbPartHeight = mb.SubMbPartHeight[mbPartIdx];

  int32_t x = InverseRasterScan(mbPartIdx, MbPartWidth, MbPartHeight, 16, 0);
  int32_t y = InverseRasterScan(mbPartIdx, MbPartWidth, MbPartHeight, 16, 1);

  int32_t xS = 0, yS = 0;
  if (mb.m_name_of_mb_type == P_8x8 || mb.m_name_of_mb_type == P_8x8ref0 ||
      mb.m_name_of_mb_type == B_8x8) {
    xS = InverseRasterScan(subMbPartIdx, SubMbPartWidth, SubMbPartHeight, 8, 0);
    yS = InverseRasterScan(subMbPartIdx, SubMbPartWidth, SubMbPartHeight, 8, 1);
  }

  RET(picture.derivation_neighbouring_partitions(
      x + xS - 1, y + yS + 0, mbPartIdx, currSubMbType, subMbPartIdx, isChroma,
      mbAddrN_A, mbPartIdxN_A, subMbPartIdxN_A));

  RET(picture.derivation_neighbouring_partitions(
      x + xS + 0, y + yS - 1, mbPartIdx, currSubMbType, subMbPartIdx, isChroma,
      mbAddrN_B, mbPartIdxN_B, subMbPartIdxN_B));

  int32_t refIdxZeroFlagA = 0, refIdxZeroFlagB = 0;
  int32_t predModeEqualFlagA = 0, predModeEqualFlagB = 0;
  int32_t condTermFlagA = 0, condTermFlagB = 0;

  //------------A--------------------
  if (mbAddrN_A >= 0) {
    if (mb.MbaffFrameFlag && mb.mb_field_decoding_flag == 0 &&
        picture.m_mbs[mbAddrN_A].mb_field_decoding_flag) {
      if (is_ref_idx_10 == 1)
        refIdxZeroFlagA =
            (picture.m_mbs[mbAddrN_A].ref_idx_l0[mbPartIdxN_A] <= 1);
      else
        refIdxZeroFlagA =
            (picture.m_mbs[mbAddrN_A].ref_idx_l1[mbPartIdxN_A] <= 1);
    } else {
      if (is_ref_idx_10 == 1)
        refIdxZeroFlagA =
            (picture.m_mbs[mbAddrN_A].ref_idx_l0[mbPartIdxN_A] <= 0);
      else
        refIdxZeroFlagA =
            (picture.m_mbs[mbAddrN_A].ref_idx_l1[mbPartIdxN_A] <= 0);
    }

    if (picture.m_mbs[mbAddrN_A].m_name_of_mb_type == B_Direct_16x16 ||
        picture.m_mbs[mbAddrN_A].m_name_of_mb_type == B_Skip)
      predModeEqualFlagA = 0;
    else if (picture.m_mbs[mbAddrN_A].m_name_of_mb_type == P_8x8 ||
             picture.m_mbs[mbAddrN_A].m_name_of_mb_type == B_8x8) {
      int32_t NumSubMbPart = 0, SubMbPartWidth = 0, SubMbPartHeight = 0;
      H264_MB_PART_PRED_MODE SubMbPredMode = MB_PRED_MODE_NA;

      RET(MacroBlock::SubMbPredMode(
          picture.m_slice->slice_header->slice_type,
          picture.m_mbs[mbAddrN_A].sub_mb_type[mbPartIdxN_A], NumSubMbPart,
          SubMbPredMode, SubMbPartWidth, SubMbPartHeight));

      if (((is_ref_idx_10 == 1 && SubMbPredMode != Pred_L0) ||
           (is_ref_idx_10 == 0 && SubMbPredMode != Pred_L1)) &&
          SubMbPredMode != BiPred)
        predModeEqualFlagA = 0;
      else
        predModeEqualFlagA = 1;
    } else {
      H264_MB_PART_PRED_MODE mb_pred_mode = MB_PRED_MODE_NA;

      RET(MacroBlock::MbPartPredMode(
          picture.m_mbs[mbAddrN_A].m_name_of_mb_type, mbPartIdxN_A,
          picture.m_mbs[mbAddrN_A].transform_size_8x8_flag, mb_pred_mode));

      if (((is_ref_idx_10 == 1 && mb_pred_mode != Pred_L0) ||
           (is_ref_idx_10 == 0 && mb_pred_mode != Pred_L1)) &&
          mb_pred_mode != BiPred)
        predModeEqualFlagA = 0;
      else
        predModeEqualFlagA = 1;
    }
  } else
    predModeEqualFlagA = 0;

  if (mbAddrN_A < 0 ||
      (picture.m_mbs[mbAddrN_A].m_name_of_mb_type == P_Skip ||
       picture.m_mbs[mbAddrN_A].m_name_of_mb_type == B_Skip) ||
      IS_INTRA_Prediction_Mode(picture.m_mbs[mbAddrN_A].m_mb_pred_mode) ||
      predModeEqualFlagA == 0 || refIdxZeroFlagA == 1)
    condTermFlagA = 0;
  else
    condTermFlagA = 1;

  //------------B--------------------
  if (mbAddrN_B >= 0) {
    if (mb.MbaffFrameFlag == 1 && mb.mb_field_decoding_flag == 0 &&
        picture.m_mbs[mbAddrN_B].mb_field_decoding_flag == 1) {
      if (is_ref_idx_10 == 1)
        refIdxZeroFlagB =
            (picture.m_mbs[mbAddrN_B].ref_idx_l0[mbPartIdxN_B] <= 1);
      else
        refIdxZeroFlagB =
            (picture.m_mbs[mbAddrN_B].ref_idx_l1[mbPartIdxN_B] <= 1);
    } else {
      if (is_ref_idx_10 == 1)
        refIdxZeroFlagB =
            (picture.m_mbs[mbAddrN_B].ref_idx_l0[mbPartIdxN_B] <= 0);
      else
        refIdxZeroFlagB =
            (picture.m_mbs[mbAddrN_B].ref_idx_l1[mbPartIdxN_B] <= 0);
    }

    if (picture.m_mbs[mbAddrN_B].m_name_of_mb_type == B_Direct_16x16 ||
        picture.m_mbs[mbAddrN_B].m_name_of_mb_type == B_Skip) {
      predModeEqualFlagB = 0;
    } else if (picture.m_mbs[mbAddrN_B].m_name_of_mb_type == P_8x8 ||
               picture.m_mbs[mbAddrN_B].m_name_of_mb_type == B_8x8) {
      int32_t NumSubMbPart = 0, SubMbPartWidth = 0, SubMbPartHeight = 0;
      H264_MB_PART_PRED_MODE SubMbPredMode = MB_PRED_MODE_NA;

      RET(MacroBlock::SubMbPredMode(
          picture.m_slice->slice_header->slice_type,
          picture.m_mbs[mbAddrN_B].sub_mb_type[mbPartIdxN_B], NumSubMbPart,
          SubMbPredMode, SubMbPartWidth, SubMbPartHeight));

      if (((is_ref_idx_10 == 1 && SubMbPredMode != Pred_L0) ||
           (is_ref_idx_10 == 0 && SubMbPredMode != Pred_L1)) &&
          SubMbPredMode != BiPred)
        predModeEqualFlagB = 0;
      else
        predModeEqualFlagB = 1;
    } else {
      H264_MB_PART_PRED_MODE mb_pred_mode = MB_PRED_MODE_NA;

      RET(MacroBlock::MbPartPredMode(
          picture.m_mbs[mbAddrN_B].m_name_of_mb_type, mbPartIdxN_B,
          picture.m_mbs[mbAddrN_B].transform_size_8x8_flag, mb_pred_mode));

      if (((is_ref_idx_10 == 1 && mb_pred_mode != Pred_L0) ||
           (is_ref_idx_10 == 0 && mb_pred_mode != Pred_L1)) &&
          mb_pred_mode != BiPred)
        predModeEqualFlagB = 0;
      else
        predModeEqualFlagB = 1;
    }
  } else
    predModeEqualFlagB = 0;

  if (mbAddrN_B < 0 ||
      (picture.m_mbs[mbAddrN_B].m_name_of_mb_type == P_Skip ||
       picture.m_mbs[mbAddrN_B].m_name_of_mb_type == B_Skip) ||
      IS_INTRA_Prediction_Mode(picture.m_mbs[mbAddrN_B].m_mb_pred_mode) ||
      predModeEqualFlagB == 0 || refIdxZeroFlagB == 1)
    condTermFlagB = 0;
  else
    condTermFlagB = 1;

  ctxIdxInc = condTermFlagA + 2 * condTermFlagB;

  return 0;
}

// 9.3.3.1.1.7 Derivation process of ctxIdxInc for the syntax elements mvd_l0 and mvd_l1
int Cabac::decode_mvd_lX(int32_t mvd_flag, int32_t mbPartIdx,
                         int32_t subMbPartIdx, int32_t isChroma,
                         int32_t &synElVal) {
  int32_t ctxIdxOffset = 47, ctxIdxInc = 0, ctxIdx = 0;
  int32_t binVal = 0;
  int32_t bypassFlag = 0;
  const int32_t uCoff = 9;

  // Table 9-34 – Syntax elements and associated types of binarization, maxBinIdxCtx, and ctxIdxOffset
  //------Table 9-34: ctxIdxOffset-prefix: --------
  if (mvd_flag == 0 || mvd_flag == 2) ctxIdxOffset = 40;

  // 9.3.3.1.1.7 Derivation process of ctxIdxInc for the syntax elements mvd_l0 and mvd_l1
  int32_t is_mvd_10 = (mvd_flag == 0 || mvd_flag == 1) ? 1 : 0;
  RET(derivation_ctxIdxInc_for_mvd_lX(is_mvd_10, mbPartIdx, subMbPartIdx,
                                      isChroma, ctxIdxOffset, ctxIdxInc));

  //---------------注意是：UEG3------------------------
  // UEG3编码是由 prefix(TU binarization) + suffix(Exp-Golomb) + signedValFlag,三部分组成

  //-----1. 先解码前缀(TU)--------
  ctxIdx = ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

  if (binVal == 0) // signedValFlag=1
    synElVal = 0;  // synElVal consists only of a prefix bit string
  else {
    synElVal = 1;
    // ctxIdxOffset + ctxIdxInc = 40(47) + 3; //Table 9-39
    ctxIdx = ctxIdxOffset + 3;

    while (binVal == 1 && synElVal < uCoff) //(11...1)b
    {
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1,2,3,4.,..,k;
      if (binVal == 0) break;
      synElVal++; // binIdx=[0,1,2,3,4,5,6,7,8] --> ctxIdxInc= [?,3,4,5,6,6,6,6,6]
          // ctxIdx = ctxIdxOffset + ctxIdxInc = 40(47) + [?,3,4,5,6,6,6,6,6]; //Table 9-39
      if (synElVal <= 4) ctxIdx++;
    }

    //-----2. 再解码后缀(Exp-Golomb)--------
    // 注意：9.3.2.3 Concatenated unary/ k-th order Exp-Golomb (UEGk) binarization process 中的伪代码表示是UEGk的编码过程，解码需要逆向过来
    // 重点是：此处的 Exp-Golomb 和9.1 Parsing process for Exp-Golomb codes中的意思不一样（NOTE：这是一个坑）

    //------Table 9-34: ctxIdxOffset-suffix: na (uses DecodeBypass)--------
    int32_t k = 3; // k取值为UEG3中的3

    if (synElVal >= uCoff) {
      RET(decodeBypass(binVal));

      while (binVal == 1) {
        synElVal += 1 << k;
        ++k;
        RET(k >= 32 - uCoff); // error: mv值过大
        RET(decodeBypass(binVal));
      }

      while (k--) {
        RET(decodeBypass(binVal));
        synElVal += binVal << k;
      }
    }

    // //signedValFlag=1代表结果是有符号整数
    if (synElVal != 0) {
      RET(decodeBypass(binVal));
      if (binVal == 1) synElVal = -synElVal; // 结果为负数
    }
  }

  return 0;
}

// 9.3.3.1.1.7 Derivation process of ctxIdxInc for the syntax elements mvd_l0 and mvd_l1
int Cabac::derivation_ctxIdxInc_for_mvd_lX(int32_t is_mvd_10, int32_t mbPartIdx,
                                           int32_t subMbPartIdx,
                                           int32_t isChroma,
                                           int32_t ctxIdxOffset,
                                           int32_t &ctxIdxInc) {

  const MacroBlock &mb = picture.m_mbs[picture.CurrMbAddr];
  const int32_t MbPartWidth = mb.MbPartWidth;
  const int32_t MbPartHeight = mb.MbPartHeight;
  const int32_t SubMbPartWidth = mb.SubMbPartWidth[mbPartIdx];
  const int32_t SubMbPartHeight = mb.SubMbPartHeight[mbPartIdx];

  int32_t x = InverseRasterScan(mbPartIdx, MbPartWidth, MbPartHeight, 16, 0);
  int32_t y = InverseRasterScan(mbPartIdx, MbPartWidth, MbPartHeight, 16, 1);

  int32_t xS = 0, yS = 0;
  if (mb.m_name_of_mb_type == P_8x8 || mb.m_name_of_mb_type == P_8x8ref0 ||
      mb.m_name_of_mb_type == B_8x8) {
    xS = InverseRasterScan(subMbPartIdx, SubMbPartWidth, SubMbPartHeight, 8, 0);
    yS = InverseRasterScan(subMbPartIdx, SubMbPartWidth, SubMbPartHeight, 8, 1);
  }

  H264_MB_TYPE currSubMbType = mb.m_name_of_sub_mb_type[mbPartIdx];

  int32_t mbAddrN_A = 0, mbPartIdxN_A = 0, subMbPartIdxN_A = 0;
  int32_t mbAddrN_B = 0, mbPartIdxN_B = 0, subMbPartIdxN_B = 0;
  RET(picture.derivation_neighbouring_partitions(
      x + xS - 1, y + yS + 0, mbPartIdx, currSubMbType, subMbPartIdx, isChroma,
      mbAddrN_A, mbPartIdxN_A, subMbPartIdxN_A));
  RET(picture.derivation_neighbouring_partitions(
      x + xS + 0, y + yS - 1, mbPartIdx, currSubMbType, subMbPartIdx, isChroma,
      mbAddrN_B, mbPartIdxN_B, subMbPartIdxN_B));

  int32_t compIdx = (ctxIdxOffset == 40) ? 0 : 1;
  int32_t predModeEqualFlagA = 0, predModeEqualFlagB = 0;
  int32_t absMvdCompA = 0, absMvdCompB = 0;

  //------------------A-------------------------
  if (mbAddrN_A >= 0 &&
      (picture.m_mbs[mbAddrN_A].m_name_of_mb_type == B_Direct_16x16 ||
       picture.m_mbs[mbAddrN_A].m_name_of_mb_type == B_Skip))
    predModeEqualFlagA = 0;
  else if (mbAddrN_A >= 0 &&
           (picture.m_mbs[mbAddrN_A].m_name_of_mb_type == P_8x8 ||
            picture.m_mbs[mbAddrN_A].m_name_of_mb_type == B_8x8)) {
    int32_t NumSubMbPart = 0, SubMbPartWidth = 0, SubMbPartHeight = 0;
    H264_MB_PART_PRED_MODE SubMbPredMode = MB_PRED_MODE_NA;

    RET(MacroBlock::SubMbPredMode(
        picture.m_slice->slice_header->slice_type,
        picture.m_mbs[mbAddrN_A].sub_mb_type[mbPartIdxN_A], NumSubMbPart,
        SubMbPredMode, SubMbPartWidth, SubMbPartHeight));

    if (((is_mvd_10 == 1 && SubMbPredMode != Pred_L0) ||
         (is_mvd_10 == 0 && SubMbPredMode != Pred_L1)) &&
        SubMbPredMode != BiPred)
      predModeEqualFlagA = 0;
    else
      predModeEqualFlagA = 1;
  } else if (mbAddrN_A >= 0) {
    H264_MB_PART_PRED_MODE mb_pred_mode = MB_PRED_MODE_NA;

    RET(MacroBlock::MbPartPredMode(
        picture.m_mbs[mbAddrN_A].m_name_of_mb_type, mbPartIdxN_A,
        picture.m_mbs[mbAddrN_A].transform_size_8x8_flag, mb_pred_mode));

    if (((is_mvd_10 == 1 && mb_pred_mode != Pred_L0) ||
         (is_mvd_10 == 0 && mb_pred_mode != Pred_L1)) &&
        mb_pred_mode != BiPred)
      predModeEqualFlagA = 0;
    else
      predModeEqualFlagA = 1;
  } else
    predModeEqualFlagA = 0;

  if (mbAddrN_A < 0 ||
      (picture.m_mbs[mbAddrN_A].m_name_of_mb_type == P_Skip ||
       picture.m_mbs[mbAddrN_A].m_name_of_mb_type == B_Skip) ||
      IS_INTRA_Prediction_Mode(picture.m_mbs[mbAddrN_A].m_mb_pred_mode) ||
      predModeEqualFlagA == 0)
    absMvdCompA = 0;
  else {
    if (compIdx == 1 && mb.MbaffFrameFlag == 1 &&
        mb.mb_field_decoding_flag == 0 &&
        picture.m_mbs[mbAddrN_A].mb_field_decoding_flag == 1) {
      if (is_mvd_10 == 1)
        absMvdCompA = ABS(picture.m_mbs[mbAddrN_A]
                              .mvd_l0[mbPartIdxN_A][subMbPartIdxN_A][compIdx]) *
                      2;
      else
        absMvdCompA = ABS(picture.m_mbs[mbAddrN_A]
                              .mvd_l1[mbPartIdxN_A][subMbPartIdxN_A][compIdx]) *
                      2;
    } else if (compIdx == 1 && mb.MbaffFrameFlag == 1 &&
               mb.mb_field_decoding_flag == 1 &&
               picture.m_mbs[mbAddrN_A].mb_field_decoding_flag == 0) {
      if (is_mvd_10 == 1)
        absMvdCompA = ABS(picture.m_mbs[mbAddrN_A]
                              .mvd_l0[mbPartIdxN_A][subMbPartIdxN_A][compIdx]) /
                      2;
      else
        absMvdCompA = ABS(picture.m_mbs[mbAddrN_A]
                              .mvd_l1[mbPartIdxN_A][subMbPartIdxN_A][compIdx]) /
                      2;
    } else {
      if (is_mvd_10 == 1)
        absMvdCompA = ABS(picture.m_mbs[mbAddrN_A]
                              .mvd_l0[mbPartIdxN_A][subMbPartIdxN_A][compIdx]);
      else
        absMvdCompA = ABS(picture.m_mbs[mbAddrN_A]
                              .mvd_l1[mbPartIdxN_A][subMbPartIdxN_A][compIdx]);
    }
  }

  //------------------B-------------------------
  if (mbAddrN_B >= 0 &&
      (picture.m_mbs[mbAddrN_B].m_name_of_mb_type == B_Direct_16x16 ||
       picture.m_mbs[mbAddrN_B].m_name_of_mb_type == B_Skip)) {
    predModeEqualFlagB = 0;
  } else if (mbAddrN_B >= 0 &&
             (picture.m_mbs[mbAddrN_B].m_name_of_mb_type == P_8x8 ||
              picture.m_mbs[mbAddrN_B].m_name_of_mb_type == B_8x8)) {
    int32_t NumSubMbPart = 0, SubMbPartWidth = 0, SubMbPartHeight = 0;
    H264_MB_PART_PRED_MODE SubMbPredMode = MB_PRED_MODE_NA;

    RET(MacroBlock::SubMbPredMode(
        picture.m_slice->slice_header->slice_type,
        picture.m_mbs[mbAddrN_B].sub_mb_type[mbPartIdxN_B], NumSubMbPart,
        SubMbPredMode, SubMbPartWidth, SubMbPartHeight));

    if (((is_mvd_10 == 1 && SubMbPredMode != Pred_L0) ||
         (is_mvd_10 == 0 && SubMbPredMode != Pred_L1)) &&
        SubMbPredMode != BiPred)
      predModeEqualFlagB = 0;
    else
      predModeEqualFlagB = 1;
  } else if (mbAddrN_B >= 0) {
    H264_MB_PART_PRED_MODE mb_pred_mode = MB_PRED_MODE_NA;

    RET(MacroBlock::MbPartPredMode(
        picture.m_mbs[mbAddrN_B].m_name_of_mb_type, mbPartIdxN_B,
        picture.m_mbs[mbAddrN_B].transform_size_8x8_flag, mb_pred_mode));

    if (((is_mvd_10 == 1 && mb_pred_mode != Pred_L0) ||
         (is_mvd_10 == 0 && mb_pred_mode != Pred_L1)) &&
        mb_pred_mode != BiPred)
      predModeEqualFlagB = 0;
    else
      predModeEqualFlagB = 1;
  } else
    predModeEqualFlagB = 0;

  if (mbAddrN_B < 0 ||
      (picture.m_mbs[mbAddrN_B].m_name_of_mb_type == P_Skip ||
       picture.m_mbs[mbAddrN_B].m_name_of_mb_type == B_Skip) ||
      IS_INTRA_Prediction_Mode(picture.m_mbs[mbAddrN_B].m_mb_pred_mode) ||
      predModeEqualFlagB == 0)
    absMvdCompB = 0;
  else {
    if (compIdx == 1 && mb.MbaffFrameFlag && mb.mb_field_decoding_flag == 0 &&
        picture.m_mbs[mbAddrN_B].mb_field_decoding_flag) {
      if (is_mvd_10 == 1)
        absMvdCompB = ABS(picture.m_mbs[mbAddrN_B]
                              .mvd_l0[mbPartIdxN_B][subMbPartIdxN_B][compIdx]) *
                      2;
      else
        absMvdCompB = ABS(picture.m_mbs[mbAddrN_B]
                              .mvd_l1[mbPartIdxN_B][subMbPartIdxN_B][compIdx]) *
                      2;
    } else if (compIdx == 1 && mb.MbaffFrameFlag && mb.mb_field_decoding_flag &&
               picture.m_mbs[mbAddrN_B].mb_field_decoding_flag == 0) {
      if (is_mvd_10 == 1)
        absMvdCompB = ABS(picture.m_mbs[mbAddrN_B]
                              .mvd_l0[mbPartIdxN_B][subMbPartIdxN_B][compIdx]) /
                      2;
      else
        absMvdCompB = ABS(picture.m_mbs[mbAddrN_B]
                              .mvd_l1[mbPartIdxN_B][subMbPartIdxN_B][compIdx]) /
                      2;
    } else {
      if (is_mvd_10 == 1)
        absMvdCompB = ABS(picture.m_mbs[mbAddrN_B]
                              .mvd_l0[mbPartIdxN_B][subMbPartIdxN_B][compIdx]);
      else
        absMvdCompB = ABS(picture.m_mbs[mbAddrN_B]
                              .mvd_l1[mbPartIdxN_B][subMbPartIdxN_B][compIdx]);
    }
  }

  if ((absMvdCompA + absMvdCompB) < 3)
    ctxIdxInc = 0;
  else if ((absMvdCompA + absMvdCompB) > 32)
    ctxIdxInc = 2;
  else
    ctxIdxInc = 1;

  return 0;
}

// 9.3.3.1.1.8 Derivation process of ctxIdxInc for the syntax element intra_chroma_pred_mode
int Cabac::decode_intra_chroma_pred_mode(int32_t &synElVal) {
  int32_t ctxIdxOffset = 0, ctxIdxInc = 0, ctxIdx = 0;
  int32_t binVal = 0;
  int32_t bypassFlag = 0;

  //------Table 9-34: ctxIdxOffset: 64--------
  ctxIdxOffset = 64;

  // Table 9-39 – Assignment of ctxIdxInc to binIdx for all ctxIdxOffset values except those related to the syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1

  // 0,1,2 (clause 9.3.3.1.1.8)
  // 9.3.3.1.1.8 Derivation process of ctxIdxInc for the syntax element intra_chroma_pred_mode
  RET(derivation_ctxIdxInc_for_intra_chroma_pred_mode(ctxIdxInc));

  //---------------注意是：TU, cMax=3------------------------
  ctxIdx = ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

  if (binVal == 0) //(0)b
    synElVal = 0;
  else {
    ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
    RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1;

    if (binVal == 0) //(10)b
      synElVal = 1;
    else {
      ctxIdx = ctxIdxOffset + 3;                  // Table 9-39
      RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2;

      if (binVal == 0) //(110)b
        synElVal = 2;
      else
        // 9.3.2.2 Truncated unary (TU) binarization process the syntax element value equal to cMax the bin string is a bit string of length cMax with all bins being equal to 1.
        synElVal = 3; // TU, cMax=3
    }
  }

  return 0;
}

// 9.3.3.1.1.8 Derivation process of ctxIdxInc for the syntax element intra_chroma_pred_mode
int Cabac::derivation_ctxIdxInc_for_intra_chroma_pred_mode(int32_t &ctxIdxInc) {
  const bool MbaffFrameFlag = picture.m_slice->slice_header->MbaffFrameFlag;
  const int32_t isChroma = 0;

  int32_t mbAddrA = 0, mbAddrB = 0;
  RET(picture.derivation_for_neighbouring_macroblocks(
      MbaffFrameFlag, picture.CurrMbAddr, mbAddrA, mbAddrB, isChroma));

  int32_t condTermFlagA = 1, condTermFlagB = 1;
  //----------A------------
  if (mbAddrA < 0 ||
      IS_INTRA_Prediction_Mode(picture.m_mbs[mbAddrA].m_mb_pred_mode) ==
          false ||
      picture.m_mbs[mbAddrA].m_name_of_mb_type == I_PCM ||
      picture.m_mbs[mbAddrA].intra_chroma_pred_mode == 0)
    condTermFlagA = 0;

  //----------B------------
  if (mbAddrB < 0 ||
      IS_INTRA_Prediction_Mode(picture.m_mbs[mbAddrB].m_mb_pred_mode) ==
          false ||
      picture.m_mbs[mbAddrB].m_name_of_mb_type == I_PCM ||
      picture.m_mbs[mbAddrB].intra_chroma_pred_mode == 0)
    condTermFlagB = 0;

  ctxIdxInc = condTermFlagA + condTermFlagB;

  return 0;
}

// 9.3.3.1.1.9 Derivation process of ctxIdxInc for the syntax element coded_block_flag
int Cabac::decode_coded_block_flag(MB_RESIDUAL_LEVEL mb_block_level,
                                   int32_t BlkIdx, int32_t iCbCr,
                                   int32_t &synElVal) {
  int32_t ctxIdxOffset = 0, ctxIdxInc = 0, ctxIdx = 0;
  int32_t binVal = 0;
  int32_t bypassFlag = 0;
  int32_t ctxBlockCat = 0;

  //--------1.先计算出ctxBlockCat的值------------
  // Table 9-42 – Specification of ctxBlockCat for the different blocks
  ctxBlockCat = mb_block_level;

  //--------2.获取ctxIdxOffset的值-----------
  // Table 9-34 – Syntax elements and associated types of binarization, maxBinIdxCtx, and ctxIdxOffset
  if (ctxBlockCat < 5) //(blocks with ctxBlockCat < 5) FL, cMax=1
    ctxIdxOffset = 85;
  else if (ctxBlockCat > 5 && ctxBlockCat < 9)
    ctxIdxOffset = 460;
  else if (ctxBlockCat > 9 && ctxBlockCat < 13)
    ctxIdxOffset = 472;
  else
    ctxIdxOffset = 1012;

  // Table 9-40 – Assignment of ctxIdxBlockCatOffset to ctxBlockCat for syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1

  const int32_t ctxIdxBlockCatOffset_arr[14] = {0, 4, 8, 12, 16, 0, 0,
                                                4, 8, 4, 0,  4,  8, 8};
  int32_t ctxIdxBlockCatOffset = ctxIdxBlockCatOffset_arr[ctxBlockCat];

  // 9.3.3.1.1.9
  RET(derivation_ctxIdxInc_for_coded_block_flag(ctxBlockCat, BlkIdx, iCbCr,
                                                ctxIdxInc));

  //--------3.计算出ctxIdx的值-----------
  ctxIdx = ctxIdxOffset + ctxIdxBlockCatOffset + ctxIdxInc;

  //--------4.进行处理 FL, cMax=1-----------
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

  synElVal = binVal;

  return 0;
}

// 9.3.3.1.1.9 Derivation process of ctxIdxInc for the syntax element coded_block_flag ctxIdxInc( ctxBlockCat )
int Cabac::derivation_ctxIdxInc_for_coded_block_flag(int32_t ctxBlockCat,
                                                     int32_t BlkIdx,
                                                     int32_t iCbCr,
                                                     int32_t &ctxIdxInc) {
  const bool MbaffFrameFlag = picture.m_slice->slice_header->MbaffFrameFlag;

  int32_t mbAddrA = 0, mbAddrB = 0;
  int32_t transBlockA = -1, transBlockB = -1;
  int32_t transBlockA_coded_block_flag = 0, transBlockB_coded_block_flag = 0;

  if (ctxBlockCat == 0 || ctxBlockCat == 6 || ctxBlockCat == 10) {
    int32_t isChroma = (iCbCr < 0) ? 0 : 1;
    RET(picture.derivation_for_neighbouring_macroblocks(
        MbaffFrameFlag, picture.CurrMbAddr, mbAddrA, mbAddrB, isChroma));

    //----------A------------
    if (mbAddrA >= 0 && picture.m_mbs[mbAddrA].m_mb_pred_mode == Intra_16x16) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_DC_pattern >> (iCbCr + 1)) &
          1;
    } else
      transBlockA = -1;
    //----------B------------
    if (mbAddrB >= 0 && picture.m_mbs[mbAddrB].m_mb_pred_mode == Intra_16x16) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_DC_pattern >> (iCbCr + 1)) &
          1;
    } else
      transBlockB = -1;
  } else if (ctxBlockCat == 1 || ctxBlockCat == 2) {
    // 6.4.11.4 Derivation process for neighbouring 4x4 luma blocks
    int32_t luma4x4BlkIdx = BlkIdx, luma4x4BlkIdxA = 0, luma4x4BlkIdxB = 0;
    int32_t isChroma = (iCbCr < 0) ? 0 : 1;

    RET(picture.derivation_for_neighbouring_4x4_luma_blocks(
        luma4x4BlkIdx, mbAddrA, mbAddrB, luma4x4BlkIdxA, luma4x4BlkIdxB,
        isChroma));

    //----------A------------
    if (mbAddrA >= 0 && picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrA].CodedBlockPatternLuma >>
          (luma4x4BlkIdxA >> 2)) &
         1) != 0 &&
        picture.m_mbs[mbAddrA].transform_size_8x8_flag == 0) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (luma4x4BlkIdxA)) &
          1;
    } else if (mbAddrA >= 0 &&
               picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
               picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip &&
               ((picture.m_mbs[mbAddrA].CodedBlockPatternLuma >>
                 (luma4x4BlkIdxA >> 2)) &
                1) != 0 &&
               picture.m_mbs[mbAddrA].transform_size_8x8_flag == 1) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (luma4x4BlkIdxA >> 2)) &
          1;
    } else
      transBlockA = -1;

    //----------B------------
    if (mbAddrB >= 0 && picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrB].CodedBlockPatternLuma >>
          (luma4x4BlkIdxB >> 2)) &
         1) != 0 &&
        picture.m_mbs[mbAddrB].transform_size_8x8_flag == 0) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (luma4x4BlkIdxB)) &
          1;
    } else if (mbAddrB >= 0 &&
               picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
               picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip &&
               ((picture.m_mbs[mbAddrB].CodedBlockPatternLuma >>
                 (luma4x4BlkIdxB >> 2)) &
                1) != 0 &&
               picture.m_mbs[mbAddrB].transform_size_8x8_flag == 1) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (luma4x4BlkIdxB >> 2)) &
          1;
    } else
      transBlockB = -1;
  } else if (ctxBlockCat == 3) {
    int32_t isChroma = 1;
    RET(picture.derivation_for_neighbouring_macroblocks(
        MbaffFrameFlag, picture.CurrMbAddr, mbAddrA, mbAddrB, isChroma));

    //----------A------------
    if (mbAddrA >= 0 && picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != I_PCM &&
        picture.m_mbs[mbAddrA].CodedBlockPatternChroma != 0) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_DC_pattern >> (iCbCr + 1)) &
          1;
    } else
      transBlockA = -1;

    //----------B------------
    if (mbAddrB >= 0 && picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != I_PCM &&
        picture.m_mbs[mbAddrB].CodedBlockPatternChroma != 0) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_DC_pattern >> (iCbCr + 1)) &
          1;
    } else
      transBlockB = -1;
  } else if (ctxBlockCat == 4) {
    int32_t chroma4x4BlkIdx = BlkIdx, chroma4x4BlkIdxA = 0,
            chroma4x4BlkIdxB = 0;

    RET(picture.derivation_for_neighbouring_4x4_chroma_blocks(
        chroma4x4BlkIdx, mbAddrA, mbAddrB, chroma4x4BlkIdxA, chroma4x4BlkIdxB));

    //----------A------------
    if (mbAddrA >= 0 && picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != I_PCM &&
        picture.m_mbs[mbAddrA].CodedBlockPatternChroma == 2) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (chroma4x4BlkIdxA)) &
          1;
    } else
      transBlockA = -1;

    //----------B------------
    if (mbAddrB >= 0 && picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != I_PCM &&
        picture.m_mbs[mbAddrB].CodedBlockPatternChroma == 2) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (chroma4x4BlkIdxB)) &
          1;
    } else
      transBlockB = -1;
  } else if (ctxBlockCat == 5) {
    int32_t luma8x8BlkIdx = BlkIdx, luma8x8BlkIdxA = 0, luma8x8BlkIdxB = 0;
    int32_t isChroma = 0;

    RET(picture.derivation_for_neighbouring_8x8_luma_block(
        luma8x8BlkIdx, mbAddrA, mbAddrB, luma8x8BlkIdxA, luma8x8BlkIdxB,
        isChroma));

    //----------A------------
    if (mbAddrA >= 0 && picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrA].CodedBlockPatternLuma >> luma8x8BlkIdx) & 1) !=
            0 &&
        picture.m_mbs[mbAddrA].transform_size_8x8_flag == 1) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (luma8x8BlkIdxA)) &
          1;
    } else
      transBlockA = -1;

    //----------B------------
    if (mbAddrB >= 0 && picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrB].CodedBlockPatternLuma >> luma8x8BlkIdx) & 1) !=
            0 &&
        picture.m_mbs[mbAddrB].transform_size_8x8_flag == 1) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (luma8x8BlkIdxB)) &
          1;
    } else
      transBlockB = -1;
  } else if (ctxBlockCat == 7 || ctxBlockCat == 8) {
    int32_t cb4x4BlkIdx = BlkIdx, cb4x4BlkIdxA = 0, cb4x4BlkIdxB = 0;

    RET(picture.derivation_for_neighbouring_4x4_chroma_blocks(
        cb4x4BlkIdx, mbAddrA, mbAddrB, cb4x4BlkIdxA, cb4x4BlkIdxB));

    //----------A------------
    if (mbAddrA >= 0 && picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrA].CodedBlockPatternLuma >> (cb4x4BlkIdxA >> 2)) &
         1) != 0 &&
        picture.m_mbs[mbAddrA].transform_size_8x8_flag == 0) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cb4x4BlkIdxA)) &
          1;
    } else if (mbAddrA >= 0 &&
               picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
               picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip &&
               ((picture.m_mbs[mbAddrA].CodedBlockPatternLuma >>
                 (cb4x4BlkIdxA >> 2)) &
                1) != 0 &&
               picture.m_mbs[mbAddrA].transform_size_8x8_flag == 1) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cb4x4BlkIdxA >> 2)) &
          1;
    } else
      transBlockA = -1;

    //----------B------------
    if (mbAddrB >= 0 && picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrB].CodedBlockPatternLuma >> (cb4x4BlkIdxB >> 2)) &
         1) != 0 &&
        picture.m_mbs[mbAddrB].transform_size_8x8_flag == 0) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cb4x4BlkIdxB)) &
          1;
    } else if (mbAddrB >= 0 &&
               picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
               picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip &&
               ((picture.m_mbs[mbAddrB].CodedBlockPatternLuma >>
                 (cb4x4BlkIdxB >> 2)) &
                1) != 0 &&
               picture.m_mbs[mbAddrB].transform_size_8x8_flag == 1) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cb4x4BlkIdxB >> 2)) &
          1;
    } else
      transBlockB = -1;
  } else if (ctxBlockCat == 9) {
    int32_t cb8x8BlkIdx = BlkIdx, cb8x8BlkIdxA = 0, cb8x8BlkIdxB = 0;

    RET(picture.derivation_for_neighbouring_8x8_chroma_blocks_for_YUV444(
        cb8x8BlkIdx, mbAddrA, mbAddrB, cb8x8BlkIdxA, cb8x8BlkIdxB));

    //----------A------------
    if (mbAddrA >= 0 && picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrA].CodedBlockPatternLuma >> cb8x8BlkIdx) & 1) !=
            0 &&
        picture.m_mbs[mbAddrA].transform_size_8x8_flag == 1) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cb8x8BlkIdxA)) &
          1;
    } else
      transBlockA = -1;

    //----------B------------
    if (mbAddrB >= 0 && picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrB].CodedBlockPatternLuma >> cb8x8BlkIdx) & 1) !=
            0 &&
        picture.m_mbs[mbAddrB].transform_size_8x8_flag == 1) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cb8x8BlkIdxB)) &
          1;
    } else
      transBlockB = -1;
  } else if (ctxBlockCat == 11 || ctxBlockCat == 12) {
    int32_t cr4x4BlkIdx = BlkIdx, cr4x4BlkIdxA = 0, cr4x4BlkIdxB = 0;

    RET(picture.derivation_for_neighbouring_4x4_chroma_blocks(
        cr4x4BlkIdx, mbAddrA, mbAddrB, cr4x4BlkIdxA, cr4x4BlkIdxB));

    //----------A------------
    if (mbAddrA >= 0 && picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrA].CodedBlockPatternLuma >> (cr4x4BlkIdxA >> 2)) &
         1) != 0 &&
        picture.m_mbs[mbAddrA].transform_size_8x8_flag == 0) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cr4x4BlkIdxA)) &
          1;
    } else if (mbAddrA >= 0 &&
               picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
               picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip &&
               ((picture.m_mbs[mbAddrA].CodedBlockPatternLuma >>
                 (cr4x4BlkIdxA >> 2)) &
                1) != 0 &&
               picture.m_mbs[mbAddrA].transform_size_8x8_flag == 1) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cr4x4BlkIdxA >> 2)) &
          1;
    } else
      transBlockA = -1;

    //----------B------------
    if (mbAddrA >= 0 && picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrB].CodedBlockPatternLuma >> (cr4x4BlkIdxB >> 2)) &
         1) != 0 &&
        picture.m_mbs[mbAddrB].transform_size_8x8_flag == 0) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cr4x4BlkIdxB)) &
          1;
    } else if (mbAddrB >= 0 &&
               picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
               picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip &&
               ((picture.m_mbs[mbAddrB].CodedBlockPatternLuma >>
                 (cr4x4BlkIdxB >> 2)) &
                1) != 0 &&
               picture.m_mbs[mbAddrB].transform_size_8x8_flag == 1) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cr4x4BlkIdxB >> 2)) &
          1;
    } else
      transBlockB = -1;
  } else {
    int32_t cr8x8BlkIdx = BlkIdx, cr8x8BlkIdxA = 0, cr8x8BlkIdxB = 0;

    RET(picture.derivation_for_neighbouring_8x8_chroma_blocks_for_YUV444(
        cr8x8BlkIdx, mbAddrA, mbAddrB, cr8x8BlkIdxA, cr8x8BlkIdxB));

    //----------A------------
    if (mbAddrA >= 0 && picture.m_mbs[mbAddrA].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrA].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrA].CodedBlockPatternLuma >> cr8x8BlkIdx) & 1) !=
            0 &&
        picture.m_mbs[mbAddrA].transform_size_8x8_flag == 1) {
      transBlockA = 1;
      transBlockA_coded_block_flag =
          (picture.m_mbs[mbAddrA].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cr8x8BlkIdxA)) &
          1;
    } else
      transBlockA = -1;

    //----------B------------
    if (mbAddrB >= 0 && picture.m_mbs[mbAddrB].m_name_of_mb_type != P_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != B_Skip &&
        picture.m_mbs[mbAddrB].m_name_of_mb_type != I_PCM &&
        ((picture.m_mbs[mbAddrB].CodedBlockPatternLuma >> cr8x8BlkIdx) & 1) !=
            0 &&
        picture.m_mbs[mbAddrB].transform_size_8x8_flag == 1) {
      transBlockB = 1;
      transBlockB_coded_block_flag =
          (picture.m_mbs[mbAddrB].coded_block_flag_AC_pattern[iCbCr + 1] >>
           (cr8x8BlkIdxB)) &
          1;
    } else
      transBlockB = -1;
  }

  int32_t condTermFlagA = 0, condTermFlagB = 0;

  //----------A------------
  if ((mbAddrA < 0 &&
       IS_INTRA_Prediction_Mode(
           picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode) == false) ||
      (mbAddrA >= 0 && transBlockA == -1 &&
       picture.m_mbs[mbAddrA].m_name_of_mb_type != I_PCM) ||
      (IS_INTRA_Prediction_Mode(
           picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode) &&
       picture.m_mbs[picture.CurrMbAddr].constrained_intra_pred_flag == 1 &&
       mbAddrA >= 0 &&
       IS_INTRA_Prediction_Mode(picture.m_mbs[mbAddrA].m_mb_pred_mode) ==
           false &&
       (picture.m_slice->slice_header->nal_unit_type >= 2 &&
        picture.m_slice->slice_header->nal_unit_type <= 4))) {
    condTermFlagA = 0;
  } else if ((mbAddrA < 0 &&
              (IS_INTRA_Prediction_Mode(
                  picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode))) ||
             picture.m_mbs[mbAddrA].m_name_of_mb_type == I_PCM)
    condTermFlagA = 1;
  else
    condTermFlagA = transBlockA_coded_block_flag;

  //----------B------------
  if ((mbAddrB < 0 &&
       IS_INTRA_Prediction_Mode(
           picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode) == false) ||
      (mbAddrB >= 0 && transBlockB == -1 &&
       picture.m_mbs[mbAddrB].m_name_of_mb_type != I_PCM) ||
      (IS_INTRA_Prediction_Mode(
           picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode) &&
       picture.m_mbs[picture.CurrMbAddr].constrained_intra_pred_flag == 1 &&
       mbAddrB >= 0 &&
       IS_INTRA_Prediction_Mode(picture.m_mbs[mbAddrB].m_mb_pred_mode) ==
           false &&
       (picture.m_slice->slice_header->nal_unit_type >= 2 &&
        picture.m_slice->slice_header->nal_unit_type <= 4))) {
    condTermFlagB = 0;
  } else if ((mbAddrB < 0 &&
              (IS_INTRA_Prediction_Mode(
                  picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode))) ||
             picture.m_mbs[mbAddrB].m_name_of_mb_type == I_PCM)
    condTermFlagB = 1;
  else
    condTermFlagB = transBlockB_coded_block_flag;

  //------ctxIdxInc( ctxBlockCat )--------------
  ctxIdxInc = condTermFlagA + 2 * condTermFlagB;

  return 0;
}

// 9.3.3.1.1.10 Derivation process of ctxIdxInc for the syntax element transform_size_8x8_flag
int Cabac::decode_transform_size_8x8_flag(int32_t &synElVal) {
  int32_t ctxIdxOffset = 0, ctxIdxInc = 0, ctxIdx = 0;
  int32_t binVal = 0;
  int32_t bypassFlag = 0;

  //------Table 9-34: ctxIdxOffset: 399--------
  ctxIdxOffset = 399;

  // Table 9-39 – Assignment of ctxIdxInc to binIdx for all ctxIdxOffset values except those related to the syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1

  // 0,1,2 (clause 9.3.3.1.1.10)
  RET(derivation_ctxIdxInc_for_transform_size_8x8_flag(ctxIdxInc));

  //---------------注意是：FL, cMax=1------------------------
  ctxIdx = ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;
  synElVal = binVal;
  return 0;
}

// 9.3.3.1.1.10 Derivation process of ctxIdxInc for the syntax element transform_size_8x8_flag
int Cabac::derivation_ctxIdxInc_for_transform_size_8x8_flag(
    int32_t &ctxIdxInc) {
  const bool MbaffFrameFlag = picture.m_slice->slice_header->MbaffFrameFlag;
  int32_t mbAddrA = 0, mbAddrB = 0, isChroma = 0;
  RET(picture.derivation_for_neighbouring_macroblocks(
      MbaffFrameFlag, picture.CurrMbAddr, mbAddrA, mbAddrB, isChroma));

  int32_t condTermFlagA = 1, condTermFlagB = 1;
  if (mbAddrA < 0 || picture.m_mbs[mbAddrA].transform_size_8x8_flag == 0)
    condTermFlagA = 0;
  if (mbAddrB < 0 || picture.m_mbs[mbAddrB].transform_size_8x8_flag == 0)
    condTermFlagB = 0;

  ctxIdxInc = condTermFlagA + condTermFlagB;
  return 0;
}

// 7.3.5.3.3 Residual block CABAC syntax
// TODO 后面需要再仔细看一下，复现CABAC算法的时候吧 <24-10-03 15:51:55, YangJing>
int Cabac::residual_block_cabac(int32_t coeffLevel[], int32_t startIdx,
                                int32_t endIdx, int32_t maxNumCoeff,
                                MB_RESIDUAL_LEVEL mb_block_level,
                                int32_t BlkIdx, int32_t iCbCr,
                                int32_t &TotalCoeff) {
  const int32_t ChromaArrayType =
      picture.m_slice->slice_header->m_sps->ChromaArrayType;

  TotalCoeff = 0;

  // 默认设置为1，意味着块总是被编码，除非显式地解码为0
  int32_t coded_block_flag = 1;

  //YUV4:4:4
  if (maxNumCoeff != 64 || ChromaArrayType == 3)
    RET(decode_coded_block_flag(mb_block_level, BlkIdx, iCbCr,
                                coded_block_flag));

  //将所有的残差系数初始化为 0
  std::fill_n(coeffLevel, maxNumCoeff, 0);

  if (!coded_block_flag) return 0;

  //遍历从 startIdx 到 endIdx 的系数，解码重要系数标志（significant_coeff_flag）和最后的重要系数标志（last_significant_coeff_flag）
  int32_t numCoeff = endIdx + 1;
  int32_t i = startIdx;

  int32_t significant_coeff_flag[64] = {0},
          last_significant_coeff_flag[64] = {0};

  while (i < numCoeff - 1) {
    int32_t &levelListIdx = i;
    RET(decode_significant_coeff_flag(mb_block_level, levelListIdx, 0,
                                      significant_coeff_flag[i]));
    if (significant_coeff_flag[i]) {
      RET(decode_significant_coeff_flag(mb_block_level, levelListIdx, 1,
                                        last_significant_coeff_flag[i]));
      // 跳出循环，因为剩下的DCT变换系数都是0了
      if (last_significant_coeff_flag[i]) numCoeff = i + 1;
    }
    i++;
  }

  //对最后一个系数进行解码，包括其绝对值减1的级别和符号。然后计算并更新系数的实际值
  int32_t numDecodAbsLevelEq1 = 0, numDecodAbsLevelGt1 = 0;
  int32_t coeff_abs_level_minus1[64] = {0}, coeff_sign_flag[64] = {0};
  RET(decode_coeff_abs_level_minus1(mb_block_level, numDecodAbsLevelEq1,
                                    numDecodAbsLevelGt1,
                                    coeff_abs_level_minus1[numCoeff - 1]));
  RET(decode_coeff_sign_flag(coeff_sign_flag[numCoeff - 1]));

  coeffLevel[numCoeff - 1] = (coeff_abs_level_minus1[numCoeff - 1] + 1) *
                             (1 - 2 * coeff_sign_flag[numCoeff - 1]);

  TotalCoeff = 1;
  if (ABS(coeffLevel[numCoeff - 1]) == 1)
    numDecodAbsLevelEq1++;
  else if (ABS(coeffLevel[numCoeff - 1]) > 1)
    numDecodAbsLevelGt1++;

  //逆序解码其它重要的系数，并更新它们的绝对值和符号
  for (i = numCoeff - 2; i >= startIdx; i--) {
    if (significant_coeff_flag[i]) {
      //index[TotalCoeff] = i;
      TotalCoeff++;
      RET(decode_coeff_abs_level_minus1(mb_block_level, numDecodAbsLevelEq1,
                                        numDecodAbsLevelGt1,
                                        coeff_abs_level_minus1[i]));
      RET(decode_coeff_sign_flag(coeff_sign_flag[i]));
      coeffLevel[i] =
          (coeff_abs_level_minus1[i] + 1) * (1 - 2 * coeff_sign_flag[i]);

      if (ABS(coeffLevel[i]) == 1)
        numDecodAbsLevelEq1++;
      else if (ABS(coeffLevel[i]) > 1)
        numDecodAbsLevelGt1++;
    }
  }

  //根据块级别和色度分量，更新相应的编码块标志模式
  if (mb_block_level == MB_RESIDUAL_Intra16x16DCLevel ||
      mb_block_level == MB_RESIDUAL_ChromaDCLevel ||
      mb_block_level == MB_RESIDUAL_CbIntra16x16DCLevel ||
      mb_block_level == MB_RESIDUAL_CrIntra16x16DCLevel) {
    picture.m_mbs[picture.CurrMbAddr].coded_block_flag_DC_pattern ^=
        1 << (iCbCr + 1);
  } else
    picture.m_mbs[picture.CurrMbAddr].coded_block_flag_AC_pattern[iCbCr + 1] ^=
        1 << BlkIdx;
  return 0;
}

// 如果last_flag=1,则表示 CABAC_decode_last_significant_coeff_flag(...)
int Cabac::decode_significant_coeff_flag(MB_RESIDUAL_LEVEL mb_block_level,
                                         int32_t levelListIdx,
                                         int32_t last_flag, int32_t &synElVal) {
  int32_t NumC8x8 = 4 / (picture.m_slice->slice_header->m_sps->SubWidthC *
                         picture.m_slice->slice_header->m_sps->SubHeightC);
  int32_t mb_field_decoding_flag =
      picture.m_mbs[picture.CurrMbAddr].mb_field_decoding_flag;
  int32_t ctxIdxOffset = 0, ctxIdxInc = 0, ctxIdx = 0;
  int32_t binVal = 0;
  int32_t bypassFlag = 0;
  int32_t ctxBlockCat = 0;

  //--------1.先计算出ctxBlockCat的值------------
  // Table 9-42 – Specification of ctxBlockCat for the different blocks

  ctxBlockCat = mb_block_level;

  //--------2.获取ctxIdxOffset的值-----------
  // Table 9-34 – Syntax elements and associated types of binarization, maxBinIdxCtx, and ctxIdxOffset
  if (ctxBlockCat < 5) //(frame coded blocks with ctxBlockCat < 5) FL, cMax=1
  {
    if (last_flag == 0) // significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 105 : 277;
    else // last_significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 166 : 338;
  } else if (ctxBlockCat == 5) {
    if (last_flag == 0) // significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 402 : 436;
    else // last_significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 417 : 451;
  } else if (ctxBlockCat > 5 && ctxBlockCat < 9) {
    if (last_flag == 0) // significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 484 : 776;
    else // last_significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 572 : 864;
  } else if (ctxBlockCat > 9 && ctxBlockCat < 13) {
    if (last_flag == 0) // significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 528 : 820;
    else // last_significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 616 : 908;
  } else if (ctxBlockCat == 9) {
    if (last_flag == 0) // significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 660 : 675;
    else // last_significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 690 : 699;
  } else // if (ctxBlockCat == 13)
  {
    if (last_flag == 0) // significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 718 : 733;
    else // last_significant_coeff_flag
      ctxIdxOffset = (mb_field_decoding_flag == 0) ? 748 : 757;
  }

  // Table 9-40 – Assignment of ctxIdxBlockCatOffset to ctxBlockCat for syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1

  const int32_t ctxIdxBlockCatOffset_arr[14] = {0,  15, 29, 44, 47, 0,  0,
                                                15, 29, 0,  0,  15, 29, 0};
  int32_t ctxIdxBlockCatOffset = ctxIdxBlockCatOffset_arr[ctxBlockCat];

  // 9.3.3.1.3 Assignment process of ctxIdxInc for syntax elements significant_coeff_flag, last_significant_coeff_flag Let the variable levelListIdx be set equal to the index of the list of transform coefficient levels as specified in clause 7.4.5.3.

  //------------------------------
  if (ctxBlockCat != 3 && ctxBlockCat != 5 && ctxBlockCat != 9 &&
      ctxBlockCat != 13) {
    // levelListIdx ranges from 0 to maxNumCoeff - 2, inclusive.
    ctxIdxInc = levelListIdx;
  } else if (ctxBlockCat == 3) {
    RET(levelListIdx < 0 || levelListIdx > 4 * NumC8x8 - 2);
    // levelListIdx ranges from 0 to 4 * NumC8x8 - 2, inclusive
    ctxIdxInc = MIN(levelListIdx / NumC8x8, 2);
  } else if (ctxBlockCat == 5 || ctxBlockCat == 9 || ctxBlockCat == 13) {
    // Table 9-43 – Mapping of scanning position to ctxIdxInc for ctxBlockCat = = 5, 9, or 13

    const int32_t ctxIdxInc_coeff[63][3] = {
        {0, 0, 0},   {1, 1, 1},   {2, 1, 1},   {3, 2, 1},   {4, 2, 1},
        {5, 3, 1},   {5, 3, 1},   {4, 4, 1},   {4, 5, 1},   {3, 6, 1},
        {3, 7, 1},   {4, 7, 1},   {4, 7, 1},   {4, 8, 1},   {5, 4, 1},
        {5, 5, 1},   {4, 6, 2},   {4, 9, 2},   {4, 10, 2},  {4, 10, 2},
        {3, 8, 2},   {3, 11, 2},  {6, 12, 2},  {7, 11, 2},  {7, 9, 2},
        {7, 9, 2},   {8, 10, 2},  {9, 10, 2},  {10, 8, 2},  {9, 11, 2},
        {8, 12, 2},  {7, 11, 2},  {7, 9, 3},   {6, 9, 3},   {11, 10, 3},
        {12, 10, 3}, {13, 8, 3},  {11, 11, 3}, {6, 12, 3},  {7, 11, 3},
        {8, 9, 4},   {9, 9, 4},   {14, 10, 4}, {10, 10, 4}, {9, 8, 4},
        {8, 13, 4},  {6, 13, 4},  {11, 9, 4},  {12, 9, 5},  {13, 10, 5},
        {11, 10, 5}, {6, 8, 5},   {9, 13, 6},  {14, 13, 6}, {10, 9, 6},
        {9, 9, 6},   {11, 10, 7}, {12, 10, 7}, {13, 14, 7}, {11, 14, 7},
        {14, 14, 8}, {10, 14, 8}, {12, 14, 8}};

    RET(levelListIdx < 0 || levelListIdx > 63);

    if (last_flag == 0) // significant_coeff_flag
      ctxIdxInc = ctxIdxInc_coeff[levelListIdx][mb_field_decoding_flag];
    else // last_significant_coeff_flag
      ctxIdxInc = ctxIdxInc_coeff[levelListIdx][2];
  }

  //--------3.计算出ctxIdx的值-----------
  ctxIdx = ctxIdxOffset + ctxIdxBlockCatOffset + ctxIdxInc;

  //---------------注意是：FL, cMax=1------------------------
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

  synElVal = binVal;

  return 0;
}

int Cabac::decode_coeff_abs_level_minus1(MB_RESIDUAL_LEVEL mb_block_level,
                                         int32_t numDecodAbsLevelEq1,
                                         int32_t numDecodAbsLevelGt1,
                                         int32_t &synElVal) {
  int32_t ctxIdxOffset = 0, ctxIdxInc = 0, ctxIdx = 0;
  int32_t binVal = 0;
  int32_t bypassFlag = 0;
  int32_t ctxBlockCat = 0;

  //--------1.先计算出ctxBlockCat的值------------
  // Table 9-42 – Specification of ctxBlockCat for the different blocks
  ctxBlockCat = mb_block_level;

  //--------2.获取ctxIdxOffset-prefix的值-----------

  // Table 9-34 – Syntax elements and associated types of binarization, maxBinIdxCtx, and ctxIdxOffset
  if (ctxBlockCat < 5) //(blocks with ctxBlockCat < 5)
    ctxIdxOffset = 227;
  else if (ctxBlockCat == 5)
    ctxIdxOffset = 426;
  else if (ctxBlockCat > 5 && ctxBlockCat < 9)
    ctxIdxOffset = 952;
  else if (ctxBlockCat > 9 && ctxBlockCat < 13)
    ctxIdxOffset = 982;
  else if (ctxBlockCat == 9)
    ctxIdxOffset = 708;
  else // if (ctxBlockCat == 13)
    ctxIdxOffset = 766;

  // Table 9-40 – Assignment of ctxIdxBlockCatOffset to ctxBlockCat for syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1

  const int32_t ctxIdxBlockCatOffset_arr[14] = {0,  10, 20, 30, 39, 0,  0,
                                                10, 20, 0,  0,  10, 20, 0};
  int32_t ctxIdxBlockCatOffset = ctxIdxBlockCatOffset_arr[ctxBlockCat];

  ctxIdxInc =
      ((numDecodAbsLevelGt1 != 0) ? 0 : MIN(4, 1 + numDecodAbsLevelEq1));

  //--------3.计算出ctxIdx的值-----------
  ctxIdx = ctxIdxOffset + ctxIdxBlockCatOffset + ctxIdxInc;

  //--------4.进行处理 UEG0 with signedValFlag=0, uCoff=14-----------

  // UEGk编码是由 prefix(TU binarization) + suffix(Exp-Golomb) + signedValFlag,三部分组成

  //-----4.1. 先解码前缀(TU)--------
  const int32_t uCoff = 14;

  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;

  synElVal = 0;
  ctxIdxInc = 5 + MIN(4 - ((ctxBlockCat == 3) ? 1 : 0),
                      numDecodAbsLevelGt1); // if (binIdx > 0)
  ctxIdx = ctxIdxOffset + ctxIdxBlockCatOffset + ctxIdxInc;

  // TU, cMax=uCoff=14;
  while (binVal == 1) //(11...1)b
  {
    synElVal++;
    if (synElVal >= uCoff) break;

    RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1,2,3,4.,..,k;
  }

  //-----4.2. 再解码后缀(Exp-Golomb)--------
  if (synElVal != uCoff) // signedValFlag=0
  {
    // synElVal为prefix(TU)的值 //synElVal consists only of a prefix bit string
  } else {
    // 注意：9.3.2.3 Concatenated unary/ k-th order Exp-Golomb (UEGk)
    // binarization process 中的伪代码表示是UEGk的编码过程，解码需要逆向过来

    //------Table 9-34: ctxIdxOffset-suffix: na (uses DecodeBypass)--------
    int32_t k = 0; // k取值为UEG0中的0

    if (synElVal >= uCoff) // if ( Abs( synElVal ) >= uCoff ) //uCoff=14
    {
      RET(decodeBypass(binVal));

      while (binVal == 1) {
        synElVal += 1 << k;
        ++k;
        RET(k >= 32 - uCoff); // error: coeff_abs_level_minus1值过大
        RET(decodeBypass(binVal));
      }

      while (k--) {
        RET(decodeBypass(binVal));
        synElVal += binVal << k;
      }
    }

    // signedValFlag=0代表结果是无符号整数，所以不需要处理最后一个符号位
  }

  return 0;
}

int Cabac::decode_coeff_sign_flag(int32_t &synElVal) {
  int32_t binVal = 0;

  //------Table 9-34: ctxIdxOffset: (uses DecodeBypass)--------

  // Table 9-39 – Assignment of ctxIdxInc to binIdx for all ctxIdxOffset values except those related to the syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1

  //---------------注意是：FL, cMax=1------------------------
  RET(decodeBypass(binVal)); // binIdx = 0;
  synElVal = binVal;
  return 0;
}

int Cabac::decode_end_of_slice_flag(int32_t &synElVal) {
  int32_t ctxIdxOffset = 0, binVal = 0, ctxIdx = 0, bypassFlag = 0;
  //------Table 9-34: ctxIdxOffset: 276--------
  ctxIdxOffset = 276;

  // Table 9-39 – Assignment of ctxIdxInc to binIdx for all ctxIdxOffset values except those related to the syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1
  //---------------注意是：FL, cMax=1------------------------
  ctxIdx = ctxIdxOffset + 0;                  // ctxIdxOffset + ctxIdxInc;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;
  synElVal = binVal;
  return 0;
}

int Cabac::decode_prev_intra4x4_or_intra8x8_pred_mode_flag(int32_t &synElVal) {
  int32_t ctxIdxOffset = 0, ctxIdx = 0;
  int32_t binVal = 0;
  int32_t bypassFlag = 0;

  //------Table 9-34: ctxIdxOffset: 68--------
  ctxIdxOffset = 68;

  // Table 9-39 – Assignment of ctxIdxInc to binIdx for all ctxIdxOffset values except those related to the syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1

  //---------------注意是：FL, cMax=1------------------------
  ctxIdx = ctxIdxOffset + 0;                  // ctxIdxInc = 0;
  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;
  synElVal = binVal;
  return 0;
}

int Cabac::decode_rem_intra4x4_or_intra8x8_pred_mode(int32_t &synElVal) {
  int32_t ctxIdxOffset = 0, ctxIdx = 0;
  int32_t binVal = 0;
  int32_t bypassFlag = 0;

  //------Table 9-34: ctxIdxOffset: 69--------
  ctxIdxOffset = 69;

  // Table 9-39 – Assignment of ctxIdxInc to binIdx for all ctxIdxOffset values except those related to the syntax elements coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1

  //---------------注意是：FL, cMax=7------------------------
  ctxIdx = ctxIdxOffset + 0; // ctxIdxInc = 0;

  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 0;
  synElVal = binVal;

  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 1;
  synElVal += binVal << 1;

  RET(decodeBin(bypassFlag, ctxIdx, binVal)); // binIdx = 2;
  synElVal += binVal << 2;

  return 0;
}

// 9.3.3.2 Arithmetic decoding process
/* 输入：在第9.3.3.1节中导出的bypassFlag、ctxIdx以及算术解码引擎的状态变量codIRange和codIOffset 
 * 输出：bin 的值*/
int Cabac::decodeBin(int32_t bypassFlag, int32_t ctxIdx, int32_t &bin) {
  int ret = 0;
  // 解码不依赖于上下文模型的状态，简单地从比特流中直接读取下一个比特
  if (bypassFlag) ret = decodeBypass(bin);
  // 解码数据流的结尾，解码终止符号
  else if (ctxIdx == 276)
    ret = decodeTerminate(bin);
  // 根据指定的上下文索引 ctxIdx 解码二进制符号
  else
    ret = decodeDecision(ctxIdx, bin);
  return ret;
}

// 9.3.3.2.3 Bypass decoding process for binary decisions
/* 输入: 切片数据的位以及变量 codIRange 和 codIOffset。  
 * 输出: 更新的变量 codIOffset 和解码值 binVal。 */
int Cabac::decodeBypass(int32_t &binVal) {
  // 1. 默认为0比特值
  binVal = 0;
  // 2. 将已经解码的位流中添加一个新的比特，在已累积的已经编码位流中进行累加
  codIOffset = (codIOffset << 1) | bs.readUn(1);
  // 3. 进行范围调整，以保持编码过程的精度和避免溢出
  if (codIOffset >= codIRange) binVal = 1, codIOffset -= codIRange;
  //RET(codIOffset >= codIRange);
  return 0;
}

// 9.3.3.2.4 Decoding process for binary decisions before termination
/* 输入: 来自切片数据的位以及变量 codIRange 和 codIOffset。  
 * 输出: 更新的变量 codIRange 和 codIOffset 以及解码值 binVal。 */
int Cabac::decodeTerminate(int32_t &binVal) {
  // 1. 默认为0比特值
  binVal = 0;
  // 2. 准备检测终止符号。对于终止符，通常会有一个特定的概率设置（此处通过减去2来调整）。
  codIRange -= 2;
  // 3-1. 不进行重整化，终止CABAC解码, codIOffset中插入的最后一位等于1。 NOTE:当解码end_of_slice_flag 时，寄存器codIOffset 中插入的最后一位被解释为rbsp_stop_one_bit。
  if (codIOffset >= codIRange)
    binVal = 1;
  else // 执行范围重归一化
    return renormD();

  return 0;
}

// 9.3.3.2.2 Renormalization process in the arithmetic decoding engine
/* 输入: 来自切片数据的位以及变量 codIRange 和 codIOffset。  
 * 输出: 更新后的变量 codIRange 和 codIOffset。 */
int Cabac::renormD() {
  /* 如果codIRange大于或等于256，则不需要重新归一化，并且RenormD过程结束；  
   * 否则（codIRange 小于 256），进入重整化循环。在此循环中，codIRange 的值加倍，即左移 1，并使用 read_bits( 1 ) 将一位移入 codIOffset 中。  */
  while (codIRange < 256) {
    codIRange = codIRange << 1;
    codIOffset = (codIOffset << 1) | bs.readUn(1);
  }
  return (codIOffset >= codIRange);
}

// 9.3.3.2.1 Arithmetic decoding process for a binary decision
/* 输入: ctxIdx、codIRange 和 codIOffset。  
 * 输出: 解码值 binVal 以及更新的变量 codIRange 和 codIOffset。*/
int Cabac::decodeDecision(int32_t ctxIdx, int32_t &binVal) {
  // 1. 取第7,8位得到查询 LPS 表的索引，这里通过量化 codIRange 来选择不同的 LPS 概率区间
  int32_t qCodIRangeIdx = (codIRange >> 6) & 0b11;
  // 2. 给定上下文 ctxIdx 相关联的状态索引 pStateIdx，决定 MPS 和 LPS 的概率模型
  int32_t pStateIdx = pStateIdxs[ctxIdx];
  // 3. 通过查表（rangeTabLPS）获得 LPS 对应的范围值 codIRangeLPS。
  int32_t codIRangeLPS = rangeTabLPS[pStateIdx][qCodIRangeIdx];

  // 4. 逐步缩小范围来逼近当前符号的概率区间
  codIRange -= codIRangeLPS;
  // 5. 取得当前上下文的 MPS（最可能符号），表明此上下文中编码符号是 0 还是 1 的概率更大。
  bool valMPS = valMPSs[ctxIdx];
  // 6. 如果当前偏移量 codIOffset 大于等于 codIRange，则说明发生了 LPS 事件（即发生了最不可能的符号）
  if (codIOffset >= codIRange) {
    // a. 将 binVal 设置为与 valMPS 相反的值
    binVal = !valMPS;
    // b. 更新 codIOffset，以便进入下一步解码
    codIOffset -= codIRange;
    // c. 更新 codIRange，以便解码新的符号
    codIRange = codIRangeLPS;

    // d. pStateIdx 为 0，反转当前上下文的 valMPS 值。状态索引为 0 时，表示 MPS 的概率非常低，因此有可能会发生 MPS 和 LPS 反转。
    if (pStateIdx == 0) valMPSs[ctxIdx] = !valMPS;
    // e. 根据 pStateIdx 使用 LPS 转移表 transIdxLPS 更新当前上下文的状态索引。这是 CABAC 中的自适应概率更新机制的一部分，根据当前符号的发生情况动态调整状态。
    pStateIdxs[ctxIdx] = transIdxLPS[pStateIdx];
  }
  // 7. 如果发生的是 MPS（最可能符号）
  else {
    // a. 直接将 binVal 设置为当前上下文的 MPS 值
    binVal = valMPS;
    // b. 根据 pStateIdx 使用 MPS 转移表 transIdxMPS 更新当前上下文的状态索引。
    pStateIdxs[ctxIdx] = transIdxMPS[pStateIdx];
  }
  // 8. 执行范围重归一化
  return renormD();
}
