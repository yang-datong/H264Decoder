#include "Frame.hpp"
#include "PictureBase.hpp"
#include "SliceHeader.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

//--------------参考帧列表重排序------------------------

// 8.2.1 Decoding process for picture order count
/* 此过程的输出为 TopFieldOrderCnt（如果适用）和 BottomFieldOrderCnt（如果适用）*/
int PictureBase::decoding_picture_order_count() {
  /* 为每个帧、场（无论是从编码场解码还是作为解码帧的一部分）或互补场对导出图像顺序计数信息，如下所示： 
   * – 每个编码帧与两个图像顺序计数相关联，称为 TopFieldOrderCnt 和BottomFieldOrderCnt 分别表示其顶部字段和底部字段。  
   * – 每个编码字段都与图片顺序计数相关联，对于编码顶部字段称为 TopFieldOrderCnt，对于底部字段称为 BottomFieldOrderCnt。  
   * – 每个互补字段对与两个图像顺序计数相关联，分别是其编码的顶部字段的 TopFieldOrderCnt 和其编码的底部字段的 BottomFieldOrderCnt。 */

  /* 比特流不应包含导致解码过程中使用的 TopFieldOrderCnt、BottomFieldOrderCnt、PicOrderCntMsb 或 FrameNumOffset 值（如第 8.2.1.1 至 8.2.1.3 条规定）超出从 -231 到 231 - 1（含）的值范围的数据。 */
  int ret = 0;
  if (m_slice.m_sps.pic_order_cnt_type == 0)
    //8.2.1.1
    ret = decoding_picture_order_count_type_0(m_parent->m_picture_previous_ref);
  else if (m_slice.m_sps.pic_order_cnt_type == 1)
    //8.2.1.2
    ret = decoding_picture_order_count_type_1(m_parent->m_picture_previous);
  else if (m_slice.m_sps.pic_order_cnt_type == 2)
    //8.2.1.3
    ret = decoding_picture_order_count_type_2(m_parent->m_picture_previous);

  if (ret != 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return ret;
  }
  /* 函数 PicOrderCnt( picX ) 指定如下： */
  PicOrderCntFunc(this);
  return 0;
}

int PictureBase::PicOrderCntFunc(PictureBase *picX) {
  if (picX->m_picture_coded_type == H264_PICTURE_CODED_TYPE_FRAME ||
      picX->m_picture_coded_type ==
          H264_PICTURE_CODED_TYPE_COMPLEMENTARY_FIELD_PAIR)
    //当前图像为帧、互补场对
    picX->PicOrderCnt = MIN(picX->TopFieldOrderCnt, picX->BottomFieldOrderCnt);
  else if (picX->m_picture_coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD)
    // 当前图像为顶场
    picX->PicOrderCnt = picX->TopFieldOrderCnt;
  else if (picX->m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD)
    // 当前图像为底场
    picX->PicOrderCnt = picX->BottomFieldOrderCnt;

  return picX->PicOrderCnt;
}

// 8.2.1.1 Decoding process for picture order count type 0
/* 当 pic_order_cnt_type 等于 0 时调用此过程。*/
/* 输入: 按照本节中指定的解码顺序的先前参考图片的PicOrderCntMsb。  
 * 输出: TopFieldOrderCnt 或 BottomFieldOrderCnt 之一或两者。 */
int PictureBase::decoding_picture_order_count_type_0(
    const PictureBase *picture_previous_ref) {

  const SliceHeader &header = m_slice.slice_header;

  uint32_t prevPicOrderCntMsb, prevPicOrderCntLsb;
  /* 变量 prevPicOrderCntMsb 和 prevPicOrderCntLsb 的推导如下： 
   * – 如果当前图片是 IDR 图片，则 prevPicOrderCntMsb 设置为等于 0，并且 prevPicOrderCntLsb 设置为等于 0。 */
  /* 否则（当前图片不是 IDR 图片），则适用以下规则： */
  /* – 如果解码顺序中的前一个参考图片包含等于 5 的 memory_management_control_operation，则适用以下规则： */
  /* – 如果解码顺序中的前一个参考图片不是底场， prevPicOrderCntMsb 被设置为等于 0，并且 prevPicOrderCntLsb 被设置为等于按照解码顺序的前一个参考图片的 TopFieldOrderCnt 的值。  */
  /* – 否则（解码顺序中的前一个参考图像是底场），prevPicOrderCntMsb 设置为等于 0，并且 prevPicOrderCntLsb 设置为等于 0。 */
  /* – 否则（解码顺序中的前一个参考图片不包括等于5的memory_management_control_operation），prevPicOrderCntMsb设置为等于解码顺序中的前一个参考图片的PicOrderCntMsb，并且设置prevPicOrderCntLsb等于解码顺序中前一个参考图片的 pic_order_cnt_lsb 的值。 */
  if (header.IdrPicFlag == 1)
    prevPicOrderCntMsb = prevPicOrderCntLsb = 0;
  else if (picture_previous_ref) {
    if (picture_previous_ref->memory_management_control_operation_5_flag) {
      if (picture_previous_ref->m_picture_coded_type !=
          H264_PICTURE_CODED_TYPE_BOTTOM_FIELD) {
        prevPicOrderCntMsb = 0;
        prevPicOrderCntLsb = picture_previous_ref->TopFieldOrderCnt;
      } else
        prevPicOrderCntMsb = prevPicOrderCntLsb = 0;
    } else {
      prevPicOrderCntMsb = picture_previous_ref->PicOrderCntMsb;
      prevPicOrderCntLsb =
          picture_previous_ref->m_slice.slice_header.pic_order_cnt_lsb;
    }
  } else {
    /* 没有参考帧 */
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

  /* 当前图片的 PicOrderCntMsb 是由以下伪代码指定的： */
  if ((header.pic_order_cnt_lsb < prevPicOrderCntLsb) &&
      ((prevPicOrderCntLsb - header.pic_order_cnt_lsb) >=
       (m_slice.m_sps.MaxPicOrderCntLsb / 2)))
    PicOrderCntMsb = prevPicOrderCntMsb + m_slice.m_sps.MaxPicOrderCntLsb;

  else if ((header.pic_order_cnt_lsb > prevPicOrderCntLsb) &&
           ((header.pic_order_cnt_lsb - prevPicOrderCntLsb) >
            (m_slice.m_sps.MaxPicOrderCntLsb / 2)))
    PicOrderCntMsb = prevPicOrderCntMsb - m_slice.m_sps.MaxPicOrderCntLsb;

  else
    PicOrderCntMsb = prevPicOrderCntMsb;

  /* 当前图片不是底场时，TopFieldOrderCnt 导出为 */
  if (m_picture_coded_type != H264_PICTURE_CODED_TYPE_BOTTOM_FIELD)
    TopFieldOrderCnt = PicOrderCntMsb + header.pic_order_cnt_lsb;

  /* 当前图片不是顶场时，按以下伪代码指定BottomFieldOrderCnt导出为 */
  //TODO 注释了
  //if (m_picture_coded_type != H264_PICTURE_CODED_TYPE_TOP_FIELD) {
  if (!m_slice.slice_header.field_pic_flag)
    BottomFieldOrderCnt = TopFieldOrderCnt + header.delta_pic_order_cnt_bottom;
  else
    BottomFieldOrderCnt = PicOrderCntMsb + header.pic_order_cnt_lsb;
  //}

  return 0;
}

// 8.2.1.2 Decoding process for picture order count type 1
/* 当 pic_order_cnt_type 等于 1 时，调用此过程 */
/* 输入: 按照本节中指定的解码顺序的前一个图片的 FrameNumOffset。  
 * 输出: TopFieldOrderCnt 或 BottomFieldOrderCnt 之一或两者。 
 * TopFieldOrderCnt 和 BottomFieldOrderCnt 的值是按照本节中指定的方式导出的。令 prevFrameNum 等于解码顺序中前一个图片的frame_num。  */
int PictureBase::decoding_picture_order_count_type_1(
    const PictureBase *picture_previous) {

  /* 当当前图片不是 IDR 图片时，变量 prevFrameNumOffset 的推导如下： 
   * – 如果解码顺序中的前一个图片包含等于 5 的memory_management_control_operation_5_flag，则 prevFrameNumOffset 设置为等于 0。 
   * – 否则（解码顺序中的前一个图片没有不包括等于5的memory_management_control_operation)，prevFrameNumOffset被设置为等于解码顺序中的前一个图片的FrameNumOffset的值。 */
  SliceHeader &header = m_slice.slice_header;

  int32_t prevFrameNumOffset = 0;
  /* 当前帧不是 IDR */
  if (!header.IdrPicFlag) {
    if (picture_previous->memory_management_control_operation_5_flag)
      prevFrameNumOffset = 0;
    else
      prevFrameNumOffset = picture_previous->FrameNumOffset;
  }

  /* 当前帧是IDR */
  if (header.IdrPicFlag)
    FrameNumOffset = 0;
  else if (picture_previous->m_slice.slice_header.frame_num > header.frame_num)
    // 前一图像的帧号比当前图像大
    FrameNumOffset = prevFrameNumOffset + m_slice.m_sps.MaxFrameNum;
  else
    FrameNumOffset = prevFrameNumOffset;

  /* 变量 absFrameNum 是由以下伪代码指定导出的： */
  if (m_slice.m_sps.num_ref_frames_in_pic_order_cnt_cycle != 0)
    absFrameNum = FrameNumOffset + header.frame_num;
  else
    absFrameNum = 0;

  if (header.nal_ref_idc == 0 && absFrameNum > 0) absFrameNum--;

  /* 当absFrameNum > 0时，picOrderCntCycleCnt和frameNumInPicOrderCntCycle导出为 */
  if (absFrameNum > 0) {
    picOrderCntCycleCnt =
        (absFrameNum - 1) / m_slice.m_sps.num_ref_frames_in_pic_order_cnt_cycle;
    frameNumInPicOrderCntCycle =
        (absFrameNum - 1) % m_slice.m_sps.num_ref_frames_in_pic_order_cnt_cycle;
  }

  /* 变量预期PicOrderCnt是由以下伪代码指定导出的： */
  if (absFrameNum > 0) {
    expectedPicOrderCnt =
        picOrderCntCycleCnt * m_slice.m_sps.ExpectedDeltaPerPicOrderCntCycle;
    for (int i = 0; i <= frameNumInPicOrderCntCycle; i++)
      expectedPicOrderCnt += m_slice.m_sps.offset_for_ref_frame[i];
  } else
    expectedPicOrderCnt = 0;

  if (header.nal_ref_idc == 0)
    expectedPicOrderCnt += m_slice.m_sps.offset_for_non_ref_pic;

  /* 变量 TopFieldOrderCnt 或 BottomFieldOrderCnt 是按以下伪代码指定导出的： */
  if (!header.field_pic_flag) {
    // 当前图像为帧
    TopFieldOrderCnt = expectedPicOrderCnt + header.delta_pic_order_cnt[0];
    BottomFieldOrderCnt = TopFieldOrderCnt +
                          m_slice.m_sps.offset_for_top_to_bottom_field +
                          header.delta_pic_order_cnt[1];
  } else if (!header.bottom_field_flag)
    // 当前图像为顶场
    TopFieldOrderCnt = expectedPicOrderCnt + header.delta_pic_order_cnt[0];
  else // 当前图像为底场
    BottomFieldOrderCnt = expectedPicOrderCnt +
                          m_slice.m_sps.offset_for_top_to_bottom_field +
                          header.delta_pic_order_cnt[0];

  return 0;
}

// 8.2.1.3 Decoding process for picture order count type 2
/* 当 pic_order_cnt_type 等于 2 时，调用此过程。
 * 输出: TopFieldOrderCnt 或 BottomFieldOrderCnt 之一或两者。  
 * 令 prevFrameNum 等于解码顺序中前一个图片的frame_num */
int PictureBase::decoding_picture_order_count_type_2(
    const PictureBase *picture_previous) {

  /* 当前图片不是 IDR 图片时，变量 prevFrameNumOffset 的推导如下： 
   * – 如果解码顺序中的前一个图片包含等于 5 的内存_管理_控制_操作，则 prevFrameNumOffset 设置为等于 0。 
   * – 否则（解码顺序中的前一个图片没有不包括等于5的memory_management_control_operation)，prevFrameNumOffset被设置为等于解码顺序中的前一个图片的FrameNumOffset的值。 */
  int32_t prevFrameNumOffset = 0;
  if (m_slice.slice_header.IdrPicFlag == 0) {
    if (picture_previous->memory_management_control_operation_5_flag)
      prevFrameNumOffset = 0;
    else
      prevFrameNumOffset = picture_previous->FrameNumOffset;
  }

  /* 当gaps_in_frame_num_value_allowed_flag等于1时，解码顺序中的前一个图片可能是由第8.2.5.2节中指定的frame_num中的间隙的解码过程推断的“不存在”帧。 */

  /* 变量 FrameNumOffset 是由以下伪代码指定导出的： */
  if (m_slice.slice_header.IdrPicFlag == 1)
    FrameNumOffset = 0;
  else if (picture_previous->m_slice.slice_header.frame_num >
           m_slice.slice_header.frame_num)
    FrameNumOffset = prevFrameNumOffset + m_slice.m_sps.MaxFrameNum;
  else
    FrameNumOffset = prevFrameNumOffset;

  /* 变量 tempPicOrderCnt 是按以下伪代码指定导出的：*/
  int32_t tempPicOrderCnt = 0;
  if (m_slice.slice_header.IdrPicFlag == 1)
    tempPicOrderCnt = 0;
  else if (m_slice.slice_header.nal_ref_idc == 0) // 当前图像为非参考图像
    tempPicOrderCnt = 2 * (FrameNumOffset + m_slice.slice_header.frame_num) - 1;
  else
    tempPicOrderCnt = 2 * (FrameNumOffset + m_slice.slice_header.frame_num);

  /* 变量 TopFieldOrderCnt 或 BottomFieldOrderCnt 是按以下伪代码指定导出的：*/
  if (!m_slice.slice_header.field_pic_flag)
    // 当前图像为帧
    TopFieldOrderCnt = BottomFieldOrderCnt = tempPicOrderCnt;
  else if (m_slice.slice_header.bottom_field_flag)
    // 当前图像为底场
    BottomFieldOrderCnt = tempPicOrderCnt;
  else
    // 当前图像为顶场
    TopFieldOrderCnt = tempPicOrderCnt;

  return 0;
}

// 8.2.4 Decoding process for reference picture lists construction
int PictureBase::decoding_reference_picture_lists_construction(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], Frame *(&RefPicList1)[16]) {

  const SliceHeader &slice_header = m_slice.slice_header;
  /* 解码的参考图片被标记为“用于短期参考”或“用于长期参考”，如比特流所指定的和第8.2.5节中所指定的。短期参考图片由frame_num 的值标识。长期参考图片被分配一个长期帧索引，该索引由比特流指定并在第 8.2.5 节中指定。 */

  // 8.2.5 Decoded reference picture marking process
  // NOTE: 这里调用8.2.5会有问题

  /* 调用条款 8.2.4.1 来指定 
   * — 变量 FrameNum、FrameNumWrap 和 PicNum 到每个短期参考图片的分配，
   * — 变量 LongTermPicNum 到每个长期参考图片的分配。 */
  int ret = decoding_picture_numbers(dpb);
  if (ret != 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return ret;
  }

  /* 参考索引是参考图片列表的索引。当解码P或SP切片时，存在单个参考图片列表RefPicList0。在对B切片进行解码时，除了RefPicList0之外，还存在第二独立参考图片列表RefPicList1。*/

  /* 在每个切片的解码过程开始时，按照以下有序步骤的指定导出参考图片列表 RefPicList0 以及 B 切片的 RefPicList1： 
   * 1. 按照以下顺序导出初始参考图片列表 RefPicList0 以及 B 切片的 RefPicList1第 8.2.4.2 条*/
  ret = init_reference_picture_lists(dpb, RefPicList0, RefPicList1);
  if (ret != 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return ret;
  }

  /* 2. 当ref_pic_list_modification_flag_l0等于1时或者当解码B切片时ref_pic_list_modification_flag_l1等于1时，初始参考图片列表RefPicList0以及对于B切片而言RefPicList1按照条款8.2.4.3中的指定进行修改。 */
  ret = modif_reference_picture_lists(RefPicList0, RefPicList1);
  if (ret != 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return ret;
  }

  /* 修改后的参考图片列表RefPicList0中的条目数量是num_ref_idx_l0_active_minus1+1，并且对于B片，
   * 修改后的参考图片列表RefPicList1中的条目数量是num_ref_idx_l1_active_minus1+1。
   * 参考图片可以出现在修改后的参考中的多个索引处图片列表RefPicList0或RefPicList1。 */
  m_RefPicList0Length = slice_header.num_ref_idx_l0_active_minus1 + 1;
  m_RefPicList1Length = slice_header.num_ref_idx_l1_active_minus1 + 1;

  return 0;
}

// 8.2.4.1 Decoding process for picture numbers
/* 当调用第8.2.4节中指定的参考图片列表构建的解码过程、第8.2.5节中指定的解码参考图片标记过程或第8.2.5.2节中指定的frame_num中的间隙的解码过程时，调用该过程。*/
/* 参考图片通过第 8.4.2.1 节中指定的参考索引来寻址。*/
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

  /* 当第 8.2.4.2.1 至 8.2.4.2.5 节中，指定的初始 RefPicList0 或 RefPicList1 中的条目数：
   * - 分别大于 num_ref_idx_l0_active_minus1 + 1 或 num_ref_idx_l1_active_minus1 + 1 时，超过位置 num_ref_idx_l0_active_minus1 或 num_ref_idx_l1_active_minus1 的额外条目应被丢弃从初始参考图片列表中。 
  * - 分别小于 num_ref_idx_l0_active_minus1 + 1 或 num_ref_idx_l1_active_minus1 + 1 时，初始参考图片列表中的剩余条目为设置等于“无参考图片”。 */
  int l0_num_ref = slice_header.num_ref_idx_l0_active_minus1 + 1;
  if (m_RefPicList0Length > l0_num_ref) {
    for (int i = l0_num_ref; i < m_RefPicList0Length; i++)
      RefPicList0[i] = NULL;
    m_RefPicList0Length = l0_num_ref;
  } else if (m_RefPicList0Length < l0_num_ref)
    for (int i = m_RefPicList0Length; i < l0_num_ref; i++)
      RefPicList0[i] = NULL;

  int l1_num_ref = slice_header.num_ref_idx_l1_active_minus1 + 1;
  if (m_RefPicList1Length > l1_num_ref) {
    for (int i = l1_num_ref; i < m_RefPicList1Length; i++)
      RefPicList1[i] = NULL;
    m_RefPicList1Length = l1_num_ref;
  } else if (m_RefPicList1Length < l1_num_ref)
    for (int i = m_RefPicList1Length; i < l1_num_ref; i++)
      RefPicList1[i] = NULL;

  return 0;
}

// 8.2.4.2.1 Initialisation process for the reference picture list for P and SP
/* RefPicList0为排序后的dpb*/
int PictureBase::init_reference_picture_list_P_SP_slices_in_frames(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], int32_t &RefPicList0Length) {
  const int32_t size_dpb = 16;

  const SliceHeader &slice_header = m_slice.slice_header;
  /* 1. 参考图片列表RefPicList0被排序，使得短期参考帧和短期互补参考场对具有比长期参考帧和长期互补参考场对更低的索引。 */
  vector<int32_t> indexTemp_short, indexTemp_long;
  for (int index = 0; index < (int)size_dpb; index++) {
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
  sort(indexTemp_short.begin(), indexTemp_short.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicNum > dpb[b]->m_picture_frame.PicNum;
       });

  /* 3. 长期参考帧和互补参考字段对从具有最低LongTermPicNum值的帧或互补字段对开始排序，并按升序进行到具有最高LongTermPicNum值的帧或互补字段对。*/
  sort(indexTemp_long.begin(), indexTemp_long.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.LongTermPicNum <
                dpb[b]->m_picture_frame.LongTermPicNum;
       });

  // 4. 生成排序后的参考序列
  RefPicList0Length = 0;
  for (int index = 0; index < (int)indexTemp_short.size(); ++index)
    RefPicList0[RefPicList0Length++] = dpb[indexTemp_short[index]];

  for (int index = 0; index < (int)indexTemp_long.size(); ++index)
    RefPicList0[RefPicList0Length++] = dpb[indexTemp_long[index]];

  //RefPicList0[ 0 ] is set equal to the short-term reference picture with PicNum = 303,
  //RefPicList0[ 1 ] is set equal to the short-term reference picture with PicNum = 302,
  //RefPicList0[ 2 ] is set equal to the short-term reference picture with PicNum = 300,
  //RefPicList0[ 3 ] is set equal to the long-term reference picture with LongTermPicNum = 0,
  //RefPicList0[ 4 ] is set equal to the long-term reference picture with LongTermPicNum = 3.

  /* TODO YangJing 这里是在做什么？ <24-08-31 00:06:06> */
  if (slice_header.MbaffFrameFlag) {
    for (int index = 0; index < RefPicList0Length; ++index) {
      Frame *&ref_list_frame = RefPicList0[index];
      Frame *&ref_list_top_filed = RefPicList0[16 + 2 * index];
      Frame *&ref_list_bottom_filed = RefPicList0[16 + 2 * index + 1];

      ref_list_top_filed = ref_list_bottom_filed = ref_list_frame;
    }
  }

  return 0;
}

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
  sort(refFrameList0ShortTerm.begin(), refFrameList0ShortTerm.end(),
       [](Frame *a, Frame *b) {
         return (a->m_picture_top_filed.FrameNumWrap <
                 b->m_picture_top_filed.FrameNumWrap) ||
                (a->m_picture_bottom_filed.FrameNumWrap <
                 b->m_picture_bottom_filed.FrameNumWrap);
       });

  /* 4. refFrameList0LongTerm 从具有最低 LongTermFrameIdx 值的参考帧开始排序，并按升序继续到具有最高 LongTermFrameIdx 值的参考帧。 */
  sort(refFrameList0LongTerm.begin(), refFrameList0LongTerm.end(),
       [](Frame *a, Frame *b) {
         return (a->m_picture_top_filed.LongTermFrameIdx <
                 b->m_picture_top_filed.LongTermFrameIdx) ||
                (a->m_picture_bottom_filed.LongTermFrameIdx <
                 b->m_picture_bottom_filed.LongTermFrameIdx);
       });

  /* 第 8.2.4.2.5 节中指定的过程是用 refFrameList0ShortTerm 和 refFrameList0LongTerm 作为输入来调用的，并且输出被分配给 RefPicList0。*/
  int ret = init_reference_picture_lists_in_fields(
      refFrameList0ShortTerm, refFrameList0LongTerm, RefPicList0,
      RefPicList0Length, 0);

  return ret;
}

// 8.2.4.2.3 Initialisation process for reference picture lists for B slices in frames
/* 当解码编码帧中的 B 切片时，会调用此初始化过程。*/
/* 为了形成参考图片列表RefPicList0和RefPicList1，指的是解码的参考帧或互补参考字段对。 */
int PictureBase::init_reference_picture_lists_B_slices_in_frames(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], Frame *(&RefPicList1)[16],
    int32_t &RefPicList0Length, int32_t &RefPicList1Length) {

  const int32_t size_dpb = 16;
  vector<int32_t> indexTemp_short_left, indexTemp_short_right, indexTemp_long;

  /* 对于B切片，参考图片列表RefPicList0和RefPicList1中的短期参考条目的顺序取决于输出顺序，如由PicOrderCnt()给出的。当 pic_order_cnt_type 等于 0 时，如第 8.2.5.2 节中指定的被标记为“不存在”的参考图片不包括在 RefPicList0 或 RefPicList1 中 */

  /* 注1 – 当gaps_in_frame_num_value_allowed_flag等于1时，编码器应使用参考图片列表修改来确保解码过程的正确操作（特别是当pic_order_cnt_type等于0时，在这种情况下，不会为“不存在”帧推断PicOrderCnt()。 */

  /* 参考图片列表RefPicList0被排序，使得短期参考条目具有比长期参考条目更低的索引,排序如下：
   * 1. 令entryShortTerm 为变量，其范围涵盖当前标记为“用于短期参考”的所有参考条目。当存在PicOrderCnt(entryShortTerm)小于PicOrderCnt(CurrPic)的entryShortTerm的某些值时，entryShortTerm的这些值按照PicOrderCnt(entryShortTerm)的降序放置在refPicList0的开头。然后，entryShortTerm 的所有剩余值（当存在时）按照 PicOrderCnt(entryShortTerm ) 的升序被附加到 refPicList0。
   * 2.对长期参考条目进行排序，从具有最低LongTermPicNum值的长期参考条目开始，并按升序进行到具有最高LongTermPicNum值的长期参考条目。 */

  for (int i = 0; i < size_dpb; i++) {
    auto &pict_f = dpb[i]->m_picture_frame;
    if (pict_f.reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_short_term_reference) {

      if (pict_f.PicOrderCnt < PicOrderCnt)
        indexTemp_short_left.push_back(i);
      else
        indexTemp_short_right.push_back(i);

    } else if (pict_f.reference_marked_type ==
               H264_PICTURE_MARKED_AS_used_for_long_term_reference)
      indexTemp_long.push_back(i);
  }

  /* 当调用该过程时，应至少有一个参考条目当前被标记为“用于参考”（即“用于短期参考”或“用于长期参考”）并且未被标记为“不存在” */
  if (indexTemp_short_left.size() + indexTemp_short_right.size() +
          indexTemp_long.size() <=
      0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

  sort(indexTemp_short_left.begin(), indexTemp_short_left.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicOrderCnt >
                dpb[b]->m_picture_frame.PicOrderCnt;
       });
  sort(indexTemp_short_right.begin(), indexTemp_short_right.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicOrderCnt <
                dpb[b]->m_picture_frame.PicOrderCnt;
       });
  sort(indexTemp_long.begin(), indexTemp_long.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.LongTermPicNum <
                dpb[b]->m_picture_frame.LongTermPicNum;
       });

  RefPicList0Length = 0;
  for (int i = 0; i < (int)indexTemp_short_left.size(); i++)
    RefPicList0[RefPicList0Length++] = dpb[indexTemp_short_left[i]];
  for (int i = 0; i < (int)indexTemp_short_right.size(); i++)
    RefPicList0[RefPicList0Length++] = dpb[indexTemp_short_right[i]];
  for (int i = 0; i < (int)indexTemp_long.size(); i++)
    RefPicList0[RefPicList0Length++] = dpb[indexTemp_long[i]];

  indexTemp_short_left.clear();
  indexTemp_short_right.clear();
  indexTemp_long.clear();

  /* 参考图片列表RefPicList1被排序，使得短期参考条目具有比长期参考条目更低的索引。其排序： 
   * 1. 令entryShortTerm 为变量，其范围涵盖当前标记为“用于短期参考”的所有参考条目。当存在PicOrderCnt(entryShortTerm)大于PicOrderCnt(CurrPic)的entryShortTerm的某些值时，entryShortTerm的这些值按照PicOrderCnt(entryShortTerm)的升序放置在refPicList1的开头。然后，entryShortTerm 的所有剩余值（当存在时）按照 PicOrderCnt(entryShortTerm ) 的降序被附加到 refPicList1。  
   * 2. 长期参考条目从具有最低 LongTermPicNum 值的长期参考帧或互补参考字段对开始排序，并按升序继续到具有最高 LongTermPicNum 值的长期参考条目。  
   * 注 2 – 非配对参考字段不用于帧间预测（与 MbaffFrameFlag 的值无关）。 */

  for (int i = 0; i < size_dpb; i++) {
    auto &pict_f = dpb[i]->m_picture_frame;
    if (pict_f.reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_short_term_reference) {

      if (pict_f.PicOrderCnt > PicOrderCnt)
        indexTemp_short_left.push_back(i);
      else
        indexTemp_short_right.push_back(i);

    } else if (pict_f.reference_marked_type ==
               H264_PICTURE_MARKED_AS_used_for_long_term_reference)
      indexTemp_long.push_back(i);
  }

  /* 当调用该过程时，应至少有一个参考条目当前被标记为“用于参考”（即“用于短期参考”或“用于长期参考”）并且未被标记为“不存在” */
  if (indexTemp_short_left.size() + indexTemp_short_right.size() +
          indexTemp_long.size() <=
      0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

  sort(indexTemp_short_left.begin(), indexTemp_short_left.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicOrderCnt <
                dpb[b]->m_picture_frame.PicOrderCnt;
       });
  sort(indexTemp_short_right.begin(), indexTemp_short_right.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicOrderCnt >
                dpb[b]->m_picture_frame.PicOrderCnt;
       });
  sort(indexTemp_long.begin(), indexTemp_long.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.LongTermPicNum <
                dpb[b]->m_picture_frame.LongTermPicNum;
       });

  RefPicList1Length = 0;
  for (int i = 0; i < (int)indexTemp_short_left.size(); i++)
    RefPicList1[RefPicList1Length++] = dpb[indexTemp_short_left[i]];

  for (int i = 0; i < (int)indexTemp_short_right.size(); i++)
    RefPicList1[RefPicList1Length++] = dpb[indexTemp_short_right[i]];

  for (int i = 0; i < (int)indexTemp_long.size(); i++)
    RefPicList1[RefPicList1Length++] = dpb[indexTemp_long[i]];

  /* 3、当参考图片列表RefPicList1具有多于一个条目并且RefPicList1与参考图片列表RefPicList0相同时，交换前两个条目RefPicList1[0]和RefPicList1[1]。*/
  bool flag = 0;
  if (RefPicList1Length > 1) {
    for (int i = 0; i < size_dpb; i++)
      if (RefPicList1[i] != RefPicList0[i]) {
        flag = 1;
        break;
      }
  }
  if (!flag) swap(RefPicList1[0], RefPicList1[1]);

  return 0;
}

// 8.2.4.2.4 Initialization process for reference picture lists for B slices in fields
// 当对编码字段中的 B 切片进行解码时，会调用此初始化过程。
/* 当调用这一过程时，应该有至少一个参考字段（可以是参考帧的字段）当前被标记为“用于参考”（即，“用于短期参考”或“用于参考”）。供长期参考”）并且没有标记为“不存在”。 */
/* TODO YangJing 未经测试 <24-08-31 15:06:47> */
int PictureBase::init_reference_picture_lists_B_slices_in_fields(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], Frame *(&RefPicList1)[16],
    int32_t &RefPicList0Length, int32_t &RefPicList1Length) {

  const int32_t size_dpb = 16;
  vector<int32_t> indexTemp_short_left, indexTemp_short_right, indexTemp_long;
  Frame *refFrameList0ShortTerm[16] = {NULL};
  Frame *refFrameList1ShortTerm[16] = {NULL};
  Frame *refFrameListLongTerm[16] = {NULL};

  /* 当解码场时，存储的参考帧的每个场被识别为具有唯一索引的单独的参考图片。参考图片列表RefPicList0和RefPicList1中的短期参考图片的顺序取决于输出顺序，如由PicOrderCnt()给出的。当pic_order_cnt_type等于0时，如条款8.2.5.2中指定的被标记为“不存在”的参考图片不包括在RefPicList0或RefPicList1中。 */

  /* 如下导出参考帧的三个有序列表：refFrameList0ShortTerm、refFrameList1ShortTerm 和refFrameListLongTerm。为了形成这些帧列表的目的，术语参考条目在下文中指的是解码参考帧、互补参考字段对或非配对参考字段。当 pic_order_cnt_type 等于 0 时，术语参考条目不指第 8.2.5.2 节中指定的标记为“不存在”的帧 */

  /* 1. 令entryShortTerm 为一个变量，范围涵盖当前标记为“用于短期参考”的所有参考条目。当存在PicOrderCnt(entryShortTerm)小于或等于PicOrderCnt(CurrPic)的entryShortTerm的某些值时，entryShortTerm的这些值按照PicOrderCnt(entryShortTerm)的降序放置在refFrameList0ShortTerm的开头。然后，entryShortTerm 的所有剩余值（如果存在）将按 PicOrderCnt(entryShortTerm ) 的升序附加到 refFrameList0ShortTerm。 */
  for (int i = 0; i < size_dpb; i++) {
    auto &pict_f = dpb[i]->m_picture_frame;
    if (pict_f.reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_short_term_reference) {

      if (pict_f.PicOrderCnt < PicOrderCnt)
        indexTemp_short_left.push_back(i);
      else
        indexTemp_short_right.push_back(i);

    } else if (pict_f.reference_marked_type ==
               H264_PICTURE_MARKED_AS_used_for_long_term_reference)
      indexTemp_long.push_back(i);
  }

  /* 当调用该过程时，应至少有一个参考条目当前被标记为“用于参考”（即“用于短期参考”或“用于长期参考”）并且未被标记为“不存在” */
  if (indexTemp_short_left.size() + indexTemp_short_right.size() +
          indexTemp_long.size() <=
      0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return -1;
  }

  sort(indexTemp_short_left.begin(), indexTemp_short_left.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicOrderCnt >
                dpb[b]->m_picture_frame.PicOrderCnt;
       });
  sort(indexTemp_short_right.begin(), indexTemp_short_right.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicOrderCnt <
                dpb[b]->m_picture_frame.PicOrderCnt;
       });

  sort(indexTemp_long.begin(), indexTemp_long.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.LongTermFrameIdx <
                dpb[b]->m_picture_frame.LongTermFrameIdx;
       });

  // 4. 生成排序后的参考序列
  int j0 = 0;

  for (int i = 0; i < (int)indexTemp_short_left.size(); i++)
    refFrameList0ShortTerm[j0++] = dpb[indexTemp_short_left[i]];

  for (int i = 0; i < (int)indexTemp_short_right.size(); i++)
    refFrameList0ShortTerm[j0++] = dpb[indexTemp_short_right[i]];

  for (int i = 0; i < (int)indexTemp_long.size(); i++)
    refFrameListLongTerm[j0++] = dpb[indexTemp_long[i]];

  /* 令entryShortTerm 为一个变量，范围涵盖当前标记为“用于短期参考”的所有参考条目。当存在具有大于PicOrderCnt(CurrPic)的PicOrderCnt(entryShortTerm)的某些entryShortTerm值时，entryShortTerm的这些值按照PicOrderCnt(entryShortTerm)的升序放置在refFrameList1ShortTerm的开头。 Rec 的所有剩余值。然后，ITU-T H.264 (08/2021) 125entryShortTerm（当存在时）按照 PicOrderCnt(entryShortTerm ) 的降序被附加到 refFrameList1ShortTerm。*/
  /* refFrameListLongTerm 从具有最低 LongTermFrameIdx 值的参考条目开始排序，并按升序继续到具有最高 LongTermFrameIdx 值的参考条目。 */
  int j1 = 0;
  for (int i = 0; i < (int)indexTemp_short_right.size(); i++)
    refFrameList1ShortTerm[j1++] = dpb[indexTemp_short_right[i]];

  for (int i = 0; i < (int)indexTemp_short_left.size(); i++)
    refFrameList1ShortTerm[j1++] = dpb[indexTemp_short_left[i]];

  /* 第 8.2.4.2.5 节中指定的过程通过作为输入给出的 refFrameList0ShortTerm 和 refFrameListLongTerm 进行调用，并将输出分配给 RefPicList0。*/
  vector<Frame *> frameLong(begin(refFrameListLongTerm),
                            end(refFrameListLongTerm));

  vector<Frame *> frame0Short(begin(refFrameList0ShortTerm),
                              end(refFrameList0ShortTerm));
  int ret = init_reference_picture_lists_in_fields(
      frame0Short, frameLong, RefPicList0, RefPicList0Length, 0);
  if (ret != 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return ret;
  }

  /* 第 8.2.4.2.5 节中指定的过程通过作为输入给出的 refFrameList1ShortTerm 和 refFrameListLongTerm 进行调用，并将输出分配给 RefPicList1。 */
  vector<Frame *> frame1Short(begin(refFrameList1ShortTerm),
                              end(refFrameList1ShortTerm));
  ret = init_reference_picture_lists_in_fields(
      frame1Short, frameLong, RefPicList1, RefPicList0Length, 1);
  if (ret != 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return ret;
  }

  /* 当参考图片列表RefPicList1具有多于一个条目并且RefPicList1与参考图片列表RefPicList0相同时，前两个条目RefPicList1[0]和RefPicList1[1]被交换。 */
  int flag = 0;
  if (j0 > 1) {
    for (int i = 0; i < size_dpb; i++)
      if (RefPicList1[i] != RefPicList0[i]) {
        flag = 1;
        break;
      }
  }

  if (!flag) swap(RefPicList1[0], RefPicList1[1]);

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
int PictureBase::modif_reference_picture_lists(Frame *(&RefPicList0)[16],
                                               Frame *(&RefPicList1)[16]) {
  int ret;
  SliceHeader &slice_header = m_slice.slice_header;
  /* 当ref_pic_list_modification_flag_l0等于1时，以下适用： */
  if (slice_header.ref_pic_list_modification_flag_l0 == 1) {
    // 1.令refIdxL0为参考图片列表RefPicList0中的索引。它最初设置为等于0。
    slice_header.refIdxL0 = 0;

    /* 2. 对应的语法元素modification_of_pic_nums_idc 按照它们在比特流中出现的顺序进行处理。对于这些语法元素中的每一个，以下适用： */
    for (int i = 0; i < slice_header.ref_pic_list_modification_count_l0; i++) {
      int32_t modif_idc = slice_header.modification_of_pic_nums_idc[0][i];

      if (modif_idc == 0 || modif_idc == 1) {
        //* — 如果modification_of_pic_nums_idc等于0或等于1，则使用refIdxL0作为输入调用第8.2.4.3.1节中指定的过程，并将输出分配给refIdxL0。
        ret = modif_reference_picture_lists_for_short_ref_pictures(
            slice_header.refIdxL0, slice_header.picNumL0Pred, modif_idc,
            slice_header.abs_diff_pic_num_minus1[0][i],
            slice_header.num_ref_idx_l0_active_minus1, RefPicList0);
      } else if (modif_idc == 2) {
        //* — 否则，如果modification_of_pic_nums_idc等于2，则使用refIdxL0作为输入调用第8.2.4.3.2节中指定的过程，并将输出分配给refIdxL0。
        ret = modif_reference_picture_lists_for_long_ref_pictures(
            slice_header.refIdxL0, slice_header.num_ref_idx_l0_active_minus1,
            slice_header.long_term_pic_num[0][i], RefPicList0);
      } else
        //* – 否则（modification_of_pic_nums_idc等于3），参考图片列表RefPicList0的修改过程完成。
        break;

      if (ret != 0) {
        std::cerr << "An error occurred on " << __FUNCTION__
                  << "():" << __LINE__ << std::endl;
        return ret;
      }
    }
  }

  /* 当当前切片是B切片并且ref_pic_list_modification_flag_l1等于1时，以下适用： */
  if (slice_header.slice_type == SLICE_B &&
      slice_header.ref_pic_list_modification_flag_l1) {

    /* 1.令refIdxL1为参考图片列表RefPicList1的索引。它最初设置为 0。 */
    slice_header.refIdxL1 = 0;

    /* 2.对应的语法元素modification_of_pic_nums_idc按照它们在比特流中出现的顺序进行处理。对于每个语法元素，以下内容适用： */
    for (int i = 0; i < slice_header.ref_pic_list_modification_count_l1; i++) {

      int32_t modif_idc = slice_header.modification_of_pic_nums_idc[1][i];
      if (modif_idc == 0 || modif_idc == 1) {
        // – 如果modification_of_pic_nums_idc等于0或等于1，则使用refIdxL1作为输入调用第8.2.4.3.1节中指定的过程，并将输出分配给refIdxL1。
        ret = modif_reference_picture_lists_for_short_ref_pictures(
            slice_header.refIdxL1, slice_header.picNumL1Pred, modif_idc,
            slice_header.abs_diff_pic_num_minus1[1][i],
            slice_header.num_ref_idx_l1_active_minus1, RefPicList1);
      } else if (modif_idc == 2) {
        // — 否则，如果modification_of_pic_nums_idc等于2，则使用refIdxL1作为输入调用第8.2.4.3.2节中指定的过程，并将输出分配给refIdxL1。
        ret = modif_reference_picture_lists_for_long_ref_pictures(
            slice_header.refIdxL1, slice_header.num_ref_idx_l1_active_minus1,
            slice_header.long_term_pic_num[1][i], RefPicList1);
      } else
        // – 否则（modification_of_pic_nums_idc等于3），参考图片列表RefPicList1的修改过程完成。 */
        break;

      if (ret != 0) {
        std::cerr << "An error occurred on " << __FUNCTION__
                  << "():" << __LINE__ << std::endl;
        return ret;
      }
    }
  }

  return 0;
}

// 8.2.4.3.1 Modification process of reference picture lists for short-term
/* 该过程的输入是索引 refIdxLX（X 为 0 或 1）
 * 该过程的输出是递增的索引 refIdxLX。*/
int PictureBase::modif_reference_picture_lists_for_short_ref_pictures(
    int32_t &refIdxLX, int32_t &picNumLXPred, const int32_t modif_idc,
    const int32_t abs_diff_pic_num_minus1,
    const int32_t num_ref_idx_lX_active_minus1, Frame *(&RefPicListX)[16]) {

  SliceHeader &slice_header = m_slice.slice_header;

  /* 变量 picNumLXNoWrap 的推导如下：*/
  int32_t picNumLXNoWrap = 0;
  if (modif_idc == 0) {
    if (picNumLXPred - (abs_diff_pic_num_minus1 + 1) < 0)
      picNumLXNoWrap =
          picNumLXPred - (abs_diff_pic_num_minus1 + 1) + slice_header.MaxPicNum;
    else
      picNumLXNoWrap = picNumLXPred - (abs_diff_pic_num_minus1 + 1);

  } else {
    if (picNumLXPred + (abs_diff_pic_num_minus1 + 1) >= slice_header.MaxPicNum)
      picNumLXNoWrap =
          picNumLXPred + (abs_diff_pic_num_minus1 + 1) - slice_header.MaxPicNum;
    else
      picNumLXNoWrap = picNumLXPred + (abs_diff_pic_num_minus1 + 1);
  }

  /* picNumLXPred 是变量 picNumLXNoWrap 的预测值。当对于切片第一次调用本子句中指定的过程时（即，对于在ref_pic_list_modification()语法中第一次出现modification_of_pic_nums_idc等于0或1），picNumL0Pred和picNumL1Pred最初被设置为等于CurrPicNum。每次分配 picNumLXNoWrap 后，picNumLXNoWrap 的值都会分配给 picNumLXPred。 */
  picNumLXPred = picNumLXNoWrap;

  /* 变量 picNumLX 是由以下伪代码指定导出的： */
  int32_t picNumLX = 0;
  if (picNumLXNoWrap > (int32_t)slice_header.CurrPicNum)
    picNumLX = picNumLXNoWrap - slice_header.MaxPicNum;
  else
    picNumLX = picNumLXNoWrap;
  /* picNumLX应等于标记为“用于短期参考”的参考图片的PicNum，并且不应等于标记为“不存在”的短期参考图片的PicNum。 */

  /* 执行以下过程以将具有短期图像编号picNumLX的图像放置到索引位置refIdxLX中，将任何其他剩余图像的位置移动到列表中的后面，并且递增refIdxLX的值。*/
  int32_t cIdx;
  for (cIdx = num_ref_idx_lX_active_minus1 + 1; cIdx > refIdxLX; cIdx--)
    RefPicListX[cIdx] = RefPicListX[cIdx - 1];

  for (cIdx = 0; cIdx < num_ref_idx_lX_active_minus1 + 1; cIdx++) {
    if (RefPicListX[cIdx]->PicNum == picNumLX &&
        RefPicListX[cIdx]->reference_marked_type ==
            H264_PICTURE_MARKED_AS_used_for_short_term_reference)
      break;
  }

  RefPicListX[refIdxLX++] = RefPicListX[cIdx];
  int32_t nIdx = refIdxLX;

  for (cIdx = refIdxLX; cIdx <= num_ref_idx_lX_active_minus1 + 1; cIdx++) {
    if (RefPicListX[cIdx] != NULL) {
      /* 如果图片RefPicListX[cIdx]被标记为“用于短期参考”，则PicNumF(RefPicListX[cIdx])是图片RefPicListX[cIdx]的PicNum，否则(图片RefPicListX[cIdx]没有被标记为“用于短期参考”)，PicNumF(RefPicListX[cIdx])等于MaxPicNum。 */
      int32_t PicNumF;
      if (RefPicListX[cIdx]->reference_marked_type ==
          H264_PICTURE_MARKED_AS_used_for_short_term_reference)
        PicNumF = RefPicListX[cIdx]->PicNum;
      else
        PicNumF = slice_header.MaxPicNum;

      if (PicNumF != picNumLX) RefPicListX[nIdx++] = RefPicListX[cIdx];
    }
  }

  /* 在该伪代码过程中，列表RefPicListX的长度暂时比最终列表所需的长度长一个元素。执行此过程后，仅需要保留列表中的 0 到 num_ref_idx_lX_active_minus1 元素。 */
  RefPicListX[num_ref_idx_lX_active_minus1 + 1] = NULL;

  return 0;
}

// 8.2.4.3.2 Modification process of reference picture lists for long-term
/* 该过程的输入是索引 refIdxLX（X 为 0 或 1）。  
 * 该过程的输出是递增的索引 refIdxLX。*/
int PictureBase::modif_reference_picture_lists_for_long_ref_pictures(
    int32_t &refIdxLX, const int32_t num_ref_idx_lX_active_minus1,
    const int32_t long_term_pic_num, Frame *(&RefPicListX)[16]) {

  /* 执行以下过程以将具有长期图片编号long_term_pic_num的图片放置到索引位置refIdxLX中，将任何其他剩余图片的位置移动到列表中的后面，并递增refIdxLX的值。 */
  int32_t cIdx = 0;
  for (cIdx = num_ref_idx_lX_active_minus1 + 1; cIdx > refIdxLX; cIdx--)
    RefPicListX[cIdx] = RefPicListX[cIdx - 1];

  for (cIdx = 0; cIdx < num_ref_idx_lX_active_minus1 + 1; cIdx++)
    if (RefPicListX[cIdx]->LongTermPicNum == long_term_pic_num) break;

  RefPicListX[refIdxLX++] = RefPicListX[cIdx];

  /* 其中函数 LongTermPicNumF( RefPicListX[ cIdx ] ) 的推导如下： */
  int32_t nIdx = refIdxLX;
  int32_t LongTermPicNumF;
  for (cIdx = refIdxLX; cIdx <= num_ref_idx_lX_active_minus1 + 1; cIdx++) {
    if (RefPicListX[cIdx]->reference_marked_type ==
        H264_PICTURE_MARKED_AS_used_for_long_term_reference)
      LongTermPicNumF = RefPicListX[cIdx]->LongTermPicNum;
    else
      LongTermPicNumF = 2 * (MaxLongTermFrameIdx + 1);

    if (LongTermPicNumF != long_term_pic_num)
      RefPicListX[nIdx++] = RefPicListX[cIdx];
  }

  /* 在该伪代码过程中，列表RefPicListX的长度暂时比最终列表所需的长度长一个元素。执行此过程后，仅需要保留列表中的 0 到 num_ref_idx_lX_active_minus1 元素。 */
  /* TODO YangJing 可能会有问题 <24-08-31 16:16:26> */
  RefPicListX[num_ref_idx_lX_active_minus1 + 1] = NULL;
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
//int PictureBase::Decoding_process_for_gaps_in_frame_num() { return 0; }

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
    uint32_t numShortTerm = 0;
    uint32_t numLongTerm = 0;

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
