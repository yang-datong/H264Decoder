#include "Frame.hpp"
#include "PictureBase.hpp"
#include "SliceHeader.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

//--------------参考帧列表重排序------------------------

// 8.2.1 Decoding process for picture order count
/* 此过程的输出为 TopFieldOrderCnt（如果适用）和 BottomFieldOrderCnt（如果适用）*/
int PictureBase::decoding_picture_order_count(uint32_t pic_order_cnt_type) {
  /* 为每个帧、场（无论是从编码场解码还是作为解码帧的一部分）或互补场对导出图像顺序计数信息，如下所示： 
   * – 每个编码帧与两个图像顺序计数相关联，称为 TopFieldOrderCnt 和BottomFieldOrderCnt 分别表示其顶部场和底部场。  
   * – 每个编码场都与图片顺序计数相关联，对于编码顶部场称为 TopFieldOrderCnt，对于底部场称为 BottomFieldOrderCnt。  
   * – 每个互补场对与两个图像顺序计数相关联，分别是其编码的顶部场的 TopFieldOrderCnt 和其编码的底部场的 BottomFieldOrderCnt。 */

  /* 比特流不应包含导致解码过程中使用的 TopFieldOrderCnt、BottomFieldOrderCnt、PicOrderCntMsb 或 FrameNumOffset 值（如第 8.2.1.1 至 8.2.1.3 条规定）超出从 -231 到 231 - 1（含）的值范围的数据。 */
  int ret = 0;

  if (pic_order_cnt_type == 0)
    //0:使用POC高低位（如果SPS中传递的POC低位，一般是最优先使用的情况）来计算POC(图像顺序计数)( 适用于大多数情况，特别是需要精确控制POC的场景)
    ret = decoding_picture_order_count_type_0(m_parent->m_picture_previous_ref);
  else if (pic_order_cnt_type == 1)
    //1:使用增量计数来计算POC(复杂，如B帧和P帧混合的情况)
    ret = decoding_picture_order_count_type_1(m_parent->m_picture_previous);
  else if (pic_order_cnt_type == 2)
    //2:使用帧号来计算POC(简单，如仅有I帧和P帧的情况)
    ret = decoding_picture_order_count_type_2(m_parent->m_picture_previous);
  RET(ret);

  /* 对于不同编码类型，需要进一步取舍当前图像的最终 POC */
  picOrderCntFunc(this);
  return 0;
}

// 8.2.1.1 Decoding process for picture order count type 0
/* 当 pic_order_cnt_type 等于 0 时调用此过程。*/
/* 输入: 按照本节中指定的解码顺序的先前参考图片的PicOrderCntMsb。  
 * 输出: TopFieldOrderCnt 或 BottomFieldOrderCnt 之一或两者。 */
int32_t PictureBase::decoding_picture_order_count_type_0(
    const PictureBase *picture_previous_ref) {

  const SliceHeader *header = m_slice->slice_header;
  //POC 的高位和低位
  uint32_t prevPicOrderCntMsb, prevPicOrderCntLsb;
  //------------------------------- 读取POC 高位、低位 ------------------------------------
  /* POC 高位,低位都被重置为 0，因为 IDR 帧通常会重置 POC 计数 */
  if (header->IdrPicFlag)
    prevPicOrderCntMsb = prevPicOrderCntLsb = 0;
  else if (picture_previous_ref) {
    /* 非 IDR 帧，并且有 参考帧，但当前Slice不被用于参考帧（特殊情况） */
    if (picture_previous_ref->mmco_5_flag) {

      /* 参考帧不是底场帧（即它是顶场帧或者帧级别的图像） */
      if (picture_previous_ref->m_picture_coded_type != BOTTOM_FIELD) {
        prevPicOrderCntMsb = 0;
        prevPicOrderCntLsb = picture_previous_ref->TopFieldOrderCnt;

        /* 参考帧是底场帧 */
      } else
        prevPicOrderCntMsb = prevPicOrderCntLsb = 0;

      /* 非 IDR 帧，并且有参考帧，当前Slice用于参考帧(一般情况） */
    } else {
      //直接从参考帧中继承 POC 的高位和低位
      prevPicOrderCntMsb = picture_previous_ref->PicOrderCntMsb;
      prevPicOrderCntLsb =
          picture_previous_ref->m_slice->slice_header->pic_order_cnt_lsb;
    }
  } else
    RET(-1);

  //------------------------------- 计算POC 高位，处理环绕问题 ------------------------------------
  const uint32_t &pic_order_cnt_lsb = header->pic_order_cnt_lsb;
  const uint32_t &MaxPicOrderCntLsb = header->m_sps->MaxPicOrderCntLsb;

  /* 1. 如果当前帧的 POC 低位小于前一帧的 POC 低位，并且两者之间的差距大于或等于 MaxPicOrderCntLsb / 2，这通常意味着发生了 POC 的环绕。例如，如果 POC 低位的最大值是 16（即 MaxPicOrderCntLsb 为 16），那么当从 14 切换到 2 时，就可能满足这个条件。 */
  if ((pic_order_cnt_lsb < prevPicOrderCntLsb) &&
      (prevPicOrderCntLsb - pic_order_cnt_lsb >= MaxPicOrderCntLsb / 2))
    //高位 PicOrderCntMsb 加上 MaxPicOrderCntLsb 的值来还原环绕
    PicOrderCntMsb = prevPicOrderCntMsb + MaxPicOrderCntLsb;

  /* 2. 如果当前帧的 POC 低位大于前一帧的 POC 低位，并且两者之间的差值大于 MaxPicOrderCntLsb / 2 */
  else if ((pic_order_cnt_lsb > prevPicOrderCntLsb) &&
           (pic_order_cnt_lsb - prevPicOrderCntLsb > MaxPicOrderCntLsb / 2))
    //高位 PicOrderCntMsb 减去 MaxPicOrderCntLsb 的值来调整高位
    PicOrderCntMsb = prevPicOrderCntMsb - MaxPicOrderCntLsb;

  else
    PicOrderCntMsb = prevPicOrderCntMsb;

  //--------------------------- 根据POC 高位、低位还原POC ------------------------------------
  /* 当前图片为Slice或Top field */
  if (m_picture_coded_type != BOTTOM_FIELD)
    TopFieldOrderCnt = PicOrderCntMsb + pic_order_cnt_lsb;

  /* 当前图片不是顶场时，按以下伪代码指定BottomFieldOrderCnt导出为 */
  if (!m_slice->slice_header->field_pic_flag)
    /* 对于一般的情况delta_pic_order_cnt_bottom为-1,及Bottom比Top刚好小1 */
    BottomFieldOrderCnt = TopFieldOrderCnt + header->delta_pic_order_cnt_bottom;
  else
    BottomFieldOrderCnt = PicOrderCntMsb + pic_order_cnt_lsb;

  return 0;
}

// 8.2.1.2 Decoding process for picture order count type 1
/* 当 pic_order_cnt_type 等于 1 时，调用此过程 */
/* 输入: 按照本节中指定的解码顺序的前一个图片的 FrameNumOffset。  
 * 输出: TopFieldOrderCnt 或 BottomFieldOrderCnt 之一或两者。 
 * TopFieldOrderCnt 和 BottomFieldOrderCnt 的值是按照本节中指定的方式导出的。令 prevFrameNum 等于解码顺序中前一个图片的frame_num。  */
/* TODO YangJing 未经测试 <24-09-16 16:07:25> */
int PictureBase::decoding_picture_order_count_type_1(
    const PictureBase *picture_previous) {
  const SliceHeader *header = m_slice->slice_header;
  const uint32_t frame_num = header->frame_num;
  const uint32_t MaxFrameNum = header->m_sps->MaxFrameNum;
  /* 帧顺序计数循环中参考帧的数量 */
  const uint32_t num_ref_frames_in_pic_order_cnt_cycle =
      header->m_sps->num_ref_frames_in_pic_order_cnt_cycle;

  /* 当当前图片不是 IDR 图片时，变量 prevFrameNumOffset 的推导如下： 
   * – 如果解码顺序中的前一个图片包含等于 5 的memory_management_control_operation_5_flag，则 prevFrameNumOffset 设置为等于 0。 
   * – 否则（解码顺序中的前一个图片没有不包括等于5的memory_management_control_operation)，prevFrameNumOffset被设置为等于解码顺序中的前一个图片的FrameNumOffset的值。 */
  int32_t prevFrameNumOffset = 0;
  /* 当前帧不是 IDR */
  if (header->IdrPicFlag == 0) {
    if (picture_previous->mmco_5_flag)
      prevFrameNumOffset = 0;
    else
      prevFrameNumOffset = picture_previous->FrameNumOffset;
  }

  /* 当前帧是IDR */
  if (header->IdrPicFlag)
    FrameNumOffset = 0;
  else if (picture_previous->m_slice->slice_header->frame_num > frame_num)
    // 前一图像的帧号比当前图像大
    FrameNumOffset = prevFrameNumOffset + MaxFrameNum;
  else
    FrameNumOffset = prevFrameNumOffset;

  /* absFrameNum 是当前帧的绝对帧号（参考帧计数）。若有一个参考帧的循环结构，此时 absFrameNum 是 FrameNumOffset 加上 frame_num。 */
  if (num_ref_frames_in_pic_order_cnt_cycle != 0)
    absFrameNum = FrameNumOffset + frame_num;
  else
    absFrameNum = 0;

  /* 如果当前帧是非参考帧则 absFrameNum 减 1，以区分非参考帧 */
  if (header->nal_ref_idc == 0 && absFrameNum > 0) absFrameNum--;

  /* 计算当前帧在 POC 计数周期中的帧号。这通过除法和取余来确定：picOrderCntCycleCnt 是 POC 循环的计数器（循环了几次），而 frameNumInPicOrderCntCycle 表示当前帧在循环中的帧号。 */
  if (absFrameNum > 0) {
    picOrderCntCycleCnt =
        (absFrameNum - 1) / num_ref_frames_in_pic_order_cnt_cycle;
    frameNumInPicOrderCntCycle =
        (absFrameNum - 1) % num_ref_frames_in_pic_order_cnt_cycle;
  }

  /* 根据参考帧周期计算的期望 POC，等于 POC循环次数 乘以每个参考帧的期望 POC 差值，并加上对应参考帧的偏移量 */
  if (absFrameNum > 0) {
    expectedPicOrderCnt =
        picOrderCntCycleCnt * header->m_sps->ExpectedDeltaPerPicOrderCntCycle;
    for (int i = 0; i <= frameNumInPicOrderCntCycle; i++)
      expectedPicOrderCnt += header->m_sps->offset_for_ref_frame[i];
  } else
    expectedPicOrderCnt = 0;

  /* 如果当前帧是非参考帧，expectedPicOrderCnt 还需加上非参考帧定义的偏移量。 */
  if (header->nal_ref_idc == 0)
    expectedPicOrderCnt += header->m_sps->offset_for_non_ref_pic;

  // 当前图像为帧
  /* 计算顶场 POC ，又计算底场 POC ，底场的 POC 是基于顶场 POC 加上顶场到底场的偏移量再加上一个增量值计算的 */
  if (header->field_pic_flag == 0) {
    TopFieldOrderCnt = expectedPicOrderCnt + header->delta_pic_order_cnt[0];
    BottomFieldOrderCnt = TopFieldOrderCnt +
                          header->m_sps->offset_for_top_to_bottom_field +
                          header->delta_pic_order_cnt[1];

    // 当前图像为底场
    /* 底场的 POC 包含顶场到底场的偏移量*/
  } else if (header->bottom_field_flag)
    BottomFieldOrderCnt = expectedPicOrderCnt + header->delta_pic_order_cnt[0] +
                          header->m_sps->offset_for_top_to_bottom_field;
  // 当前图像为顶场
  /* 顶场的 POC 是相对于期望 POC 增加一个增量值 */
  else
    TopFieldOrderCnt = expectedPicOrderCnt + header->delta_pic_order_cnt[0];

  return 0;
}

// 8.2.1.3 Decoding process for picture order count type 2
/* 当 pic_order_cnt_type 等于 2 时，调用此过程。
 * 输出: TopFieldOrderCnt 或 BottomFieldOrderCnt 之一或两者。  
 * 令 prevFrameNum 等于解码顺序中前一个图片的frame_num */
int PictureBase::decoding_picture_order_count_type_2(
    const PictureBase *picture_previous) {

  //------------------------------- 读取POC FrameNumOffset 并处理环绕问题 --------------------------------
  /* 当前图片不是 IDR 图片时，变量 prevFrameNumOffset 的推导如下： 
   * – 如果解码顺序中的前一个图片包含等于，memory_management_control_operation_5_flag则 prevFrameNumOffset 设置为等于 0。 
   * – 否则（解码顺序中的前一个图片没有不包括等于5的memory_management_control_operation)，prevFrameNumOffset被设置为等于解码顺序中的前一个图片的FrameNumOffset的值。 */
  const SliceHeader *header = m_slice->slice_header;
  const uint32_t frame_num = header->frame_num;
  const uint32_t MaxFrameNum = header->m_sps->MaxFrameNum;

  int32_t prevFrameNumOffset = 0;
  if (header->IdrPicFlag == 0) {
    if (picture_previous->mmco_5_flag)
      prevFrameNumOffset = 0;
    else
      prevFrameNumOffset = picture_previous->FrameNumOffset;
  }
  /* 当gaps_in_frame_num_value_allowed_flag等于1时，解码顺序中的前一个图片可能是由第8.2.5.2节中指定的frame_num中的间隙的解码过程推断的“不存在”帧。 */

  /* IDR 帧是解码参考点，通常在视频序列的开始重置POC计数 */
  if (header->IdrPicFlag) FrameNumOffset = 0;
  /* 非IDR帧，前一帧的 frame_num 大于当前帧的 frame_num说明发生了环绕 */
  else if (picture_previous->m_slice->slice_header->frame_num > frame_num)
    FrameNumOffset = prevFrameNumOffset + MaxFrameNum;
  /* 没有环绕，FrameNumOffset 保持和前一帧一样 */
  else
    FrameNumOffset = prevFrameNumOffset;

  //--------------------------- 计算POC -------------------------------------
  int32_t tempPicOrderCnt = 0;
  /* IDR 帧是解码参考点，通常在视频序列的开始重置POC计数 */
  if (header->IdrPicFlag) tempPicOrderCnt = 0;
  /* 非参考帧的 POC 值总是比它的 frame_num 的两倍稍微小一点，使得非参考帧（比如 B 帧）可以插入其中（NOTE:实际上一般含有B帧的GOP不会在这里处理）*/
  else if (m_slice->slice_header->nal_ref_idc == 0)
    tempPicOrderCnt = 2 * (FrameNumOffset + frame_num) - 1;
  /* 参考帧，POC 值为2倍帧号，这样做是为了给B帧插入留下空间 */
  else
    tempPicOrderCnt = 2 * (FrameNumOffset + frame_num);

  /* 对于帧编码，顶场和底场的 POC 值相同 */
  if (!m_slice->slice_header->field_pic_flag)
    TopFieldOrderCnt = BottomFieldOrderCnt = tempPicOrderCnt;
  /* 场与场之间可能的不同解码顺序 */
  else if (m_slice->slice_header->bottom_field_flag)
    BottomFieldOrderCnt = tempPicOrderCnt;
  else
    TopFieldOrderCnt = tempPicOrderCnt;

  return 0;
}

int PictureBase::picOrderCntFunc(PictureBase *pic) {
  const H264_PICTURE_CODED_TYPE &coded_type = pic->m_picture_coded_type;
  /* 当前图像是帧或互补场对，取顶场 POC  和 底场 POC 中的最小值（在互补场对中，两个场应作为一个整体显示）*/
  if (coded_type == FRAME || coded_type == COMPLEMENTARY_FIELD_PAIR)
    pic->PicOrderCnt = MIN(pic->TopFieldOrderCnt, pic->BottomFieldOrderCnt);

  // 当前图像为顶场
  else if (coded_type == TOP_FIELD)
    pic->PicOrderCnt = pic->TopFieldOrderCnt;

  // 当前图像为底场
  else if (coded_type == BOTTOM_FIELD)
    pic->PicOrderCnt = pic->BottomFieldOrderCnt;

  return pic->PicOrderCnt;
}

// 8.2.4 Decoding process for reference picture lists construction
int PictureBase::decoding_ref_picture_lists_construction(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], Frame *(&RefPicList1)[16]) {
  /* 解码的参考图片被标记为“用于短期参考”或“用于长期参考”，如比特流所指定的和第8.2.5节中所指定的。短期参考图片由frame_num 的值标识。长期参考图片被分配一个长期帧索引，该索引由比特流指定并在第 8.2.5 节中指定。 */

  /* 8.2.4.1 Decoding process for picture numbers，主要完全以下两件事：
   * — 变量 FrameNum、FrameNumWrap 和 PicNum 到每个短期参考图片的分配，
   * — 变量 LongTermPicNum 到每个长期参考图片的分配。 */
  int ret = decoding_picture_numbers(dpb);
  RET(ret);

  /* 参考索引是参考图片列表的索引。当解码P或SP切片时，存在单个参考图片列表RefPicList0（前参考）。在对B切片进行解码时，还存在第二独立参考图片列表RefPicList1(后参考)。*/
  /* 1. 在每个切片的解码过程开始时，按照以下顺序导出初始参考图片列表 RefPicList0 以及 B 切片的 RefPicList1第 8.2.4.2 条*/
  ret = init_ref_picture_lists(dpb, RefPicList0, RefPicList1);
  RET(ret);

  /* 2. 当ref_pic_list_modification_flag_l0等于1时或者当解码B切片时ref_pic_list_modification_flag_l1等于1时，初始参考图片列表RefPicList0以及对于B切片而言RefPicList1按照条款8.2.4.3中的指定进行修改。 */
  //在某些场景中，slice header 中可能包含修改参考帧列表的指令。这一步执行了对参考帧列表的修改操作
  ret = modif_ref_picture_lists(RefPicList0, RefPicList1);
  RET(ret);

  /* 修改后的参考图片列表RefPicList0中的条目数量是num_ref_idx_l0_active_minus1+1，并且对于B片，
   * 修改后的参考图片列表RefPicList1中的条目数量是num_ref_idx_l1_active_minus1+1。
   * 参考图片可以出现在修改后的参考中的多个索引处图片列表RefPicList0或RefPicList1。 */
  m_RefPicList0Length = m_slice->slice_header->num_ref_idx_l0_active_minus1 + 1;
  m_RefPicList1Length = m_slice->slice_header->num_ref_idx_l1_active_minus1 + 1;
  return 0;
}

/* 处理picture.FrameNumWrap回环 */
inline void updateFrameNumWrap(PictureBase &pic, int32_t FrameNum,
                               uint32_t MaxFrameNum) {
  if (pic.reference_marked_type == SHORT_REF) {
    /* DPB中的FrameNum比当前Slice的要大说明出现了回环 */
    if (pic.FrameNum > FrameNum)
      pic.FrameNumWrap = pic.FrameNum - MaxFrameNum;
    else
      pic.FrameNumWrap = pic.FrameNum;
  }
}

// 处理短期参考帧或场对，更新 PicNum 和检测顶场和底场的 FrameNumWrap 是否一致
inline int updateShortTermReference(PictureBase &pic_frame,
                                    PictureBase &pic_top,
                                    PictureBase &pic_bottom, int &PicNum) {
  if (pic_frame.reference_marked_type == SHORT_REF)
    PicNum = pic_frame.PicNum = pic_frame.FrameNumWrap;

  if (pic_top.reference_marked_type == SHORT_REF &&
      pic_bottom.reference_marked_type == SHORT_REF) {
    pic_bottom.PicNum = pic_bottom.FrameNumWrap;
    PicNum = pic_top.PicNum = pic_top.FrameNumWrap;
    RET(pic_top.PicNum != pic_bottom.PicNum);
  }
  return 0;
}

// 处理长期参考帧或场对，更新 LongTermPicNum，并检查顶场和底场的 LongTermPicNum 是否一致。
inline int updateLongTermReference(PictureBase &pic_frame, PictureBase &pic_top,
                                   PictureBase &pict_bottom,
                                   int &LongTermPicNum) {
  if (pic_frame.reference_marked_type == LONG_REF)
    LongTermPicNum = pic_frame.LongTermPicNum = pic_frame.LongTermFrameIdx;

  if (pic_top.reference_marked_type == LONG_REF &&
      pict_bottom.reference_marked_type == LONG_REF) {
    pict_bottom.LongTermPicNum = pict_bottom.LongTermFrameIdx;
    LongTermPicNum = pic_top.LongTermPicNum = pic_top.LongTermFrameIdx;
    RET(pic_top.LongTermPicNum != pict_bottom.LongTermPicNum);
  }
  return 0;
}

// 更新PicNum或LongTermPicNum的函数
int updatePicNum(PictureBase &field, int m_picture_coded_type,
                 bool isLongTerm) {
  int baseNum = isLongTerm ? field.LongTermFrameIdx : field.FrameNumWrap;
  int offset = (m_picture_coded_type == TOP_FIELD) ? 1 : 0;

  if (isLongTerm)
    field.LongTermPicNum = 2 * baseNum + offset;
  else
    field.PicNum = 2 * baseNum + offset;

  return isLongTerm ? field.LongTermPicNum : field.PicNum;
}

// 8.2.4.1 Decoding process for picture numbers
/* 当调用第8.2.4节中指定的参考图片列表构建的解码过程、第8.2.5节中指定的解码参考图片标记过程或第8.2.5.2节中指定的frame_num中的间隙的解码过程时，调用该过程。*/
/* 参考图片通过第 8.4.2.1 节中指定的参考索引来寻址。*/
int PictureBase::decoding_picture_numbers(Frame *(&dpb)[16]) {
  const SliceHeader *header = m_slice->slice_header;
  FrameNum = header->frame_num;

  /* 变量 FrameNum、FrameNumWrap、PicNum、LongTermFrameIdx 和 LongTermPicNum 用于第 8.2.4.2 节中参考图片列表的初始化过程、第 8.2.4.3 节中参考图片列表的修改过程、第 8.2.5 节中的解码参考图片标记过程。以及第8.2.5.2节中frame_num中间隙的解码过程。 */

  //--------------------------------------- 处理FrameNumWrap --------------------------------------------
  //FrameNum 是解码顺序中的帧编号，而 MaxFrameNum 是允许的最大帧编号加一，定义了一个周期性边界，超过这个边界帧编号将回绕到 0。FrameNumWrap 是一个处理过的帧编号，用来适应帧编号的周期性回绕。NOTE: 前面在解码单个Slice的POC时候其实已经做个这个事情，这里是对缓存中的短、长参考帧进行一次整体和处理
  for (int i = 0; i < MAX_DPB; i++) {
    auto &pict_f = dpb[i]->m_picture_frame;
    auto &pict_t = dpb[i]->m_picture_top_filed;
    auto &pict_b = dpb[i]->m_picture_bottom_filed;

    if (!pict_f.m_slice || !pict_f.m_slice->slice_header->m_sps) continue;

    const uint32_t MaxFrameNum =
        pict_f.m_slice->slice_header->m_sps->MaxFrameNum;

    /* 帧，顶场，底场 */
    updateFrameNumWrap(pict_f, FrameNum, MaxFrameNum);
    updateFrameNumWrap(pict_t, FrameNum, MaxFrameNum);
    updateFrameNumWrap(pict_b, FrameNum, MaxFrameNum);
  }

  //--------------------------------------- 处理PicNum LongTermPicNum -------------------------------------
  /* 每个长期参考图片都有一个关联的 LongTermFrameIdx 值（按照第 8.2.5 节的规定分配给它） */
  // 8.2.5 Decoded reference picture marking process
  // 向每个短期参考图片分配变量PicNum，并且向每个长期参考图片分配变量LongTermPicNum
  int ret = 0;
  /* 帧编码 */
  if (header->field_pic_flag == 0) {
    for (int i = 0; i < MAX_DPB; i++) {
      // 获取各自的frame、top field和bottom field
      auto &pict_f = dpb[i]->m_picture_frame;
      auto &pict_t = dpb[i]->m_picture_top_filed;
      auto &pict_b = dpb[i]->m_picture_bottom_filed;
      /* 对于每个短期参考帧或互补参考场对 */
      ret = updateShortTermReference(pict_f, pict_t, pict_b, dpb[i]->PicNum);
      /* 对于每个长期参考帧或长期互补参考场对 */
      ret = updateLongTermReference(pict_f, pict_t, pict_b,
                                    dpb[i]->LongTermPicNum);
      RET(ret);
    }
    /* 场编码 */
  } else {
    for (int i = 0; i < MAX_DPB; i++) {
      auto &pict_t = dpb[i]->m_picture_top_filed;
      auto &pict_b = dpb[i]->m_picture_bottom_filed;
      bool isLongTerm;

      // 处理顶场
      if (pict_t.reference_marked_type == SHORT_REF ||
          pict_t.reference_marked_type == LONG_REF) {
        isLongTerm = pict_t.reference_marked_type == LONG_REF;
        dpb[i]->PicNum = updatePicNum(pict_t, m_picture_coded_type, isLongTerm);
      }

      // 处理底场
      if (pict_b.reference_marked_type == SHORT_REF ||
          pict_b.reference_marked_type == LONG_REF) {
        isLongTerm = pict_b.reference_marked_type == LONG_REF;
        dpb[i]->PicNum = updatePicNum(pict_b, m_picture_coded_type, isLongTerm);
      }
    }
  }

  return 0;
}

// 8.2.4.2 Initialisation process for reference picture lists
/* 当解码 P、SP 或 B 片头时会调用此初始化过程。 */
int PictureBase::init_ref_picture_lists(Frame *(&dpb)[16],
                                        Frame *(&RefPicList0)[16],
                                        Frame *(&RefPicList1)[16]) {
  const SliceHeader *header = m_slice->slice_header;
  const uint32_t slice_type = header->slice_type;
  int ret = 0;

  /* 初始化P、SP Slice的参考列表 */
  if (slice_type == SLICE_P || slice_type == SLICE_SP) {
    //对于帧编码
    //TODO 奇怪，为什么场编码也会进入到这里呢? <24-09-16 21:32:10, YangJing>
    if (m_picture_coded_type == FRAME)
      ret = init_ref_picture_list_P_SP_in_frames(dpb, RefPicList0,
                                                 m_RefPicList0Length);
    //对于场编码
    else if (m_picture_coded_type == TOP_FIELD ||
             m_picture_coded_type == BOTTOM_FIELD)
      ret = init_ref_picture_list_P_SP_in_fields(dpb, RefPicList0,
                                                 m_RefPicList0Length);

    /* 初始化B Slice的参考列表 */
  } else if (slice_type == SLICE_B) {
    //对于帧编码
    if (m_picture_coded_type == FRAME)
      ret = init_ref_picture_lists_B_in_frames(dpb, RefPicList0, RefPicList1,
                                               m_RefPicList0Length,
                                               m_RefPicList1Length);

    //对于场编码
    else if (m_picture_coded_type == TOP_FIELD ||
             m_picture_coded_type == BOTTOM_FIELD)
      ret = init_ref_picture_lists_B_in_fields(dpb, RefPicList0, RefPicList1,
                                               m_RefPicList0Length,
                                               m_RefPicList1Length);
  }
  RET(ret);

  /* 当第 8.2.4.2.1 至 8.2.4.2.5 节中，指定的初始 RefPicList0 或 RefPicList1 中的条目数：
   * - 分别大于 num_ref_idx_l0_active_minus1 + 1 或 num_ref_idx_l1_active_minus1 + 1 时，超过位置 num_ref_idx_l0_active_minus1 或 num_ref_idx_l1_active_minus1 的额外条目应被丢弃从初始参考图片列表中。 
  * - 分别小于 num_ref_idx_l0_active_minus1 + 1 或 num_ref_idx_l1_active_minus1 + 1 时，初始参考图片列表中的剩余条目为设置等于“无参考图片”。 */
  const uint32_t l0_num_ref = header->num_ref_idx_l0_active_minus1 + 1;
  if (m_RefPicList0Length > l0_num_ref) {
    for (uint32_t i = l0_num_ref; i < m_RefPicList0Length; i++)
      RefPicList0[i] = nullptr;
    m_RefPicList0Length = l0_num_ref;
  } else if (m_RefPicList0Length < l0_num_ref)
    for (uint32_t i = m_RefPicList0Length; i < l0_num_ref; i++)
      RefPicList0[i] = nullptr;
  //RefPicList0[i]->slice->slice_header->nal_ref_idc = 0;
  /* TODO YangJing 这里修改为非参考帧， <24-09-16 23:34:16> */

  const uint32_t l1_num_ref = header->num_ref_idx_l1_active_minus1 + 1;
  if (m_RefPicList1Length > l1_num_ref) {
    for (uint32_t i = l1_num_ref; i < m_RefPicList1Length; i++)
      RefPicList1[i] = nullptr;
    m_RefPicList1Length = l1_num_ref;
  } else if (m_RefPicList1Length < l1_num_ref)
    for (uint32_t i = m_RefPicList1Length; i < l1_num_ref; i++)
      RefPicList1[i] = nullptr;
  /* TODO YangJing 这里修改为非参考帧， <24-09-16 23:34:16> */

  return 0;
}

// 8.2.4.2.1 Initialisation process for the reference picture list for P and SP
/* RefPicList0为排序后的dpb*/
int PictureBase::init_ref_picture_list_P_SP_in_frames(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], uint32_t &RefPicList0Length) {
  /* 1. 参考图片列表RefPicList0排序，使得短期参考帧 < 长期参考帧 */

  /* 先把短期参考帧、长期参考帧的“索引”分开装 */
  vector<int32_t> indexTemp_short, indexTemp_long;
  for (int index = 0; index < (int)MAX_DPB; index++) {
    auto &pict_f = dpb[index]->m_picture_frame;
    if (pict_f.reference_marked_type == SHORT_REF)
      indexTemp_short.push_back(index);
    else if (pict_f.reference_marked_type == LONG_REF)
      indexTemp_long.push_back(index);
  }

  /* 当调用该过程时，应当有至少一个参考帧或互补参考场对当前被标记为“用于参考”（即，“用于短期参考”或“用于长期参考”） ）并且未标记为“不存在”*/
  RET(indexTemp_short.size() + indexTemp_long.size() <= 0);

  //排序的规则为：让时间最相近的优先

  /* 2. 短期参考帧，按照PicNum值实现 “降序” */
  sort(indexTemp_short.begin(), indexTemp_short.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicNum > dpb[b]->m_picture_frame.PicNum;
       });
  /* 3. 长期参考帧，按照LongTermPicNum值实现 “升序” */
  sort(indexTemp_long.begin(), indexTemp_long.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.LongTermPicNum <
                dpb[b]->m_picture_frame.LongTermPicNum;
       });

  // 4. 组合排序后的参考序列
  RefPicList0Length = 0;
  for (int index = 0; index < (int)indexTemp_short.size(); ++index)
    RefPicList0[RefPicList0Length++] = dpb[indexTemp_short[index]];

  for (int index = 0; index < (int)indexTemp_long.size(); ++index)
    RefPicList0[RefPicList0Length++] = dpb[indexTemp_long[index]];

  /* 比如对于参考列表中存在5帧图像，经过排序后应该是如下顺序： 
  RefPicList0[ 0 ] is set equal to the short-term reference picture with PicNum = 303,
  RefPicList0[ 1 ] is set equal to the short-term reference picture with PicNum = 302,
  RefPicList0[ 2 ] is set equal to the short-term reference picture with PicNum = 300,
  RefPicList0[ 3 ] is set equal to the long-term reference picture with LongTermPicNum = 0,
  RefPicList0[ 4 ] is set equal to the long-term reference picture with LongTermPicNum = 3.*/

  /* TODO YangJing 这里是在做什么？ <24-08-31 00:06:06> */
  /* 注 — 无论 MbaffFrameFlag 的值如何，非配对参考场都不用于解码帧的帧间预测。 */
  //if (slice_header->MbaffFrameFlag) {
  //for (uint32_t index = 0; index < RefPicList0Length; ++index) {
  //Frame *&ref_list_frame = RefPicList0[index];
  //Frame *&ref_list_top_filed = RefPicList0[16 + 2 * index];
  //Frame *&ref_list_bottom_filed = RefPicList0[16 + 2 * index + 1];

  //ref_list_top_filed = ref_list_bottom_filed = ref_list_frame;
  //}
  //}

  return 0;
}

//8.2.4.2.2 Initialization process for the reference picture list for P and SP slices in fields
/* 与PictureBase::init_reference_picture_list_P_SP_slices_in_frames同理 */
/* 注 – 当对场进行解码时，可供参考的图片数量实际上至少是对解码顺序中相同位置的帧进行解码时的两倍。*/
/* TODO YangJing 这个函数并没有经过测试验证 <24-08-31 00:49:10> */
int PictureBase::init_ref_picture_list_P_SP_in_fields(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], uint32_t &RefPicList0Length) {
  /* 1. 参考图片列表RefPicList0排序，使得短期互补参考场对 < 长期互补参考场对 */

  /* 先把短期参考场对、长期参考场对的“索引”分开装 */
  vector<Frame *> refFrameList0ShortTerm, refFrameList0LongTerm;
  for (int index = 0; index < (int)MAX_DPB; index++) {
    auto &pict_t = dpb[index]->m_picture_top_filed;
    auto &pict_b = dpb[index]->m_picture_bottom_filed;
    if (pict_t.reference_marked_type == SHORT_REF ||
        pict_b.reference_marked_type == SHORT_REF)
      refFrameList0ShortTerm.push_back(dpb[index]);
    else if (pict_t.reference_marked_type == LONG_REF ||
             pict_b.reference_marked_type == LONG_REF)
      refFrameList0LongTerm.push_back(dpb[index]);
  }

  /*2. "当前场"（上面是指"参考场")是互补参考场对的第二场（按解码顺序）并且第一场被标记为“用于短期参考”时，第一场被包括在短期参考帧列表refFrameList0ShortTerm中。*/
  Frame *curPic = this->m_parent;
  if (m_picture_coded_type == BOTTOM_FIELD) {
    if (curPic->m_picture_top_filed.reference_marked_type == SHORT_REF)
      refFrameList0ShortTerm.push_back(curPic);

    else if (curPic->m_picture_top_filed.reference_marked_type == LONG_REF)
      refFrameList0LongTerm.push_back(curPic);
  }

  /* 当调用这一过程时，应该有至少一个参考场（可以是参考帧的场）当前被标记为“用于参考”（即，“用于短期参考”或“用于参考”）。供长期参考”）并且没有标记为“不存在”。 */
  RET(refFrameList0ShortTerm.size() + refFrameList0LongTerm.size() == 0);

  /* 3. 短期互补场对，按照FrameNumWrap值实现 “降序” */
  sort(refFrameList0ShortTerm.begin(), refFrameList0ShortTerm.end(),
       [](Frame *a, Frame *b) {
         return (a->m_picture_top_filed.FrameNumWrap <
                 b->m_picture_top_filed.FrameNumWrap) ||
                (a->m_picture_bottom_filed.FrameNumWrap <
                 b->m_picture_bottom_filed.FrameNumWrap);
       });
  /* 4. 长期参考互补参考场对，按照LongTermFrameIdx值实现 “升序” */
  sort(refFrameList0LongTerm.begin(), refFrameList0LongTerm.end(),
       [](Frame *a, Frame *b) {
         return (a->m_picture_top_filed.LongTermFrameIdx <
                 b->m_picture_top_filed.LongTermFrameIdx) ||
                (a->m_picture_bottom_filed.LongTermFrameIdx <
                 b->m_picture_bottom_filed.LongTermFrameIdx);
       });

  /* 第 8.2.4.2.5 节中指定的过程是用 refFrameList0ShortTerm 和 refFrameList0LongTerm 作为输入来调用的，并且输出被分配给 RefPicList0。（组合排序后的参考序列） */
  init_ref_picture_lists_in_fields(refFrameList0ShortTerm,
                                   refFrameList0LongTerm, RefPicList0,
                                   RefPicList0Length, 0);

  return 0;
}

// 8.2.4.2.3 Initialisation process for reference picture lists for B slices in frames
/* 当解码编码帧中的 B 切片时，会调用此初始化过程。*/
/* 为了形成参考图片列表RefPicList0和RefPicList1，指的是解码的参考帧或互补参考场对。 */
int PictureBase::init_ref_picture_lists_B_in_frames(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], Frame *(&RefPicList1)[16],
    uint32_t &RefPicList0Length, uint32_t &RefPicList1Length) {

  /* 对于B切片，参考图片列表RefPicList0和RefPicList1中的短期参考条目的顺序取决于输出顺序，如由PicOrderCnt()给出的。当 pic_order_cnt_type 等于 0 时，如第 8.2.5.2 节中指定的被标记为“不存在”的参考图片不包括在 RefPicList0 或 RefPicList1 中 */

  //------------------------------------ RefPicList0（向前参考） ----------------------------------------
  /* 1. 先把短期参考帧、长期参考帧的“索引”分开装。对于短期参考帧而言，由于是B帧存在POC乱序，故需要按照先后POC进行分开存储，其中PicOrderCnt 小于当前帧的短期参考帧为"较老的帧"，反之为"教新的帧"*/
  vector<int32_t> indexTemp_short_left, indexTemp_short_right, indexTemp_long;
  for (int32_t i = 0; i < MAX_DPB; i++) {
    auto &pict_f = dpb[i]->m_picture_frame;
    if (pict_f.reference_marked_type == SHORT_REF) {
      if (pict_f.PicOrderCnt < PicOrderCnt)
        indexTemp_short_left.push_back(i);
      else
        indexTemp_short_right.push_back(i);
    } else if (pict_f.reference_marked_type == LONG_REF)
      indexTemp_long.push_back(i);
  }

  /* 当调用该过程时，应至少有一个参考条目当前被标记为“用于参考”（即“用于短期参考”或“用于长期参考”）并且未被标记为“不存在” */
  RET(indexTemp_short_left.size() + indexTemp_short_right.size() +
          indexTemp_long.size() <=
      0);

  //排序的规则为：让时间最相近的优先

  /* 2. 短期参考帧，小于当前POC（含有IDR帧），按照POC值实现 “降序”，一般是先编码的 */
  sort(indexTemp_short_left.begin(), indexTemp_short_left.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicOrderCnt >
                dpb[b]->m_picture_frame.PicOrderCnt;
       });
  /* 3. 短期参考帧，大于当前POC（不含有IDR帧），按照POC值实现 “升序”，一般是后编码的 */
  sort(indexTemp_short_right.begin(), indexTemp_short_right.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicOrderCnt <
                dpb[b]->m_picture_frame.PicOrderCnt;
       });
  /* 4. 长期参考帧，按照LongTermPicNum值实现 “升序” */
  sort(indexTemp_long.begin(), indexTemp_long.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.LongTermPicNum <
                dpb[b]->m_picture_frame.LongTermPicNum;
       });

  // 4. 组合排序后的参考序列
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

  //------------------------------------ RefPicList1（向后参考） ----------------------------------------
  /* 1. 先把短期参考帧、长期参考帧的“索引”分开装。对于短期参考帧而言，由于是B帧存在POC乱序，故需要按照先后POC进行分开存储，其中PicOrderCnt 大于当前帧的短期参考帧为"较新的帧"，反之为"教旧的帧"*/
  for (int32_t i = 0; i < MAX_DPB; i++) {
    auto &pict_f = dpb[i]->m_picture_frame;
    if (pict_f.reference_marked_type == SHORT_REF) {
      if (pict_f.PicOrderCnt > PicOrderCnt)
        indexTemp_short_left.push_back(i);
      else
        indexTemp_short_right.push_back(i);
    } else if (pict_f.reference_marked_type == LONG_REF)
      indexTemp_long.push_back(i);
  }

  /* 当调用该过程时，应至少有一个参考条目当前被标记为“用于参考”（即“用于短期参考”或“用于长期参考”）并且未被标记为“不存在” */
  RET(indexTemp_short_left.size() + indexTemp_short_right.size() +
          indexTemp_long.size() <=
      0);

  /* 2. 短期参考帧，大于当前POC（不含有IDR帧），按照PicNum值实现 “升序”，一般是后编码的(因为是向后参考，所以这里时间最相近的“后”一帧是最优先的 */
  sort(indexTemp_short_left.begin(), indexTemp_short_left.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicOrderCnt <
                dpb[b]->m_picture_frame.PicOrderCnt;
       });
  /* 3. 短期参考帧，小于当前POC（含有IDR帧），按照PicNum值实现 “降序”，一般是先编码的 */
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
  bool isSame = true;
  for (uint32_t i = 0; i < RefPicList1Length; i++)
    if (RefPicList1[i] != RefPicList0[i]) {
      isSame = false;
      break;
    }
  if (isSame) swap(RefPicList1[0], RefPicList1[1]);
  return 0;
}

// 8.2.4.2.4 Initialization process for reference picture lists for B slices in fields
// 当对编码场中的 B 切片进行解码时，会调用此初始化过程。
/* 当调用这一过程时，应该有至少一个参考场（可以是参考帧的场）当前被标记为“用于参考”（即，“用于短期参考”或“用于参考”）。供长期参考”）并且没有标记为“不存在”。 */
/* TODO YangJing 未经测试，大概率有问题 <24-08-31 15:06:47> */
int PictureBase::init_ref_picture_lists_B_in_fields(
    Frame *(&dpb)[16], Frame *(&RefPicList0)[16], Frame *(&RefPicList1)[16],
    uint32_t &RefPicList0Length, uint32_t &RefPicList1Length) {
  Frame *refFrameList0ShortTerm[16] = {nullptr};
  Frame *refFrameList1ShortTerm[16] = {nullptr};
  Frame *refFrameListLongTerm[16] = {nullptr};

  /* 当解码场时，存储的参考帧的每个场被识别为具有唯一索引的单独的参考图片。参考图片列表RefPicList0和RefPicList1中的短期参考图片的顺序取决于输出顺序，如由PicOrderCnt()给出的。当pic_order_cnt_type等于0时，如条款8.2.5.2中指定的被标记为“不存在”的参考图片不包括在RefPicList0或RefPicList1中。 */

  //------------------------------------ RefPicList0（向前参考） ----------------------------------------
  /* 与帧一样的逻辑（前参考） */
  vector<int32_t> indexTemp_short_left, indexTemp_short_right, indexTemp_long;
  for (int32_t i = 0; i < MAX_DPB; i++) {
    auto &pict_f = dpb[i]->m_picture_frame;
    if (pict_f.reference_marked_type == SHORT_REF) {
      if (pict_f.PicOrderCnt < PicOrderCnt)
        indexTemp_short_left.push_back(i);
      else
        indexTemp_short_right.push_back(i);
    } else if (pict_f.reference_marked_type == LONG_REF)
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

  /* 2. 短期参考场，小于当前POC（含有IDR帧），按照POC值实现 “降序”，一般是先编码的 */
  sort(indexTemp_short_left.begin(), indexTemp_short_left.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicOrderCnt >
                dpb[b]->m_picture_frame.PicOrderCnt;
       });
  /* 3. 短期参考场，大于当前POC（不含有IDR帧），按照POC值实现 “升序”，一般是后编码的 */
  sort(indexTemp_short_right.begin(), indexTemp_short_right.end(),
       [&dpb](int32_t a, int32_t b) {
         return dpb[a]->m_picture_frame.PicOrderCnt <
                dpb[b]->m_picture_frame.PicOrderCnt;
       });
  /* 4. 长期参考场，按照LongTermPicNum值实现 “升序” */
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
  int ret = init_ref_picture_lists_in_fields(frame0Short, frameLong,
                                             RefPicList0, RefPicList0Length, 0);
  RET(ret);

  /* 第 8.2.4.2.5 节中指定的过程通过作为输入给出的 refFrameList1ShortTerm 和 refFrameListLongTerm 进行调用，并将输出分配给 RefPicList1。 */
  vector<Frame *> frame1Short(begin(refFrameList1ShortTerm),
                              end(refFrameList1ShortTerm));
  ret = init_ref_picture_lists_in_fields(frame1Short, frameLong, RefPicList1,
                                         RefPicList1Length, 1);
  RET(ret);

  /* 当参考图片列表RefPicList1具有多于一个条目并且RefPicList1与参考图片列表RefPicList0相同时，前两个条目RefPicList1[0]和RefPicList1[1]被交换。 */
  bool isSame = true;
  for (uint32_t i = 0; i < RefPicList1Length; i++)
    if (RefPicList1[i] != RefPicList0[i]) {
      isSame = false;
      break;
    }
  if (isSame) swap(RefPicList1[0], RefPicList1[1]);
  return 0;
}

void process_ref_picture_lists_in_fields(vector<Frame *> &refFrameList,
                                         Frame *(&RefPicListX)[16],
                                         int32_t &index,
                                         H264_PICTURE_CODED_TYPE &coded_type,
                                         int32_t list_end) {
  for (int i = 0; i < list_end; i++) {
    Frame *frame = refFrameList[i];
    // top field
    if (frame->m_picture_top_filed.m_picture_coded_type == coded_type) {
      RefPicListX[index] = frame;
      frame->m_pic_coded_type_marked_as_refrence = coded_type;
      frame->reference_marked_type = SHORT_REF;
      coded_type = (coded_type == TOP_FIELD) ? BOTTOM_FIELD : TOP_FIELD;
      index++;
    }
    // bottom field
    if (frame->m_picture_bottom_filed.m_picture_coded_type == coded_type) {
      RefPicListX[index] = frame;
      frame->m_pic_coded_type_marked_as_refrence = coded_type;
      frame->reference_marked_type = SHORT_REF;
      coded_type = (coded_type == TOP_FIELD) ? BOTTOM_FIELD : TOP_FIELD;
      index++;
    }
  }
}

// 8.2.4.2.5 Initialisation process for reference picture lists in fields
int PictureBase::init_ref_picture_lists_in_fields(
    vector<Frame *>(&refFrameListXShortTerm),
    vector<Frame *>(&refFrameListXLongTerm), Frame *(&RefPicListX)[16],
    uint32_t &RefPicListXLength, int32_t listX) {

  const int32_t size = 16;
  int32_t index = 0;
  H264_PICTURE_CODED_TYPE coded_type = m_picture_coded_type;

  // Process short term reference frames
  process_ref_picture_lists_in_fields(refFrameListXShortTerm, RefPicListX,
                                      index, coded_type, size);
  // Process long term reference frames
  process_ref_picture_lists_in_fields(refFrameListXLongTerm, RefPicListX, index,
                                      coded_type, size);
  RefPicListXLength = index;
  return 0;
}

// 8.2.4.3 Modification process for reference picture lists
/* 修改指的是调整参考帧列表的优先级，由于修改的操作问题，这里可能会出现重复参考帧的问题（一般会出现在最后一个）*/
int PictureBase::modif_ref_picture_lists(Frame *(&RefPicList0)[16],
                                         Frame *(&RefPicList1)[16]) {
  SliceHeader *header = m_slice->slice_header;
  /* 输出 */
  int32_t &refIdxL0 = header->refIdxL0;
  int32_t &refIdxL1 = header->refIdxL1;
  int32_t &picNumL0Pred = header->picNumL0Pred;
  int32_t &picNumL1Pred = header->picNumL1Pred;

  /* 输入 */
  const uint32_t &num_ref_idx_l0_active_minus1 =
      header->num_ref_idx_l0_active_minus1;
  const uint32_t &num_ref_idx_l1_active_minus1 =
      header->num_ref_idx_l1_active_minus1;

  /* 当对于Slice第一次调用本子句中指定的过程时（即，对于在ref_pic_list_modification()语法中第一次出现modification_of_pic_nums_idc等于0或1），picNumL0Pred和picNumL1Pred最初被设置为等于CurrPicNum。 */
  header->picNumL0Pred = header->picNumL1Pred = header->CurrPicNum;

  int ret;
  if (header->ref_pic_list_modification_flag_l0) {
    // 1.令refIdxL0为参考图片列表RefPicList0中的索引。它最初设置为等于0。
    refIdxL0 = 0;
    for (int i = 0; i < header->ref_pic_list_modification_count_l0; i++) {
      int32_t modif_idc = header->modification_of_pic_nums_idc[0][i];
      //* — 使用refIdxL0作为输入调用第8.2.4.3.1节中指定的过程，并将输出分配给refIdxL0。
      if ((modif_idc & ~1) == 0) {
        ret = modif_ref_picture_lists_for_short_ref_pictures(
            refIdxL0, picNumL0Pred, modif_idc,
            header->abs_diff_pic_num_minus1[0][i] + 1,
            num_ref_idx_l0_active_minus1, RefPicList0);
        //* — 使用refIdxL0作为输入调用第8.2.4.3.2节中指定的过程，并将输出分配给refIdxL0。
      } else if (modif_idc == 2) {
        ret = modif_ref_picture_lists_for_long_ref_pictures(
            refIdxL0, num_ref_idx_l0_active_minus1,
            header->long_term_pic_num[0][i], RefPicList0);
        //* – 否则（modification_of_pic_nums_idc等于3），参考图片列表RefPicList0的修改过程完成。
      } else
        break;
      RET(ret);
    }
  }

  /* 当前切片是B切片并且ref_pic_list_modification_flag_l1等于1时，以下适用： */
  if (header->slice_type == SLICE_B &&
      header->ref_pic_list_modification_flag_l1) {
    /* 1.令refIdxL1为参考图片列表RefPicList1的索引。它最初设置为 0。 */
    refIdxL1 = 0;
    /* 2.对应的语法元素modification_of_pic_nums_idc按照它们在比特流中出现的顺序进行处理。对于每个语法元素，以下内容适用： */
    for (int i = 0; i < header->ref_pic_list_modification_count_l1; i++) {
      int32_t modif_idc = header->modification_of_pic_nums_idc[1][i];
      // – 使用refIdxL1作为输入调用第8.2.4.3.1节中指定的过程，并将输出分配给refIdxL1。
      if ((modif_idc & ~1) == 0) {
        ret = modif_ref_picture_lists_for_short_ref_pictures(
            refIdxL1, picNumL1Pred, modif_idc,
            header->abs_diff_pic_num_minus1[1][i] + 1,
            num_ref_idx_l1_active_minus1, RefPicList1);
        // — 使用refIdxL1作为输入调用第8.2.4.3.2节中指定的过程，并将输出分配给refIdxL1。
      } else if (modif_idc == 2) {
        ret = modif_ref_picture_lists_for_long_ref_pictures(
            refIdxL1, num_ref_idx_l1_active_minus1,
            header->long_term_pic_num[1][i], RefPicList1);
      } else
        break;
      RET(ret);
    }
  }
  return 0;
}

// 8.2.4.3.1 Modification process of reference picture lists for short-term
/* 该过程的输入是索引 refIdxLX（X 为 0 或 1）
 * 该过程的输出是递增的索引 refIdxLX。*/
int PictureBase::modif_ref_picture_lists_for_short_ref_pictures(
    int32_t &refIdxLX, int32_t &picNumLXPred, int32_t modif_idc,
    int32_t abs_diff_pic_num, int32_t num_ref_idx_lX_active_minus1,
    Frame *(&RefPicListX)[16]) {

  const SliceHeader *header = m_slice->slice_header;
  const int MaxPicNum = header->MaxPicNum;

  /* 计算picNumLXNoWrap（未包裹的短期参考帧编号），参考帧编号有最大值限制（MaxPicNum），所以如果差值计算导致超出范围，需要进行“包裹”（wrap-around）处理。 */
  int32_t picNumLXNoWrap = 0;
  /* 减法操作：参考帧编号需要减去 abs_diff_pic_num，以找到之前的一个参考帧编号 */
  if (modif_idc == 0) {
    //预测图像编号 - 当前参考帧与当前帧之间的 PicNum 的绝对差值
    if (picNumLXPred - abs_diff_pic_num < 0)
      picNumLXNoWrap = picNumLXPred - abs_diff_pic_num + MaxPicNum;
    else
      picNumLXNoWrap = picNumLXPred - abs_diff_pic_num;

    /* 加法操作：参考帧编号需要加上 abs_diff_pic_num，以找到之前的一个参考帧编号 */
  } else {
    //预测图像编号 + 当前参考帧与当前帧之间的 PicNum 的绝对差值
    if (picNumLXPred + abs_diff_pic_num >= MaxPicNum)
      picNumLXNoWrap = picNumLXPred + abs_diff_pic_num - MaxPicNum;
    else
      picNumLXNoWrap = picNumLXPred + abs_diff_pic_num;
  }

  /* 每次分配 picNumLXNoWrap 后，picNumLXNoWrap 的值都会分配给 picNumLXPred */
  picNumLXPred = picNumLXNoWrap;

  /* 对picNumLXNoWrap去环绕得到picNumLX */
  int32_t picNumLX = picNumLXNoWrap;
  if (picNumLXNoWrap > header->CurrPicNum)
    picNumLX = picNumLXNoWrap - MaxPicNum;

  /* TODO:picNumLX应等于标记为“用于短期参考”的参考图片的PicNum，并且不应等于标记为“不存在”的短期参考图片的PicNum。 */

  /* 执行以下过程以将具有短期图像编号picNumLX的图像放置到索引位置refIdxLX中，将任何其他剩余图像的位置移动到列表中的后面，并且递增refIdxLX的值。*/

  int32_t cIdx;
  /* 在待修改的帧处，复制一份，并将之后的帧整体往后移动一位，如：
  RefPicListX = {0x5555555f8ff0, 0x5555555f71a0, 0x5555555f5350, 0x0 <repeats 13 times>}
  refIdxLX = 0时，移动后为：
  RefPicListX = {0x5555555f8ff0, 0x5555555f8ff0, 0x5555555f71a0, 0x5555555f5350, 0x0 <repeats 12 times>}
                    (新增）*/
  for (cIdx = num_ref_idx_lX_active_minus1 + 1; cIdx > refIdxLX; cIdx--)
    RefPicListX[cIdx] = RefPicListX[cIdx - 1];

  for (cIdx = 0; cIdx < num_ref_idx_lX_active_minus1 + 1; cIdx++) {
    /* 根据需要修改的具体帧编号，且该帧编号为被参考帧状态，得到该帧在参考列表中的索引值 */
    if (RefPicListX[cIdx]->PicNum == picNumLX &&
        RefPicListX[cIdx]->reference_marked_type == SHORT_REF)
      break;
  }

  /*将待修改的帧，放到前面挪出空位的索引处，则有(假设cIdx为0）：
  RefPicListX = {0x5555555f8ff0, 0x5555555f8ff0, 0x5555555f71a0, 0x5555555f5350, 0x0 <repeats 12 times>}
              (由RefPicListX[0]覆盖，所以这里并没有变化）*/
  RefPicListX[refIdxLX++] = RefPicListX[cIdx];

  /* TODO YangJing 还有问题，在某些情况下，前两帧可能会出现重复问题 <24-09-17 14:23:00> */
  int32_t nIdx = refIdxLX;
  /* 从挪动的后面列表开始遍历 */
  for (cIdx = refIdxLX; cIdx < num_ref_idx_lX_active_minus1 + 2; cIdx++) {
    if (RefPicListX[cIdx]) {
      int32_t PicNumF;
      /* 图片被标记为“用于短期参考”，则PicNumF(RefPicListX[cIdx])是图片RefPicListX[cIdx]的PicNum
       * 否则(图片RefPicListX[cIdx]没有被标记为“用于短期参考”)，PicNumF(RefPicListX[cIdx])等于MaxPicNum。 */
      if (RefPicListX[cIdx]->reference_marked_type == SHORT_REF)
        PicNumF = RefPicListX[cIdx]->PicNum;
      else
        PicNumF = MaxPicNum;

      /* 对于其他帧（非修改的帧）依次放在修改帧的后面 */
      if (PicNumF != picNumLX) RefPicListX[nIdx++] = RefPicListX[cIdx];
    }
  }
  /* 最后处理得到列表为：
   * refpiclistx = {0x5555555f8ff0, 0x5555555f71a0, 0x5555555f5350, 0x5555555f5350, 0x0 <repeats 12 times>}*/

  /* 除去最后一位多余的帧，还原参考列表实际可用帧长度 */
  RefPicListX[num_ref_idx_lX_active_minus1 + 1] = nullptr;
  /* 最后处理得到列表为：
   * ref pic list = {0x5555555f8ff0, 0x5555555f71a0, 0x5555555f5350, 0x0, 0x0 <repeats 12 times>}*/
  return 0;
}

// 8.2.4.3.2 Modification process of reference picture lists for long-term
/* 该过程的输入是索引 refIdxLX（X 为 0 或 1）。  
 * 该过程的输出是递增的索引 refIdxLX。*/
int PictureBase::modif_ref_picture_lists_for_long_ref_pictures(
    int32_t &refIdxLX, int32_t num_ref_idx_lX_active_minus1,
    int32_t long_term_pic_num, Frame *(&RefPicListX)[16]) {

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
    if (RefPicListX[cIdx]->reference_marked_type == LONG_REF)
      LongTermPicNumF = RefPicListX[cIdx]->LongTermPicNum;
    else
      LongTermPicNumF = 2 * (MaxLongTermFrameIdx + 1);

    if (LongTermPicNumF != long_term_pic_num)
      RefPicListX[nIdx++] = RefPicListX[cIdx];
  }

  /* 在该伪代码过程中，列表RefPicListX的长度暂时比最终列表所需的长度长一个元素。执行此过程后，仅需要保留列表中的 0 到 num_ref_idx_lX_active_minus1 元素。 */
  /* TODO YangJing 可能会有问题 <24-08-31 16:16:26> */
  RefPicListX[num_ref_idx_lX_active_minus1 + 1] = nullptr;
  return 0;
}

// 8.2.5 Decoded reference picture marking process
// 8.2.5.1 Sequence of operations for decoded reference picture marking process
int PictureBase::decoded_reference_picture_marking(Frame *(&dpb)[16]) {
  const SliceHeader *header = m_slice->slice_header;
  Frame *curPic = this->m_parent;
  // IDR帧
  if (header->IdrPicFlag) {
    // 将所有已解码参考图片均标记为"未用于参考"，以及编码类型标记为"未知"
    for (int i = 0; i < MAX_DPB; i++) {
      dpb[i]->reference_marked_type = UNUSED_REF;
      dpb[i]->m_pic_coded_type_marked_as_refrence = UNKNOWN;
      // 需要先将之前解码的所有帧输出去
      if (dpb[i] != curPic) dpb[i]->m_picture_coded_type = UNKNOWN;
    }
    // 当前帧被标记为短期参考帧，并且没有长期参考帧索引
    if (header->long_term_reference_flag == false) {
      MaxLongTermFrameIdx = NA;
      reference_marked_type = SHORT_REF;
      curPic->reference_marked_type = SHORT_REF;
    }
    // 当前帧被标记为长期参考帧，并且长期参考帧索引为初始值0
    else {
      reference_marked_type = LONG_REF;
      LongTermFrameIdx = 0, MaxLongTermFrameIdx = 0;
      curPic->reference_marked_type = LONG_REF;
    }
    // 当前图像为帧类型，则当前的标记类型也设为参考帧类型
    if (curPic->m_picture_coded_type == FRAME)
      curPic->m_pic_coded_type_marked_as_refrence = FRAME;
    else
      curPic->m_pic_coded_type_marked_as_refrence = COMPLEMENTARY_FIELD_PAIR;
  }

  // 普通帧
  else {
    if (header->adaptive_ref_pic_marking_mode_flag)
      // 自适应内存控制管理参考帧
      adaptive_memory_control_decoded_reference_picture_marking(dpb);
    else
      // 使用滑动窗口机制来管理参考帧
      sliding_window_decoded_reference_picture_marking(dpb);
  }

  // 当当前图片不是IDR图片并且没有被标记为“用于长期参考”时，它被标记为“用于短期参考”
  if (header->IdrPicFlag == false && mmco_6_flag == false) {
    MaxLongTermFrameIdx = NA;
    reference_marked_type = SHORT_REF;
    curPic->reference_marked_type = SHORT_REF;
    if (curPic->m_picture_coded_type == FRAME)
      curPic->m_pic_coded_type_marked_as_refrence = FRAME;
    else
      curPic->m_pic_coded_type_marked_as_refrence = COMPLEMENTARY_FIELD_PAIR;
  }

  return 0;
}

// 8.2.5.3 Sliding window decoded reference picture marking process
// 滑动窗口解码参考图像的标识过程
int PictureBase::sliding_window_decoded_reference_picture_marking(
    Frame *(&dpb)[16]) {
  const PictureBase &m_picture_top_filed = m_parent->m_picture_top_filed;
  uint32_t max_num_ref_frames =
      m_slice->slice_header->m_sps->max_num_ref_frames;
  // 当前图片为底场，且顶已被标记为“用于短期参考”，则当前图片和互补参考场对也被标记为“用于短期参考”，且当前图像参考类型标记为互补场对
  if (this->m_picture_coded_type == BOTTOM_FIELD &&
      (m_picture_top_filed.reference_marked_type == SHORT_REF)) {
    reference_marked_type = SHORT_REF;
    m_parent->reference_marked_type = SHORT_REF;
    m_parent->m_pic_coded_type_marked_as_refrence = COMPLEMENTARY_FIELD_PAIR;
  } else {

    // 遍历参考帧缓冲区，统计当前缓冲区中短期参考帧和长期参考帧的数量
    uint32_t numShortTerm = 0, numLongTerm = 0;
    for (int i = 0; i < MAX_DPB; i++) {
      auto marked_f = dpb[i]->m_picture_frame.reference_marked_type;
      auto marked_t = dpb[i]->m_picture_top_filed.reference_marked_type;
      auto marked_b = dpb[i]->m_picture_bottom_filed.reference_marked_type;

      if (marked_f == SHORT_REF || marked_t == SHORT_REF ||
          marked_b == SHORT_REF)
        numShortTerm++;
      if (marked_f == LONG_REF || marked_t == LONG_REF || marked_b == LONG_REF)
        numLongTerm++;
    }

    // 当前参考帧的总数达到了最大参考帧数，并且存在短期参考帧，则丢弃最旧的短期参考帧
    if (numShortTerm + numLongTerm == MAX(max_num_ref_frames, 1) &&
        numShortTerm > 0) {
      PictureBase *refPic = nullptr;
      int32_t index = -1;

      //遍历参考帧缓冲区，查找最旧的短期参考帧。最旧的短期参考帧通过比较"帧号(编码顺序)"来确定
      for (int i = 0; i < MAX_DPB; i++) {
        //检查完整帧（m_picture_frame）、顶场（m_picture_top_filed）和底场（m_picture_bottom_filed）的参考标记，确保找到最旧的短期参考帧。
        auto marked_f = dpb[i]->m_picture_frame.reference_marked_type;
        auto marked_t = dpb[i]->m_picture_top_filed.reference_marked_type;
        auto marked_b = dpb[i]->m_picture_bottom_filed.reference_marked_type;
        // 帧类型
        if (marked_f == SHORT_REF) {
          // 初始默认为首帧
          if (index == -1) index = i, refPic = &dpb[i]->m_picture_frame;
          // 比较帧号，哪个更“旧”
          else if (dpb[i]->m_picture_frame.FrameNumWrap <
                   dpb[index]->m_picture_frame.FrameNumWrap)
            index = i, refPic = &dpb[i]->m_picture_frame;
        }
        // 场类型(顶场)，逻辑同帧类型
        if (marked_t == SHORT_REF) {
          if (index == -1)
            index = i, refPic = &dpb[i]->m_picture_top_filed;
          else if (dpb[i]->m_picture_top_filed.FrameNumWrap <
                   dpb[index]->m_picture_top_filed.FrameNumWrap)
            index = i, refPic = &dpb[i]->m_picture_top_filed;
        }
        // 场类型(底场)，逻辑同帧类型
        if (marked_b == SHORT_REF) {
          if (index == -1)
            index = i, refPic = &dpb[i]->m_picture_bottom_filed;
          else if (dpb[i]->m_picture_bottom_filed.FrameNumWrap <
                   dpb[index]->m_picture_bottom_filed.FrameNumWrap)
            index = i, refPic = &dpb[i]->m_picture_bottom_filed;
        }
      }

      // 如果找到了最旧的短期参考帧，则将其标记为“未使用参考帧”
      if (index >= 0 && refPic != nullptr) {
        refPic->reference_marked_type = UNUSED_REF;
        auto code_type = refPic->m_parent->m_picture_coded_type;
        auto &mark_type = refPic->m_parent->reference_marked_type;
        if (code_type == FRAME)
          mark_type = UNUSED_REF,
          refPic->m_parent->m_pic_coded_type_marked_as_refrence = UNKNOWN;
        else if (code_type == COMPLEMENTARY_FIELD_PAIR) {
          mark_type = UNUSED_REF,
          refPic->m_parent->m_pic_coded_type_marked_as_refrence = UNKNOWN;
          refPic->m_parent->m_picture_top_filed.reference_marked_type =
              UNUSED_REF;
          refPic->m_parent->m_picture_bottom_filed.reference_marked_type =
              UNUSED_REF;
        }
      }
    }
  }

  return 0;
}

// 8.2.5.4.1 Marking process of a short-term reference picture as "unused for reference"
int marking_short_term_ref_picture_as_unused_for_ref(Frame *(&dpb)[16],
                                                     int32_t picNumX,
                                                     bool field_pic_flag) {

  for (int32_t i = 0; i < MAX_DPB; i++) {
    Frame *pic = dpb[i];
    auto &marked_f = pic->m_picture_frame.reference_marked_type;
    auto &marked_t = pic->m_picture_top_filed.reference_marked_type;
    auto &marked_b = pic->m_picture_bottom_filed.reference_marked_type;

    auto pic_type_t = pic->m_picture_top_filed.m_picture_coded_type;
    auto pic_type_b = pic->m_picture_bottom_filed.m_picture_coded_type;

    auto picNum_f = pic->m_picture_frame.PicNum;
    auto picNum_t = pic->m_picture_top_filed.PicNum;
    auto picNum_b = pic->m_picture_bottom_filed.PicNum;

    // picNumX指定的短期参考帧或短期互补参考场对及其两个场都被标记为“未用于参考”
    if (field_pic_flag == false) {
      if ((marked_f == SHORT_REF && picNum_f == picNumX) ||
          (marked_t == SHORT_REF && picNum_t == picNumX) ||
          (marked_b == SHORT_REF && picNum_b == picNumX)) {
        pic->reference_marked_type = UNUSED_REF;
        pic->m_pic_coded_type_marked_as_refrence = UNKNOWN; //TODO
        marked_f = UNUSED_REF, marked_t = UNUSED_REF, marked_b = UNUSED_REF;
      }
    }
    // picNumX指定的短期参考场被标记为“未使用”
    else {
      if (marked_t == SHORT_REF && picNum_t == picNumX) {
        pic->reference_marked_type = marked_b;
        pic->m_pic_coded_type_marked_as_refrence = pic_type_b;
        marked_f = UNUSED_REF, marked_t = UNUSED_REF;
      } else if (marked_b == SHORT_REF && picNum_b == picNumX) {
        pic->reference_marked_type = marked_t;
        pic->m_pic_coded_type_marked_as_refrence = pic_type_t;
        marked_f = UNUSED_REF, marked_b = UNUSED_REF;
      }
    }
  }
  return 0;
}

// 8.2.5.4.2 Marking process of a long-term reference picture as "unused for reference"
// NOTE: 逻辑同marking_short_term_ref_picture_as_unused_for_ref()
int marking_long_term_ref_picture_as_unused_for_ref(Frame *(&dpb)[16],
                                                    int32_t longPicNumX,
                                                    bool field_pic_flag) {
  for (int32_t i = 0; i < MAX_DPB; i++) {
    Frame *pic = dpb[i];
    auto &marked_f = pic->m_picture_frame.reference_marked_type;
    auto &marked_t = pic->m_picture_top_filed.reference_marked_type;
    auto &marked_b = pic->m_picture_bottom_filed.reference_marked_type;

    auto pic_type_t = pic->m_picture_top_filed.m_picture_coded_type;
    auto pic_type_b = pic->m_picture_bottom_filed.m_picture_coded_type;

    auto longPicNum_f = pic->m_picture_frame.LongTermPicNum;
    auto longPicNum_t = pic->m_picture_top_filed.LongTermPicNum;
    auto longPicNum_b = pic->m_picture_bottom_filed.LongTermPicNum;

    // LongTermPicNum等于long_term_pic_num的长期参考帧或长期互补参考场对，并且其两个场都被标记为“未用于参考”
    if (field_pic_flag == false) {
      if ((marked_f == LONG_REF && longPicNum_f == longPicNumX) ||
          (marked_t == LONG_REF && longPicNum_t == longPicNumX) ||
          (marked_b == LONG_REF && longPicNum_b == longPicNumX)) {
        pic->reference_marked_type = UNUSED_REF;
        pic->m_pic_coded_type_marked_as_refrence = UNKNOWN;
        marked_f = UNUSED_REF, marked_t = UNUSED_REF, marked_b = UNUSED_REF;
      }
    } else {
      if (marked_t == LONG_REF && longPicNum_t == longPicNumX) {
        pic->reference_marked_type = marked_b;
        pic->m_pic_coded_type_marked_as_refrence = pic_type_b;
        marked_f = UNUSED_REF, marked_t = UNUSED_REF;
      } else if (marked_b == LONG_REF && longPicNum_b == longPicNumX) {
        pic->reference_marked_type = marked_t;
        pic->m_pic_coded_type_marked_as_refrence = pic_type_t;
        marked_f = UNUSED_REF, marked_b = UNUSED_REF;
      }
    }
  }
  return 0;
}

// 8.2.5.4.3 Assignment process of a LongTermFrameIdx to a short-term reference picture
int assignment_longTermFrameIdx_to_short_term_ref_picture(
    Frame *(&dpb)[16], int32_t picNumX, int32_t long_term_frame_idx,
    bool field_pic_flag, int32_t &LongTermFrameIdx) {

  int32_t picNumXIndex = -1;
  for (int32_t index = 0; index < MAX_DPB; index++) {
    Frame *pic = dpb[index];
    auto &marked_f = pic->m_picture_frame.reference_marked_type;
    auto &marked_t = pic->m_picture_top_filed.reference_marked_type;
    auto &marked_b = pic->m_picture_bottom_filed.reference_marked_type;

    auto picNum_t = pic->m_picture_top_filed.PicNum;
    auto picNum_b = pic->m_picture_bottom_filed.PicNum;
    // 当等于long_term_frame_idx的LongTermFrameIdx已经被分配给长期参考帧或长期互补参考场对时，该帧或互补场对及其两个场被标记为“未用于参考”。
    if (marked_f == LONG_REF &&
        pic->m_picture_frame.LongTermFrameIdx == long_term_frame_idx) {
      pic->reference_marked_type = UNUSED_REF;
      pic->m_pic_coded_type_marked_as_refrence = UNKNOWN;
      marked_f = UNUSED_REF, marked_t = UNUSED_REF, marked_b = UNUSED_REF;
    }

    // 当 LongTermFrameIdx 已分配给参考场，并且该参考场不是包括 picNumX 指定的图片的互补场对的一部分时，该场被标记为“未用于参考”
    if ((marked_t == SHORT_REF && picNum_t == picNumX) ||
        (marked_b == SHORT_REF && picNum_b == picNumX))
      picNumXIndex = index;
  }

  for (int32_t index = 0; index < MAX_DPB; index++) {
    Frame *pic = dpb[index];
    auto &marked_f = pic->m_picture_frame.reference_marked_type;
    auto &marked_t = pic->m_picture_top_filed.reference_marked_type;
    auto &marked_b = pic->m_picture_bottom_filed.reference_marked_type;

    auto pic_type_t = pic->m_picture_top_filed.m_picture_coded_type;
    auto pic_type_b = pic->m_picture_bottom_filed.m_picture_coded_type;

    auto longFrameIdx_t = pic->m_picture_top_filed.LongTermFrameIdx;
    auto longFrameIdx_b = pic->m_picture_bottom_filed.LongTermFrameIdx;

    if (marked_t == LONG_REF && longFrameIdx_t == long_term_frame_idx) {
      if (index != picNumXIndex) {
        pic->reference_marked_type = UNUSED_REF;
        pic->m_pic_coded_type_marked_as_refrence = pic_type_b;
        marked_f = marked_b, marked_t = UNUSED_REF;
      }
    } else if (marked_b == LONG_REF && longFrameIdx_b == long_term_frame_idx) {
      if (index != picNumXIndex) {
        pic->reference_marked_type = UNUSED_REF;
        pic->m_pic_coded_type_marked_as_refrence = pic_type_t;
        marked_f = marked_t, marked_b = UNUSED_REF;
      }
    }
  }

  for (int32_t i = 0; i < MAX_DPB; i++) {
    Frame *pic = dpb[i];
    auto &marked_f = pic->m_picture_frame.reference_marked_type;
    auto &marked_t = pic->m_picture_top_filed.reference_marked_type;
    auto &marked_b = pic->m_picture_bottom_filed.reference_marked_type;

    auto picNum_f = pic->m_picture_frame.PicNum;
    auto picNum_t = pic->m_picture_top_filed.PicNum;
    auto picNum_b = pic->m_picture_bottom_filed.PicNum;

    auto pic_type_f = pic->m_picture_frame.m_picture_coded_type;

    auto &longFrameIdx_t = pic->m_picture_top_filed.LongTermFrameIdx;
    auto &longFrameIdx_b = pic->m_picture_bottom_filed.LongTermFrameIdx;

    // 标记短期参考帧或短期补充参考picNumX指定的场对及其两个场都从“用于短期参考”更改为“用于长期参考”，并分配LongTermFrameIdx等于long_term_frame_idx
    if (field_pic_flag == false) {
      if ((marked_f == SHORT_REF && picNum_f == picNumX) ||
          (marked_t == SHORT_REF && picNum_t == picNumX) ||
          (marked_b == SHORT_REF && picNum_b == picNumX)) {
        LongTermFrameIdx = long_term_frame_idx;
        pic->reference_marked_type = LONG_REF;
        pic->m_pic_coded_type_marked_as_refrence = FRAME;
        marked_f = LONG_REF;
        if (pic->m_picture_coded_type == COMPLEMENTARY_FIELD_PAIR)
          marked_t = LONG_REF, marked_b = LONG_REF;
      }
    }
    // 当该场是参考帧或互补参考场对的一部分，且同一参考帧或互补参考场对的另一个场也标记为“用于长期参考”时，该参考帧或互补参考场对对也被标记为“用于长期参考”，并指定 LongTermFrameIdx 等于 long_term_frame_idx
    else {
      if (marked_t == SHORT_REF && picNum_t == picNumX) {
        marked_t = LONG_REF;
        longFrameIdx_t = long_term_frame_idx;
        if (marked_b == LONG_REF) {
          pic->reference_marked_type = LONG_REF;
          if (pic_type_f == FRAME)
            pic->m_pic_coded_type_marked_as_refrence = FRAME;
          else if (pic_type_f == COMPLEMENTARY_FIELD_PAIR)
            pic->m_pic_coded_type_marked_as_refrence = COMPLEMENTARY_FIELD_PAIR;
          marked_f = LONG_REF;
          longFrameIdx_b = long_term_frame_idx;
        }

      } else if (marked_b == SHORT_REF && picNum_b == picNumX) {
        marked_b = LONG_REF;
        longFrameIdx_b = long_term_frame_idx;
        if (marked_t == LONG_REF) {
          pic->reference_marked_type = LONG_REF;
          if (pic_type_f == FRAME)
            pic->m_pic_coded_type_marked_as_refrence = FRAME;
          else if (pic_type_f == COMPLEMENTARY_FIELD_PAIR)
            pic->m_pic_coded_type_marked_as_refrence = COMPLEMENTARY_FIELD_PAIR;
          marked_f = LONG_REF;
          longFrameIdx_t = long_term_frame_idx;
        }
      }
    }
  }
  return 0;
}

// 8.2.5.4.4 Decoding process for MaxLongTermFrameIdx
int decoding_maxLongTermFrameIdx(Frame *(&dpb)[16],
                                 int32_t max_long_term_frame_idx,
                                 int32_t &MaxLongTermFrameIdx) {
  // LongTermFrameIdx大于max_long_term_frame_idx并且被标记为“用于长期参考”的所有图片被标记为“未用于参考”
  for (int32_t i = 0; i < MAX_DPB; i++) {
    Frame *pic = dpb[i];
    auto &marked_f = pic->m_picture_frame.reference_marked_type;
    auto &marked_t = pic->m_picture_top_filed.reference_marked_type;
    auto &marked_b = pic->m_picture_bottom_filed.reference_marked_type;

    auto &longFrameIdx_f = pic->m_picture_frame.LongTermFrameIdx;
    auto &longFrameIdx_t = pic->m_picture_top_filed.LongTermFrameIdx;
    auto &longFrameIdx_b = pic->m_picture_bottom_filed.LongTermFrameIdx;

    if (longFrameIdx_f > max_long_term_frame_idx && marked_f == LONG_REF)
      marked_f = UNUSED_REF;
    if (longFrameIdx_t > max_long_term_frame_idx && marked_t == LONG_REF)
      marked_t = UNUSED_REF;
    if (longFrameIdx_b > max_long_term_frame_idx && marked_b == LONG_REF)
      marked_b = UNUSED_REF;

    if (pic->m_picture_coded_type == FRAME ||
        pic->m_picture_coded_type == COMPLEMENTARY_FIELD_PAIR)
      pic->m_pic_coded_type_marked_as_refrence = UNKNOWN,
      pic->reference_marked_type = UNUSED_REF;
  }

  MaxLongTermFrameIdx =
      (max_long_term_frame_idx + 1 == 0) ? NA : max_long_term_frame_idx;
  return 0;
}

// 8.2.5.4.5 Marking process of all reference pictures as "unused for reference" and setting MaxLongTermFrameIdx to "no long-term frame indices"
int marking_all_ref_pictures_as_unused_for_ref(Frame *(&dpb)[16],
                                               int32_t &MaxLongTermFrameIdx) {
  for (int32_t i = 0; i < MAX_DPB; i++) {
    Frame *pic = dpb[i];
    pic->m_pic_coded_type_marked_as_refrence = UNKNOWN;
    pic->reference_marked_type = UNUSED_REF;
    pic->m_picture_frame.reference_marked_type = UNUSED_REF;
    pic->m_picture_top_filed.reference_marked_type = UNUSED_REF;
    pic->m_picture_bottom_filed.reference_marked_type = UNUSED_REF;
  }

  // MaxLongTermFrameIdx被设置为等于“无长期帧索引”
  MaxLongTermFrameIdx = NA;
  return 0;
}

// 8.2.5.4.6 Process for assigning a long-term frame index to the current picture
int assigning_long_term_frame_index_to_the_current_picture(
    Frame *(&dpb)[16], H264_PICTURE_CODED_TYPE m_picture_coded_type,
    bool field_pic_flag, int32_t long_term_frame_idx,
    int32_t max_long_term_frame_idx, Frame *m_parent,
    PICTURE_MARKED_AS &reference_marked_type, int32_t &LongTermFrameIdx) {

  for (int32_t i = 0; i < MAX_DPB; i++) {
    Frame *pic = dpb[i];
    auto &marked_f = pic->m_picture_frame.reference_marked_type;
    auto &marked_t = pic->m_picture_top_filed.reference_marked_type;
    auto &marked_b = pic->m_picture_bottom_filed.reference_marked_type;

    auto longFrameIdx_f = pic->m_picture_frame.LongTermFrameIdx;
    auto longFrameIdx_t = pic->m_picture_top_filed.LongTermFrameIdx;
    auto longFrameIdx_b = pic->m_picture_bottom_filed.LongTermFrameIdx;

    auto pic_type_t = pic->m_picture_top_filed.m_picture_coded_type;
    auto pic_type_b = pic->m_picture_bottom_filed.m_picture_coded_type;

    // 当等于long_term_frame_idx的变量LongTermFrameIdx已经分配给长期参考帧或长期互补参考字段对时，该帧或互补字段对及其两个字段被标记为“未用于参考”。当 LongTermFrameIdx 已分配给参考字段，并且该参考字段不是包括当前图片的互补字段对的一部分时，该字段被标记为“未用于参考”。
    if (longFrameIdx_t == LongTermFrameIdx &&
        m_parent->m_picture_coded_type == COMPLEMENTARY_FIELD_PAIR &&
        (pic != m_parent)) {
      pic->reference_marked_type = marked_b;
      pic->m_pic_coded_type_marked_as_refrence = pic_type_b;
      marked_t = UNUSED_REF;
    } else if (longFrameIdx_b == LongTermFrameIdx &&
               m_parent->m_picture_coded_type == COMPLEMENTARY_FIELD_PAIR &&
               (pic != m_parent)) {
      pic->reference_marked_type = marked_t;
      pic->m_pic_coded_type_marked_as_refrence = pic_type_t;
      marked_b = UNUSED_REF;
    }

    //标记当前解码参考图像后，至少有一个场标记为“用于参考”的帧总数，加上至少有一个场标记为“用于参考”的互补场对的数量，加上标记为“用于参考”的非配对字段不得大于 Max( max_num_ref_frames, 1 )。
    if (longFrameIdx_f == max_long_term_frame_idx && marked_f == LONG_REF) {
      marked_f = UNUSED_REF;
      pic->reference_marked_type = UNUSED_REF;
      pic->m_pic_coded_type_marked_as_refrence = UNKNOWN;
      if (pic->m_picture_coded_type == COMPLEMENTARY_FIELD_PAIR)
        marked_t = UNUSED_REF, marked_b = UNUSED_REF;
    }
  }

  //当前图片被标记为“用于长期参考”并分配等于long_term_frame_idx的LongTermFrameIdx。
  reference_marked_type = LONG_REF, LongTermFrameIdx = long_term_frame_idx;
  // 其两个字段也被标记为“用于长期参考”
  if (field_pic_flag == false) {
    if (m_parent->m_picture_coded_type == COMPLEMENTARY_FIELD_PAIR) {
      m_parent->m_picture_top_filed.reference_marked_type = LONG_REF;
      m_parent->m_picture_bottom_filed.reference_marked_type = LONG_REF;
      m_parent->m_picture_top_filed.LongTermFrameIdx = long_term_frame_idx;
      m_parent->m_picture_bottom_filed.LongTermFrameIdx = long_term_frame_idx;
    }
  }
  // 互补参考场对的第一场也被标记为“用于长期参考”
  else if (field_pic_flag && m_picture_coded_type == BOTTOM_FIELD &&
           m_parent->m_picture_top_filed.reference_marked_type == LONG_REF) {
    m_parent->reference_marked_type = LONG_REF,
    LongTermFrameIdx = long_term_frame_idx;
  }

  return 0;
}

// 8.2.5.4 Adaptive memory control decoded reference picture marking process
int PictureBase::adaptive_memory_control_decoded_reference_picture_marking(
    Frame *(&dpb)[16]) {
  const SliceHeader *header = m_slice->slice_header;
  const bool field_pic_flag = header->field_pic_flag;

  for (int32_t i = 0; i < header->dec_ref_pic_marking_count; i++) {
    auto dec_marking = header->m_dec_ref_pic_marking[i];
    const int32_t mmoc = dec_marking.memory_management_control_operation;
    // 一个短期参考帧的帧号（由编码器给出帧号），它表示当前帧要操作的目标参考帧
    const int32_t picNumX =
        header->CurrPicNum - dec_marking.difference_of_pic_nums_minus1 - 1;
    // 一个长期参考帧的帧号（由编码器给出帧号），它表示当前帧要操作的目标参考帧
    const int32_t longPicNumX = dec_marking.long_term_pic_num_2;
    // 一个长期参考帧的索引（由编码器给出帧号），每个长期参考帧都有一个唯一的索引
    const int32_t long_term_frame_idx = dec_marking.long_term_frame_idx;
    // 长期参考帧索引的最大值。编解码器必须确保长期参考帧的索引不超过这个最大值
    const int32_t max_long_term_frame_idx =
        dec_marking.max_long_term_frame_idx_plus1 - 1;

    if (mmoc == 0) break;
    /* 1. 将某个短期参考帧标记为“未使用参考帧” */
    if (mmoc == 1)
      marking_short_term_ref_picture_as_unused_for_ref(dpb, picNumX,
                                                       field_pic_flag);
    /* 2. 将某个长期参考帧标记为“未使用参考帧” */
    else if (mmoc == 2)
      marking_long_term_ref_picture_as_unused_for_ref(dpb, longPicNumX,
                                                      field_pic_flag);
    /* 3. 将某个短期参考帧转换为长期参考帧，并指定其长期参考帧索引 */
    else if (mmoc == 3)
      assignment_longTermFrameIdx_to_short_term_ref_picture(
          dpb, picNumX, long_term_frame_idx, field_pic_flag, LongTermFrameIdx);
    /* 4. 将所有短期参考帧标记为“未使用参考帧” */
    else if (mmoc == 4)
      decoding_maxLongTermFrameIdx(dpb, max_long_term_frame_idx,
                                   MaxLongTermFrameIdx);
    /* 5. 将所有参考帧（短期和长期）标记为“未使用参考帧”，通常用于 IDR 帧 */
    else if (mmoc == 5) {
      marking_all_ref_pictures_as_unused_for_ref(dpb, MaxLongTermFrameIdx);
      mmco_5_flag = true;
    }
    /* 6. 分配一个长期帧索引给当前图像 */
    else if (mmoc == 6) {
      assigning_long_term_frame_index_to_the_current_picture(
          dpb, m_picture_coded_type, field_pic_flag, long_term_frame_idx,
          MaxLongTermFrameIdx, m_parent, reference_marked_type,
          LongTermFrameIdx);
      mmco_6_flag = true;
    }
  }

  return 0;
}
