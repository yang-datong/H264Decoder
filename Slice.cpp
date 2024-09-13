#include "Slice.hpp"
#include "Frame.hpp"

Slice::Slice() : slice_header(m_sps, m_pps), slice_data(m_sps, m_pps) {};

void Slice::addMacroblock(std::shared_ptr<MacroBlock> macroblock) {
  _macroblocks.push_back(macroblock);
}

int Slice::encode() {
  // Implement slice encoding logic
  //for (auto &mb : _macroblocks) {
  //mb->encode();
  //}
  return 0;
}

int Slice::decode(BitStream &bitStream, Frame *(&dpb)[16], SPS &sps, PPS &pps,
                  Frame *frame) {
  //----------------帧----------------------------------
  frame->m_picture_coded_type = H264_PICTURE_CODED_TYPE_FRAME;
  frame->m_picture_frame.m_picture_coded_type = H264_PICTURE_CODED_TYPE_FRAME;
  frame->m_picture_frame.m_parent = frame;
  memcpy(frame->m_picture_frame.m_dpb, dpb, sizeof(Nalu *) * GOP_SIZE);
  frame->m_current_picture_ptr = &(frame->m_picture_frame);
  frame->m_picture_frame.init(*this);

  //----------------顶场-------------------------------
  frame->m_picture_top_filed.m_picture_coded_type =
      H264_PICTURE_CODED_TYPE_TOP_FIELD;
  frame->m_picture_top_filed.m_parent = frame;
  frame->m_picture_top_filed.init(*this);

  //----------------底场-------------------------------
  frame->m_picture_bottom_filed.m_picture_coded_type =
      H264_PICTURE_CODED_TYPE_BOTTOM_FIELD;
  frame->m_picture_bottom_filed.m_parent = frame;
  frame->m_picture_bottom_filed.init(*this);

  /* 当前解码的Slice为场编码（可能下一帧又是帧编码了）即隔行扫描方式 */
  /* TODO YangJing slice_header.field_pic_flag <24-09-11 17:34:02> */
  if (slice_header.field_pic_flag) // 场编码->顶场，底场
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
  //std::cout << "\t场编码(暂不处理)" << std::endl;
  //else  // 帧
  //std::cout << "\t帧编码" << std::endl;

  slice_data.parseSliceData(bitStream, frame->m_picture_frame);
  return 0;
}
