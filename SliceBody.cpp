#include "SliceBody.hpp"
#include "CH264Golomb.hpp"
#include "Nalu.hpp"

/* Rec. ITU-T H.264 (08/2021) 56 */
int SliceBody::parseSliceData(BitStream &bs, PictureBase &picture) {
  CH264Cabac cabac;
  /* CABAC编码 */
  if (m_pps.entropy_coding_mode_flag) {
    std::cout << "\tCABAC编码（哥伦布熵编码）" << std::endl;
    while (!bs.byte_aligned())
      cabac_alignment_one_bit = bs.readU1(); // 2 f(1)

    cabac.Initialisation_process_for_context_variables(
        (H264_SLIECE_TYPE)slice_header.slice_type, slice_header.cabac_init_idc,
        slice_header.SliceQPY);
    // cabac初始化环境变量

    cabac.Initialisation_process_for_the_arithmetic_decoding_engine(bs);
    // cabac初始化解码引擎
  }

  if (slice_header.MbaffFrameFlag == 0)
    mb_field_decoding_flag = slice_header.field_pic_flag;

  CurrMbAddr =
      slice_header.first_mb_in_slice * (1 + slice_header.MbaffFrameFlag);

  picture.CurrMbAddr = CurrMbAddr;

  slice_header.picNumL0Pred = slice_header.CurrPicNum;
  slice_header.picNumL1Pred = slice_header.CurrPicNum;

  if (picture.m_slice_cnt == 0) {
    picture.Decoding_process_for_picture_order_count(); // 解码POC
    if (m_sps.frame_mbs_only_flag == 0) {
      // 有可能出现场帧或场宏块
      std::cout << "hi~" << std::endl;
      exit(0);
    }

    //--------参考帧重排序------------
    // 只有当前帧为P帧，B帧时，才会对参考图像数列表组进行重排序
    if (slice_header.slice_type == H264_SLIECE_TYPE_P ||
        slice_header.slice_type == H264_SLIECE_TYPE_SP ||
        slice_header.slice_type == H264_SLIECE_TYPE_B) {
      std::cout << "hi~" << std::endl;
      exit(0);
      picture.Decoding_process_for_reference_picture_lists_construction(
          picture.m_dpb, picture.m_RefPicList0, picture.m_RefPicList1);

      //--------------
      for (int i = 0; i < picture.m_RefPicList0Length; ++i) {
        printf("m_PicNumCnt=%d(%s); PicOrderCnt=%d; m_RefPicList0[%d]: %s; "
               "PicOrderCnt=%d; PicNum=%d; PicNumCnt=%d;\n",
               picture.m_PicNumCnt,
               H264_SLIECE_TYPE_TO_STR(slice_header.slice_type),
               picture.PicOrderCnt, i,
               (picture.m_RefPicList0[i])
                   ? H264_SLIECE_TYPE_TO_STR(
                         picture.m_RefPicList0[i]
                             ->m_picture_frame.m_h264_slice_header.slice_type)
                   : "UNKNOWN",
               (picture.m_RefPicList0[i])
                   ? picture.m_RefPicList0[i]->m_picture_frame.PicOrderCnt
                   : -1,
               (picture.m_RefPicList0[i])
                   ? picture.m_RefPicList0[i]->m_picture_frame.PicNum
                   : -1,
               (picture.m_RefPicList0[i])
                   ? picture.m_RefPicList0[i]->m_picture_frame.m_PicNumCnt
                   : -1);
      }
      for (int i = 0; i < picture.m_RefPicList1Length; ++i) {
        printf("m_PicNumCnt=%d(%s); PicOrderCnt=%d; m_RefPicList1[%d]: %s; "
               "PicOrderCnt=%d; PicNum=%d; PicNumCnt=%d;\n",
               picture.m_PicNumCnt,
               H264_SLIECE_TYPE_TO_STR(slice_header.slice_type),
               picture.PicOrderCnt, i,
               (picture.m_RefPicList1[i])
                   ? H264_SLIECE_TYPE_TO_STR(
                         picture.m_RefPicList1[i]
                             ->m_picture_frame.m_h264_slice_header.slice_type)
                   : "UNKNOWN",
               (picture.m_RefPicList1[i])
                   ? picture.m_RefPicList1[i]->m_picture_frame.PicOrderCnt
                   : -1,
               (picture.m_RefPicList1[i])
                   ? picture.m_RefPicList1[i]->m_picture_frame.PicNum
                   : -1,
               (picture.m_RefPicList1[i])
                   ? picture.m_RefPicList1[i]->m_picture_frame.m_PicNumCnt
                   : -1);
      }
    }
  }

  picture.m_slice_cnt++;

  //-------------------------------
  bool is_need_skip_read_mb_field_decoding_flag = false;

  do {
    if (slice_header.slice_type != SLICE_I &&
        slice_header.slice_type != SLICE_SI) {
      if (!m_pps.entropy_coding_mode_flag) {
        std::cout << "hi~" << std::endl;
        exit(0);
      } else {
        /* CABAC编码开始 */
        picture.mb_x = (CurrMbAddr % (picture.PicWidthInMbs *
                                      (1 + slice_header.MbaffFrameFlag))) /
                       (1 + slice_header.MbaffFrameFlag);

        picture.mb_y =
            (CurrMbAddr /
             (picture.PicWidthInMbs * (1 + slice_header.MbaffFrameFlag)) *
             (1 + slice_header.MbaffFrameFlag)) +
            ((CurrMbAddr %
              (picture.PicWidthInMbs * (1 + slice_header.MbaffFrameFlag))) %
             (1 + slice_header.MbaffFrameFlag));
        picture.CurrMbAddr = CurrMbAddr;

        // //因为解码mb_skip_flag需要事先知道MbaffFrameFlag的值
        picture.m_mbs[picture.CurrMbAddr].slice_number = slice_number;
        // 因为解码mb_skip_flag需要事先知道slice_id的值（从0开始）

        if (slice_header.MbaffFrameFlag) {
          std::cout << "hi~" << std::endl;
          exit(0);
        }

        //-------------解码mb_skip_flag-----------------------
        if (slice_header.MbaffFrameFlag && CurrMbAddr % 2 == 1 &&
            prevMbSkipped) {
          // 如果是bottom field macroblock
          std::cout << "hi~" << std::endl;
          exit(0);
          // mb_skip_flag = mb_skip_flag_next_mb;
        } else
          cabac.CABAC_decode_mb_skip_flag(picture, bs, CurrMbAddr,
                                          mb_skip_flag);

        //------------------------------------
        if (mb_skip_flag == 1) {
          picture.mb_cnt++;
          std::cout << "hi~" << std::endl;
          exit(0);
        }
        moreDataFlag = !mb_skip_flag;
        /* CABAC编码结束 */
      }
    }
    if (moreDataFlag) {
      if (slice_header.MbaffFrameFlag &&
          (CurrMbAddr % 2 == 0 || (CurrMbAddr % 2 == 1 && prevMbSkipped))) {
        /* 表示本宏块是属于一个宏块对中的一个 */
        std::cout << "hi~" << std::endl;
        exit(0);
      }

      picture.mb_x = (CurrMbAddr % (picture.PicWidthInMbs *
                                    (1 + slice_header.MbaffFrameFlag))) /
                     (1 + slice_header.MbaffFrameFlag);
      picture.mb_y =
          (CurrMbAddr /
           (picture.PicWidthInMbs * (1 + slice_header.MbaffFrameFlag)) *
           (1 + slice_header.MbaffFrameFlag)) +
          ((CurrMbAddr %
            (picture.PicWidthInMbs * (1 + slice_header.MbaffFrameFlag))) %
           (1 + slice_header.MbaffFrameFlag));
      picture.CurrMbAddr =
          CurrMbAddr; // picture.mb_x + picture.mb_y * picture.PicWidthInMbs;
      picture.mb_cnt++;

      //--------熵解码------------
      picture.m_mbs[picture.CurrMbAddr].macroblock_layer(bs, picture, *this,
                                                         cabac);
      // 2 | 3 | 4

      //--------帧内/间预测------------
      //--------反量化------------
      //--------反变换------------

      int32_t isChroma = 0;
      int32_t isChromaCb = 0;
      int32_t BitDepth = 0;

      int32_t picWidthInSamplesL = picture.PicWidthInSamplesL;
      int32_t picWidthInSamplesC = picture.PicWidthInSamplesC;

      uint8_t *pic_buff_luma = picture.m_pic_buff_luma;
      uint8_t *pic_buff_cb = picture.m_pic_buff_cb;
      uint8_t *pic_buff_cr = picture.m_pic_buff_cr;
      // 帧内预测
      if (picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode == Intra_4x4) {
        isChroma = 0;
        isChromaCb = 0;
        BitDepth = picture.m_h264_slice_header.m_sps.BitDepthY;

        picture.transform_decoding_process_for_4x4_luma_residual_blocks(
            isChroma, isChromaCb, BitDepth, picWidthInSamplesL, pic_buff_luma);

        isChromaCb = 1;
        picture.transform_decoding_process_for_chroma_samples(
            isChromaCb, picWidthInSamplesC, pic_buff_cb);

        isChromaCb = 0;
        picture.transform_decoding_process_for_chroma_samples(
            isChromaCb, picWidthInSamplesC, pic_buff_cr);

      } else if (picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode ==
                 Intra_8x8) {
        isChroma = 0;
        isChromaCb = 0;
        BitDepth = picture.m_h264_slice_header.m_sps.BitDepthY;

        picture.transform_decoding_process_for_8x8_luma_residual_blocks(
            isChroma, isChromaCb, BitDepth, picWidthInSamplesL,
            picture.m_mbs[picture.CurrMbAddr].LumaLevel8x8, pic_buff_luma);

        isChromaCb = 1;
        picture.transform_decoding_process_for_chroma_samples(
            isChromaCb, picWidthInSamplesC, pic_buff_cb);

        isChromaCb = 0;
        picture.transform_decoding_process_for_chroma_samples(
            isChromaCb, picWidthInSamplesC, pic_buff_cr);

      } else if (picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode ==
                 Intra_16x16) {
        isChroma = 0;
        isChromaCb = 0;
        BitDepth = picture.m_h264_slice_header.m_sps.BitDepthY;
        int32_t QP1 = picture.m_mbs[picture.CurrMbAddr].QP1Y;

        picture
            .transform_decoding_process_for_luma_samples_of_Intra_16x16_macroblock_prediction_mode(
                isChroma, BitDepth, QP1, picWidthInSamplesL,
                picture.m_mbs[picture.CurrMbAddr].Intra16x16DCLevel,
                picture.m_mbs[picture.CurrMbAddr].Intra16x16ACLevel,
                pic_buff_luma);
        isChromaCb = 1;
        picture.transform_decoding_process_for_chroma_samples(
            isChromaCb, picWidthInSamplesC, pic_buff_cb);

        isChromaCb = 0;
        picture.transform_decoding_process_for_chroma_samples(
            isChromaCb, picWidthInSamplesC, pic_buff_cr);
      } else if (picture.m_mbs[picture.CurrMbAddr].m_name_of_mb_type == I_PCM)
        // 说明该宏块没有残差，也没有预测值，码流中的数据直接为原始像素值
        exit(0);
      else
        exit(0);
    }

    if (!m_pps.entropy_coding_mode_flag) {
      // moreDataFlag = bs.more_rbsp_data();
      exit(0);
    } else {
      if (slice_header.slice_type != H264_SLIECE_TYPE_I &&
          slice_header.slice_type != H264_SLIECE_TYPE_SI) {
        // prevMbSkipped = mb_skip_flag;
        exit(0);
      }

      if (slice_header.MbaffFrameFlag && CurrMbAddr % 2 == 0) {
        // moreDataFlag = 1;
        exit(0);
      } else {
        cabac.CABAC_decode_end_of_slice_flag(picture, bs, end_of_slice_flag);
        moreDataFlag = !end_of_slice_flag;
      }
    }
    CurrMbAddr = NextMbAddress(CurrMbAddr);
  } while (moreDataFlag);

  if (picture.mb_cnt == picture.PicSizeInMbs)
    picture.m_is_decode_finished = 1;

  slice_id++;
  slice_number++;
  return 0;
}

int SliceBody::set_mb_skip_flag(int32_t &mb_skip_flag, PictureBase &picture,
                                BitStream &bs) {
  picture.mb_x =
      (CurrMbAddr % (m_sps.PicWidthInMbs * (1 + slice_header.MbaffFrameFlag))) /
      (1 + slice_header.MbaffFrameFlag);
  picture.mb_y =
      (CurrMbAddr / (m_sps.PicWidthInMbs * (1 + slice_header.MbaffFrameFlag)) *
       (1 + slice_header.MbaffFrameFlag)) +
      ((CurrMbAddr %
        (m_sps.PicWidthInMbs * (1 + slice_header.MbaffFrameFlag))) %
       (1 + slice_header.MbaffFrameFlag));
  picture.CurrMbAddr = CurrMbAddr;

  // picture.m_mbs[picture.CurrMbAddr].slice.MbaffFrameFlag =
  // slice.MbaffFrameFlag;
  // //因为解码mb_skip_flag需要事先知道slice.MbaffFrameFlag的值
  picture.m_mbs[picture.CurrMbAddr].slice_number =
      slice_header.slice_number; // 因为解码mb_skip_flag需要事先知道slice_id的值

  if (slice_header.MbaffFrameFlag) {
    if (CurrMbAddr % 2 == 0) // 顶场宏块
    {
      if (picture.mb_x == 0 &&
          picture.mb_y >=
              2) // 注意：此处在T-REC-H.264-201704-S!!PDF-E.pdf文档中，并没有明确写出来，所以这是一个坑
      {
        // When slice.MbaffFrameFlag is equal to 1 and mb_field_decoding_flag is
        // not present for both the top and the bottom macroblock of a
        // macroblock pair
        if (picture.mb_x > 0 &&
            picture.m_mbs[CurrMbAddr - 2].slice_number ==
                slice_header.slice_number) // the left of the current macroblock
                                           // pair in the same slice
        {
          mb_field_decoding_flag =
              picture.m_mbs[CurrMbAddr - 2].mb_field_decoding_flag;
        } else if (picture.mb_y > 0 &&
                   picture.m_mbs[CurrMbAddr - 2 * m_sps.PicWidthInMbs]
                           .slice_number ==
                       slice_header
                           .slice_number) // above the current macroblock pair
                                          // in the same slice
        {
          mb_field_decoding_flag =
              picture.m_mbs[CurrMbAddr - 2 * m_sps.PicWidthInMbs]
                  .mb_field_decoding_flag;
        } else {
          mb_field_decoding_flag = 0; // is inferred to be equal to 0
        }
      }
    }

    picture.m_mbs[picture.CurrMbAddr].mb_field_decoding_flag =
        mb_field_decoding_flag; // 因为解码mb_skip_flag需要事先知道mb_field_decoding_flag的值
  }

  //-------------解码mb_skip_flag-----------------------
  if (slice_header.MbaffFrameFlag && CurrMbAddr % 2 == 1 && prevMbSkipped) {
    // 如果是bottom field macroblock
    mb_skip_flag = slice_header.mb_skip_flag_next_mb;
  } else {
    // cabac.CABAC_decode_mb_skip_flag(picture, bs, CurrMbAddr,
    //                                 mb_skip_flag); // 2 ae(v)
  }

  //------------------------------------
  if (mb_skip_flag == 1) {
    // 表示本宏块没有残差数据，相应的像素值只需要利用之前已经解码的I/P帧来预测获得
    picture.mb_cnt++;
    if (slice_header.MbaffFrameFlag) {
      if (CurrMbAddr % 2 == 0) {
        // 只需要处理top field macroblock
        picture.m_mbs[picture.CurrMbAddr].mb_skip_flag =
            mb_skip_flag; // 因为解码mb_skip_flag_next_mb需要事先知道前面顶场宏块的mb_skip_flag值
        picture.m_mbs[picture.CurrMbAddr + 1].slice_number =
            slice_header
                .slice_number; // 因为解码mb_skip_flag需要事先知道slice_id的值
        picture.m_mbs[picture.CurrMbAddr + 1].mb_field_decoding_flag =
            mb_field_decoding_flag; // 特别注意：底场宏块和顶场宏块的mb_field_decoding_flag值是相同的

        // cabac.CABAC_decode_mb_skip_flag(
        //     picture, bs, CurrMbAddr + 1,
        //     mb_skip_flag_next_mb); // 2 ae(v) 先读取底场宏块的mb_skip_flag

        if (slice_header.mb_skip_flag_next_mb ==
            0) // 如果底场宏块mb_skip_flag=0
        {
          //          cabac.CABAC_decode_mb_field_decoding_flag(
          //              picture, bs,
          //              mb_field_decoding_flag); // 2 u(1) | ae(v)
          //                                       //
          //                                       再读取底场宏块的mb_field_decoding_flag
          //
          //          is_need_skip_read_mb_field_decoding_flag = true;
        } else // if (mb_skip_flag_next_mb == 1)
        {
          // When slice.MbaffFrameFlag is equal to 1 and mb_field_decoding_flag
          // is not present for both the top and the bottom macroblock of
          // a macroblock pair
          if (picture.mb_x > 0 &&
              picture.m_mbs[CurrMbAddr - 2].slice_number ==
                  slice_header
                      .slice_number) // the left of the current macroblock pair
                                     // in the same slice
          {
            mb_field_decoding_flag =
                picture.m_mbs[CurrMbAddr - 2].mb_field_decoding_flag;
          } else if (picture.mb_y > 0 &&
                     picture.m_mbs[CurrMbAddr - 2 * m_sps.PicWidthInMbs]
                             .slice_number ==
                         slice_header
                             .slice_number) // above the current macroblock
                                            // pair in the same slice
          {
            mb_field_decoding_flag =
                picture.m_mbs[CurrMbAddr - 2 * m_sps.PicWidthInMbs]
                    .mb_field_decoding_flag;
          } else {
            mb_field_decoding_flag = 0; // is inferred to be equal to 0
          }
        }
      }
    }

    //-----------------------------------------------------------------
    //    picture.m_mbs[picture.CurrMbAddr].macroblock_layer_mb_skip(picture,
    //    cabac); // 2 | 3 | 4

    // The inter prediction process for P and B macroblocks is specified
    // in clause 8.4 with inter prediction samples being the output.
    // picture.Inter_prediction_process(); // 帧间预测
  }
  return 0;
}

int SliceBody::NextMbAddress(int n) {
  int i = n + 1;
  while (i < m_idr.PicSizeInMbs &&
         slice_header.MbToSliceGroupMap[i] != slice_header.MbToSliceGroupMap[n])
    i++;
  return i;
}

int SliceBody::macroblock_layer(BitStream &bs, PictureBase &picture) {
  int is_ae = m_pps.entropy_coding_mode_flag; // ae(v)表示CABAC编码
  int mb_type = 0;

  int32_t i = 0;
  CH264Golomb gb;
  int32_t mbPartIdx = 0;
  int32_t transform_size_8x8_flag_temp = 0;

  picture.Inverse_macroblock_scanning_process(
      slice_header.MbaffFrameFlag, CurrMbAddr, mb_field_decoding_flag,
      picture.m_mbs[CurrMbAddr].m_mb_position_x,
      picture.m_mbs[CurrMbAddr].m_mb_position_y);

  if (is_ae) // ae(v) 表示CABAC编码
  {
    // cabac.CABAC_decode_mb_type(picture, bs, mb_type); // 2 ue(v) | ae(v)
  } else // ue(v) 表示CAVLC编码
  {
    mb_type = bs.readUE();
  }
  const int I_PCM = 25;
  if (mb_type == I_PCM) {
    while (!bs.byte_aligned())
      uint8_t pcm_alignment_zero_bit = bs.readU1();
    int32_t pcm_sample_luma[256]; // 3 u(v)
    for (int i = 0; i < 256; i++) {
      int32_t v = m_sps.BitDepthY;
      pcm_sample_luma[i] = bs.readUn(v); // 3 u(v)
    }
    for (int i = 0; i < 2 * m_sps.MbWidthC * m_sps.MbHeightC; i++)
      m_sps.pcm_sample_chroma[i] = bs.readUn(m_sps.BitDepthC);
  } else {
    bool noSubMbPartSizeLessThan8x8Flag = 1;
    //    if (mb_type != I_NxN && MbPartPredMode(mb_type, 0) != Intra_16x16 &&
    //        NumMbPart(mb_type) == 4) {
    //      sub_mb_pred(mb_type);
    //
    //      for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
    //        if (sub_mb_type[mbPartIdx] != B_Direct_8x8) {
    //          if (NumSubMbPart(sub_mb_type[mbPartIdx]) > 1)
    //            noSubMbPartSizeLessThan8x8Flag = 0;
    //        } else if (!direct_8x8_inference_flag)
    //          noSubMbPartSizeLessThan8x8Flag = 0;
    //    } else {
    //      if (transform_8x8_mode_flag &&mb_type = = I_NxN)
    //        transform_size_8x8_flag;
    //      mb_pred(mb_type);
    //    }
    //    if (MbPartPredMode(mb_type, 0) != Intra_16x16) {
    //      coded_block_pattern;
    //      if (CodedBlockPatternLuma > 0 && transform_8x8_mode_flag &&
    //          mb_type != I_NxN && noSubMbPartSizeLessThan8x8Flag &&
    //          (mb_type != B_Direct_16x16 | | direct_8x8_inference_flag))
    //        transform_size_8x8_flag;
    //    }
    //    if (CodedBlockPatternLuma > 0 | | CodedBlockPatternChroma > 0 | |
    //            MbPartPredMode(mb_type, 0) = = Intra_16x16) {
    //      // mb_qp_delta;
    //      residual(0, 15);
    //    }
  }
  return 0;
}
