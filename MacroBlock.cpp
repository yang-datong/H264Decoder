#include "MacroBlock.hpp"
#include "BitStream.hpp"
#include "CH264Golomb.hpp"
#include "Constants.hpp"
#include "H264Cabac.hpp"
#include "H264ResidualBlockCavlc.hpp"
#include "PictureBase.hpp"
#include "SliceHeader.hpp"
#include "Type.hpp"

/* TODO  <24-09-01 02:44:59, YangJing> 再把这个类的内容整理一下，然后理解一下 */

// 7.3.5 Macroblock layer syntax
int MacroBlock::macroblock_layer(BitStream &bs, PictureBase &picture,
                                 const SliceBody &slice_data,
                                 CH264Cabac &cabac) {
  /* ------------------ 设置别名 ------------------ */
  SliceHeader &header = picture.m_slice.slice_header;
  SPS &sps = picture.m_slice.m_sps;
  PPS &pps = picture.m_slice.m_pps;
  /* ------------------  End ------------------ */

  _is_cabac = pps.entropy_coding_mode_flag; // 是否CABAC编码

  /* TODO YangJing 初始化 <24-09-01 02:07:46> */
  this->_cabac = &cabac;
  this->_gb = new CH264Golomb();
  this->_bs = &bs;

  initFromSlice(header, slice_data);

  constrained_intra_pred_flag = pps.constrained_intra_pred_flag;

  //-----------------------------
  process_decode_mb_type(picture, header, header.slice_type);

  if (m_mb_type_fixed == I_PCM) {
    while (!bs.byte_aligned())
      pcm_alignment_zero_bit = bs.readUn(1);

    for (int i = 0; i < 256; i++)
      pcm_sample_luma[i] = bs.readUn(sps.BitDepthY);

    for (int i = 0; i < 2 * sps.MbWidthC * sps.MbHeightC; i++)
      pcm_sample_chroma[i] = bs.readUn(sps.BitDepthC);

  } else {
    int32_t noSubMbPartSizeLessThan8x8Flag = 1;
    int32_t transform_size_8x8_flag_temp = 0;

    if (m_name_of_mb_type != I_NxN && m_mb_pred_mode != Intra_16x16 &&
        m_NumMbPart == 4) {
      sub_mb_pred(picture, slice_data);
      for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        if (m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8) {
          if (NumSubMbPartFunc(mbPartIdx) > 1)
            noSubMbPartSizeLessThan8x8Flag = 0;
        } else if (!sps.direct_8x8_inference_flag)
          noSubMbPartSizeLessThan8x8Flag = 0;
      }
    } else {
      if (pps.transform_8x8_mode_flag && m_name_of_mb_type == I_NxN)
        process_transform_size_8x8_flag(transform_size_8x8_flag_temp);
      mb_pred(picture, slice_data);
    }

    if (m_mb_pred_mode != Intra_16x16) {
      process_coded_block_pattern(sps.ChromaArrayType);
      if (CodedBlockPatternLuma > 0 && pps.transform_8x8_mode_flag &&
          m_name_of_mb_type != I_NxN && noSubMbPartSizeLessThan8x8Flag &&
          (m_name_of_mb_type != B_Direct_16x16 ||
           sps.direct_8x8_inference_flag))
        process_transform_size_8x8_flag(transform_size_8x8_flag_temp);
    }

    if (CodedBlockPatternLuma > 0 || CodedBlockPatternChroma > 0 ||
        m_mb_pred_mode == Intra_16x16) {
      process_mb_qp_delta();
      residual(picture, 0, 15);
    }
  }

  /* TODO YangJing 下面是干嘛？ <24-09-01 00:03:51> */
  //---------------------------------------------------
  if (mb_qp_delta < (int32_t)(-(26 + (int32_t)sps.QpBdOffsetY / 2)) ||
      mb_qp_delta > (25 + (int32_t)sps.QpBdOffsetY / 2)) {
    LOG_WARN("mb_qp_delta=(%d) is out of range [%d,%d].\n", mb_qp_delta,
             (-(26 + (int32_t)sps.QpBdOffsetY / 2)),
             (25 + (int32_t)sps.QpBdOffsetY / 2));
    mb_qp_delta = CLIP3((-(26 + (int32_t)sps.QpBdOffsetY / 2)),
                        (25 + (int32_t)sps.QpBdOffsetY / 2), mb_qp_delta);
  }

  QPY = ((header.QPY_prev + mb_qp_delta + 52 + 2 * sps.QpBdOffsetY) %
         (52 + sps.QpBdOffsetY)) -
        sps.QpBdOffsetY;

  QP1Y = QPY + sps.QpBdOffsetY;
  header.QPY_prev = QPY;

  TransformBypassModeFlag =
      (sps.qpprime_y_zero_transform_bypass_flag == 1 && QP1Y == 0) ? 1 : 0;

  return 0;
}

int MacroBlock::macroblock_layer_mb_skip(PictureBase &picture,
                                         const SliceBody &slice_data,
                                         CH264Cabac &cabac) {
  int ret = 0;
  // int32_t mbPartIdx = 0;

  /* TODO YangJing 初始化 <24-09-01 02:07:46> */
  this->_cabac = &cabac;
  this->_gb = new CH264Golomb();

  SliceHeader &slice_header = picture.m_slice.slice_header;

  field_pic_flag = slice_header.field_pic_flag;
  bottom_field_flag = slice_header.bottom_field_flag;
  mb_skip_flag = slice_data.mb_skip_flag;
  mb_field_decoding_flag = slice_data.mb_field_decoding_flag;
  MbaffFrameFlag = slice_header.MbaffFrameFlag;
  constrained_intra_pred_flag =
      picture.m_slice.m_pps.constrained_intra_pred_flag;
  disable_deblocking_filter_idc = slice_header.disable_deblocking_filter_idc;
  CurrMbAddr = slice_data.CurrMbAddr;
  slice_id = slice_data.slice_id;
  slice_number = slice_data.slice_number;
  m_slice_type = slice_header.slice_type;
  FilterOffsetA = slice_header.FilterOffsetA;
  FilterOffsetB = slice_header.FilterOffsetB;

  ret = picture.Inverse_macroblock_scanning_process(
      slice_header.MbaffFrameFlag, CurrMbAddr, mb_field_decoding_flag,
      picture.m_mbs[CurrMbAddr].m_mb_position_x,
      picture.m_mbs[CurrMbAddr].m_mb_position_y);
  RETURN_IF_FAILED(ret != 0, ret);

  //    int is_ae = picture.m_slice.m_pps.entropy_coding_mode_flag;
  //    //ae(v)表示CABAC编码

  if (slice_header.slice_type == SLICE_P ||
      slice_header.slice_type == SLICE_SP) {
    mb_type = 5; // inferred: P_Skip: no further data is present for the
                 // macroblock in the bitstream.
  } else if (slice_header.slice_type == SLICE_B) {
    mb_type = 23; // inferred: B_Skip: no further data is present for the
                  // macroblock in the bitstream.
  }

  //    ret = fix_mb_type(slice_header.slice_type, mb_type, m_slice_type_fixed,
  //    m_mb_type_fixed); //需要立即修正mb_type的值 RETURN_IF_FAILED(ret != 0,
  //    ret);

  m_slice_type_fixed = slice_header.slice_type;
  m_mb_type_fixed = mb_type;

  //-----------------------------------------------
  // int32_t noSubMbPartSizeLessThan8x8Flag = 1;
  ret = MbPartPredMode(m_slice_type_fixed, transform_size_8x8_flag,
                       m_mb_type_fixed, 0, m_NumMbPart, CodedBlockPatternChroma,
                       CodedBlockPatternLuma, Intra16x16PredMode,
                       m_name_of_mb_type, m_mb_pred_mode);
  RETURN_IF_FAILED(ret != 0, ret);

  //    RETURN_IF_FAILED(m_name_of_mb_type == P_Skip || m_name_of_mb_type ==
  //    B_Skip, -1);

  //---------------------------------------------------
  mb_qp_delta = 0;

  QPY = ((slice_header.QPY_prev + mb_qp_delta + 52 +
          2 * picture.m_slice.m_sps.QpBdOffsetY) %
         (52 + picture.m_slice.m_sps.QpBdOffsetY)) -
        picture.m_slice.m_sps.QpBdOffsetY; // (7-37)
  QP1Y = QPY + picture.m_slice.m_sps.QpBdOffsetY;
  slice_header.QPY_prev = QPY;

  ret = set_mb_type_X_slice_info();
  // 因CABAC会用到MbPartWidth/MbPartHeight信息，所以需要尽可能提前设置相关值
  RETURN_IF_FAILED(ret != 0, ret);

  return 0;
}

int MacroBlock::getMbPartWidthAndHeight(H264_MB_TYPE name_of_mb_type,
                                        int32_t &_MbPartWidth,
                                        int32_t &_MbPartHeight) {
  int ret = 0;

  if (name_of_mb_type >= P_L0_16x16 && name_of_mb_type <= P_Skip) {
    _MbPartWidth =
        mb_type_P_SP_slices_define[name_of_mb_type - P_L0_16x16].MbPartWidth;
    _MbPartHeight =
        mb_type_P_SP_slices_define[name_of_mb_type - P_L0_16x16].MbPartHeight;
  } else if (name_of_mb_type >= B_Direct_16x16 && name_of_mb_type <= B_Skip) {
    _MbPartWidth =
        mb_type_B_slices_define[name_of_mb_type - B_Direct_16x16].MbPartWidth;
    _MbPartHeight =
        mb_type_B_slices_define[name_of_mb_type - B_Direct_16x16].MbPartHeight;
  } else if (name_of_mb_type >= P_L0_8x8 && name_of_mb_type <= P_L0_4x4) {
    _MbPartWidth =
        sub_mb_type_P_mbs_define[name_of_mb_type - P_L0_8x8].SubMbPartWidth;
    _MbPartHeight =
        sub_mb_type_P_mbs_define[name_of_mb_type - P_L0_8x8].SubMbPartHeight;
  } else if (name_of_mb_type >= B_Direct_8x8 && name_of_mb_type <= B_Bi_4x4) {
    _MbPartWidth =
        sub_mb_type_B_mbs_define[name_of_mb_type - B_Direct_8x8].SubMbPartWidth;
    _MbPartHeight = sub_mb_type_B_mbs_define[name_of_mb_type - B_Direct_8x8]
                        .SubMbPartHeight;
  } else {
    ret = -1;
  }

  return ret;
}

int MacroBlock::SubMbPredModeFunc(int32_t slice_type, int32_t sub_mb_type,
                                  int32_t &NumSubMbPart,
                                  H264_MB_PART_PRED_MODE &SubMbPredMode,
                                  int32_t &SubMbPartWidth,
                                  int32_t &SubMbPartHeight) {
  if (slice_type == SLICE_I) {
    printf("Unknown slice_type=%d;\n", slice_type);
    return -1;
  } else if (slice_type == SLICE_P) {
    if (sub_mb_type >= 0 && sub_mb_type <= 4) {
      NumSubMbPart = sub_mb_type_P_mbs_define[sub_mb_type].NumSubMbPart;
      SubMbPredMode = sub_mb_type_P_mbs_define[sub_mb_type].SubMbPredMode;
      SubMbPartWidth = sub_mb_type_P_mbs_define[sub_mb_type].SubMbPartWidth;
      SubMbPartHeight = sub_mb_type_P_mbs_define[sub_mb_type].SubMbPartHeight;
    } else {
      printf("sub_mb_type_P_mbs_define: sub_mb_type=%d; Must be in [-1..5]\n",
             sub_mb_type);
      return -1;
    }
  } else if (slice_type == SLICE_B) {
    if (sub_mb_type >= 0 && sub_mb_type <= 12) {
      NumSubMbPart = sub_mb_type_B_mbs_define[sub_mb_type].NumSubMbPart;
      SubMbPredMode = sub_mb_type_B_mbs_define[sub_mb_type].SubMbPredMode;
      SubMbPartWidth = sub_mb_type_B_mbs_define[sub_mb_type].SubMbPartWidth;
      SubMbPartHeight = sub_mb_type_B_mbs_define[sub_mb_type].SubMbPartHeight;
    } else {
      printf("sub_mb_type_B_mbs_define: sub_mb_type=%d; Must be in [0..12]\n",
             sub_mb_type);
      return -1;
    }
  } else {
    printf("Unknown slice_type=%d;\n", slice_type);
    return -1;
  }

  return 0;
}

// 7.3.5.1 Macroblock prediction syntax
int MacroBlock::mb_pred(PictureBase &picture, const SliceBody &slice_data) {
  /* ------------------ 设置别名 ------------------ */
  SliceHeader &header = picture.m_slice.slice_header;
  SPS &sps = picture.m_slice.m_sps;
  /* ------------------  End ------------------ */
  int ret;
  if (m_mb_pred_mode == Intra_4x4 || m_mb_pred_mode == Intra_8x8 ||
      m_mb_pred_mode == Intra_16x16) {
    if (m_mb_pred_mode == Intra_4x4) {
      for (int luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; luma4x4BlkIdx++) {
        process_prev_intra4x4_pred_mode_flag(luma4x4BlkIdx);
        if (!prev_intra4x4_pred_mode_flag[luma4x4BlkIdx])
          process_rem_intra4x4_pred_mode(luma4x4BlkIdx);
      }
    }
    if (m_mb_pred_mode == Intra_8x8) {
      for (int luma8x8BlkIdx = 0; luma8x8BlkIdx < 4; luma8x8BlkIdx++) {
        process_prev_intra8x8_pred_mode_flag(luma8x8BlkIdx);
        if (!prev_intra8x8_pred_mode_flag[luma8x8BlkIdx])
          process_rem_intra8x8_pred_mode(luma8x8BlkIdx);
      }
    }

    if (sps.ChromaArrayType == 1 || sps.ChromaArrayType == 2)
      process_intra_chroma_pred_mode();

  } else if (m_mb_pred_mode != Direct) {
    H264_MB_PART_PRED_MODE mb_pred_mode = MB_PRED_MODE_NA;

    int32_t NumMbPart = m_NumMbPart;
    for (int mbPartIdx = 0; mbPartIdx < NumMbPart; mbPartIdx++) {
      ret = MbPartPredMode2(m_name_of_mb_type, mbPartIdx,
                            transform_size_8x8_flag, mb_pred_mode);
      RETURN_IF_FAILED(ret != 0, ret);

      if ((header.num_ref_idx_l0_active_minus1 > 0 ||
           slice_data.mb_field_decoding_flag != header.field_pic_flag) &&
          mb_pred_mode != Pred_L1)
        process_ref_idx_l0(mbPartIdx, picture.m_RefPicList0Length,
                           slice_data.mb_field_decoding_flag);
    }

    for (int mbPartIdx = 0; mbPartIdx < NumMbPart; mbPartIdx++) {
      ret = MbPartPredMode2(m_name_of_mb_type, mbPartIdx,
                            transform_size_8x8_flag, mb_pred_mode);
      RETURN_IF_FAILED(ret != 0, ret);

      if ((header.num_ref_idx_l1_active_minus1 > 0 ||
           slice_data.mb_field_decoding_flag != header.field_pic_flag) &&
          mb_pred_mode != Pred_L0)
        process_ref_idx_l1(mbPartIdx, picture.m_RefPicList1Length,
                           slice_data.mb_field_decoding_flag);
    }

    for (int mbPartIdx = 0; mbPartIdx < NumMbPart; mbPartIdx++) {
      ret = MbPartPredMode2(m_name_of_mb_type, mbPartIdx,
                            transform_size_8x8_flag, mb_pred_mode);
      if (mb_pred_mode != Pred_L1) {
        for (int compIdx = 0; compIdx < 2; compIdx++)
          process_mvd_l0(mbPartIdx, compIdx);
      }
    }

    for (int mbPartIdx = 0; mbPartIdx < NumMbPart; mbPartIdx++) {
      ret = MbPartPredMode2(m_name_of_mb_type, mbPartIdx,
                            transform_size_8x8_flag, mb_pred_mode);
      if (mb_pred_mode != Pred_L0) {
        for (int compIdx = 0; compIdx < 2; compIdx++)
          process_mvd_l1(mbPartIdx, compIdx);
      }
    }
  }

  return 0;
}

/*
 * Page 58/80/812
 * 7.3.5.2 Sub-macroblock prediction syntax
 */
int MacroBlock::sub_mb_pred(PictureBase &picture, const SliceBody &slice_data) {
  int ret = 0;
  int32_t mbPartIdx = 0;
  int32_t subMbPartIdx = 0;
  int32_t compIdx = 0;

  SliceHeader &slice_header = picture.m_slice.slice_header;

  int is_ae =
      picture.m_slice.m_pps.entropy_coding_mode_flag; // ae(v)表示CABAC编码

  //--------------------------
  for (mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    if (is_ae) // ae(v) 表示CABAC编码
    {
      ret =
          _cabac->decode_sub_mb_type(sub_mb_type[mbPartIdx]); // 2 ue(v) | ae(v)
      RETURN_IF_FAILED(ret != 0, ret);
    } else // ue(v) 表示CAVLC编码
    {
      sub_mb_type[mbPartIdx] = _gb->get_ue_golomb(*_bs); // 2 ue(v) | ae(v)
    }

    //--------------------------------------------------
    if (m_slice_type_fixed == SLICE_P && sub_mb_type[mbPartIdx] >= 0 &&
        sub_mb_type[mbPartIdx] <= 3) {
      m_name_of_sub_mb_type[mbPartIdx] =
          sub_mb_type_P_mbs_define[sub_mb_type[mbPartIdx]].name_of_sub_mb_type;
      m_sub_mb_pred_mode[mbPartIdx] =
          sub_mb_type_P_mbs_define[sub_mb_type[mbPartIdx]].SubMbPredMode;
      NumSubMbPart[mbPartIdx] =
          sub_mb_type_P_mbs_define[sub_mb_type[mbPartIdx]].NumSubMbPart;
      SubMbPartWidth[mbPartIdx] =
          sub_mb_type_P_mbs_define[sub_mb_type[mbPartIdx]].SubMbPartWidth;
      SubMbPartHeight[mbPartIdx] =
          sub_mb_type_P_mbs_define[sub_mb_type[mbPartIdx]].SubMbPartHeight;
    } else if (m_slice_type_fixed == SLICE_B && sub_mb_type[mbPartIdx] >= 0 &&
               sub_mb_type[mbPartIdx] <= 12) {
      m_name_of_sub_mb_type[mbPartIdx] =
          sub_mb_type_B_mbs_define[sub_mb_type[mbPartIdx]].name_of_sub_mb_type;
      m_sub_mb_pred_mode[mbPartIdx] =
          sub_mb_type_B_mbs_define[sub_mb_type[mbPartIdx]].SubMbPredMode;
      NumSubMbPart[mbPartIdx] =
          sub_mb_type_B_mbs_define[sub_mb_type[mbPartIdx]].NumSubMbPart;
      SubMbPartWidth[mbPartIdx] =
          sub_mb_type_B_mbs_define[sub_mb_type[mbPartIdx]].SubMbPartWidth;
      SubMbPartHeight[mbPartIdx] =
          sub_mb_type_B_mbs_define[sub_mb_type[mbPartIdx]].SubMbPartHeight;
    } else {
      printf("m_slice_type=%d; m_slice_type_fixed=%d; sub_mb_type[%d]=%d;\n",
             m_slice_type, m_slice_type_fixed, mbPartIdx,
             sub_mb_type[mbPartIdx]);
      return -1;
    }
  }

  for (mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    //-----mb_type is one of 3=P_8x8, 4=P_8x8ref0, 22=B_8x8----------
    if ((slice_header.num_ref_idx_l0_active_minus1 > 0 ||
         slice_data.mb_field_decoding_flag != slice_header.field_pic_flag) &&
        m_name_of_mb_type != P_8x8ref0 &&
        m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
        m_sub_mb_pred_mode[mbPartIdx] !=
            Pred_L1) // SubMbPredMode(sub_mb_type[ mbPartIdx ]) != Pred_L1)
    {
      if (is_ae) // ae(v) 表示CABAC编码
      {
        int32_t ref_idx_flag = 0;

        ret =
            _cabac->decode_ref_idx_lX(ref_idx_flag, mbPartIdx,
                                      ref_idx_l0[mbPartIdx]); // 2 te(v) | ae(v)
        RETURN_IF_FAILED(ret != 0, ret);
      } else // ue(v) 表示CAVLC编码
      {
        int range = picture.m_RefPicList0Length - 1;

        if (slice_data.mb_field_decoding_flag ==
            1) // 注意：此处是个坑，T-REC-H.264-201704-S!!PDF-E.pdf
               // 文档中并未明确写出来
        {
          range = picture.m_RefPicList0Length * 2 - 1;
        }

        ref_idx_l0[mbPartIdx] =
            _gb->get_te_golomb(*_bs, range); // 2 te(v) | ae(v)
      }
    }
  }

  for (mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    //-----mb_type is one of 3=P_8x8, 4=P_8x8ref0, 22=B_8x8----------
    if ((slice_header.num_ref_idx_l1_active_minus1 > 0 ||
         slice_data.mb_field_decoding_flag != slice_header.field_pic_flag) &&
        m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
        m_sub_mb_pred_mode[mbPartIdx] !=
            Pred_L0) // SubMbPredMode(sub_mb_type[ mbPartIdx ]) != Pred_L0)
    {
      if (is_ae) // ae(v) 表示CABAC编码
      {
        int32_t ref_idx_flag = 1;

        ret =
            _cabac->decode_ref_idx_lX(ref_idx_flag, mbPartIdx,
                                      ref_idx_l1[mbPartIdx]); // 2 te(v) | ae(v)
        RETURN_IF_FAILED(ret != 0, ret);
      } else // ue(v) 表示CAVLC编码
      {
        int range = picture.m_RefPicList1Length - 1;

        if (slice_data.mb_field_decoding_flag ==
            1) // 注意：此处是个坑，T-REC-H.264-201704-S!!PDF-E.pdf
               // 文档中并未明确写出来
        {
          range = picture.m_RefPicList1Length * 2 - 1;
        }

        ref_idx_l1[mbPartIdx] =
            _gb->get_te_golomb(*_bs, range); // 2 te(v) | ae(v)
      }
    }
  }

  for (mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    //-----mb_type is one of 3=P_8x8, 4=P_8x8ref0, 22=B_8x8----------
    if (m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
        m_sub_mb_pred_mode[mbPartIdx] !=
            Pred_L1) // SubMbPredMode(sub_mb_type[ mbPartIdx ]) != Pred_L1)
    {
      // for (subMbPartIdx = 0; subMbPartIdx < NumSubMbPart(sub_mb_type[
      // mbPartIdx ]); subMbPartIdx++)
      for (subMbPartIdx = 0; subMbPartIdx < NumSubMbPart[mbPartIdx];
           subMbPartIdx++) {
        for (compIdx = 0; compIdx < 2; compIdx++) {
          if (is_ae) // ae(v) 表示CABAC编码
          {
            int32_t mvd_flag = compIdx;
            int32_t isChroma = 0;

            ret = _cabac->decode_mvd_lX(
                mvd_flag, mbPartIdx, subMbPartIdx, isChroma,
                mvd_l0[mbPartIdx][subMbPartIdx][compIdx]); // 2 ue(v) | ae(v)
            RETURN_IF_FAILED(ret != 0, ret);
          } else // ue(v) 表示CAVLC编码
          {
            mvd_l0[mbPartIdx][subMbPartIdx][compIdx] =
                _gb->get_se_golomb(*_bs); // 2 se(v) | ae(v)
          }
        }
      }
    }
  }

  for (mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
    //-----mb_type is one of 3=P_8x8, 4=P_8x8ref0, 22=B_8x8----------
    if (m_name_of_sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
        m_sub_mb_pred_mode[mbPartIdx] !=
            Pred_L0) // SubMbPredMode(sub_mb_type[ mbPartIdx ]) != Pred_L0)
    {
      // for (subMbPartIdx = 0; subMbPartIdx < NumSubMbPart(sub_mb_type[
      // mbPartIdx ]); subMbPartIdx++)
      for (subMbPartIdx = 0; subMbPartIdx < NumSubMbPart[mbPartIdx];
           subMbPartIdx++) {
        for (compIdx = 0; compIdx < 2; compIdx++) {
          if (is_ae) // ae(v) 表示CABAC编码
          {
            int32_t mvd_flag = 2 + compIdx;
            int32_t isChroma = 0;

            ret = _cabac->decode_mvd_lX(
                mvd_flag, mbPartIdx, subMbPartIdx, isChroma,
                mvd_l1[mbPartIdx][subMbPartIdx][compIdx]); // 2 ue(v) | ae(v)
            RETURN_IF_FAILED(ret != 0, ret);
          } else // ue(v) 表示CAVLC编码
          {
            mvd_l1[mbPartIdx][subMbPartIdx][compIdx] =
                _gb->get_se_golomb(*_bs); // 2 se(v) | ae(v)
          }
        }
      }
    }
  }

  return 0;
}

int MacroBlock::NumSubMbPartFunc(int mbPartIdx) {
  int NumSubMbPart;
  if (m_slice_type_fixed == SLICE_P && sub_mb_type[mbPartIdx] >= 0 &&
      sub_mb_type[mbPartIdx] <= 3) {
    NumSubMbPart =
        sub_mb_type_P_mbs_define[sub_mb_type[mbPartIdx]].NumSubMbPart;
  } else if (m_slice_type_fixed == SLICE_B && sub_mb_type[mbPartIdx] >= 0 &&
             sub_mb_type[mbPartIdx] <= 3) {
    NumSubMbPart =
        sub_mb_type_B_mbs_define[sub_mb_type[mbPartIdx]].NumSubMbPart;
  } else {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }
  return NumSubMbPart;
}

string MacroBlock::getNameOfMbTypeStr(H264_MB_TYPE name_of_mb_type) {
  switch (name_of_mb_type) {
  case MB_TYPE_NA: {
    return "MB_TYPE_NA";
    break;
  }
  case I_NxN: {
    return "I_NxN";
    break;
  }
  case I_16x16_0_0_0: {
    return "I_16x16_0_0_0";
    break;
  }
  case I_16x16_1_0_0: {
    return "I_16x16_1_0_0";
    break;
  }
  case I_16x16_2_0_0: {
    return "I_16x16_2_0_0";
    break;
  }
  case I_16x16_3_0_0: {
    return "I_16x16_3_0_0";
    break;
  }
  case I_16x16_0_1_0: {
    return "I_16x16_0_1_0";
    break;
  }
  case I_16x16_1_1_0: {
    return "I_16x16_1_1_0";
    break;
  }
  case I_16x16_2_1_0: {
    return "I_16x16_2_1_0";
    break;
  }
  case I_16x16_3_1_0: {
    return "I_16x16_3_1_0";
    break;
  }
  case I_16x16_0_2_0: {
    return "I_16x16_0_2_0";
    break;
  }
  case I_16x16_1_2_0: {
    return "I_16x16_1_2_0";
    break;
  }
  case I_16x16_2_2_0: {
    return "I_16x16_2_2_0";
    break;
  }
  case I_16x16_3_2_0: {
    return "I_16x16_3_2_0";
    break;
  }
  case I_16x16_0_0_1: {
    return "I_16x16_0_0_1";
    break;
  }
  case I_16x16_1_0_1: {
    return "I_16x16_1_0_1";
    break;
  }
  case I_16x16_2_0_1: {
    return "I_16x16_2_0_1";
    break;
  }
  case I_16x16_3_0_1: {
    return "I_16x16_3_0_1";
    break;
  }
  case I_16x16_0_1_1: {
    return "I_16x16_0_1_1";
    break;
  }
  case I_16x16_1_1_1: {
    return "I_16x16_1_1_1";
    break;
  }
  case I_16x16_2_1_1: {
    return "I_16x16_2_1_1";
    break;
  }
  case I_16x16_3_1_1: {
    return "I_16x16_3_1_1";
    break;
  }
  case I_16x16_0_2_1: {
    return "I_16x16_0_2_1";
    break;
  }
  case I_16x16_1_2_1: {
    return "I_16x16_1_2_1";
    break;
  }
  case I_16x16_2_2_1: {
    return "I_16x16_2_2_1";
    break;
  }
  case I_16x16_3_2_1: {
    return "I_16x16_3_2_1";
    break;
  }
  case I_PCM: {
    return "I_PCM";
    break;
  }
  case SI: {
    return "SI";
    break;
  }
  case P_L0_16x16: {
    return "P_L0_16x16";
    break;
  }
  case P_L0_L0_16x8: {
    return "P_L0_L0_16x8";
    break;
  }
  case P_L0_L0_8x16: {
    return "P_L0_L0_8x16";
    break;
  }
  case P_8x8: {
    return "P_8x8";
    break;
  }
  case P_8x8ref0: {
    return "P_8x8ref0";
    break;
  }
  case P_Skip: {
    return "P_Skip";
    break;
  }
  case B_Direct_16x16: {
    return "B_Direct_16x16";
    break;
  }
  case B_L0_16x16: {
    return "B_L0_16x16";
    break;
  }
  case B_L1_16x16: {
    return "B_L1_16x16";
    break;
  }
  case B_Bi_16x16: {
    return "B_Bi_16x16";
    break;
  }
  case B_L0_L0_16x8: {
    return "B_L0_L0_16x8";
    break;
  }
  case B_L0_L0_8x16: {
    return "B_L0_L0_8x16";
    break;
  }
  case B_L1_L1_16x8: {
    return "B_L1_L1_16x8";
    break;
  }
  case B_L1_L1_8x16: {
    return "B_L1_L1_8x16";
    break;
  }
  case B_L0_L1_16x8: {
    return "B_L0_L1_16x8";
    break;
  }
  case B_L0_L1_8x16: {
    return "B_L0_L1_8x16";
    break;
  }
  case B_L1_L0_16x8: {
    return "B_L1_L0_16x8";
    break;
  }
  case B_L1_L0_8x16: {
    return "B_L1_L0_8x16";
    break;
  }
  case B_L0_Bi_16x8: {
    return "B_L0_Bi_16x8";
    break;
  }
  case B_L0_Bi_8x16: {
    return "B_L0_Bi_8x16";
    break;
  }
  case B_L1_Bi_16x8: {
    return "B_L1_Bi_16x8";
    break;
  }
  case B_L1_Bi_8x16: {
    return "B_L1_Bi_8x16";
    break;
  }
  case B_Bi_L0_16x8: {
    return "B_Bi_L0_16x8";
    break;
  }
  case B_Bi_L0_8x16: {
    return "B_Bi_L0_8x16";
    break;
  }
  case B_Bi_L1_16x8: {
    return "B_Bi_L1_16x8";
    break;
  }
  case B_Bi_L1_8x16: {
    return "B_Bi_L1_8x16";
    break;
  }
  case B_Bi_Bi_16x8: {
    return "B_Bi_Bi_16x8";
    break;
  }
  case B_Bi_Bi_8x16: {
    return "B_Bi_Bi_8x16";
    break;
  }
  case B_8x8: {
    return "B_8x8";
    break;
  }
  case B_Skip: {
    return "B_Skip";
    break;
  }
  case P_L0_8x8: {
    return "P_L0_8x8";
    break;
  }
  case P_L0_8x4: {
    return "P_L0_8x4";
    break;
  }
  case P_L0_4x8: {
    return "P_L0_4x8";
    break;
  }
  case P_L0_4x4: {
    return "P_L0_4x4";
    break;
  }
  case B_Direct_8x8: {
    return "B_Direct_8x8";
    break;
  }
  case B_L0_8x8: {
    return "B_L0_8x8";
    break;
  }
  case B_L1_8x8: {
    return "B_L1_8x8";
    break;
  }
  case B_Bi_8x8: {
    return "B_Bi_8x8";
    break;
  }
  case B_L0_8x4: {
    return "B_L0_8x4";
    break;
  }
  case B_L0_4x8: {
    return "B_L0_4x8";
    break;
  }
  case B_L1_8x4: {
    return "B_L1_8x4";
    break;
  }
  case B_L1_4x8: {
    return "B_L1_4x8";
    break;
  }
  case B_Bi_8x4: {
    return "B_Bi_8x4";
    break;
  }
  case B_Bi_4x8: {
    return "B_Bi_4x8";
    break;
  }
  case B_L0_4x4: {
    return "B_L0_4x4";
    break;
  }
  case B_L1_4x4: {
    return "B_L1_4x4";
    break;
  }
  case B_Bi_4x4: {
    return "B_Bi_4x4";
    break;
  }
  default: {
    return "MB_TYPE_NA";
    break;
  }
  }

  return "MB_TYPE_NA";
}

int MacroBlock::fix_mb_type(const int32_t slice_type_raw,
                            const int32_t mb_type_raw,
                            int32_t &slice_type_fixed, int32_t &mb_type_fixed) {
  slice_type_fixed = slice_type_raw;
  mb_type_fixed = mb_type_raw;

  if ((slice_type_raw % 5) == SLICE_I) {
    // 不需要修正
  } else if ((slice_type_raw % 5) == SLICE_SI) {
    // The macroblock types for SI slices are specified in Tables 7-12 and 7-11.
    // The mb_type value 0 is specified in Table 7-12 and the mb_type values 1
    // to 26 are specified in Table 7-11, indexed by subtracting 1 from the
    // value of mb_type.
    if (mb_type_raw == 0) {
      // 不需要修正
    } else if (mb_type_raw >= 1 && mb_type_raw <= 26) {
      slice_type_fixed = SLICE_I;
      mb_type_fixed = mb_type_raw - 1; // 说明 SI slices 中含有I宏块
    } else {
      printf("SI slices: mb_type_raw=%d; Must be in [0..26]\n", mb_type_raw);
      return -1;
    }
  } else if ((slice_type_raw % 5) == SLICE_P ||
             (slice_type_raw % 5) == SLICE_SP) {
    // The macroblock types for P and SP slices are specified in Tables 7-13 and
    // 7-11. mb_type values 0 to 4 are specified in Table 7-13 and mb_type
    // values 5 to 30 are specified in Table 7-11, indexed by subtracting 5 from
    // the value of mb_type.
    if (mb_type_raw >= 0 && mb_type_raw <= 4) {
      // 不需要修正
    } else if (mb_type_raw >= 5 && mb_type_raw <= 30) {
      slice_type_fixed = SLICE_I;
      mb_type_fixed = mb_type_raw - 5; // 说明 P and SP slices 中含有I宏块
    } else {
      printf("P and SP slices: mb_type_raw=%d; Must be in [0..30]\n",
             mb_type_raw);
      return -1;
    }
  } else if ((slice_type_raw % 5) == SLICE_B) {
    // The macroblock types for B slices are specified in Tables 7-14 and 7-11.
    // The mb_type values 0 to 22 are specified in Table 7-14 and the mb_type
    // values 23 to 48 are specified in Table 7-11, indexed by subtracting 23
    // from the value of mb_type.
    if (mb_type_raw >= 0 && mb_type_raw <= 22) {
      // 不需要修正
    } else if (mb_type_raw >= 23 && mb_type_raw <= 48) {
      slice_type_fixed = SLICE_I;
      mb_type_fixed = mb_type_raw - 23; // 说明 B slices 中含有I宏块
    } else {
      printf("B slices: mb_type_raw=%d; Must be in [0..48]\n", mb_type_raw);
      return -1;
    }
  }

  return 0;
}

int MacroBlock::MbPartPredMode(
    int32_t slice_type, int32_t transform_size_8x8_flag, int32_t _mb_type,
    int32_t index, int32_t &NumMbPart, int32_t &CodedBlockPatternChroma,
    int32_t &CodedBlockPatternLuma, int32_t &_Intra16x16PredMode,
    H264_MB_TYPE &name_of_mb_type, H264_MB_PART_PRED_MODE &mb_pred_mode) {
  int ret = 0;

  if ((slice_type % 5) == SLICE_I) {
    if (_mb_type == 0) {
      if (transform_size_8x8_flag == 0) {
        name_of_mb_type = mb_type_I_slices_define[0].name_of_mb_type;
        mb_pred_mode = mb_type_I_slices_define[0].MbPartPredMode;
      } else // if (transform_size_8x8_flag == 1)
      {
        name_of_mb_type = mb_type_I_slices_define[1].name_of_mb_type;
        mb_pred_mode = mb_type_I_slices_define[1].MbPartPredMode;
      }
    } else if (_mb_type >= 1 && _mb_type <= 25) {
      name_of_mb_type = mb_type_I_slices_define[_mb_type + 1].name_of_mb_type;
      CodedBlockPatternChroma =
          mb_type_I_slices_define[_mb_type + 1].CodedBlockPatternChroma;
      CodedBlockPatternLuma =
          mb_type_I_slices_define[_mb_type + 1].CodedBlockPatternLuma;
      _Intra16x16PredMode =
          mb_type_I_slices_define[_mb_type + 1].Intra16x16PredMode;
      mb_pred_mode = mb_type_I_slices_define[_mb_type + 1].MbPartPredMode;
    } else {
      printf("mb_type_I_slices_define: _mb_type=%d; Must be in [0..25]\n",
             _mb_type);
      return -1;
    }
  } else if ((slice_type % 5) == SLICE_SI) {
    if (_mb_type == 0) {
      name_of_mb_type = mb_type_SI_slices_define[0].name_of_mb_type;
      mb_pred_mode = mb_type_SI_slices_define[0].MbPartPredMode;
    } else {
      printf("mb_type_SI_slices_define: _mb_type=%d; Must be in [0..0]\n",
             _mb_type);
      return -1;
    }
  } else if ((slice_type % 5) == SLICE_P || (slice_type % 5) == SLICE_SP) {
    if (_mb_type >= 0 && _mb_type <= 5) {
      name_of_mb_type = mb_type_P_SP_slices_define[_mb_type].name_of_mb_type;
      NumMbPart = mb_type_P_SP_slices_define[_mb_type].NumMbPart;
      if (index == 0) {
        mb_pred_mode = mb_type_P_SP_slices_define[_mb_type].MbPartPredMode0;
      } else // if (index == 1)
      {
        mb_pred_mode = mb_type_P_SP_slices_define[_mb_type].MbPartPredMode1;
      }
    } else {
      printf("mb_type_P_SP_slices_define: _mb_type=%d; Must be in [0..5]\n",
             _mb_type);
      return -1;
    }
  } else if ((slice_type % 5) == SLICE_B) {
    if (_mb_type >= 0 && _mb_type <= 23) {
      name_of_mb_type = mb_type_B_slices_define[_mb_type].name_of_mb_type;
      NumMbPart = mb_type_B_slices_define[_mb_type].NumMbPart;
      if (index == 0) {
        mb_pred_mode = mb_type_B_slices_define[_mb_type].MbPartPredMode0;
      } else // if (index == 1)
      {
        mb_pred_mode = mb_type_B_slices_define[_mb_type].MbPartPredMode1;
      }
    } else {
      printf("mb_type_B_slices_define: _mb_type=%d; Must be in [0..23]\n",
             _mb_type);
      return -1;
    }
  } else {
    printf("Unknown slice_type=%d;\n", slice_type);
    return -1;
  }

  return ret;
}

int MacroBlock::MbPartPredMode2(H264_MB_TYPE name_of_mb_type, int32_t mbPartIdx,
                                int32_t transform_size_8x8_flag,
                                H264_MB_PART_PRED_MODE &mb_pred_mode) {
  int ret = 0;

  if (name_of_mb_type == I_NxN) {
    if (mbPartIdx == 0) {
      if (transform_size_8x8_flag == 0) {
        mb_pred_mode = mb_type_I_slices_define[0].MbPartPredMode;
      } else // if (transform_size_8x8_flag == 1)
      {
        mb_pred_mode = mb_type_I_slices_define[1].MbPartPredMode;
      }
    } else {
      printf("mbPartIdx=%d; Must be in [0..0]\n", mbPartIdx);
      return -1;
    }
  } else if (name_of_mb_type >= I_16x16_0_0_0 &&
             name_of_mb_type <= I_16x16_3_2_1) {
    if (mbPartIdx == 0) {
      mb_pred_mode =
          mb_type_I_slices_define[mbPartIdx - I_16x16_0_0_0].MbPartPredMode;
    } else {
      printf("mbPartIdx=%d; Must be in [0..0]\n", mbPartIdx);
      return -1;
    }
  }

  else if (name_of_mb_type == SI) {
    if (mbPartIdx == 0) {
      mb_pred_mode = mb_type_SI_slices_define[0].MbPartPredMode;
    } else {
      printf("mbPartIdx=%d; Must be in [0..0]\n", mbPartIdx);
      return -1;
    }
  }

  else if (name_of_mb_type == P_L0_16x16 || name_of_mb_type == P_Skip) {
    if (mbPartIdx == 0) {
      mb_pred_mode = Pred_L0;
    } else {
      printf("mbPartIdx=%d; Must be in [0..0]\n", mbPartIdx);
      return -1;
    }
  } else if (name_of_mb_type >= P_L0_L0_16x8 &&
             name_of_mb_type <= P_L0_L0_8x16) {
    if (mbPartIdx == 0) {
      mb_pred_mode = mb_type_P_SP_slices_define[name_of_mb_type - P_L0_16x16]
                         .MbPartPredMode0;
    } else if (mbPartIdx == 1) {
      mb_pred_mode = mb_type_P_SP_slices_define[name_of_mb_type - P_L0_16x16]
                         .MbPartPredMode1;
    } else {
      printf("mbPartIdx=%d; Must be in [0..1]\n", mbPartIdx);
      return -1;
    }
  } else if (name_of_mb_type >= P_8x8 && name_of_mb_type <= P_8x8ref0) {
    if (mbPartIdx >= 0 && mbPartIdx <= 3) {
      mb_pred_mode = Pred_L0;
    } else {
      printf("mbPartIdx=%d; Must be in [0..3]\n", mbPartIdx);
      return -1;
    }
  }

  else if (name_of_mb_type == B_L0_16x16) {
    if (mbPartIdx == 0) {
      mb_pred_mode = Pred_L0;
    } else {
      printf("mbPartIdx=%d; Must be in [0..0]\n", mbPartIdx);
      return -1;
    }
  } else if (name_of_mb_type == B_L1_16x16) {
    if (mbPartIdx == 0) {
      mb_pred_mode = Pred_L1;
    } else {
      printf("mbPartIdx=%d; Must be in [0..0]\n", mbPartIdx);
      return -1;
    }
  } else if (name_of_mb_type == B_Bi_16x16) {
    if (mbPartIdx == 0) {
      mb_pred_mode = BiPred;
    } else {
      printf("mbPartIdx=%d; Must be in [0..0]\n", mbPartIdx);
      return -1;
    }
  } else if (name_of_mb_type >= B_Direct_16x16 && name_of_mb_type <= B_Skip) {
    if (mbPartIdx == 0) {
      mb_pred_mode = mb_type_B_slices_define[name_of_mb_type - B_Direct_16x16]
                         .MbPartPredMode0;
    } else if (mbPartIdx == 1) {
      mb_pred_mode = mb_type_B_slices_define[name_of_mb_type - B_Direct_16x16]
                         .MbPartPredMode1;
    } else if (mbPartIdx == 2 || mbPartIdx == 3) {
      mb_pred_mode = MB_PRED_MODE_NA;
    } else {
      printf("mbPartIdx=%d; Must be in [0..3]\n", mbPartIdx);
      return -1;
    }
  } else {
    printf("Invaild value: name_of_mb_type=%d;\n", name_of_mb_type);
    return -1;
  }

  return ret;
}

int MacroBlock::set_mb_type_X_slice_info() {
  // int32_t mbPartIdx = 0;

  if ((m_slice_type_fixed % 5) == SLICE_I) {
    if (m_mb_type_fixed == 0) {
      if (transform_size_8x8_flag == 0) {
        mb_type_I_slice = mb_type_I_slices_define[0];
      } else // if (transform_size_8x8_flag == 1)
      {
        mb_type_I_slice = mb_type_I_slices_define[1];
      }
    } else if (m_mb_type_fixed >= 1 && m_mb_type_fixed <= 25) {
      mb_type_I_slice = mb_type_I_slices_define[m_mb_type_fixed + 1];
    } else {
      printf(
          "mb_type_I_slices_define: m_mb_type_fixed=%d; Must be in [0..25]\n",
          m_mb_type_fixed);
      return -1;
    }
  } else if ((m_slice_type_fixed % 5) == SLICE_SI) {
    if (m_mb_type_fixed == 0) {
      mb_type_SI_slice = mb_type_SI_slices_define[0];
    } else {
      printf(
          "mb_type_SI_slices_define: m_mb_type_fixed=%d; Must be in [0..0]\n",
          m_mb_type_fixed);
      return -1;
    }
  } else if ((m_slice_type_fixed % 5) == SLICE_P ||
             (m_slice_type_fixed % 5) == SLICE_SP) {
    if (m_mb_type_fixed >= 0 && m_mb_type_fixed <= 5) {
      mb_type_P_SP_slice = mb_type_P_SP_slices_define[m_mb_type_fixed];
    } else {
      printf(
          "mb_type_P_SP_slices_define: m_mb_type_fixed=%d; Must be in [0..5]\n",
          m_mb_type_fixed);
      return -1;
    }

    MbPartWidth = mb_type_P_SP_slice.MbPartWidth;
    MbPartHeight = mb_type_P_SP_slice.MbPartHeight;
    m_NumMbPart = mb_type_P_SP_slice.NumMbPart;

    //---------------------------------------------
    // for (mbPartIdx = 0; mbPartIdx < m_NumMbPart; mbPartIdx++)
    //{
    // sub_mb_type_P_slice[mbPartIdx] = sub_mb_type_P_mbs_define[sub_mb_type[
    // mbPartIdx ]]; NumSubMbPart[mbPartIdx] =
    // sub_mb_type_P_mbs_define[sub_mb_type[ mbPartIdx ]].NumSubMbPart;
    // SubMbPartWidth[mbPartIdx] = sub_mb_type_P_mbs_define[sub_mb_type[
    // mbPartIdx ]].SubMbPartWidth; SubMbPartHeight[mbPartIdx] =
    // sub_mb_type_P_mbs_define[sub_mb_type[ mbPartIdx ]].SubMbPartHeight;
    //}
  } else if ((m_slice_type_fixed % 5) == SLICE_B) {
    if (m_mb_type_fixed >= 0 && m_mb_type_fixed <= 23) {
      mb_type_B_slice = mb_type_B_slices_define[m_mb_type_fixed];
    } else {
      printf(
          "mb_type_B_slices_define: m_mb_type_fixed=%d; Must be in [0..23]\n",
          m_mb_type_fixed);
      return -1;
    }

    MbPartWidth = mb_type_B_slice.MbPartWidth;
    MbPartHeight = mb_type_B_slice.MbPartHeight;
    m_NumMbPart = mb_type_B_slice.NumMbPart;

    //---------------------------------------------
    // for (mbPartIdx = 0; mbPartIdx < m_NumMbPart; mbPartIdx++)
    //{
    // sub_mb_type_B_slice[mbPartIdx] = sub_mb_type_B_mbs_define[sub_mb_type[
    // mbPartIdx ]]; NumSubMbPart[mbPartIdx] =
    // sub_mb_type_B_mbs_define[sub_mb_type[ mbPartIdx ]].NumSubMbPart;
    // SubMbPartWidth[mbPartIdx] = sub_mb_type_B_mbs_define[sub_mb_type[
    // mbPartIdx ]].SubMbPartWidth; SubMbPartHeight[mbPartIdx] =
    // sub_mb_type_B_mbs_define[sub_mb_type[ mbPartIdx ]].SubMbPartHeight;
    //}
  } else {
    printf("Unknown mb_type=%d; m_mb_type_fixed=%d;\n", mb_type,
           m_mb_type_fixed);
    return -1;
  }

  return 0;
}

void MacroBlock::initFromSlice(SliceHeader &header,
                               const SliceBody &slice_data) {
  field_pic_flag = header.field_pic_flag;
  bottom_field_flag = header.bottom_field_flag;
  mb_skip_flag = slice_data.mb_skip_flag;
  mb_field_decoding_flag = slice_data.mb_field_decoding_flag;
  MbaffFrameFlag = header.MbaffFrameFlag;
  disable_deblocking_filter_idc = header.disable_deblocking_filter_idc;
  CurrMbAddr = slice_data.CurrMbAddr;
  slice_id = slice_data.slice_id;
  slice_number = slice_data.slice_number;
  m_slice_type = header.slice_type;
  FilterOffsetA = header.FilterOffsetA;
  FilterOffsetB = header.FilterOffsetB;
}

int MacroBlock::process_decode_mb_type(PictureBase &picture,
                                       SliceHeader &header,
                                       const int32_t slice_type) {
  int ret = picture.Inverse_macroblock_scanning_process(
      header.MbaffFrameFlag, CurrMbAddr, mb_field_decoding_flag,
      picture.m_mbs[CurrMbAddr].m_mb_position_x,
      picture.m_mbs[CurrMbAddr].m_mb_position_y);
  RETURN_IF_FAILED(ret != 0, ret);

  if (_is_cabac)
    // ae(v) 表示CABAC编码
    ret = _cabac->decode_mb_type(mb_type);
  else
    // ue(v) 表示CAVLC编码
    mb_type = _gb->get_ue_golomb(*_bs);

  RETURN_IF_FAILED(ret != 0, ret);

  ret = fix_mb_type(slice_type, mb_type, m_slice_type_fixed,
                    m_mb_type_fixed); // 需要立即修正mb_type的值
  RETURN_IF_FAILED(ret != 0, ret);

  // 因CABAC会用到MbPartWidth/MbPartHeight信息，所以需要尽可能提前设置相关值
  ret = set_mb_type_X_slice_info();
  RETURN_IF_FAILED(ret != 0, ret);

  ret = MbPartPredMode(m_slice_type_fixed, transform_size_8x8_flag,
                       m_mb_type_fixed, 0, m_NumMbPart, CodedBlockPatternChroma,
                       CodedBlockPatternLuma, Intra16x16PredMode,
                       m_name_of_mb_type, m_mb_pred_mode);
  RETURN_IF_FAILED(ret != 0, ret);
  return 0;
}

int MacroBlock::process_transform_size_8x8_flag(
    int32_t &transform_size_8x8_flag_temp) {

  int ret;
  if (_is_cabac) {
    ret = _cabac->decode_transform_size_8x8_flag(transform_size_8x8_flag_temp);
    RETURN_IF_FAILED(ret != 0, ret);
  } else
    transform_size_8x8_flag_temp = _bs->readUn(1);

  if (transform_size_8x8_flag_temp != transform_size_8x8_flag) {
    transform_size_8x8_flag = transform_size_8x8_flag_temp;
    ret = MbPartPredMode(m_slice_type_fixed, transform_size_8x8_flag,
                         m_mb_type_fixed, 0, m_NumMbPart,
                         CodedBlockPatternChroma, CodedBlockPatternLuma,
                         Intra16x16PredMode, m_name_of_mb_type, m_mb_pred_mode);
    RETURN_IF_FAILED(ret != 0, ret);

    if ((m_slice_type_fixed % 5) == SLICE_I && m_mb_type_fixed == 0)
      mb_type_I_slice =
          mb_type_I_slices_define[(transform_size_8x8_flag == 0) ? 0 : 1];
  }

  return 0;
}

int MacroBlock::process_coded_block_pattern(const uint32_t ChromaArrayType) {
  if (_is_cabac) {
    int ret = _cabac->decode_coded_block_pattern(coded_block_pattern);
    RETURN_IF_FAILED(ret != 0, ret);
  } else
    coded_block_pattern =
        _gb->get_me_golomb(*_bs, ChromaArrayType, m_mb_pred_mode);

  CodedBlockPatternLuma = coded_block_pattern % 16;
  CodedBlockPatternChroma = coded_block_pattern / 16;
  return 0;
}

int MacroBlock::process_mb_qp_delta() {
  int ret;
  if (_is_cabac) {
    ret = _cabac->decode_mb_qp_delta(mb_qp_delta); // 2 se(v) | ae(v)
    RETURN_IF_FAILED(ret != 0, ret);
  } else
    mb_qp_delta = _gb->get_se_golomb(*_bs); // 2 se(v) | ae(v)
  return 0;
}

int MacroBlock::process_prev_intra4x4_pred_mode_flag(const int luma4x4BlkIdx) {
  int ret;
  if (_is_cabac) {
    ret =
        _cabac
            ->decode_prev_intra4x4_pred_mode_flag_or_prev_intra8x8_pred_mode_flag(
                prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]);
    RETURN_IF_FAILED(ret != 0, ret);
  } else
    prev_intra4x4_pred_mode_flag[luma4x4BlkIdx] = _bs->readUn(1);
  return 0;
}

int MacroBlock::process_rem_intra4x4_pred_mode(const int luma4x4BlkIdx) {
  int ret;
  if (_is_cabac) {
    ret = _cabac->decode_rem_intra4x4_pred_mode_or_rem_intra8x8_pred_mode(
        rem_intra4x4_pred_mode[luma4x4BlkIdx]);
    RETURN_IF_FAILED(ret != 0, ret);
  } else
    rem_intra4x4_pred_mode[luma4x4BlkIdx] = _bs->readUn(3);
  return 0;
}

int MacroBlock::process_prev_intra8x8_pred_mode_flag(const int luma8x8BlkIdx) {

  int ret;
  if (_is_cabac) {
    ret =
        _cabac
            ->decode_prev_intra4x4_pred_mode_flag_or_prev_intra8x8_pred_mode_flag(
                prev_intra8x8_pred_mode_flag[luma8x8BlkIdx]);

    RETURN_IF_FAILED(ret != 0, ret);
  } else
    prev_intra8x8_pred_mode_flag[luma8x8BlkIdx] = _bs->readUn(1);

  return 0;
}
int MacroBlock::process_rem_intra8x8_pred_mode(const int luma8x8BlkIdx) {
  int ret;
  if (_is_cabac) {
    ret = _cabac->decode_rem_intra4x4_pred_mode_or_rem_intra8x8_pred_mode(
        rem_intra8x8_pred_mode[luma8x8BlkIdx]);
    RETURN_IF_FAILED(ret != 0, ret);
  } else
    rem_intra8x8_pred_mode[luma8x8BlkIdx] = _bs->readUn(3);
  return 0;
}

int MacroBlock::process_intra_chroma_pred_mode() {
  int ret;
  if (_is_cabac) {
    ret = _cabac->decode_intra_chroma_pred_mode(intra_chroma_pred_mode);
    RETURN_IF_FAILED(ret != 0, ret);
  } else
    intra_chroma_pred_mode = _gb->get_ue_golomb(*_bs);
  return 0;
}

int MacroBlock::process_ref_idx_l0(const int mbPartIdx,
                                   const int32_t RefPicList0Length,
                                   const int32_t mb_field_decoding_flag) {
  int ret;
  if (_is_cabac) {
    int32_t ref_idx_flag = 0;

    ret = _cabac->decode_ref_idx_lX(ref_idx_flag, mbPartIdx,
                                    ref_idx_l0[mbPartIdx]);
    RETURN_IF_FAILED(ret != 0, ret);
  } else {
    int range = RefPicList0Length - 1;

    if (mb_field_decoding_flag == 1) {
      // 注意：此处是个坑，T-REC-H.264-201704-S!!PDF-E.pdf
      // 文档中并未明确写出来
      range = RefPicList0Length * 2 - 1;
    }

    ref_idx_l0[mbPartIdx] = _gb->get_te_golomb(*_bs, range);
  }
  return 0;
}

int MacroBlock::process_ref_idx_l1(const int mbPartIdx,
                                   const int32_t RefPicList1Length,
                                   const int32_t mb_field_decoding_flag) {
  int ret;
  if (_is_cabac) {
    int32_t ref_idx_flag = 1;

    ret = _cabac->decode_ref_idx_lX(ref_idx_flag, mbPartIdx,
                                    ref_idx_l1[mbPartIdx]);
    RETURN_IF_FAILED(ret != 0, ret);
  } else {
    int range = RefPicList1Length - 1;

    if (mb_field_decoding_flag == 1) {
      // 注意：此处是个坑，T-REC-H.264-201704-S!!PDF-E.pdf
      // 文档中并未明确写出来
      range = RefPicList1Length * 2 - 1;
    }

    ref_idx_l1[mbPartIdx] = _gb->get_te_golomb(*_bs, range);
  }

  return 0;
}

int MacroBlock::process_mvd_l0(const int mbPartIdx, const int compIdx) {
  int ret;
  if (_is_cabac) {
    int32_t mvd_flag = compIdx;
    int32_t subMbPartIdx = 0;
    int32_t isChroma = 0;

    ret = _cabac->decode_mvd_lX(mvd_flag, mbPartIdx, subMbPartIdx, isChroma,
                                mvd_l0[mbPartIdx][0][compIdx]);
    RETURN_IF_FAILED(ret != 0, ret);
  } else
    mvd_l0[mbPartIdx][0][compIdx] = _gb->get_se_golomb(*_bs);
  return 0;
}

int MacroBlock::process_mvd_l1(const int mbPartIdx, const int compIdx) {
  int ret;
  if (_is_cabac) {
    int32_t mvd_flag = 2 + compIdx;
    int32_t subMbPartIdx = 0;
    int32_t isChroma = 0;

    ret = _cabac->decode_mvd_lX(mvd_flag, mbPartIdx, subMbPartIdx, isChroma,
                                mvd_l1[mbPartIdx][0][compIdx]);
    RETURN_IF_FAILED(ret != 0, ret);
  } else
    mvd_l1[mbPartIdx][0][compIdx] = _gb->get_se_golomb(*_bs);
  return 0;
}

/*
 * Page 59/81/812
 * 7.3.5.3 Residual data syntax
 */
int MacroBlock::residual(PictureBase &picture, int32_t startIdx,
                         int32_t endIdx) {
  int ret = 0;
  int32_t i = 0;
  int32_t iCbCr = 0;
  int32_t i8x8 = 0;
  int32_t i4x4 = 0;
  int32_t BlkIdx = 0;
  int32_t TotalCoeff = 0; // 该 4x4 block的残差中，总共有多少个非零系数
  CH264ResidualBlockCavlc cavlc;

  int is_ae =
      picture.m_slice.m_pps.entropy_coding_mode_flag; // ae(v)表示CABAC编码

  //    if (!picture.m_slice.m_pps.entropy_coding_mode_flag)
  //    {
  //        residual_block = residual_block_cavlc;
  //    }
  //    else
  //    {
  //        residual_block = residual_block_cabac;
  //    }

  ret = residual_luma(picture, i16x16DClevel, i16x16AClevel, level4x4, level8x8,
                      startIdx, endIdx, MB_RESIDUAL_Intra16x16DCLevel,
                      MB_RESIDUAL_Intra16x16ACLevel); // 3 | 4
  RETURN_IF_FAILED(ret != 0, ret);

  memcpy(Intra16x16DCLevel, i16x16DClevel, sizeof(int32_t) * 16);
  memcpy(Intra16x16ACLevel, i16x16AClevel, sizeof(int32_t) * 16 * 16);
  memcpy(LumaLevel4x4, level4x4, sizeof(int32_t) * 16 * 16);
  memcpy(LumaLevel8x8, level8x8, sizeof(int32_t) * 4 * 64);

  //-----------------------
  if (picture.m_slice.m_sps.ChromaArrayType == 1 ||
      picture.m_slice.m_sps.ChromaArrayType == 2) {
    int32_t NumC8x8 = 4 / (picture.m_slice.m_sps.SubWidthC *
                           picture.m_slice.m_sps.SubHeightC);
    for (iCbCr = 0; iCbCr < 2; iCbCr++) {
      if ((CodedBlockPatternChroma & 3) &&
          startIdx == 0) // chroma DC residual present
      {
        BlkIdx = 0;
        TotalCoeff = 0;

        if (is_ae) // ae(v) 表示CABAC编码
        {
          ret = _cabac->residual_block_cabac(
              ChromaDCLevel[iCbCr], 0, 4 * NumC8x8 - 1, 4 * NumC8x8,
              MB_RESIDUAL_ChromaDCLevel, BlkIdx, iCbCr,
              TotalCoeff); // 3 | 4
          RETURN_IF_FAILED(ret != 0, ret);
        } else // ue(v) 表示CAVLC编码
        {
          MB_RESIDUAL_LEVEL mb_level = (iCbCr == 0)
                                           ? MB_RESIDUAL_ChromaDCLevelCb
                                           : MB_RESIDUAL_ChromaDCLevelCr;

          ret = cavlc.residual_block_cavlc(picture, *_bs, ChromaDCLevel[iCbCr],
                                           0, 4 * NumC8x8 - 1, 4 * NumC8x8,
                                           mb_level, m_mb_pred_mode, BlkIdx,
                                           TotalCoeff); // 3 | 4
          RETURN_IF_FAILED(ret != 0, ret);
        }

        mb_chroma_4x4_non_zero_count_coeff[iCbCr][BlkIdx] = TotalCoeff;
      } else {
        for (i = 0; i < 4 * NumC8x8; i++) {
          ChromaDCLevel[iCbCr][i] = 0;
        }
      }
    }

    for (iCbCr = 0; iCbCr < 2; iCbCr++) {
      for (i8x8 = 0; i8x8 < NumC8x8; i8x8++) {
        for (i4x4 = 0; i4x4 < 4; i4x4++) {
          if (CodedBlockPatternChroma & 2) // chroma AC residual present
          {
            BlkIdx = i8x8 * 4 + i4x4;
            TotalCoeff = 0;

            if (is_ae) // ae(v) 表示CABAC编码
            {
              ret = _cabac->residual_block_cabac(
                  ChromaACLevel[iCbCr][i8x8 * 4 + i4x4], MAX(0, startIdx - 1),
                  endIdx - 1, 15, MB_RESIDUAL_ChromaACLevel, BlkIdx, iCbCr,
                  TotalCoeff); // 3 | 4
              RETURN_IF_FAILED(ret != 0, ret);
            } else // ue(v) 表示CAVLC编码
            {
              MB_RESIDUAL_LEVEL mb_level = (iCbCr == 0)
                                               ? MB_RESIDUAL_ChromaACLevelCb
                                               : MB_RESIDUAL_ChromaACLevelCr;

              ret = cavlc.residual_block_cavlc(
                  picture, *_bs, ChromaACLevel[iCbCr][i8x8 * 4 + i4x4],
                  MAX(0, startIdx - 1), endIdx - 1, 15, mb_level,
                  m_mb_pred_mode, BlkIdx, TotalCoeff); // 3 | 4
              RETURN_IF_FAILED(ret != 0, ret);
            }

            mb_chroma_4x4_non_zero_count_coeff[iCbCr][BlkIdx] = TotalCoeff;
          } else {
            for (i = 0; i < 15; i++) {
              ChromaACLevel[iCbCr][i8x8 * 4 + i4x4][i] = 0;
            }
          }
        }
      }
    }
  } else if (picture.m_slice.m_sps.ChromaArrayType == 3) {
    ret =
        residual_luma(picture, i16x16DClevel, i16x16AClevel, level4x4, level8x8,
                      startIdx, endIdx, MB_RESIDUAL_CbIntra16x16DCLevel,
                      MB_RESIDUAL_CbIntra16x16ACLevel); // 3 | 4
    RETURN_IF_FAILED(ret != 0, ret);

    memcpy(CbIntra16x16DCLevel, i16x16DClevel, sizeof(int32_t) * 16);
    memcpy(CbIntra16x16ACLevel, i16x16AClevel, sizeof(int32_t) * 16 * 16);
    memcpy(CbLevel4x4, level4x4, sizeof(int32_t) * 16 * 16);
    memcpy(CbLevel8x8, level8x8, sizeof(int32_t) * 4 * 64);

    ret =
        residual_luma(picture, i16x16DClevel, i16x16AClevel, level4x4, level8x8,
                      startIdx, endIdx, MB_RESIDUAL_CrIntra16x16DCLevel,
                      MB_RESIDUAL_CrIntra16x16ACLevel); // 3 | 4
    RETURN_IF_FAILED(ret != 0, ret);

    memcpy(CrIntra16x16DCLevel, i16x16DClevel, sizeof(int32_t) * 16);
    memcpy(CrIntra16x16ACLevel, i16x16AClevel, sizeof(int32_t) * 16 * 16);
    memcpy(CrLevel4x4, level4x4, sizeof(int32_t) * 16 * 16);
    memcpy(CrLevel8x8, level8x8, sizeof(int32_t) * 4 * 64);
  }

  return 0;
}

/*
 * Page 60/82/812
 * 7.3.5.3.1 Residual luma syntax
 */
int MacroBlock::residual_luma(PictureBase &picture,
                              int32_t (&i16x16DClevel)[16],
                              int32_t (&i16x16AClevel)[16][16],
                              int32_t (&level4x4)[16][16],
                              int32_t (&level8x8)[4][64], int32_t startIdx,
                              int32_t endIdx,
                              MB_RESIDUAL_LEVEL mb_residual_level_dc,
                              MB_RESIDUAL_LEVEL mb_residual_level_ac) {
  int ret = 0;
  int32_t i = 0;
  int32_t i8x8 = 0;
  int32_t i4x4 = 0;
  int32_t BlkIdx = 0;
  int32_t TotalCoeff = 0; // 该 4x4 block的残差中，总共有多少个非零系数
  // H264_MB_TYPE name_of_mb_type2 = MB_TYPE_NA;
  CH264ResidualBlockCavlc cavlc;

  int is_ae =
      picture.m_slice.m_pps.entropy_coding_mode_flag; // ae(v)表示CABAC编码

  if (startIdx == 0 &&
      m_mb_pred_mode ==
          Intra_16x16) // MbPartPredMode(mb_type, 0) == Intra_16x16)
  {
    BlkIdx = 0;
    TotalCoeff = 0;

    if (is_ae) // ae(v) 表示CABAC编码
    {
      ret = _cabac->residual_block_cabac(i16x16DClevel, 0, 15, 16,
                                         mb_residual_level_dc, BlkIdx, -1,
                                         TotalCoeff); // 3 | 4
      RETURN_IF_FAILED(ret != 0, ret);
    } else // ue(v) 表示CAVLC编码
    {
      ret = cavlc.residual_block_cavlc(picture, *_bs, i16x16DClevel, 0, 15, 16,
                                       mb_residual_level_dc, m_mb_pred_mode,
                                       BlkIdx, TotalCoeff); // 3 | 4
      RETURN_IF_FAILED(ret != 0, ret);
    }

    mb_luma_4x4_non_zero_count_coeff[BlkIdx] = TotalCoeff;
  }

  for (i8x8 = 0; i8x8 < 4; i8x8++) {
    if (!transform_size_8x8_flag ||
        !picture.m_slice.m_pps.entropy_coding_mode_flag) {
      for (i4x4 = 0; i4x4 < 4; i4x4++) {
        if (CodedBlockPatternLuma & (1 << i8x8)) {
          BlkIdx = i8x8 * 4 + i4x4;
          TotalCoeff = 0;

          if (m_mb_pred_mode == Intra_16x16) {
            if (is_ae) // ae(v) 表示CABAC编码
            {
              ret = _cabac->residual_block_cabac(
                  i16x16AClevel[i8x8 * 4 + i4x4], MAX(0, startIdx - 1),
                  endIdx - 1, 15, MB_RESIDUAL_Intra16x16ACLevel, BlkIdx, -1,
                  TotalCoeff); // 3 | 4
              RETURN_IF_FAILED(ret != 0, ret);
            } else // ue(v) 表示CAVLC编码
            {
              ret = cavlc.residual_block_cavlc(
                  picture, *_bs, i16x16AClevel[i8x8 * 4 + i4x4],
                  MAX(0, startIdx - 1), endIdx - 1, 15,
                  MB_RESIDUAL_Intra16x16ACLevel, m_mb_pred_mode, BlkIdx,
                  TotalCoeff); // 3
              RETURN_IF_FAILED(ret != 0, ret);
            }
          } else // Intra_4x4 or Intra_8x8
          {
            if (is_ae) // ae(v) 表示CABAC编码
            {
              ret = _cabac->residual_block_cabac(
                  level4x4[i8x8 * 4 + i4x4], startIdx, endIdx, 16,
                  MB_RESIDUAL_LumaLevel4x4, BlkIdx, -1,
                  TotalCoeff); // 3 | 4
              RETURN_IF_FAILED(ret != 0, ret);
            } else // ue(v) 表示CAVLC编码
            {
              ret = cavlc.residual_block_cavlc(
                  picture, *_bs, level4x4[i8x8 * 4 + i4x4], startIdx, endIdx,
                  16, MB_RESIDUAL_LumaLevel4x4, m_mb_pred_mode, BlkIdx,
                  TotalCoeff); // 3 | 4
              RETURN_IF_FAILED(ret != 0, ret);
            }
          }

          mb_luma_4x4_non_zero_count_coeff[BlkIdx] = TotalCoeff;
          mb_luma_8x8_non_zero_count_coeff[i8x8] +=
              mb_luma_4x4_non_zero_count_coeff[i8x8 * 4 + i4x4];
        } else if (m_mb_pred_mode == Intra_16x16) {
          for (i = 0; i < 15; i++) {
            i16x16AClevel[i8x8 * 4 + i4x4][i] = 0;
          }
        } else {
          for (i = 0; i < 16; i++) {
            level4x4[i8x8 * 4 + i4x4][i] = 0;
          }
        }

        if (!picture.m_slice.m_pps.entropy_coding_mode_flag &&
            transform_size_8x8_flag) {
          for (i = 0; i < 16; i++) {
            level8x8[i8x8][4 * i + i4x4] = level4x4[i8x8 * 4 + i4x4][i];
          }
          mb_luma_8x8_non_zero_count_coeff[i8x8] +=
              mb_luma_4x4_non_zero_count_coeff[i8x8 * 4 + i4x4];
        }
      }
    } else if (CodedBlockPatternLuma & (1 << i8x8)) {
      BlkIdx = i8x8;
      TotalCoeff = 0;

      if (is_ae) // ae(v) 表示CABAC编码
      {
        ret = _cabac->residual_block_cabac(
            level8x8[i8x8], 4 * startIdx, 4 * endIdx + 3, 64,
            MB_RESIDUAL_LumaLevel8x8, BlkIdx, -1, TotalCoeff); // 3 | 4
        RETURN_IF_FAILED(ret != 0, ret);
      } else // ue(v) 表示CAVLC编码
      {
        ret = cavlc.residual_block_cavlc(
            picture, *_bs, level8x8[i8x8], 4 * startIdx, 4 * endIdx + 3, 64,
            MB_RESIDUAL_LumaLevel8x8, m_mb_pred_mode, BlkIdx,
            TotalCoeff); // 3 | 4
        RETURN_IF_FAILED(ret != 0, ret);
      }

      mb_luma_8x8_non_zero_count_coeff[i8x8] = TotalCoeff;
    } else {
      for (i = 0; i < 64; i++) {
        level8x8[i8x8][i] = 0;
      }
    }
  }

  return 0;
}
