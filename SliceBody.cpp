#include "SliceBody.hpp"
#include "BitStream.hpp"
#include "Frame.hpp"
#include "PictureBase.hpp"

int SliceBody::DecodeCABAC(CH264Cabac &cabac, BitStream &bs,
                           SliceHeader &header) {
  if (m_pps.entropy_coding_mode_flag) {
    /* CABAC(上下文自适应二进制算术编码) */

    while (!bs.byte_aligned())
      bs.readU1(); //cabac_alignment_one_bit

    cabac.Initialisation_process_for_context_variables(
        (H264_SLIECE_TYPE)header.slice_type, header.cabac_init_idc,
        header.SliceQPY);
    // cabac初始化环境变量

    cabac.Initialisation_process_for_the_arithmetic_decoding_engine(bs);
    // cabac初始化解码引擎
  }
  return 0;
}

/* Rec. ITU-T H.264 (08/2021) 56 */
int SliceBody::parseSliceData(BitStream &bs, PictureBase &picture) {
  SliceHeader &header = picture.m_slice.slice_header;

  /* CABAC编码 */
  CH264Cabac cabac;
  DecodeCABAC(cabac, bs, header);

  /* 是否MBAFF编码 */
  if (header.MbaffFrameFlag == 0)
    mb_field_decoding_flag = header.field_pic_flag;

  CurrMbAddr = header.first_mb_in_slice * (1 + header.MbaffFrameFlag);

  picture.CurrMbAddr = CurrMbAddr;

  header.picNumL0Pred = header.CurrPicNum;
  header.picNumL1Pred = header.CurrPicNum;

  if (picture.m_slice_cnt == 0) {
    picture.Decoding_process_for_picture_order_count(); // 解码POC
    if (m_sps.frame_mbs_only_flag == 0) {
      /* 场编码 */
      std::cout << "hi~" << __LINE__ << std::endl;
      exit(0);
    }

    //--------参考帧重排序------------
    // 只有当前帧为P帧，B帧时，才会对参考图像数列表组进行重排序
    if (header.slice_type == H264_SLIECE_TYPE_P ||
        header.slice_type == H264_SLIECE_TYPE_SP ||
        header.slice_type == H264_SLIECE_TYPE_B) {
      picture.Decoding_process_for_reference_picture_lists_construction(
          picture.m_dpb, picture.m_RefPicList0, picture.m_RefPicList1);

      //--------------
      string sliceType = "UNKNOWN";
      int PicOrderCnt = -1, PicNum = -1, PicNumCnt = -1;

      for (int i = 0; i < picture.m_RefPicList0Length; ++i) {
        if (picture.m_RefPicList0[i]) {
          sliceType = H264_SLIECE_TYPE_TO_STR(
              picture.m_RefPicList0[i]
                  ->m_picture_frame.m_slice.slice_header.slice_type);
          PicOrderCnt = picture.m_RefPicList0[i]->m_picture_frame.PicOrderCnt;
          PicNum = picture.m_RefPicList0[i]->m_picture_frame.PicNum;
          PicNumCnt = picture.m_RefPicList0[i]->m_picture_frame.m_PicNumCnt;

          printf("\tm_PicNumCnt=%d(%s); PicOrderCnt=%d; "
                 "m_RefPicList0[%d]: %s; "
                 "PicOrderCnt=%d; PicNum=%d; PicNumCnt=%d;\n",
                 picture.m_PicNumCnt,
                 H264_SLIECE_TYPE_TO_STR(header.slice_type),
                 picture.PicOrderCnt, i, sliceType.c_str(), PicOrderCnt, PicNum,
                 PicNumCnt);
        }
      }

      PicOrderCnt = -1, PicNum = -1, PicNumCnt = -1;
      for (int i = 0; i < picture.m_RefPicList1Length; ++i) {
        if (picture.m_RefPicList1[i]) {
          sliceType = H264_SLIECE_TYPE_TO_STR(
              picture.m_RefPicList1[i]
                  ->m_picture_frame.m_slice.slice_header.slice_type);
          PicOrderCnt = picture.m_RefPicList1[i]->m_picture_frame.PicOrderCnt;
          PicNum = picture.m_RefPicList1[i]->m_picture_frame.PicNum;
          PicNumCnt = picture.m_RefPicList1[i]->m_picture_frame.m_PicNumCnt;

          printf("\tm_PicNumCnt=%d(%s); PicOrderCnt=%d; "
                 "m_RefPicList1[%d]: %s; "
                 "PicOrderCnt=%d; PicNum=%d; PicNumCnt=%d;\n",
                 picture.m_PicNumCnt,
                 H264_SLIECE_TYPE_TO_STR(header.slice_type),
                 picture.PicOrderCnt, i, sliceType.c_str(), PicOrderCnt, PicNum,
                 PicNumCnt);
        }
      }
    }
  }

  picture.m_slice_cnt++;

  //-------------------------------
  //bool is_need_skip_read_mb_field_decoding_flag = false;

  do {
    if (header.slice_type != SLICE_I && header.slice_type != SLICE_SI) {
      if (!m_pps.entropy_coding_mode_flag) {
        std::cout << "hi~" << __LINE__ << std::endl;
        exit(0);
      } else {
        /* CABAC编码开始 */
        picture.mb_x = (CurrMbAddr %
                        (picture.PicWidthInMbs * (1 + header.MbaffFrameFlag))) /
                       (1 + header.MbaffFrameFlag);

        picture.mb_y = (CurrMbAddr /
                        (picture.PicWidthInMbs * (1 + header.MbaffFrameFlag)) *
                        (1 + header.MbaffFrameFlag)) +
                       ((CurrMbAddr % (picture.PicWidthInMbs *
                                       (1 + header.MbaffFrameFlag))) %
                        (1 + header.MbaffFrameFlag));
        picture.CurrMbAddr = CurrMbAddr;

        // //因为解码mb_skip_flag需要事先知道MbaffFrameFlag的值
        picture.m_mbs[picture.CurrMbAddr].slice_number = slice_number;
        // 因为解码mb_skip_flag需要事先知道slice_id的值（从0开始）

        if (header.MbaffFrameFlag) {
          std::cout << "hi~" << __LINE__ << std::endl;
          exit(0);
        }

        //-------------解码mb_skip_flag-----------------------
        if (header.MbaffFrameFlag && CurrMbAddr % 2 == 1 && prevMbSkipped) {
          // 如果是bottom field macroblock
          std::cout << "hi~" << __LINE__ << std::endl;
          exit(0);
          // mb_skip_flag = mb_skip_flag_next_mb;
        } else
          cabac.CABAC_decode_mb_skip_flag(picture, bs, CurrMbAddr,
                                          mb_skip_flag);

        //------------------------------------
        if (mb_skip_flag == 1) {
          // 表示本宏块没有残差数据，相应的像素值只需要利用之前已经解码的I/P帧来预测获得
          // 首个IDR帧不会进这里，紧跟其后的P帧会进这里（可能会进）
          picture.mb_cnt++;
          if (header.MbaffFrameFlag) {
            if (CurrMbAddr % 2 == 0) // 只需要处理top field macroblock
            {
              picture.m_mbs[picture.CurrMbAddr].mb_skip_flag =
                  mb_skip_flag; // 因为解码mb_skip_flag_next_mb需要事先知道前面顶场宏块的mb_skip_flag值
              picture.m_mbs[picture.CurrMbAddr + 1].slice_number =
                  slice_number; // 因为解码mb_skip_flag需要事先知道slice_id的值
              picture.m_mbs[picture.CurrMbAddr + 1].mb_field_decoding_flag =
                  mb_field_decoding_flag; // 特别注意：底场宏块和顶场宏块的mb_field_decoding_flag值是相同的

              cabac.CABAC_decode_mb_skip_flag(
                  picture, bs, CurrMbAddr + 1,
                  mb_skip_flag_next_mb); // 2 ae(v) 先读取底场宏块的mb_skip_flag

              if (mb_skip_flag_next_mb == 0) // 如果底场宏块mb_skip_flag=0
              {
                cabac.CABAC_decode_mb_field_decoding_flag(
                    picture, bs,
                    mb_field_decoding_flag); // 2 u(1) | ae(v)
                // 再读取底场宏块的mb_field_decoding_flag

                //is_need_skip_read_mb_field_decoding_flag = true;
              } else // if (mb_skip_flag_next_mb == 1)
              {
                // When MbaffFrameFlag is equal to 1 and mb_field_decoding_flag
                // is not present for both the top and the bottom macroblock of
                // a macroblock pair
                if (picture.mb_x > 0 &&
                    picture.m_mbs[CurrMbAddr - 2].slice_number ==
                        slice_number) // the left of the current macroblock pair
                                      // in the same slice
                {
                  mb_field_decoding_flag =
                      picture.m_mbs[CurrMbAddr - 2].mb_field_decoding_flag;
                } else if (picture.mb_y > 0 &&
                           picture.m_mbs[CurrMbAddr - 2 * picture.PicWidthInMbs]
                                   .slice_number ==
                               slice_number) // above the current macroblock
                                             // pair in the same slice
                {
                  mb_field_decoding_flag =
                      picture.m_mbs[CurrMbAddr - 2 * picture.PicWidthInMbs]
                          .mb_field_decoding_flag;
                } else {
                  mb_field_decoding_flag = 0; // is inferred to be equal to 0
                }
              }
            }
          }

          //-----------------------------------------------------------------
          picture.m_mbs[picture.CurrMbAddr].macroblock_layer_mb_skip(
              picture, *this, cabac); // 2 | 3 | 4

          // The inter prediction process for P and B macroblocks is specified
          // in clause 8.4 with inter prediction samples being the output.
          picture.Inter_prediction_process(); // 帧间预测
        }
        moreDataFlag = !mb_skip_flag;
        /* CABAC编码结束 */
      }
    }
    if (moreDataFlag) {
      if (header.MbaffFrameFlag &&
          (CurrMbAddr % 2 == 0 || (CurrMbAddr % 2 == 1 && prevMbSkipped))) {
        /* 表示本宏块是属于一个宏块对中的一个 */
        std::cout << "hi~" << __LINE__ << std::endl;
        exit(0);
      }

      picture.mb_x =
          (CurrMbAddr % (picture.PicWidthInMbs * (1 + header.MbaffFrameFlag))) /
          (1 + header.MbaffFrameFlag);
      picture.mb_y =
          (CurrMbAddr / (picture.PicWidthInMbs * (1 + header.MbaffFrameFlag)) *
           (1 + header.MbaffFrameFlag)) +
          ((CurrMbAddr %
            (picture.PicWidthInMbs * (1 + header.MbaffFrameFlag))) %
           (1 + header.MbaffFrameFlag));
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
        BitDepth = picture.m_slice.m_sps.BitDepthY;

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
        BitDepth = picture.m_slice.m_sps.BitDepthY;

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
        BitDepth = picture.m_slice.m_sps.BitDepthY;
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
      else {

        // P,B帧，I帧不会进这里
        picture.Inter_prediction_process(); // 帧间预测

        BitDepth = picture.m_slice.m_sps.BitDepthY;

        //-------残差-----------
        if (picture.m_mbs[picture.CurrMbAddr].transform_size_8x8_flag == 0) {
          picture.transform_decoding_process_for_4x4_luma_residual_blocks_inter(
              isChroma, isChromaCb, BitDepth, picWidthInSamplesL,
              pic_buff_luma);
        } else {
          picture.transform_decoding_process_for_8x8_luma_residual_blocks_inter(
              isChroma, isChromaCb, BitDepth, picWidthInSamplesL,
              picture.m_mbs[picture.CurrMbAddr].LumaLevel8x8, pic_buff_luma);
        }

        isChromaCb = 1;
        picture.transform_decoding_process_for_chroma_samples_inter(
            isChromaCb, picWidthInSamplesC, pic_buff_cb);

        isChromaCb = 0;
        picture.transform_decoding_process_for_chroma_samples_inter(
            isChromaCb, picWidthInSamplesC, pic_buff_cr);
      }
    }

    if (!m_pps.entropy_coding_mode_flag) {
      // moreDataFlag = bs.more_rbsp_data();
      exit(0);
    } else {
      if (header.slice_type != H264_SLIECE_TYPE_I &&
          header.slice_type != H264_SLIECE_TYPE_SI) {
        prevMbSkipped = mb_skip_flag;
      }

      if (header.MbaffFrameFlag && CurrMbAddr % 2 == 0) {
        // moreDataFlag = 1;
        exit(0);
      } else {
        cabac.CABAC_decode_end_of_slice_flag(picture, bs, end_of_slice_flag);
        moreDataFlag = !end_of_slice_flag;
      }
    }
    CurrMbAddr = NextMbAddress(CurrMbAddr, header);
  } while (moreDataFlag);

  if (picture.mb_cnt == picture.PicSizeInMbs) picture.m_is_decode_finished = 1;

  slice_id++;
  slice_number++;
  return 0;
}

int SliceBody::NextMbAddress(int n, SliceHeader &slice_header) {
  int i = n + 1;
  while (i < m_idr.PicSizeInMbs &&
         slice_header.MbToSliceGroupMap[i] != slice_header.MbToSliceGroupMap[n])
    i++;
  return i;
}
