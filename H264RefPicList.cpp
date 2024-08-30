#include "Frame.hpp"
#include "PictureBase.hpp"
#include <cstdint>
#include <vector>

//--------------参考帧列表重排序------------------------
// 8.2.1 Decoding process for picture order count (only needed to be invoked for one slice of a picture)
int PictureBase::Decoding_process_for_picture_order_count() {
  int ret = 0;

  if (m_slice.m_sps.pic_order_cnt_type == 0) {
    ret = Decoding_process_for_picture_order_count_type_0(
        m_parent->m_picture_previous_ref);
    RETURN_IF_FAILED(ret != 0, ret);
  } else if (m_slice.m_sps.pic_order_cnt_type == 1) {
    ret = Decoding_process_for_picture_order_count_type_1(
        m_parent->m_picture_previous);
    RETURN_IF_FAILED(ret != 0, ret);
  } else if (m_slice.m_sps.pic_order_cnt_type == 2) {
    ret = Decoding_process_for_picture_order_count_type_2(
        m_parent->m_picture_previous);
    RETURN_IF_FAILED(ret != 0, ret);
  }

  PicOrderCntFunc(this); // 设置this->PicOrderCnt字段值

  return ret;
}

// 8.2.1.1 Decoding process for picture order count type 0
// This process is invoked when pic_order_cnt_type is equal to 0.
int PictureBase::Decoding_process_for_picture_order_count_type_0(
    const PictureBase *picture_previous_ref) {
  int32_t prevPicOrderCntMsb = 0;
  int32_t prevPicOrderCntLsb = 0;

  if (m_slice.slice_header.IdrPicFlag == 1) // IDR picture
  {
    prevPicOrderCntMsb = 0;
    prevPicOrderCntLsb = 0;
  } else // if (m_slice.slice_header.m_nal_unit.IdrPicFlag != 1)
  {
    RETURN_IF_FAILED(picture_previous_ref == NULL, -1);

    if (picture_previous_ref->memory_management_control_operation_5_flag ==
        1) // If the previous reference picture in decoding order included a
           // memory_management_control_operation equal to 5
    {
      if (picture_previous_ref->m_picture_coded_type !=
          H264_PICTURE_CODED_TYPE_BOTTOM_FIELD) // If the previous reference
                                                // picture in decoding order is
                                                // not a bottom field
      {
        prevPicOrderCntMsb = 0;
        prevPicOrderCntLsb = picture_previous_ref->TopFieldOrderCnt;
      } else // if (picture_previous_ref.m_picture_coded_type ==
             // H264_PICTURE_CODED_TYPE_BOTTOM_FIELD) //the previous reference
             // picture in decoding order is a bottom field
      {
        prevPicOrderCntMsb = 0;
        prevPicOrderCntLsb = 0;
      }
    } else // the previous reference picture in decoding order did not include a
           // memory_management_control_operation equal to 5
    {
      prevPicOrderCntMsb = picture_previous_ref->PicOrderCntMsb;
      prevPicOrderCntLsb =
          picture_previous_ref->m_slice.slice_header.pic_order_cnt_lsb;
    }
  }

  //--------------------------
  if ((m_slice.slice_header.pic_order_cnt_lsb < prevPicOrderCntLsb) &&
      ((prevPicOrderCntLsb - m_slice.slice_header.pic_order_cnt_lsb) >=
       (m_slice.m_sps.MaxPicOrderCntLsb / 2))) {
    PicOrderCntMsb = prevPicOrderCntMsb + m_slice.m_sps.MaxPicOrderCntLsb;
  } else if ((m_slice.slice_header.pic_order_cnt_lsb > prevPicOrderCntLsb) &&
             ((m_slice.slice_header.pic_order_cnt_lsb - prevPicOrderCntLsb) >
              (m_slice.m_sps.MaxPicOrderCntLsb / 2))) {
    PicOrderCntMsb = prevPicOrderCntMsb - m_slice.m_sps.MaxPicOrderCntLsb;
  } else {
    PicOrderCntMsb = prevPicOrderCntMsb;
  }

  //--------------------------
  if (m_picture_coded_type !=
      H264_PICTURE_CODED_TYPE_BOTTOM_FIELD) // When the current picture is not a
  // bottom field 当前图像为非底场
  {
    TopFieldOrderCnt = PicOrderCntMsb + m_slice.slice_header.pic_order_cnt_lsb;
  }

  if (m_picture_coded_type !=
      H264_PICTURE_CODED_TYPE_TOP_FIELD) // When the current picture is not a
                                         // top field
  {
    if (!m_slice.slice_header.field_pic_flag) // 当前图像为帧
    {
      BottomFieldOrderCnt =
          TopFieldOrderCnt + m_slice.slice_header.delta_pic_order_cnt_bottom;
    } else // 当前图像为底场
    {
      BottomFieldOrderCnt =
          PicOrderCntMsb + m_slice.slice_header.pic_order_cnt_lsb;
    }
  }

  return 0;
}

// 8.2.1.2 Decoding process for picture order count type 1
// This process is invoked when pic_order_cnt_type is equal to 1.
int PictureBase::Decoding_process_for_picture_order_count_type_1(
    const PictureBase *picture_previous) {
  RETURN_IF_FAILED(
      m_slice.slice_header.IdrPicFlag != 1 && picture_previous == NULL, -1);

  int32_t prevFrameNumOffset = 0;

  //--------------prevFrameNumOffset----------------
  if (m_slice.slice_header.IdrPicFlag != 1) // not IDR picture
  {
    if (picture_previous->memory_management_control_operation_5_flag ==
        1) // If the previous picture in decoding order included a
           // memory_management_control_operation equal to 5
    {
      prevFrameNumOffset = 0;
    } else {
      prevFrameNumOffset = picture_previous->FrameNumOffset;
    }
  }

  //--------------FrameNumOffset----------------
  if (m_slice.slice_header.IdrPicFlag == 1) // IDR图像
  {
    FrameNumOffset = 0;
  } else if (picture_previous->m_slice.slice_header.frame_num >
             m_slice.slice_header.frame_num) // 前一图像的帧号比当前图像大
  {
    FrameNumOffset = prevFrameNumOffset + m_slice.m_sps.MaxFrameNum;
  } else {
    FrameNumOffset = prevFrameNumOffset;
  }

  //--------------absFrameNum----------------
  if (m_slice.m_sps.num_ref_frames_in_pic_order_cnt_cycle != 0) {
    absFrameNum = FrameNumOffset + m_slice.slice_header.frame_num;
  } else {
    absFrameNum = 0;
  }

  if (m_slice.slice_header.nal_ref_idc == 0 && absFrameNum > 0) {
    absFrameNum = absFrameNum - 1;
  }

  if (absFrameNum > 0) {
    picOrderCntCycleCnt =
        (absFrameNum - 1) / m_slice.m_sps.num_ref_frames_in_pic_order_cnt_cycle;
    frameNumInPicOrderCntCycle =
        (absFrameNum - 1) % m_slice.m_sps.num_ref_frames_in_pic_order_cnt_cycle;
  }

  //--------------expectedPicOrderCnt----------------
  if (absFrameNum > 0) {
    expectedPicOrderCnt =
        picOrderCntCycleCnt * m_slice.m_sps.ExpectedDeltaPerPicOrderCntCycle;
    for (int i = 0; i <= frameNumInPicOrderCntCycle; i++) {
      expectedPicOrderCnt =
          expectedPicOrderCnt + m_slice.m_sps.offset_for_ref_frame[i];
    }
  } else {
    expectedPicOrderCnt = 0;
  }

  if (m_slice.slice_header.nal_ref_idc == 0) {
    expectedPicOrderCnt =
        expectedPicOrderCnt + m_slice.m_sps.offset_for_non_ref_pic;
  }

  //--------------TopFieldOrderCnt or BottomFieldOrderCnt----------------
  if (!m_slice.slice_header.field_pic_flag) // 当前图像为帧
  {
    TopFieldOrderCnt =
        expectedPicOrderCnt + m_slice.slice_header.delta_pic_order_cnt[0];
    BottomFieldOrderCnt = TopFieldOrderCnt +
                          m_slice.m_sps.offset_for_top_to_bottom_field +
                          m_slice.slice_header.delta_pic_order_cnt[1];
  } else if (!m_slice.slice_header.bottom_field_flag) // 当前图像为顶场
  {
    TopFieldOrderCnt =
        expectedPicOrderCnt + m_slice.slice_header.delta_pic_order_cnt[0];
  } else // 当前图像为底场
  {
    BottomFieldOrderCnt = expectedPicOrderCnt +
                          m_slice.m_sps.offset_for_top_to_bottom_field +
                          m_slice.slice_header.delta_pic_order_cnt[0];
  }

  return 0;
}

// 8.2.1.3 Decoding process for picture order count type 2
// This process is invoked when pic_order_cnt_type is equal to 2.
int PictureBase::Decoding_process_for_picture_order_count_type_2(
    const PictureBase *picture_previous) {
  RETURN_IF_FAILED(
      m_slice.slice_header.IdrPicFlag != 1 && picture_previous == NULL, -1);

  int32_t prevFrameNumOffset = 0;

  //--------------prevFrameNumOffset----------------
  if (m_slice.slice_header.IdrPicFlag != 1) // not IDR picture
  {
    if (picture_previous->memory_management_control_operation_5_flag ==
        1) // If the previous picture in decoding order included a
           // memory_management_control_operation equal to 5
    {
      prevFrameNumOffset = 0;
    } else {
      prevFrameNumOffset = picture_previous->FrameNumOffset;
    }
  }

  //--------------FrameNumOffset----------------
  if (m_slice.slice_header.IdrPicFlag == 1) {
    FrameNumOffset = 0;
  } else if (picture_previous->m_slice.slice_header.frame_num >
             m_slice.slice_header.frame_num) {
    FrameNumOffset = prevFrameNumOffset + m_slice.m_sps.MaxFrameNum;
  } else {
    FrameNumOffset = prevFrameNumOffset;
  }

  //--------------tempPicOrderCnt----------------
  int32_t tempPicOrderCnt = 0;

  if (m_slice.slice_header.IdrPicFlag == 1) {
    tempPicOrderCnt = 0;
  } else if (m_slice.slice_header.nal_ref_idc == 0) // 当前图像为非参考图像
  {
    tempPicOrderCnt = 2 * (FrameNumOffset + m_slice.slice_header.frame_num) - 1;
  } else {
    tempPicOrderCnt = 2 * (FrameNumOffset + m_slice.slice_header.frame_num);
  }

  //--------------TopFieldOrderCnt or BottomFieldOrderCnt----------------
  if (!m_slice.slice_header.field_pic_flag) // 当前图像为帧
  {
    TopFieldOrderCnt = tempPicOrderCnt;
    BottomFieldOrderCnt = tempPicOrderCnt;
  } else if (m_slice.slice_header.bottom_field_flag) // 当前图像为底场
  {
    BottomFieldOrderCnt = tempPicOrderCnt;
  } else // 当前图像为顶场
  {
    TopFieldOrderCnt = tempPicOrderCnt;
  }

  return 0;
}

// 8.2.4 Decoding process for reference picture lists construction
int PictureBase::decoding_reference_picture_lists_construction(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], Frame *(&RefPicList1)[16]) {
  int ret = 0;
  SliceHeader &slice_header = m_slice.slice_header;

  /* 解码的参考图片被标记为“用于短期参考”或“用于长期参考”，如比特流所指定的和第8.2.5节中所指定的。短期参考图片由frame_num 的值标识。长期参考图片被分配一个长期帧索引，该索引由比特流指定并在第 8.2.5 节中指定。 */

  // 8.2.5 Decoded reference picture marking process
  // NOTE: 这里调用8.2.5会有问题

  /* 调用条款 8.2.4.1 来指定 
   * — 变量 FrameNum、FrameNumWrap 和 PicNum 到每个短期参考图片的分配，
   * — 变量 LongTermPicNum 到每个长期参考图片的分配。 */

  // 8.2.4.1 Decoding process for picture numbers
  /* 参考图片通过第 8.4.2.1 节中指定的参考索引来寻址。*/
  ret = decoding_picture_numbers(dpb);
  RETURN_IF_FAILED(ret != 0, ret);

  /* 参考索引是参考图片列表的索引。当解码P或SP切片时，存在单个参考图片列表RefPicList0。在对B切片进行解码时，除了RefPicList0之外，还存在第二独立参考图片列表RefPicList1。*/

  /* 在每个切片的解码过程开始时，按照以下有序步骤的指定导出参考图片列表 RefPicList0 以及 B 切片的 RefPicList1： 
   * 1. 按照以下顺序导出初始参考图片列表 RefPicList0 以及 B 切片的 RefPicList1第 8.2.4.2 条  
   */

  /* 8.2.4.2 Initialization process for reference picture lists */
  ret = init_reference_picture_lists(dpb, RefPicList0, RefPicList1);
  RETURN_IF_FAILED(ret != 0, ret);

  /* 2. 当ref_pic_list_modification_flag_l0等于1时或者当解码B切片时ref_pic_list_modification_flag_l1等于1时，初始参考图片列表RefPicList0以及对于B切片而言RefPicList1按照条款8.2.4.3中的指定进行修改。 */
  ret = modif_reference_picture_lists(RefPicList0, RefPicList1);
  RETURN_IF_FAILED(ret != 0, ret);

  /* 修改后的参考图片列表RefPicList0中的条目数量是num_ref_idx_l0_active_minus1+1，并且对于B片，修改后的参考图片列表RefPicList1中的条目数量是num_ref_idx_l1_active_minus1+1。参考图片可以出现在修改后的参考中的多个索引处图片列表RefPicList0或RefPicList1。 */
  m_RefPicList0Length = slice_header.num_ref_idx_l0_active_minus1 + 1;
  m_RefPicList1Length = slice_header.num_ref_idx_l1_active_minus1 + 1;

  return 0;
}

// 8.2.4.1 Decoding process for picture numbers
/* 当调用第8.2.4节中指定的参考图片列表构建的解码过程、第8.2.5节中指定的解码参考图片标记过程或第8.2.5.2节中指定的frame_num中的间隙的解码过程时，调用该过程。*/
int PictureBase::decoding_picture_numbers(Frame *(&dpb)[16]) {
  SliceHeader &slice_header = m_slice.slice_header;
  const int size_dpb = 16;

  /* 变量 FrameNum、FrameNumWrap、PicNum、LongTermFrameIdx 和 LongTermPicNum 用于第 8.2.4.2 节中参考图片列表的初始化过程、第 8.2.4.3 节中参考图片列表的修改过程、第 8.2.5 节中的解码参考图片标记过程。以及第8.2.5.2节中frame_num中间隙的解码过程。 */

  /* 对于每个短期参考图片，变量 FrameNum 和 FrameNumWrap 被分配如下。首先，将FrameNum设置为等于对应短期参考图片的Slice Header中已解码的语法元素frame_num。然后变量 FrameNumWrap 导出为:*/
  FrameNum = slice_header.frame_num;

  for (int i = 0; i < size_dpb; i++) {
    /* 帧 */
    auto &pict_f = dpb[i]->m_picture_frame;
    auto &pict_f_frameNum = pict_f.FrameNum;
    auto &pict_f_frameNumWrap = pict_f.FrameNumWrap;
    int MaxFrameNum = pict_f.m_slice.m_sps.MaxFrameNum;

    if (pict_f.reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_short_term_reference) {

      if (pict_f_frameNum > FrameNum)
        pict_f_frameNumWrap = pict_f_frameNum - MaxFrameNum;
      else
        pict_f.FrameNumWrap = pict_f_frameNum;
    }

    /* 顶场 */
    auto &pict_t = dpb[i]->m_picture_top_filed;
    auto &pict_t_frameNum = pict_t.FrameNum;
    auto &pict_t_frameNumWrap = pict_t.FrameNumWrap;

    if (pict_t.reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_short_term_reference) {

      if (pict_t.FrameNum > FrameNum)
        pict_t_frameNumWrap = pict_t_frameNum - MaxFrameNum;
      else
        pict_t_frameNumWrap = pict_t_frameNum;
    }

    /* 底场 */
    auto &pict_b = dpb[i]->m_picture_bottom_filed;
    auto &pict_b_frameNum = pict_b.FrameNum;
    auto &pict_b_frameNumWrap = pict_b.FrameNumWrap;

    if (pict_b.reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_short_term_reference) {

      if (pict_b_frameNum > FrameNum)
        pict_b_frameNumWrap = pict_b_frameNum - MaxFrameNum;
      else
        pict_b_frameNumWrap = pict_b_frameNum;
    }
  }

  /* 每个长期参考图片都有一个关联的 LongTermFrameIdx 值（按照第 8.2.5 节的规定分配给它） */
  // 8.2.5 Decoded reference picture marking process
  // NOTE:

  /* 向每个短期参考图片分配变量PicNum，并且向每个长期参考图片分配变量LongTermPicNum。这些变量的值取决于当前图片的field_pic_flag和bottom_field_flag的值，它们的设置如下： */
  if (!slice_header.field_pic_flag) {
    /* 帧编码 */
    for (int i = 0; i < size_dpb; i++) {
      auto &reference_marked_type_f =
          dpb[i]->m_picture_frame.reference_marked_type;
      auto &picNum1_f = dpb[i]->m_picture_frame.PicNum;
      auto &picNum2 = dpb[i]->PicNum;

      /* 对于每个短期参考帧或互补参考场对： */
      if (reference_marked_type_f ==
          H264_PICTURE_MARKED_AS_used_for_short_term_reference)
        picNum2 = picNum1_f = dpb[i]->m_picture_frame.FrameNumWrap;

      auto &reference_marked_type_t =
          dpb[i]->m_picture_top_filed.reference_marked_type;
      auto &reference_marked_type_b =
          dpb[i]->m_picture_bottom_filed.reference_marked_type;
      auto &picNum1_t = dpb[i]->m_picture_top_filed.PicNum;
      auto &picNum1_b = dpb[i]->m_picture_bottom_filed.PicNum;

      if (reference_marked_type_t ==
              H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
          reference_marked_type_b ==
              H264_PICTURE_MARKED_AS_used_for_short_term_reference) {
        picNum1_t = dpb[i]->m_picture_top_filed.FrameNumWrap;
        picNum1_b = dpb[i]->m_picture_bottom_filed.FrameNumWrap;
        picNum2 = picNum1_t;

        if (picNum1_t != picNum1_b) {
          std::cerr << "An error occurred on " << __FUNCTION__
                    << "():" << __LINE__ << std::endl;
          return -1;
        }
      }

      /* 对于每个长期参考帧或长期互补参考场对： */
      auto &pict_f = dpb[i]->m_picture_frame;
      if (pict_f.reference_marked_type ==
          H264_PICTURE_MARKED_AS_used_for_long_term_reference)
        dpb[i]->LongTermPicNum = pict_f.LongTermPicNum =
            pict_f.LongTermFrameIdx;

      auto &pict_t = dpb[i]->m_picture_top_filed;
      auto &pict_b = dpb[i]->m_picture_bottom_filed;
      if (pict_t.reference_marked_type ==
              H264_PICTURE_MARKED_AS_used_for_long_term_reference &&
          pict_b.reference_marked_type ==
              H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
        pict_t.LongTermPicNum = pict_t.LongTermFrameIdx;
        pict_b.LongTermPicNum = pict_b.LongTermFrameIdx;
        dpb[i]->LongTermPicNum = pict_t.LongTermPicNum;
        if (pict_t.LongTermPicNum != pict_b.LongTermPicNum) {
          std::cerr << "An error occurred on " << __FUNCTION__
                    << "():" << __LINE__ << std::endl;
          return -1;
        }
      }
    }
  } else if (slice_header.field_pic_flag) {
    /* 场编码 */

    for (int i = 0; i < size_dpb; i++) {
      /* ------------------ 对于每个短期参考字段，以下内容适用 ------------------  */
      /* 顶场 */
      auto &pict_t = dpb[i]->m_picture_top_filed;
      if (pict_t.reference_marked_type ==
          H264_PICTURE_MARKED_AS_used_for_short_term_reference) {

        if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD)
          /* 如果参考字段与当前字段具有相同的奇偶校验（对于顶场而言） */
          pict_t.PicNum = 2 * pict_t.FrameNumWrap + 1;
        else if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD)
          /* 参考字段与当前字段具有相反的奇偶校验 （对于顶场而言）*/
          pict_t.PicNum = 2 * pict_t.FrameNumWrap;

        dpb[i]->PicNum = pict_t.PicNum;
      }

      /* 底场 */
      auto &pict_b = dpb[i]->m_picture_bottom_filed;
      if (pict_b.reference_marked_type ==
          H264_PICTURE_MARKED_AS_used_for_short_term_reference) {

        if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD)
          /* 如果参考字段与当前字段具有相同的奇偶校验（对于底场而言） */
          pict_b.PicNum = 2 * pict_b.FrameNumWrap + 1;
        else if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD)
          /* 参考字段与当前字段具有相反的奇偶校验 （对于底场而言）*/
          pict_b.PicNum = 2 * pict_b.FrameNumWrap;

        dpb[i]->PicNum = pict_b.PicNum;
      }

      /* ------------------ 对于每个长期参考字段，以下内容适用 ------------------  */
      if (pict_t.reference_marked_type ==
          H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
        if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD)
          pict_t.LongTermPicNum = 2 * pict_t.LongTermFrameIdx + 1;
        else if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD)
          pict_t.LongTermPicNum = 2 * pict_t.LongTermFrameIdx;

        dpb[i]->LongTermPicNum = pict_t.LongTermPicNum;
      }

      if (pict_b.reference_marked_type ==
          H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
        if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD)
          pict_b.LongTermPicNum = 2 * pict_b.LongTermFrameIdx + 1;
        else if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD)
          pict_b.LongTermPicNum = 2 * pict_b.LongTermFrameIdx;

        dpb[i]->LongTermPicNum = pict_b.LongTermPicNum;
      }
    }
  }

  return 0;
}

// 8.2.4.2 Initialisation process for reference picture lists
/* 当解码 P、SP 或 B 片头时会调用此初始化过程。 */
int PictureBase::init_reference_picture_lists(Frame *(&dpb)[16],
                                              Frame *(&RefPicList0)[16],
                                              Frame *(&RefPicList1)[16]) {

  /* 当第 8.2.4.2.1 至 8.2.4.2.5 节中，指定的初始 RefPicList0 或 RefPicList1 中的条目数：
   * - 分别大于 num_ref_idx_l0_active_minus1 + 1 或 num_ref_idx_l1_active_minus1 + 1 时，超过位置 num_ref_idx_l0_active_minus1 或 num_ref_idx_ 的额外条目l1_active_minus1 被丢弃从初始参考图片列表中。 
  * - 分别小于 num_ref_idx_l0_active_minus1 + 1 或 num_ref_idx_l1_active_minus1 + 1 时，初始参考图片列表中的剩余条目为设置等于“无参考图片”。 */

  SliceHeader &slice_header = m_slice.slice_header;

  /* RefPicList0 和 RefPicList1 具有第 8.2.4.2.1 至 8.2.4.2.5 节中指定的初始条目。*/
  int ret = 0;
  if (slice_header.slice_type == SLICE_P ||
      slice_header.slice_type == SLICE_SP) {

    /* RefPicList0为排序后的dpb*/

    /* 初始化P、SP Slice的参考列表，'列表'是因为参考帧不一定只有一个(对于帧编码） */
    if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_FRAME)
      ret = init_reference_picture_list_P_SP_slices_in_frames(
          dpb, RefPicList0, m_RefPicList0Length);

    /* 初始化P、SP Slice的参考列表，'列表'是因为参考帧不一定只有一个(对于场编码） */
    else if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD ||
             m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD)
      ret = init_reference_picture_list_P_SP_slices_in_fields(
          dpb, RefPicList0, m_RefPicList0Length);

  } else if (slice_header.slice_type == SLICE_B) {

    /* 初始化B Slice的参考列表，'列表'是因为参考帧不一定只有一个(对于帧编码） */
    if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_FRAME)
      ret = init_reference_picture_lists_B_slices_in_frames(
          dpb, RefPicList0, RefPicList1, m_RefPicList0Length,
          m_RefPicList1Length);

    /* 初始化B Slice的参考列表，'列表'是因为参考帧不一定只有一个(对于场编码） */
    else if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD ||
             m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD)
      ret = init_reference_picture_lists_B_slices_in_fields(
          dpb, RefPicList0, RefPicList1, m_RefPicList0Length,
          m_RefPicList1Length);
  }
  if (ret != 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

  // When the number of entries in the initial RefPicList0 or RefPicList1
  // produced as specified in clauses 8.2.4.2.1 through 8.2.4.2.5 is greater
  // than num_ref_idx_l0_active_minus1 + 1 or num_ref_idx_l1_active_minus1 + 1,
  // respectively, the extra entries past position num_ref_idx_l0_active_minus1
  // or num_ref_idx_l1_active_minus1 are discarded from the initial reference
  // picture list.
  if (m_RefPicList0Length > slice_header.num_ref_idx_l0_active_minus1 + 1) {
    for (int i = slice_header.num_ref_idx_l0_active_minus1 + 1;
         i < m_RefPicList0Length; i++) {
      RefPicList0[i] = NULL;
    }
    m_RefPicList0Length = slice_header.num_ref_idx_l0_active_minus1 + 1;
  }

  if (m_RefPicList1Length > slice_header.num_ref_idx_l1_active_minus1 + 1) {
    for (int i = slice_header.num_ref_idx_l1_active_minus1 + 1;
         i < m_RefPicList1Length; i++) {
      RefPicList1[i] = NULL;
    }
    m_RefPicList1Length = slice_header.num_ref_idx_l1_active_minus1 + 1;
  }

  // When the number of entries in the initial RefPicList0 or RefPicList1
  // produced as specified in clauses 8.2.4.2.1 through 8.2.4.2.5 is less than
  // num_ref_idx_l0_active_minus1 + 1 or num_ref_idx_l1_active_minus1 + 1,
  // respectively, the remaining entries in the initial reference picture list
  // are set equal to "no reference picture".
  if (m_RefPicList0Length < slice_header.num_ref_idx_l0_active_minus1 + 1) {
    for (int i = m_RefPicList0Length;
         i < slice_header.num_ref_idx_l0_active_minus1 + 1; i++) {
      RefPicList0[i] = NULL; // FIXME: set equal to "no reference picture".
    }
  }

  if (m_RefPicList1Length < slice_header.num_ref_idx_l1_active_minus1 + 1) {
    for (int i = m_RefPicList1Length;
         i < slice_header.num_ref_idx_l1_active_minus1 + 1; i++) {
      RefPicList1[i] = NULL; // FIXME: set equal to "no reference picture".
    }
  }

  return 0;
}

// 8.2.4.2.1 Initialisation process for the reference picture list for P and SP
/* RefPicList0为排序后的dpb*/
int PictureBase::init_reference_picture_list_P_SP_slices_in_frames(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], int32_t &RefPicList0Length) {
  const int32_t size_dpb = 16;

  SliceHeader &slice_header = m_slice.slice_header;
  /* 1. 参考图片列表RefPicList0被排序，使得短期参考帧和短期互补参考场对具有比长期参考帧和长期互补参考场对更低的索引。 */
  int index = 0;
  vector<int32_t> indexTemp_short, indexTemp_long;
  for (index = 0; index < size_dpb; index++) {
    auto &pict_f = dpb[index]->m_picture_frame;
    if (pict_f.reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_short_term_reference)
      indexTemp_short.push_back(index);
    else if (pict_f.reference_marked_type ==
             H264_PICTURE_MARKED_AS_used_for_long_term_reference)
      indexTemp_long.push_back(index);
  }

  /* 当调用该过程时，应当有至少一个参考帧或互补参考字段对当前被标记为“用于参考”（即，“用于短期参考”或“用于长期参考”） ）并且未标记为“不存在”*/
  if (indexTemp_short.size() + indexTemp_long.size() <= 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

  /* 2. 短期参考帧和互补参考场对从具有最高PicNum值的帧或互补场对开始排序，并按降序进行到具有最低PicNum值的帧或互补场对。 */
  //sort(indexTemp_short.begin(), indexTemp_short.end(), greater<int32_t>());
  /* TODO YangJing 错了 <24-08-31 01:23:34> */
  for (int i = 0; i < (int)indexTemp_short.size() - 1; i++) {
    for (int j = 0; j < (int)indexTemp_short.size() - i - 1; j++) {
      if (dpb[indexTemp_short[j]]->m_picture_frame.PicNum <
          dpb[indexTemp_short[j + 1]]->m_picture_frame.PicNum) // 降序排列
      {
        int32_t temp = indexTemp_short[j];
        indexTemp_short[j] = indexTemp_short[j + 1];
        indexTemp_short[j + 1] = temp;
      }
    }
  }

  /* 3. 长期参考帧和互补参考字段对从具有最低LongTermPicNum值的帧或互补字段对开始排序，并按升序进行到具有最高LongTermPicNum值的帧或互补字段对。*/
  //sort(indexTemp_long.begin(), indexTemp_long.end());
  /* TODO YangJing 错了 <24-08-31 01:23:34> */
  for (int i = 0; i < (int)indexTemp_long.size() - 1; i++) {
    for (int j = 0; j < (int)indexTemp_long.size() - i - 1; j++) {
      if (dpb[indexTemp_long[j]]->m_picture_frame.LongTermPicNum >
          dpb[indexTemp_long[j + 1]]
              ->m_picture_frame.LongTermPicNum) // 升序排列
      {
        int32_t temp = indexTemp_long[j];
        indexTemp_long[j] = indexTemp_long[j + 1];
        indexTemp_long[j + 1] = temp;
      }
    }
  }

  // 4. 生成排序后的参考序列
  int j = 0;
  for (index = 0; index < indexTemp_short.size(); ++index)
    RefPicList0[j++] = dpb[indexTemp_short[index]];

  for (index = 0; index < indexTemp_long.size(); ++index)
    RefPicList0[j++] = dpb[indexTemp_long[index]];

  RefPicList0Length = j;

  //RefPicList0[ 0 ] is set equal to the short-term reference picture with PicNum = 303,
  //RefPicList0[ 1 ] is set equal to the short-term reference picture with PicNum = 302,
  //RefPicList0[ 2 ] is set equal to the short-term reference picture with PicNum = 300,
  //RefPicList0[ 3 ] is set equal to the long-term reference picture with LongTermPicNum = 0,
  //RefPicList0[ 4 ] is set equal to the long-term reference picture with LongTermPicNum = 3.

  /* TODO YangJing 这里是在做什么？ <24-08-31 00:06:06> */
  if (slice_header.MbaffFrameFlag) {
    for (index = 0; index < RefPicList0Length; ++index) {
      Frame *ref_list_frame = RefPicList0[index];
      Frame *&ref_list_top_filed = RefPicList0[16 + 2 * index];
      Frame *&ref_list_bottom_filed = RefPicList0[16 + 2 * index + 1];

      ref_list_top_filed = ref_list_frame;
      ref_list_bottom_filed = ref_list_frame;
    }
  }

  return 0;
}

//bool compareByLongTermFrameIdx(const Frame *f1, const Frame *f2) {
//  return (f1->m_picture_top_filed.LongTermFrameIdx >
//          f2->m_picture_top_filed.LongTermFrameIdx) ||
//         (f1->m_picture_bottom_filed.LongTermFrameIdx >
//          f2->m_picture_bottom_filed.LongTermFrameIdx);
//}

//bool compareByFrameNumWrap(const Frame *f1, const Frame *f2) {
//  return (f1->m_picture_top_filed.FrameNumWrap <
//          f2->m_picture_top_filed.FrameNumWrap) ||
//         (f1->m_picture_bottom_filed.FrameNumWrap <
//          f2->m_picture_bottom_filed.FrameNumWrap);
//}

//8.2.4.2.2 Initialization process for the reference picture list for P and SP slices in fields
/* 与PictureBase::init_reference_picture_list_P_SP_slices_in_frames同理 */
/* 注 – 当对场进行解码时，可供参考的图片数量实际上至少是对解码顺序中相同位置的帧进行解码时的两倍。*/
/* TODO YangJing 这个函数并没有经过测试验证 <24-08-31 00:49:10> */
int PictureBase::init_reference_picture_list_P_SP_slices_in_fields(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], int32_t &RefPicList0Length) {
  const int32_t size_dpb = 16;
  int index = 0;
  vector<Frame *> refFrameList0ShortTerm, refFrameList0LongTerm;
  /* 1. 参考图片列表RefPicList0被排序，使得短期参考帧和短期互补参考场对具有比长期参考帧和长期互补参考场对更低的索引。 */
  for (index = 0; index < size_dpb; index++) {

    auto &pict_t = dpb[index]->m_picture_top_filed;
    auto &pict_b = dpb[index]->m_picture_bottom_filed;

    if (pict_t.reference_marked_type ==
            H264_PICTURE_MARKED_AS_used_for_short_term_reference ||
        pict_b.reference_marked_type ==
            H264_PICTURE_MARKED_AS_used_for_short_term_reference)
      refFrameList0ShortTerm.push_back(dpb[index]);
    else if (dpb[index]->m_picture_top_filed.reference_marked_type ==
                 H264_PICTURE_MARKED_AS_used_for_long_term_reference ||
             dpb[index]->m_picture_bottom_filed.reference_marked_type ==
                 H264_PICTURE_MARKED_AS_used_for_long_term_reference)
      refFrameList0LongTerm.push_back(dpb[index]);
  }

  /*2. 当当前场是互补参考场对的第二场（按解码顺序）并且第一场被标记为“用于短期参考”时，第一场被包括在短期参考帧列表refFrameList0ShortTerm中。*/
  Frame *curPic = m_parent;
  if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD) {
    if (curPic->m_picture_top_filed.reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_short_term_reference)
      refFrameList0ShortTerm.push_back(curPic);
    else if (curPic->m_picture_top_filed.reference_marked_type ==
             H264_PICTURE_MARKED_AS_used_for_long_term_reference)
      refFrameList0LongTerm.push_back(curPic);
  }

  /* 当调用这一过程时，应该有至少一个参考字段（可以是参考帧的字段）当前被标记为“用于参考”（即，“用于短期参考”或“用于参考”）。供长期参考”）并且没有标记为“不存在”。 */
  if (refFrameList0ShortTerm.size() + refFrameList0LongTerm.size() == 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

  /* 3. refFrameList0ShortTerm 从具有最高 FrameNumWrap 值的参考帧开始排序，并按降序继续到具有最低 FrameNumWrap 值的参考帧。 */
  //sort(refFrameList0ShortTerm.begin(), refFrameList0ShortTerm.end(),
  //compareByFrameNumWrap);
  //TODO  <24-08-31 02:50:26, YangJing>
  for (int i = 0; i < refFrameList0ShortTerm.size() - 1; i++) {
    for (int j = 0; j < refFrameList0ShortTerm.size() - i - 1; j++) {
      if (refFrameList0ShortTerm[j]->m_picture_top_filed.FrameNumWrap <
              refFrameList0ShortTerm[j + 1]->m_picture_top_filed.FrameNumWrap ||
          refFrameList0ShortTerm[j]->m_picture_bottom_filed.FrameNumWrap <
              refFrameList0ShortTerm[j + 1]
                  ->m_picture_bottom_filed.FrameNumWrap) // 降序排列
      {
        Frame *temp = refFrameList0ShortTerm[j];
        refFrameList0ShortTerm[j] = refFrameList0ShortTerm[j + 1];
        refFrameList0ShortTerm[j + 1] = temp;
      }
    }
  }

  /* 4. refFrameList0LongTerm 从具有最低 LongTermFrameIdx 值的参考帧开始排序，并按升序继续到具有最高 LongTermFrameIdx 值的参考帧。 */
  //sort(refFrameList0LongTerm.begin(), refFrameList0LongTerm.end(),
  //compareByLongTermFrameIdx);
  //TODO  <24-08-31 02:50:22, YangJing>
  for (int i = 0; i < refFrameList0LongTerm.size() - 1; i++) {
    for (int j = 0; j < refFrameList0LongTerm.size() - i - 1; j++) {
      if (refFrameList0LongTerm[j]->m_picture_top_filed.LongTermFrameIdx >
              refFrameList0LongTerm[j + 1]
                  ->m_picture_top_filed.LongTermFrameIdx ||
          refFrameList0LongTerm[j]->m_picture_top_filed.LongTermFrameIdx >
              refFrameList0LongTerm[j + 1]
                  ->m_picture_top_filed.LongTermFrameIdx) // 升序排列
      {
        Frame *temp = refFrameList0LongTerm[j];
        refFrameList0LongTerm[j] = refFrameList0LongTerm[j + 1];
        refFrameList0LongTerm[j + 1] = temp;
      }
    }
  }

  /* 第 8.2.4.2.5 节中指定的过程是用 refFrameList0ShortTerm 和 refFrameList0LongTerm 作为输入来调用的，并且输出被分配给 RefPicList0。*/
  int ret = init_reference_picture_lists_in_fields(
      refFrameList0ShortTerm, refFrameList0LongTerm, RefPicList0,
      RefPicList0Length, 0);

  return ret;
}

//bool compareByPicOrderCnt(const Frame *f1, const Frame *f2) {}

// 8.2.4.2.3 Initialisation process for reference picture lists for B slices in frames
/* 当解码编码帧中的 B 切片时，会调用此初始化过程。*/
/* 为了形成参考图片列表RefPicList0和RefPicList1，指的是解码的参考帧或互补参考字段对。 */
/* TODO YangJing 做到这个函数来了。。。。。！！！！气死我了，以后一定要看警告 <24-08-31 03:01:42> */
int PictureBase::init_reference_picture_lists_B_slices_in_frames(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], Frame *(&RefPicList1)[16],
    int32_t &RefPicList0Length, int32_t &RefPicList1Length) {

  const int32_t size_dpb = 16;
  vector<int32_t> indexTemp_short_left, indexTemp_short_right, indexTemp_long,
      indexTemp3;

  /* 对于B切片，参考图片列表RefPicList0和RefPicList1中的短期参考条目的顺序取决于输出顺序，如由PicOrderCnt()给出的。当 pic_order_cnt_type 等于 0 时，如第 8.2.5.2 节中指定的被标记为“不存在”的参考图片不包括在 RefPicList0 或 RefPicList1 中 */

  /* 注1 – 当gaps_in_frame_num_value_allowed_flag等于1时，编码器应使用参考图片列表修改来确保解码过程的正确操作（特别是当pic_order_cnt_type等于0时，在这种情况下，不会为“不存在”帧推断PicOrderCnt()。 */

  /* 参考图片列表RefPicList0被排序，使得短期参考条目具有比长期参考条目更低的索引,排序如下：
   * 1. 令entryShortTerm 为变量，其范围涵盖当前标记为“用于短期参考”的所有参考条目。当存在PicOrderCnt(entryShortTerm)小于PicOrderCnt(CurrPic)的entryShortTerm的某些值时，entryShortTerm的这些值按照PicOrderCnt(entryShortTerm)的降序放置在refPicList0的开头。然后，entryShortTerm 的所有剩余值（当存在时）按照 PicOrderCnt(entryShortTerm ) 的升序被附加到 refPicList0。
   * 2.对长期参考条目进行排序，从具有最低LongTermPicNum值的长期参考条目开始，并按升序进行到具有最高LongTermPicNum值的长期参考条目。 */

  int index = 0;
  //-------------RefPicList0----------------
  // 1. 先把所有的短期参考帧放到数组的前半部分，把所有的长期参考帧放到数组的后半部分
  for (index = 0; index < size_dpb; index++) {
    auto &pict_f = dpb[index]->m_picture_frame;
    if (pict_f.reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_short_term_reference) {

      if (pict_f.PicOrderCnt < PicOrderCnt)
        indexTemp_short_left.push_back(index);
      else
        indexTemp_short_right.push_back(index);

    } else if (pict_f.reference_marked_type ==
               H264_PICTURE_MARKED_AS_used_for_long_term_reference)
      indexTemp_long.push_back(index);

    else
      indexTemp3.push_back(index);
  }

  /* 当调用该过程时，应至少有一个参考条目当前被标记为“用于参考”（即“用于短期参考”或“用于长期参考”）并且未被标记为“不存在” */
  if (indexTemp_short_left.size() + indexTemp_short_right.size() +
          indexTemp_long.size() <=
      0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

  // 2. 按帧号(PicOrderCnt)按顺序排列短期参考帧(冒泡排序)
  //sort(indexTemp_short_left.begin(),indexTemp_short_left.end(),compareByPicOrderCnt);
  //TODO  <24-08-31 02:59:25, YangJing>
  for (index = 0; index < (int)indexTemp_short_left.size() - 1; index++) {
    for (int j = 0; j < (int)indexTemp_short_left.size() - index - 1; j++) {
      if (dpb[indexTemp_short_left[j]]->m_picture_frame.PicOrderCnt <
          dpb[indexTemp_short_left[j + 1]]
              ->m_picture_frame.PicOrderCnt) // 降序排列
      {
        int32_t temp = indexTemp_short_left[j];
        indexTemp_short_left[j] = indexTemp_short_left[j + 1];
        indexTemp_short_left[j + 1] = temp;
      }
    }
  }

  for (index = 0; index < (int)indexTemp_short_right.size() - 1; index++) {
    for (int j = 0; j < (int)indexTemp_short_right.size() - index - 1; j++) {
      if (dpb[indexTemp_short_right[j]]->m_picture_frame.PicOrderCnt >
          dpb[indexTemp_short_right[j + 1]]
              ->m_picture_frame.PicOrderCnt) // 升序排列
      {
        int32_t temp = indexTemp_short_right[j]; // 交换元素值
        indexTemp_short_right[j] = indexTemp_short_right[j + 1];
        indexTemp_short_right[j + 1] = temp;
      }
    }
  }

  // 3. 按帧号(LongTermPicNum)升序排列长期参考帧(冒泡排序)
  for (index = 0; index < (int)indexTemp_long.size() - 1; index++) {
    for (int j = 0; j < (int)indexTemp_long.size() - index - 1; j++) {
      if (dpb[indexTemp_long[j]]->m_picture_frame.LongTermPicNum >
          dpb[indexTemp_long[j + 1]]
              ->m_picture_frame.LongTermPicNum) // 升序排列
      {
        int32_t temp = indexTemp_long[j]; // 交换元素值
        indexTemp_long[j] = indexTemp_long[j + 1];
        indexTemp_long[j + 1] = temp;
      }
    }
  }

  // 4. 生成排序后的参考序列
  int len = 0;
  for (index = 0; index < indexTemp_short_left.size(); index++)
    RefPicList0[len++] = dpb[indexTemp_short_left[index]];

  for (index = 0; index < indexTemp_short_right.size(); index++)
    RefPicList0[len++] = dpb[indexTemp_short_right[index]];

  for (index = 0; index < indexTemp_long.size(); index++)
    RefPicList0[len++] = dpb[indexTemp_long[index]];

  RefPicList0Length = len;

  //-------------RefPicList1----------------
  indexTemp_short_left.clear();
  indexTemp_short_right.clear();
  indexTemp_long.clear();
  indexTemp3.clear();

  // 1.
  // 先把所有的短期参考帧放到数组的前半部分，把所有的长期参考帧放到数组的后半部分
  for (index = 0; index < size_dpb; index++) {
    auto &pict_f = dpb[index]->m_picture_frame;
    if (pict_f.reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_short_term_reference) {

      if (pict_f.PicOrderCnt > PicOrderCnt)
        indexTemp_short_left.push_back(index);
      else
        indexTemp_short_right.push_back(index);

    } else if (pict_f.reference_marked_type ==
               H264_PICTURE_MARKED_AS_used_for_long_term_reference)
      indexTemp_long.push_back(index);
    else
      indexTemp3.push_back(index);
  }

  /* 当调用该过程时，应至少有一个参考条目当前被标记为“用于参考”（即“用于短期参考”或“用于长期参考”）并且未被标记为“不存在” */
  if (indexTemp_short_left.size() + indexTemp_short_right.size() +
          indexTemp_long.size() <=
      0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

  // 2. 按帧号(PicOrderCnt)按顺序排列短期参考帧(冒泡排序)
  for (index = 0; index < (int)indexTemp_short_left.size() - 1; index++) {
    for (len = 0; len < (int)indexTemp_short_left.size() - index - 1; len++) {
      if (dpb[indexTemp_short_left[len]]->m_picture_frame.PicOrderCnt >
          dpb[indexTemp_short_left[len + 1]]
              ->m_picture_frame.PicOrderCnt) // 升序排列
      {
        int32_t temp = indexTemp_short_left[len]; // 交换元素值
        indexTemp_short_left[len] = indexTemp_short_left[len + 1];
        indexTemp_short_left[len + 1] = temp;
      }
    }
  }

  for (index = 0; index < (int)indexTemp_short_right.size() - 1; index++) {
    for (len = 0; len < (int)indexTemp_short_right.size() - index - 1; len++) {
      if (dpb[indexTemp_short_right[len]]->m_picture_frame.PicOrderCnt <
          dpb[indexTemp_short_right[len + 1]]
              ->m_picture_frame.PicOrderCnt) // 降序排列
      {
        int32_t temp = indexTemp_short_right[len]; // 交换元素值
        indexTemp_short_right[len] = indexTemp_short_right[len + 1];
        indexTemp_short_right[len + 1] = temp;
      }
    }
  }

  // 3. 按帧号(LongTermPicNum)升序排列长期参考帧(冒泡排序)
  for (index = 0; index < (int)indexTemp_long.size() - 1; index++) {
    for (len = 0; len < (int)indexTemp_long.size() - index - 1; len++) {
      if (dpb[indexTemp_long[len]]->m_picture_frame.LongTermPicNum >
          dpb[indexTemp_long[len + 1]]
              ->m_picture_frame.LongTermPicNum) // 升序排列
      {
        int32_t temp = indexTemp_long[len];
        indexTemp_long[len] = indexTemp_long[len + 1];
        indexTemp_long[len + 1] = temp;
      }
    }
  }

  // 4. 生成排序后的参考序列
  len = 0;
  for (index = 0; index < indexTemp_short_left.size(); index++)
    RefPicList1[len++] = dpb[indexTemp_short_left[index]];

  for (index = 0; index < indexTemp_short_right.size(); index++)
    RefPicList1[len++] = dpb[indexTemp_short_right[index]];

  for (index = 0; index < indexTemp_long.size(); index++)
    RefPicList1[len++] = dpb[indexTemp_long[index]];

  RefPicList1Length = len;

  //--------------------------
  // When the reference picture list RefPicList1 has more than one entry and
  // RefPicList1 is identical to the reference picture list RefPicList0, the
  // first two entries RefPicList1[ 0 ] and RefPicList1[ 1 ] are switched
  int flag = 0;
  if (indexTemp_short_left.size() + indexTemp_short_right.size() +
          indexTemp_long.size() >
      1) {
    for (index = 0; index < size_dpb; index++) {
      if (RefPicList1[index] != RefPicList0[index]) {
        flag = 1;
        break;
      }
    }
  }

  if (flag == 0) // the first two entries RefPicList1[ 0 ] and RefPicList1[ 1 ]
                 // are switched
  {
    Frame *tmp = RefPicList1[0];
    RefPicList1[0] = RefPicList1[1];
    RefPicList1[1] = tmp;
  }

  return 0;
}

// 8.2.4.2.4 Initialisation process for reference picture lists for B slices in
// fields This initialisation process is invoked when decoding a B slice in a
// coded field.
int PictureBase::init_reference_picture_lists_B_slices_in_fields(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], Frame *(&RefPicList1)[16],
    int32_t &RefPicList0Length, int32_t &RefPicList1Length) {

  int ret = 0;
  int32_t size_dpb = 16;
  int32_t i = 0;
  int32_t j = 0;
  int32_t short_refs_count_left = 0;
  int32_t short_refs_count_right = 0;
  int32_t long_refs_count = 0;
  //int32_t not_refs_count = 0;

  int32_t indexTemp_short_left[16] = {0};
  int32_t indexTemp_short_right[16] = {0};
  int32_t indexTemp_long[16] = {0};
  //int32_t indexTemp3[16] = {0};

  Frame *refFrameList0ShortTerm[16] = {NULL};
  Frame *refFrameList1ShortTerm[16] = {NULL};
  Frame *refFrameListLongTerm[16] = {NULL};

  // FIXME: When pic_order_cnt_type is equal to 0, reference pictures that are
  // marked as "non-existing" as specified
  //        in clause 8.2.5.2 are not included in either RefPicList0 or
  //        RefPicList1.

  //-------------RefPicList0----------------
  // 1.
  // 先把所有的短期参考帧放到数组的前半部分，把所有的长期参考帧放到数组的后半部分
  for (i = 0; i < size_dpb; i++) {
    if (dpb[i]->m_picture_frame.reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_short_term_reference) {
      if (dpb[i]->m_picture_frame.PicOrderCnt < PicOrderCnt) {
        indexTemp_short_left[short_refs_count_left] = i;
        short_refs_count_left++;
      } else // if (RefPicList0[i].m_picture_frame.PicOrderCnt >= PicOrderCnt)
      {
        indexTemp_short_right[short_refs_count_right] = i;
        short_refs_count_right++;
      }
    } else if (dpb[i]->m_picture_frame.reference_marked_type ==
               H264_PICTURE_MARKED_AS_used_for_long_term_reference) // reference
                                                                    // frame or
    // complementary
    // reference
    // field
    // pair
    {
      indexTemp_long[long_refs_count] = i;
      long_refs_count++;
    } else {
      //indexTemp3[not_refs_count] = i;
      //not_refs_count++;
    }
  }

  // there shall be at least one reference frame or complementary reference
  // field pair that is currently marked as "used for reference"
  RETURN_IF_FAILED(
      short_refs_count_left + short_refs_count_right + long_refs_count == 0,
      -1);

  // 2. 按帧号降序排列短期参考帧(冒泡排序)
  for (i = 0; i < short_refs_count_left - 1; i++) {
    for (j = 0; j < short_refs_count_left - i - 1; j++) {
      if (dpb[indexTemp_short_left[j]]->m_picture_frame.PicOrderCnt <
          dpb[indexTemp_short_left[j + 1]]
              ->m_picture_frame.PicOrderCnt) // 降序排列
      {
        int32_t temp = indexTemp_short_left[j];
        indexTemp_short_left[j] = indexTemp_short_left[j + 1];
        indexTemp_short_left[j + 1] = temp;
      }
    }
  }

  for (i = 0; i < short_refs_count_right - 1; i++) {
    for (j = 0; j < short_refs_count_right - i - 1; j++) {
      if (dpb[indexTemp_short_right[j]]->m_picture_frame.PicOrderCnt >
          dpb[indexTemp_short_right[j + 1]]
              ->m_picture_frame.PicOrderCnt) // 升序排列
      {
        int32_t temp = indexTemp_short_right[j];
        indexTemp_short_right[j] = indexTemp_short_right[j + 1];
        indexTemp_short_right[j + 1] = temp;
      }
    }
  }

  // 3. 按帧号升序排列长期参考帧(冒泡排序)
  for (i = 0; i < long_refs_count - 1; i++) {
    for (j = 0; j < long_refs_count - i - 1; j++) {
      if (dpb[indexTemp_long[j]]->m_picture_frame.LongTermFrameIdx >
          dpb[indexTemp_long[j + 1]]
              ->m_picture_frame.LongTermFrameIdx) // 升序排列
      {
        int32_t temp = indexTemp_long[j];
        indexTemp_long[j] = indexTemp_long[j + 1];
        indexTemp_long[j + 1] = temp;
      }
    }
  }

  // 4. 生成排序后的参考序列
  j = 0;

  for (i = 0; i < short_refs_count_left; i++) {
    refFrameList0ShortTerm[j++] = dpb[indexTemp_short_left[i]];
  }

  for (i = 0; i < short_refs_count_right; i++) {
    refFrameList0ShortTerm[j++] = dpb[indexTemp_short_right[i]];
  }

  for (i = 0; i < long_refs_count; i++) {
    refFrameListLongTerm[j++] = dpb[indexTemp_long[i]];
  }

  //-------------RefPicList1----------------
  j = 0;

  for (i = 0; i < short_refs_count_right; i++) {
    refFrameList1ShortTerm[j++] = dpb[indexTemp_short_right[i]];
  }

  for (i = 0; i < short_refs_count_left; i++) {
    refFrameList1ShortTerm[j++] = dpb[indexTemp_short_left[i]];
  }

  //-------------------------
  // 8.2.4.2.5 Initialisation process for reference picture lists in fields
  // TODO 严重注意，这里要记得关闭注释，这里是暂时性注释掉 <24-08-31 00:51:59, YangJing>
  //ret = init_reference_picture_lists_in_fields(
  //refFrameList0ShortTerm, refFrameListLongTerm, RefPicList0,
  //RefPicList0Length, 0);
  RETURN_IF_FAILED(ret != 0, -1);

  // 8.2.4.2.5 Initialisation process for reference picture lists in fields
  // TODO 严重注意，这里要记得关闭注释，这里是暂时性注释掉 <24-08-31 00:51:59, YangJing>
  //ret = init_reference_picture_lists_in_fields(
  //refFrameList1ShortTerm, refFrameListLongTerm, RefPicList1,
  //RefPicList0Length, 1);
  RETURN_IF_FAILED(ret != 0, -1);

  //--------------------------
  // When the reference picture list RefPicList1 has more than one entry and
  // RefPicList1 is identical to the reference picture list RefPicList0, the
  // first two entries RefPicList1[ 0 ] and RefPicList1[ 1 ] are switched
  int flag = 0;
  if (short_refs_count_left + short_refs_count_right + long_refs_count > 1) {
    for (i = 0; i < size_dpb; i++) {
      if (RefPicList1[i] !=
          RefPicList0[i]) // FIXME: RefPicList1 is identical to the reference
                          // picture list RefPicList0
      {
        flag = 1;
        break;
      }
    }
  }

  if (flag == 0) // the first two entries RefPicList1[ 0 ] and RefPicList1[ 1 ]
                 // are switched
  {
    Frame *tmp = RefPicList1[0];
    RefPicList1[0] = RefPicList1[1];
    RefPicList1[1] = tmp;
  }

  return 0;
}

// 8.2.4.2.5 Initialisation process for reference picture lists in fields
int PictureBase::init_reference_picture_lists_in_fields(
    vector<Frame *>(&refFrameListXShortTerm),
    vector<Frame *>(&refFrameListXLongTerm), Frame *(&RefPicListX)[16],
    int32_t &RefPicListXLength, int32_t listX) {
  int32_t size = 16;
  int32_t i = 0;
  int32_t j = 0;
  int32_t index = 0;

  H264_PICTURE_CODED_TYPE coded_type = m_picture_coded_type;

  //-----------------------------
  for (i = 0; i < size; i++) {
    if (refFrameListXShortTerm[i]->m_picture_top_filed.m_picture_coded_type ==
        coded_type) // starting with a field that has the same parity as the
                    // current field (when present).
    {
      RefPicListX[index] = refFrameListXShortTerm[i];
      RefPicListX[index]->m_picture_coded_type_marked_as_refrence = coded_type;
      RefPicListX[index]->reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_short_term_reference;
      coded_type = (coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD)
                       ? H264_PICTURE_CODED_TYPE_BOTTOM_FIELD
                       : H264_PICTURE_CODED_TYPE_TOP_FIELD;
      index++;
      j = i;
    }

    if (refFrameListXShortTerm[i]
            ->m_picture_bottom_filed.m_picture_coded_type ==
        coded_type) // starting with a field that has the same parity as the
                    // current field (when present).
    {
      RefPicListX[index] = refFrameListXShortTerm[i];
      RefPicListX[index]->m_picture_coded_type_marked_as_refrence = coded_type;
      RefPicListX[index]->reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_short_term_reference;
      coded_type = (coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD)
                       ? H264_PICTURE_CODED_TYPE_BOTTOM_FIELD
                       : H264_PICTURE_CODED_TYPE_TOP_FIELD;
      index++;
      j = i;
    }
  }

  // FIXME:
  // When there are no more short-term reference fields of the alternate parity
  // in the ordered list of frames refFrameListXShortTerm, the next not yet
  // indexed fields of the available parity are inserted into RefPicListX in the
  // order in which they occur in the ordered list of frames
  // refFrameListXShortTerm.
  for (i = j + 1; i < size; i++) {
    if (refFrameListXShortTerm[i]->m_picture_top_filed.m_picture_coded_type ==
        H264_PICTURE_CODED_TYPE_TOP_FIELD) {
      RefPicListX[index] = refFrameListXShortTerm[i];
      RefPicListX[index]->m_picture_coded_type_marked_as_refrence =
          H264_PICTURE_CODED_TYPE_TOP_FIELD;
      RefPicListX[index]->reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_short_term_reference;
      index++;
    }

    if (refFrameListXShortTerm[i]
            ->m_picture_bottom_filed.m_picture_coded_type ==
        H264_PICTURE_CODED_TYPE_BOTTOM_FIELD) {
      RefPicListX[index] = refFrameListXShortTerm[i];
      RefPicListX[index]->m_picture_coded_type_marked_as_refrence =
          H264_PICTURE_CODED_TYPE_BOTTOM_FIELD;
      RefPicListX[index]->reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_short_term_reference;
      index++;
    }
  }

  //-----------------------------
  coded_type = m_picture_coded_type;
  j = 0;

  for (i = 0; i < size; i++) {
    if (refFrameListXLongTerm[i]->m_picture_top_filed.m_picture_coded_type ==
        coded_type) // starting with a field that has the same parity as the
                    // current field (when present).
    {
      RefPicListX[index] = refFrameListXLongTerm[i];
      RefPicListX[index]->m_picture_coded_type_marked_as_refrence = coded_type;
      RefPicListX[index]->reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_long_term_reference;
      coded_type = (coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD)
                       ? H264_PICTURE_CODED_TYPE_BOTTOM_FIELD
                       : H264_PICTURE_CODED_TYPE_TOP_FIELD;
      index++;
      j = i;
    }

    if (refFrameListXLongTerm[i]->m_picture_bottom_filed.m_picture_coded_type ==
        coded_type) // starting with a field that has the same parity as the
                    // current field (when present).
    {
      RefPicListX[index] = refFrameListXLongTerm[i];
      RefPicListX[index]->m_picture_coded_type_marked_as_refrence = coded_type;
      RefPicListX[index]->reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_long_term_reference;
      coded_type = (coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD)
                       ? H264_PICTURE_CODED_TYPE_BOTTOM_FIELD
                       : H264_PICTURE_CODED_TYPE_TOP_FIELD;
      index++;
      j = i;
    }
  }

  // FIXME:
  // When there are no more long-term reference fields of the alternate parity
  // in the ordered list of frames refFrameListLongTerm, the next not yet
  // indexed fields of the available parity are inserted into RefPicListX in the
  // order in which they occur in the ordered list of frames
  // refFrameListLongTerm.
  for (i = j + 1; i < size; i++) {
    if (refFrameListXLongTerm[i]->m_picture_top_filed.m_picture_coded_type ==
        H264_PICTURE_CODED_TYPE_TOP_FIELD) {
      RefPicListX[index] = refFrameListXLongTerm[i];
      RefPicListX[index]->m_picture_coded_type_marked_as_refrence =
          H264_PICTURE_CODED_TYPE_TOP_FIELD;
      RefPicListX[index]->reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_long_term_reference;
      index++;
    }

    if (refFrameListXLongTerm[i]->m_picture_bottom_filed.m_picture_coded_type ==
        H264_PICTURE_CODED_TYPE_BOTTOM_FIELD) {
      RefPicListX[index] = refFrameListXLongTerm[i];
      RefPicListX[index]->m_picture_coded_type_marked_as_refrence =
          H264_PICTURE_CODED_TYPE_BOTTOM_FIELD;
      RefPicListX[index]->reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_long_term_reference;
      index++;
    }
  }

  RefPicListXLength = index;

  return 0;
}

// 8.2.4.3 Modification process for reference picture lists
// 参考图像列表的重排序过程
int PictureBase::modif_reference_picture_lists(Frame *(&RefPicList0)[16],
                                               Frame *(&RefPicList1)[16]) {
  int ret = 0;
  SliceHeader &slice_header = m_slice.slice_header;

  if (slice_header.ref_pic_list_modification_flag_l0 == 1) {
    slice_header.refIdxL0 = 0;

    for (int i = 0; i < slice_header.ref_pic_list_modification_count_l0; i++) {
      if (slice_header.modification_of_pic_nums_idc[0][i] == 0 ||
          slice_header.modification_of_pic_nums_idc[0][i] == 1) {
        ret =
            Modification_process_of_reference_picture_lists_for_short_term_reference_pictures(
                slice_header.refIdxL0, slice_header.picNumL0Pred,
                slice_header.modification_of_pic_nums_idc[0][i],
                slice_header.abs_diff_pic_num_minus1[0][i],
                slice_header.num_ref_idx_l0_active_minus1, RefPicList0);
        RETURN_IF_FAILED(ret != 0, ret);
      } else if (slice_header.modification_of_pic_nums_idc[0][i] == 2) {
        ret =
            Modification_process_of_reference_picture_lists_for_long_term_reference_pictures(
                slice_header.refIdxL0, slice_header.picNumL0Pred,
                slice_header.num_ref_idx_l0_active_minus1,
                slice_header.long_term_pic_num[0][i], RefPicList0);
        RETURN_IF_FAILED(ret != 0, ret);
      } else // if (slice_header.modification_of_pic_nums_idc[0][i] == 3)
      {
        break; // the modification process for reference picture list
               // RefPicList0 is finished.
      }
    }
  }

  if (slice_header.slice_type == SLICE_B &&
      slice_header.ref_pic_list_modification_flag_l1 == 1) {
    slice_header.refIdxL1 = 0;

    for (int i = 0; i < slice_header.ref_pic_list_modification_count_l1; i++) {
      if (slice_header.modification_of_pic_nums_idc[1][i] == 0 ||
          slice_header.modification_of_pic_nums_idc[1][i] == 1) {
        ret =
            Modification_process_of_reference_picture_lists_for_short_term_reference_pictures(
                slice_header.refIdxL1, slice_header.picNumL1Pred,
                slice_header.modification_of_pic_nums_idc[1][i],
                slice_header.abs_diff_pic_num_minus1[1][i],
                slice_header.num_ref_idx_l1_active_minus1, RefPicList1);
        RETURN_IF_FAILED(ret != 0, ret);
      } else if (slice_header.modification_of_pic_nums_idc[1][i] == 2) {
        ret =
            Modification_process_of_reference_picture_lists_for_long_term_reference_pictures(
                slice_header.refIdxL1, slice_header.picNumL1Pred,
                slice_header.num_ref_idx_l1_active_minus1,
                slice_header.long_term_pic_num[1][i], RefPicList1);
        RETURN_IF_FAILED(ret != 0, ret);
      } else // if (slice_header.modification_of_pic_nums_idc[1][i] == 3)
      {
        break; // he modification process for reference picture list RefPicList1
               // is finished.
      }
    }
  }

  return 0;
}

// 8.2.4.3.1 Modification process of reference picture lists for short-term
// reference pictures
int PictureBase::
    Modification_process_of_reference_picture_lists_for_short_term_reference_pictures(
        int32_t &refIdxLX, int32_t &picNumLXPred,
        int32_t modification_of_pic_nums_idc, int32_t abs_diff_pic_num_minus1,
        int32_t num_ref_idx_lX_active_minus1, Frame *(&RefPicListX)[16]) {
  int32_t picNumLXNoWrap = 0;
  int32_t picNumLX = 0;
  int32_t cIdx = 0;
  int32_t nIdx = 0;

  SliceHeader &slice_header = m_slice.slice_header;

  if (modification_of_pic_nums_idc == 0) {
    if (picNumLXPred - (abs_diff_pic_num_minus1 + 1) < 0) {
      picNumLXNoWrap =
          picNumLXPred - (abs_diff_pic_num_minus1 + 1) + slice_header.MaxPicNum;
    } else {
      picNumLXNoWrap = picNumLXPred - (abs_diff_pic_num_minus1 + 1);
    }
  } else // if (modification_of_pic_nums_idc == 1)
  {
    if (picNumLXPred + (abs_diff_pic_num_minus1 + 1) >=
        slice_header.MaxPicNum) {
      picNumLXNoWrap =
          picNumLXPred + (abs_diff_pic_num_minus1 + 1) - slice_header.MaxPicNum;
    } else {
      picNumLXNoWrap = picNumLXPred + (abs_diff_pic_num_minus1 + 1);
    }
  }

  picNumLXPred = picNumLXNoWrap;

  //--------------------
  if (picNumLXNoWrap > (int32_t)slice_header.CurrPicNum) {
    picNumLX = picNumLXNoWrap - slice_header.MaxPicNum;
  } else {
    picNumLX = picNumLXNoWrap;
  }

  //--------------------
  for (cIdx = num_ref_idx_lX_active_minus1 + 1; cIdx > refIdxLX; cIdx--) {
    RefPicListX[cIdx] = RefPicListX[cIdx - 1];
  }

  //--------------------
  for (cIdx = 0; cIdx < num_ref_idx_lX_active_minus1 + 1; cIdx++) {
    if (RefPicListX[cIdx]->PicNum == picNumLX &&
        RefPicListX[cIdx]->reference_marked_type ==
            H264_PICTURE_MARKED_AS_used_for_short_term_reference) {
      break;
    }
  }

  RefPicListX[refIdxLX++] = RefPicListX[cIdx]; // short-term reference picture
                                               // with PicNum equal to picNumLX
  nIdx = refIdxLX;

  for (cIdx = refIdxLX; cIdx <= num_ref_idx_lX_active_minus1 + 1; cIdx++) {
    if (RefPicListX[cIdx] != NULL) {
      int32_t PicNumF = (RefPicListX[cIdx]->reference_marked_type ==
                         H264_PICTURE_MARKED_AS_used_for_short_term_reference)
                            ? RefPicListX[cIdx]->PicNum
                            : slice_header.MaxPicNum;
      if (PicNumF !=
          picNumLX) // if ( PicNumF( RefPicListX[ cIdx ] ) != picNumLX )
      {
        RefPicListX[nIdx++] = RefPicListX[cIdx];
      }
    }
  }

  // Within this pseudo-code procedure, the length of the list RefPicListX is
  // temporarily made one element longer than the length needed for the final
  // list. After the execution of this procedure, only elements 0 through
  // num_ref_idx_lX_active_minus1 of the list need to be retained.
  RefPicListX[num_ref_idx_lX_active_minus1 + 1] =
      NULL; // 列表最后一个元素是多余的

  return 0;
}

// 8.2.4.3.2 Modification process of reference picture lists for long-term
// reference pictures
int PictureBase::
    Modification_process_of_reference_picture_lists_for_long_term_reference_pictures(
        int32_t &refIdxLX, int32_t picNumLXPred,
        int32_t num_ref_idx_lX_active_minus1, int32_t long_term_pic_num,
        Frame *(&RefPicListX)[16]) {
  int32_t cIdx = 0;
  int32_t nIdx = 0;

  for (cIdx = num_ref_idx_lX_active_minus1 + 1; cIdx > refIdxLX; cIdx--) {
    RefPicListX[cIdx] = RefPicListX[cIdx - 1];
  }

  //--------------------
  for (cIdx = 0; cIdx < num_ref_idx_lX_active_minus1 + 1; cIdx++) {
    if (RefPicListX[cIdx]->LongTermPicNum == long_term_pic_num) {
      break;
    }
  }

  RefPicListX[refIdxLX++] =
      RefPicListX[cIdx]; // long-term reference picture with LongTermPicNum
                         // equal to long_term_pic_num
  nIdx = refIdxLX;

  for (cIdx = refIdxLX; cIdx <= num_ref_idx_lX_active_minus1 + 1; cIdx++) {
    int32_t LongTermPicNumF =
        (RefPicListX[cIdx]->reference_marked_type ==
         H264_PICTURE_MARKED_AS_used_for_long_term_reference)
            ? RefPicListX[cIdx]->LongTermPicNum
            : (2 * (MaxLongTermFrameIdx + 1));
    if (LongTermPicNumF != long_term_pic_num) {
      RefPicListX[nIdx++] = RefPicListX[cIdx];
    }
  }

  return 0;
}

// 8.2.5 Decoded reference picture marking process
// This process is invoked for decoded pictures when nal_ref_idc is not equal to
// 0.
int PictureBase::Decoded_reference_picture_marking_process(Frame *(&dpb)[16]) {
  int ret = 0;
  ret =
      Sequence_of_operations_for_decoded_reference_picture_marking_process(dpb);
  return ret;
}

// 8.2.5.1 Sequence of operations for decoded reference picture marking process
int PictureBase::
    Sequence_of_operations_for_decoded_reference_picture_marking_process(
        Frame *(&dpb)[16]) {
  int ret = 0;
  SliceHeader &slice_header = m_slice.slice_header;
  int32_t size_dpb = 16;

  // 1. All slices of the current picture are decoded.
  //    RETURN_IF_FAILED(m_is_decode_finished == 0, -1);

  // 2. Depending on whether the current picture is an IDR picture, the
  // following applies:
  if (slice_header.IdrPicFlag == 1) // IDR picture
  {
    // All reference pictures are marked as "unused for reference"
    for (int i = 0; i < size_dpb; i++) {
      dpb[i]->reference_marked_type =
          H264_PICTURE_MARKED_AS_unused_for_reference;
      dpb[i]->m_picture_coded_type_marked_as_refrence =
          H264_PICTURE_CODED_TYPE_UNKNOWN;
      if (dpb[i] != this->m_parent) {
        dpb[i]->m_picture_coded_type =
            H264_PICTURE_CODED_TYPE_UNKNOWN; // FIXME:
        // 需要先将之前解码的所有帧输出去
      }
    }

    if (slice_header.long_term_reference_flag == 0) // 标记自己为哪一种参考帧
    {
      reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_short_term_reference;
      MaxLongTermFrameIdx = NA; //"no long-term frame indices".
      m_parent->reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_short_term_reference;

      if (m_parent->m_picture_coded_type == H264_PICTURE_CODED_TYPE_FRAME) {
        m_parent->m_picture_coded_type_marked_as_refrence =
            H264_PICTURE_CODED_TYPE_FRAME;
      } else {
        m_parent->m_picture_coded_type_marked_as_refrence =
            H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR;
      }
    } else // if (slice_header.long_term_reference_flag == 1)
    {
      reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_long_term_reference;
      LongTermFrameIdx = 0;
      MaxLongTermFrameIdx = 0;
      m_parent->reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_long_term_reference;

      if (m_parent->m_picture_coded_type == H264_PICTURE_CODED_TYPE_FRAME) {
        m_parent->m_picture_coded_type_marked_as_refrence =
            H264_PICTURE_CODED_TYPE_FRAME;
      } else {
        m_parent->m_picture_coded_type_marked_as_refrence =
            H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR;
      }
    }
  } else // the current picture is not an IDR picture
  {
    if (slice_header.adaptive_ref_pic_marking_mode_flag == 0) {
      // 8.2.5.3 Sliding window decoded reference picture marking process
      // 滑动窗口解码参考图像的标识过程
      ret = Sliding_window_decoded_reference_picture_marking_process(dpb);
      RETURN_IF_FAILED(ret != 0, ret);
    } else // if (slice_header.adaptive_ref_pic_marking_mode_flag == 1)
    {
      // 8.2.5.4 Adaptive memory control decoded reference picture marking
      // process
      ret = Adaptive_memory_control_decoded_reference_picture_marking_process(
          dpb);
      RETURN_IF_FAILED(ret != 0, ret);
    }
  }

  // 3. When the current picture is not an IDR picture and it was not marked as
  // "used for long-term reference" by memory_management_control_operation equal
  // to 6, it is marked as "used for short-term reference".
  if (slice_header.IdrPicFlag != 1 // the current picture is not an IDR picture
      && memory_management_control_operation_6_flag != 6) {
    reference_marked_type =
        H264_PICTURE_MARKED_AS_used_for_short_term_reference;
    MaxLongTermFrameIdx = NA; //"no long-term frame indices".
    m_parent->reference_marked_type =
        H264_PICTURE_MARKED_AS_used_for_short_term_reference;

    if (m_parent->m_picture_coded_type == H264_PICTURE_CODED_TYPE_FRAME) {
      m_parent->m_picture_coded_type_marked_as_refrence =
          H264_PICTURE_CODED_TYPE_FRAME;
    } else {
      m_parent->m_picture_coded_type_marked_as_refrence =
          H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR;
    }
  }

  return 0;
}

// 8.2.5.2 Decoding process for gaps in frame_num
// This process is invoked when frame_num is not equal to PrevRefFrameNum and is
// not equal to ( PrevRefFrameNum + 1 ) % MaxFrameNum. NOTE 1 – Although this
// process is specified as a subclause within clause 8.2.5 (which defines a
// process that is invoked only when
//          nal_ref_idc is not equal to 0), this process may also be invoked
//          when nal_ref_idc is equal to 0 (as specified in clause 8). The
//          reasons for the location of this clause within the structure of this
//          Recommendation | International Standard are historical.
// NOTE 2 – This process can only be invoked for a conforming bitstream when
// gaps_in_frame_num_value_allowed_flag is equal to 1.
//          When gaps_in_frame_num_value_allowed_flag is equal to 0 and
//          frame_num is not equal to PrevRefFrameNum and is not equal to (
//          PrevRefFrameNum + 1 ) % MaxFrameNum, the decoding process should
//          infer an unintentional loss of pictures.
int PictureBase::Decoding_process_for_gaps_in_frame_num() { return 0; }

// 8.2.5.3 Sliding window decoded reference picture marking process
// 滑动窗口解码参考图像的标识过程 This process is invoked when
// adaptive_ref_pic_marking_mode_flag is equal to 0.
int PictureBase::Sliding_window_decoded_reference_picture_marking_process(
    Frame *(&dpb)[16]) {
  if (m_picture_coded_type ==
          H264_PICTURE_CODED_TYPE_BOTTOM_FIELD // If the current picture is a
      // coded field that is the second
      // field in decoding order of a
      // complementary reference field
      // pair
      &&
      (m_parent->m_picture_top_filed.reference_marked_type ==
       H264_PICTURE_MARKED_AS_used_for_short_term_reference)) // the first field
  // has been marked
  // as "used for
  // short-term
  // reference"
  {
    // the current picture and the complementary reference field pair are also
    // marked as "used for short-term reference".
    reference_marked_type =
        H264_PICTURE_MARKED_AS_used_for_short_term_reference;
    m_parent->reference_marked_type =
        H264_PICTURE_MARKED_AS_used_for_short_term_reference;
    m_parent->m_picture_coded_type_marked_as_refrence =
        H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR;
  } else {
    int32_t size_dpb = 16;
    int32_t numShortTerm = 0;
    int32_t numLongTerm = 0;

    for (int i = 0; i < size_dpb; i++) {
      if (dpb[i]->m_picture_frame.reference_marked_type ==
              H264_PICTURE_MARKED_AS_used_for_short_term_reference ||
          dpb[i]->m_picture_top_filed.reference_marked_type ==
              H264_PICTURE_MARKED_AS_used_for_short_term_reference ||
          dpb[i]->m_picture_bottom_filed.reference_marked_type ==
              H264_PICTURE_MARKED_AS_used_for_short_term_reference) {
        numShortTerm++;
      }

      if (dpb[i]->m_picture_frame.reference_marked_type ==
              H264_PICTURE_MARKED_AS_used_for_long_term_reference ||
          dpb[i]->m_picture_top_filed.reference_marked_type ==
              H264_PICTURE_MARKED_AS_used_for_long_term_reference ||
          dpb[i]->m_picture_bottom_filed.reference_marked_type ==
              H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
        numLongTerm++;
      }
    }

    if (numShortTerm + numLongTerm ==
            MAX(m_slice.m_sps.max_num_ref_frames, 1) &&
        numShortTerm > 0) {
      PictureBase *refPic = NULL;
      int32_t FrameNumWrap_smallest_index = -1;

      // When numShortTerm + numLongTerm is equal to Max( max_num_ref_frames, 1
      // ), the condition that numShortTerm is greater than 0 shall be
      // fulfilled, and the short-term reference frame, complementary reference
      // field pair or non-paired reference field that has the smallest value of
      // FrameNumWrap is marked as "unused for reference". When it is a frame or
      // a complementary field pair, both of its fields are also marked as
      // "unused for reference".
      for (int i = 0; i < size_dpb; i++) {
        if (dpb[i]->m_picture_frame.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_short_term_reference ||
            dpb[i]->m_picture_top_filed.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_short_term_reference ||
            dpb[i]->m_picture_bottom_filed.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_short_term_reference) {
          if (dpb[i]->m_picture_frame.reference_marked_type ==
              H264_PICTURE_MARKED_AS_used_for_short_term_reference) {
            if (FrameNumWrap_smallest_index == -1) {
              FrameNumWrap_smallest_index = i;
              refPic = &dpb[i]->m_picture_frame;
            }

            if (dpb[i]->m_picture_frame.FrameNumWrap <
                dpb[FrameNumWrap_smallest_index]
                    ->m_picture_frame.FrameNumWrap) {
              FrameNumWrap_smallest_index = i;
              refPic = &dpb[i]->m_picture_frame;
            }
          }

          if (dpb[i]->m_picture_top_filed.reference_marked_type ==
                  H264_PICTURE_MARKED_AS_used_for_short_term_reference ||
              dpb[i]->m_picture_top_filed.reference_marked_type ==
                  H264_PICTURE_MARKED_AS_used_for_short_term_reference) {
            if (FrameNumWrap_smallest_index == -1) {
              FrameNumWrap_smallest_index = i;
              refPic = &dpb[i]->m_picture_top_filed;
            }

            if (dpb[i]->m_picture_top_filed.FrameNumWrap <
                dpb[FrameNumWrap_smallest_index]
                    ->m_picture_top_filed.FrameNumWrap) {
              FrameNumWrap_smallest_index = i;
              refPic = &dpb[i]->m_picture_top_filed;
            }
          }

          if (dpb[i]->m_picture_bottom_filed.reference_marked_type ==
              H264_PICTURE_MARKED_AS_used_for_short_term_reference) {
            if (FrameNumWrap_smallest_index == -1) {
              FrameNumWrap_smallest_index = i;
              refPic = &dpb[i]->m_picture_bottom_filed;
            }

            if (dpb[i]->m_picture_bottom_filed.FrameNumWrap <
                dpb[FrameNumWrap_smallest_index]
                    ->m_picture_bottom_filed.FrameNumWrap) {
              FrameNumWrap_smallest_index = i;
              refPic = &dpb[i]->m_picture_bottom_filed;
            }
          }
        }
      }

      if (FrameNumWrap_smallest_index >= 0 && refPic != NULL) {
        refPic->reference_marked_type =
            H264_PICTURE_MARKED_AS_unused_for_reference;

        if (refPic->m_parent->m_picture_coded_type ==
            H264_PICTURE_CODED_TYPE_FRAME) {
          refPic->m_parent->reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
          refPic->m_parent->m_picture_coded_type_marked_as_refrence =
              H264_PICTURE_CODED_TYPE_UNKNOWN;
        } else if (refPic->m_parent->m_picture_coded_type ==
                   H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR) {
          refPic->m_parent->reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
          refPic->m_parent->m_picture_coded_type_marked_as_refrence =
              H264_PICTURE_CODED_TYPE_UNKNOWN;
          refPic->m_parent->m_picture_top_filed.reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
          refPic->m_parent->m_picture_bottom_filed.reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
        }
      }
    }
  }

  return 0;
}

// 8.2.5.4 Adaptive memory control decoded reference picture marking process
// This process is invoked when adaptive_ref_pic_marking_mode_flag is equal
// to 1.
int PictureBase::
    Adaptive_memory_control_decoded_reference_picture_marking_process(
        Frame *(&dpb)[16]) {
  SliceHeader &slice_header = m_slice.slice_header;

  int32_t size_dpb = 16;
  int32_t i = 0;
  int32_t j = 0;

  for (j = 0; j < slice_header.dec_ref_pic_marking_count; j++) {
    // The memory_management_control_operation command with value of 0 specifies
    // the end of memory_management_control_operation commands.
    if (slice_header.m_dec_ref_pic_marking[j]
            .memory_management_control_operation == 0) {
      break;
    }

    // 8.2.5.4.1 Marking process of a short-term reference picture as "unused
    // for reference" 将短期图像标记为“不用于参考”
    if (slice_header.m_dec_ref_pic_marking[j]
            .memory_management_control_operation == 1) {
      int32_t picNumX =
          slice_header.CurrPicNum -
          (slice_header.m_dec_ref_pic_marking[j].difference_of_pic_nums_minus1 +
           1);
      if (slice_header.field_pic_flag == 0) {
        for (i = 0; i < size_dpb; i++) {
          if ((dpb[i]->m_picture_frame.reference_marked_type ==
                   H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
               dpb[i]->m_picture_frame.PicNum == picNumX) ||
              (dpb[i]->m_picture_top_filed.reference_marked_type ==
                   H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
               dpb[i]->m_picture_top_filed.PicNum == picNumX) ||
              (dpb[i]->m_picture_bottom_filed.reference_marked_type ==
                   H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
               dpb[i]->m_picture_bottom_filed.PicNum == picNumX)) {
            dpb[i]->reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_frame.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_top_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_bottom_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
          }
        }
      } else // if (field_pic_flag == 1)
      {
        for (i = 0; i < size_dpb; i++) {
          if (dpb[i]->m_picture_top_filed.reference_marked_type ==
                  H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
              dpb[i]->m_picture_top_filed.PicNum == picNumX) {
            dpb[i]->reference_marked_type =
                dpb[i]->m_picture_bottom_filed.reference_marked_type;
            dpb[i]->m_picture_coded_type_marked_as_refrence =
                dpb[i]->m_picture_bottom_filed.m_picture_coded_type;
            dpb[i]->m_picture_frame.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_top_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
          } else if (dpb[i]->m_picture_bottom_filed.reference_marked_type ==
                         H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
                     dpb[i]->m_picture_bottom_filed.PicNum == picNumX) {
            dpb[i]->reference_marked_type =
                dpb[i]->m_picture_top_filed.reference_marked_type;
            dpb[i]->m_picture_coded_type_marked_as_refrence =
                dpb[i]->m_picture_top_filed.m_picture_coded_type;
            dpb[i]->m_picture_frame.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_bottom_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
          }
        }
      }
    }

    // 8.2.5.4.2 Marking process of a long-term reference picture as "unused for
    // reference" 将长期图像标记为“不用于参考”
    else if (slice_header.m_dec_ref_pic_marking[j]
                 .memory_management_control_operation == 2) {
      if (slice_header.field_pic_flag == 0) {
        for (i = 0; i < size_dpb; i++) {
          if (dpb[i]->m_picture_frame.reference_marked_type ==
                  H264_PICTURE_MARKED_AS_used_for_long_term_reference &&
              dpb[i]->m_picture_frame.LongTermPicNum ==
                  slice_header.m_dec_ref_pic_marking[j].long_term_pic_num_2) {
            dpb[i]->reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_coded_type_marked_as_refrence =
                H264_PICTURE_CODED_TYPE_UNKNOWN;
            dpb[i]->m_picture_frame.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_top_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_bottom_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
          }
        }
      } else // if (field_pic_flag == 1) //FIXME: but the marking of the other
             // field in the same reference frame or complementary reference
             // field pair is not changed.
      {
        for (i = 0; i < size_dpb; i++) {
          if (dpb[i]->m_picture_top_filed.reference_marked_type ==
                  H264_PICTURE_MARKED_AS_used_for_long_term_reference &&
              dpb[i]->m_picture_top_filed.LongTermPicNum ==
                  slice_header.m_dec_ref_pic_marking[j].long_term_pic_num_2) {
            dpb[i]->reference_marked_type =
                dpb[i]->m_picture_bottom_filed.reference_marked_type;
            dpb[i]->m_picture_coded_type_marked_as_refrence =
                dpb[i]->m_picture_bottom_filed.m_picture_coded_type;
            dpb[i]->m_picture_frame.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_top_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
          } else if (dpb[i]->m_picture_bottom_filed.reference_marked_type ==
                         H264_PICTURE_MARKED_AS_used_for_long_term_reference &&
                     dpb[i]->m_picture_bottom_filed.LongTermPicNum ==
                         slice_header.m_dec_ref_pic_marking[j]
                             .long_term_pic_num_2) {
            dpb[i]->reference_marked_type =
                dpb[i]->m_picture_top_filed.reference_marked_type;
            dpb[i]->m_picture_coded_type_marked_as_refrence =
                dpb[i]->m_picture_top_filed.m_picture_coded_type;
            dpb[i]->m_picture_frame.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_bottom_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
          }
        }
      }
    }

    // 8.2.5.4.3 Assignment process of a LongTermFrameIdx to a short-term
    // reference picture 分配LongTermFrameIdx给一个短期参考图像
    else if (slice_header.m_dec_ref_pic_marking[j]
                 .memory_management_control_operation == 3) {
      int32_t picNumX =
          slice_header.CurrPicNum -
          (slice_header.m_dec_ref_pic_marking[j].difference_of_pic_nums_minus1 +
           1);

      // When LongTermFrameIdx equal to long_term_frame_idx is already assigned
      // to a long-term reference frame or a long-term complementary reference
      // field pair, that frame or complementary field pair and both of its
      // fields are marked as "unused for reference".
      for (i = 0; i < size_dpb; i++) {
        if (dpb[i]->m_picture_frame.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_long_term_reference &&
            dpb[i]->m_picture_frame.LongTermFrameIdx ==
                slice_header.m_dec_ref_pic_marking[j].long_term_frame_idx) {
          dpb[i]->reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
          dpb[i]->m_picture_coded_type_marked_as_refrence =
              H264_PICTURE_CODED_TYPE_UNKNOWN;
          dpb[i]->m_picture_frame.reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
          dpb[i]->m_picture_top_filed.reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
          dpb[i]->m_picture_bottom_filed.reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
        }
      }

      // When LongTermFrameIdx is already assigned to a reference field, and
      // that reference field is not part of a complementary field pair that
      // includes the picture specified by picNumX, that field is marked as
      // "unused for reference".
      int32_t picNumXIndex = -1;
      for (i = 0; i < size_dpb; i++) {
        if (dpb[i]->m_picture_top_filed.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
            dpb[i]->m_picture_top_filed.PicNum == picNumX) {
          picNumXIndex = i;
          break;
        } else if (dpb[i]->m_picture_bottom_filed.reference_marked_type ==
                       H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
                   dpb[i]->m_picture_bottom_filed.PicNum == picNumX) {
          picNumXIndex = i;
          break;
        }
      }

      for (i = 0; i < size_dpb; i++) {
        if (dpb[i]->m_picture_top_filed.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_long_term_reference &&
            dpb[i]->m_picture_top_filed.LongTermFrameIdx ==
                slice_header.m_dec_ref_pic_marking[j].long_term_frame_idx) {
          if (i != picNumXIndex) {
            dpb[i]->reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_coded_type_marked_as_refrence =
                dpb[i]->m_picture_bottom_filed.m_picture_coded_type;
            dpb[i]->m_picture_frame.reference_marked_type =
                dpb[i]->m_picture_bottom_filed.reference_marked_type;
            dpb[i]->m_picture_top_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
          }
        } else if (dpb[i]->m_picture_bottom_filed.reference_marked_type ==
                       H264_PICTURE_MARKED_AS_used_for_long_term_reference &&
                   dpb[i]->m_picture_bottom_filed.LongTermFrameIdx ==
                       slice_header.m_dec_ref_pic_marking[j]
                           .long_term_frame_idx) {
          if (i != picNumXIndex) {
            dpb[i]->reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_coded_type_marked_as_refrence =
                dpb[i]->m_picture_top_filed.m_picture_coded_type;
            dpb[i]->m_picture_frame.reference_marked_type =
                dpb[i]->m_picture_top_filed.reference_marked_type;
            dpb[i]->m_picture_bottom_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
          }
        }
      }

      //-----------------------------------------
      if (slice_header.field_pic_flag == 0) {
        for (i = 0; i < size_dpb; i++) {
          if (dpb[i]->m_picture_frame.reference_marked_type ==
                  H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
              dpb[i]->m_picture_frame.PicNum == picNumX) {
            dpb[i]->reference_marked_type =
                H264_PICTURE_MARKED_AS_used_for_long_term_reference;
            dpb[i]->m_picture_coded_type_marked_as_refrence =
                H264_PICTURE_CODED_TYPE_FRAME;
            dpb[i]->m_picture_frame.reference_marked_type =
                H264_PICTURE_MARKED_AS_used_for_long_term_reference;

            if (dpb[i]->m_picture_coded_type ==
                H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR) {
              dpb[i]->m_picture_top_filed.reference_marked_type =
                  H264_PICTURE_MARKED_AS_used_for_long_term_reference;
              dpb[i]->m_picture_bottom_filed.reference_marked_type =
                  H264_PICTURE_MARKED_AS_used_for_long_term_reference;
            }

            LongTermFrameIdx =
                slice_header.m_dec_ref_pic_marking[j].long_term_frame_idx;
          }
        }
      } else // if (field_pic_flag == 1) //FIXME: What meaning 'When the field
      // is part of a reference frame or a complementary reference field
      // pair'
      {
        for (i = 0; i < size_dpb; i++) {
          if (dpb[i]->m_picture_top_filed.reference_marked_type ==
                  H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
              dpb[i]->m_picture_top_filed.PicNum == picNumX) {
            dpb[i]->m_picture_top_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_used_for_long_term_reference;

            if (dpb[i]->m_picture_bottom_filed.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
              dpb[i]->reference_marked_type =
                  H264_PICTURE_MARKED_AS_used_for_long_term_reference;

              if (dpb[i]->m_picture_frame.m_picture_coded_type ==
                  H264_PICTURE_CODED_TYPE_FRAME) {
                dpb[i]->m_picture_coded_type_marked_as_refrence =
                    H264_PICTURE_CODED_TYPE_FRAME;
              } else if (dpb[i]->m_picture_frame.m_picture_coded_type ==
                         H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR) {
                dpb[i]->m_picture_coded_type_marked_as_refrence =
                    H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR;
              }

              dpb[i]->m_picture_frame.reference_marked_type =
                  H264_PICTURE_MARKED_AS_used_for_long_term_reference;
              dpb[i]->m_picture_bottom_filed.LongTermFrameIdx =
                  slice_header.m_dec_ref_pic_marking[j].long_term_frame_idx;
            }

            dpb[i]->m_picture_top_filed.LongTermFrameIdx =
                slice_header.m_dec_ref_pic_marking[j].long_term_frame_idx;
          } else if (dpb[i]->m_picture_bottom_filed.reference_marked_type ==
                         H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
                     dpb[i]->m_picture_bottom_filed.PicNum == picNumX) {
            dpb[i]->m_picture_bottom_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_used_for_long_term_reference;

            if (dpb[i]->m_picture_top_filed.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
              dpb[i]->reference_marked_type =
                  H264_PICTURE_MARKED_AS_used_for_long_term_reference;

              if (dpb[i]->m_picture_frame.m_picture_coded_type ==
                  H264_PICTURE_CODED_TYPE_FRAME) {
                dpb[i]->m_picture_coded_type_marked_as_refrence =
                    H264_PICTURE_CODED_TYPE_FRAME;
              } else if (dpb[i]->m_picture_frame.m_picture_coded_type ==
                         H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR) {
                dpb[i]->m_picture_coded_type_marked_as_refrence =
                    H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR;
              }

              dpb[i]->m_picture_frame.reference_marked_type =
                  H264_PICTURE_MARKED_AS_used_for_long_term_reference;
              dpb[i]->m_picture_top_filed.LongTermFrameIdx =
                  slice_header.m_dec_ref_pic_marking[j].long_term_frame_idx;
            }

            dpb[i]->m_picture_bottom_filed.LongTermFrameIdx =
                slice_header.m_dec_ref_pic_marking[j].long_term_frame_idx;
          }
        }
      }
    }

    // 8.2.5.4.4 Decoding process for MaxLongTermFrameIdx
    // 基于MaxLongTermFrameIdx的标记过程
    else if (slice_header.m_dec_ref_pic_marking[j]
                 .memory_management_control_operation == 4) {
      // All pictures for which LongTermFrameIdx is greater than
      // max_long_term_frame_idx_plus1 − 1 and that are marked as "used for
      // long-term reference" are marked as "unused for reference".
      for (i = 0; i < size_dpb; i++) {
        if (dpb[i]->m_picture_frame.LongTermFrameIdx >
                (int)slice_header.m_dec_ref_pic_marking[j]
                        .max_long_term_frame_idx_plus1 -
                    1 &&
            dpb[i]->m_picture_frame.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
          dpb[i]->m_picture_frame.reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
        }

        if (dpb[i]->m_picture_top_filed.LongTermFrameIdx >
                (int)slice_header.m_dec_ref_pic_marking[j]
                        .max_long_term_frame_idx_plus1 -
                    1 &&
            dpb[i]->m_picture_top_filed.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
          dpb[i]->m_picture_top_filed.reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
        }

        if (dpb[i]->m_picture_bottom_filed.LongTermFrameIdx >
                (int)slice_header.m_dec_ref_pic_marking[j]
                        .max_long_term_frame_idx_plus1 -
                    1 &&
            dpb[i]->m_picture_bottom_filed.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
          dpb[i]->m_picture_bottom_filed.reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
        }

        if (dpb[i]->m_picture_coded_type == H264_PICTURE_CODED_TYPE_FRAME ||
            dpb[i]->m_picture_coded_type ==
                H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR) {
          dpb[i]->m_picture_coded_type_marked_as_refrence =
              H264_PICTURE_CODED_TYPE_UNKNOWN;
          dpb[i]->reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
        }
      }

      if (slice_header.m_dec_ref_pic_marking[j].max_long_term_frame_idx_plus1 ==
          0) {
        MaxLongTermFrameIdx = -1; //"no long-term frame indices"
      } else                      // if (max_long_term_frame_idx_plus1 > 0)
      {
        MaxLongTermFrameIdx = slice_header.m_dec_ref_pic_marking[j]
                                  .max_long_term_frame_idx_plus1 -
                              1;
      }
    }

    // 8.2.5.4.5 Marking process of all reference pictures as "unused for
    // reference" and setting MaxLongTermFrameIdx to "no long-term frame
    // indices" 所有参考图像标记为“不用于参考”
    else if (slice_header.m_dec_ref_pic_marking[j]
                 .memory_management_control_operation == 5) {
      for (i = 0; i < size_dpb; i++) {
        dpb[i]->m_picture_coded_type_marked_as_refrence =
            H264_PICTURE_CODED_TYPE_UNKNOWN;
        dpb[i]->reference_marked_type =
            H264_PICTURE_MARKED_AS_unused_for_reference;
        dpb[i]->m_picture_frame.reference_marked_type =
            H264_PICTURE_MARKED_AS_unused_for_reference;
        dpb[i]->m_picture_top_filed.reference_marked_type =
            H264_PICTURE_MARKED_AS_unused_for_reference;
        dpb[i]->m_picture_bottom_filed.reference_marked_type =
            H264_PICTURE_MARKED_AS_unused_for_reference;
      }

      MaxLongTermFrameIdx = NA;
      memory_management_control_operation_5_flag = 1;
    }

    // 8.2.5.4.6 Process for assigning a long-term frame index to the current
    // picture 分配一个长期帧索引给当前图像
    else if (slice_header.m_dec_ref_pic_marking[j]
                 .memory_management_control_operation == 6) {
      //int32_t top_field_index = -1;

      for (i = 0; i < size_dpb; i++) {
        // When a variable LongTermFrameIdx equal to long_term_frame_idx is
        // already assigned to a long-term reference frame or a long-term
        // complementary reference field pair, that frame or complementary field
        // pair and both of its fields are marked as "unused for reference".
        if (dpb[i]->m_picture_frame.LongTermFrameIdx ==
                (int)slice_header.m_dec_ref_pic_marking[j]
                        .max_long_term_frame_idx_plus1 -
                    1 &&
            dpb[i]->m_picture_frame.reference_marked_type ==
                H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
          dpb[i]->m_picture_frame.reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
          dpb[i]->reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
          dpb[i]->m_picture_coded_type_marked_as_refrence =
              H264_PICTURE_CODED_TYPE_UNKNOWN;

          if (dpb[i]->m_picture_coded_type ==
              H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR) {
            dpb[i]->m_picture_top_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
            dpb[i]->m_picture_bottom_filed.reference_marked_type =
                H264_PICTURE_MARKED_AS_unused_for_reference;
          }
        }

        // When LongTermFrameIdx is already assigned to a reference field, and
        // that reference field is not part of a complementary field pair that
        // includes the current picture, that field is marked as "unused for
        // reference".
        if (dpb[i]->m_picture_top_filed.LongTermFrameIdx == LongTermFrameIdx &&
            m_parent->m_picture_coded_type ==
                H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR &&
            (dpb[i] != m_parent)) {
          dpb[i]->reference_marked_type =
              dpb[i]->m_picture_bottom_filed.reference_marked_type;
          dpb[i]->m_picture_coded_type_marked_as_refrence =
              dpb[i]->m_picture_bottom_filed.m_picture_coded_type;
          dpb[i]->m_picture_top_filed.reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
        } else if (dpb[i]->m_picture_bottom_filed.LongTermFrameIdx ==
                       LongTermFrameIdx &&
                   m_parent->m_picture_coded_type ==
                       H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR &&
                   (dpb[i] != m_parent)) {
          dpb[i]->reference_marked_type =
              dpb[i]->m_picture_top_filed.reference_marked_type;
          dpb[i]->m_picture_coded_type_marked_as_refrence =
              dpb[i]->m_picture_top_filed.m_picture_coded_type;
          dpb[i]->m_picture_bottom_filed.reference_marked_type =
              H264_PICTURE_MARKED_AS_unused_for_reference;
        }
      }

      reference_marked_type =
          H264_PICTURE_MARKED_AS_used_for_long_term_reference;
      LongTermFrameIdx =
          slice_header.m_dec_ref_pic_marking[j].long_term_frame_idx;
      memory_management_control_operation_6_flag = 6;

      // When field_pic_flag is equal to 0, both its fields are also marked as
      // "used for long-term reference" and assigned LongTermFrameIdx equal to
      // long_term_frame_idx.
      if (slice_header.field_pic_flag == 0) {
        // FIXME: what meaning 'both its fields'?
        if (m_parent->m_picture_coded_type ==
            H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR) {
          m_parent->m_picture_top_filed.reference_marked_type =
              H264_PICTURE_MARKED_AS_used_for_long_term_reference;
          m_parent->m_picture_top_filed.LongTermFrameIdx =
              slice_header.m_dec_ref_pic_marking[j].long_term_frame_idx;
          m_parent->m_picture_bottom_filed.reference_marked_type =
              H264_PICTURE_MARKED_AS_used_for_long_term_reference;
          m_parent->m_picture_bottom_filed.LongTermFrameIdx =
              slice_header.m_dec_ref_pic_marking[j].long_term_frame_idx;
        }
      }

      // When field_pic_flag is equal to 1 and the current picture is the second
      // field (in decoding order) of a complementary reference field pair, and
      // the first field of the complementary reference field pair is also
      // currently marked as "used for long-term reference", the complementary
      // reference field pair is also marked as "used for long-term reference"
      // and assigned LongTermFrameIdx equal to long_term_frame_idx.
      if (slice_header.field_pic_flag == 1 &&
          m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD &&
          m_parent->m_picture_top_filed.reference_marked_type ==
              H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
        m_parent->reference_marked_type =
            H264_PICTURE_MARKED_AS_used_for_long_term_reference;
        LongTermFrameIdx =
            slice_header.m_dec_ref_pic_marking[j].long_term_frame_idx;
      }

      // After marking the current decoded reference picture, the total number
      // of frames with at least one field marked as "used for reference", plus
      // the number of complementary field pairs with at least one field marked
      // as "used for reference", plus the number of non-paired fields marked as
      // "used for reference" shall not be greater than Max( max_num_ref_frames,
      // 1 ).
    }
  }

  return 0;
}
