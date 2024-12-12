#include "Frame.hpp"
#include "Image.hpp"
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
  slice->decode(bitStream, dpb, gop.m_spss[gop.curr_sps_id],
                gop.m_ppss[gop.curr_pps_id], this);
  // 去块滤波器
  //DeblockingFilter deblockingFilter;
  //deblockingFilter.deblocking_filter_process(&m_picture_frame);
  return 0;
}

int Frame::reset() {
  m_picture_coded_type = UNKNOWN;
  m_pic_coded_type_marked_as_refrence = UNKNOWN;

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
  reference_marked_type = UNKOWN;
  m_is_decode_finished = 0;
  m_is_in_use = true;

  return 0;
}
