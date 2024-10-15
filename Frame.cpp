#include "Frame.hpp"
#include "DeblockingFilter.hpp"
#include "Slice.hpp"
#include "SliceHeader.hpp"

//void Frame::addSlice(Slice *slice) { slices.push_back(slice); }

void Frame::encode() {
  // Implement frame encoding logic
  //for (auto &slice : slices) {
  //slice->encode();
  //}
}

void Frame::decode() {}

int Frame::decode(BitStream &bitStream, Frame *(&dpb)[16], GOP &gop) {
  static int index = 0;
  string output_file;
  const uint32_t slice_type = slice->slice_header->slice_type % 5;
  //for (auto &slice : slices) {
  if (slice_type == SLICE_I)
    output_file = "output_I_" + to_string(index++) + ".bmp";
  else if (slice_type == SLICE_P)
    output_file = "output_P_" + to_string(index++) + ".bmp";
  else if (slice_type == SLICE_B)
    output_file = "output_B_" + to_string(index++) + ".bmp";
  else {
    std::cerr << "Unrecognized slice type:" << slice->slice_header->slice_type
              << std::endl;
    return -1;
  }
  slice->decode(bitStream, dpb, gop.m_spss[gop.curr_sps_id],
                gop.m_ppss[gop.curr_pps_id], this);
  // 去块滤波器
  /* TODO YangJing 这里函数要认真看 <24-10-14 05:44:27> */
  DeblockingFilter deblockingFilter;
  deblockingFilter.deblocking_filter_process(&m_picture_frame);
  m_picture_frame.saveToBmpFile(output_file.c_str());
  //}
  return 0;
}

int Frame::reset() {
  m_picture_coded_type = PICTURE_CODED_TYPE_UNKNOWN;
  m_picture_coded_type_marked_as_refrence = PICTURE_CODED_TYPE_UNKNOWN;

  TopFieldOrderCnt = 0;
  BottomFieldOrderCnt = 0;
  PicOrderCntMsb = 0;
  PicOrderCntLsb = 0;
  FrameNumOffset = 0;
  absFrameNum = 0;
  picOrderCntCycleCnt = 0;
  frameNumInPicOrderCntCycle = 0;
  expectedPicOrderCnt = 0;
  PicOrderCnt = 0;
  PicNum = 0;
  LongTermPicNum = 0;
  reference_marked_type = PICTURE_MARKED_AS_unkown;
  m_is_decode_finished = 0;
  m_is_in_use = 1; // 正在使用状态

  return 0;
}
