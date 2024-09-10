#include "Frame.hpp"
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
  //for (auto &slice : slices) {
  slice->decode(bitStream, dpb, gop.m_spss[0], gop.m_ppss[0], this);
  if (slice->slice_header.slice_type == SLICE_I)
    output_file = "output_I_" + to_string(index++) + ".bmp";
  else if (slice->slice_header.slice_type == SLICE_P)
    output_file = "output_P_" + to_string(index++) + ".bmp";
  else if (slice->slice_header.slice_type == SLICE_B)
    output_file = "output_B_" + to_string(index++) + ".bmp";
  else {
    std::cerr << "未知帧" << std::endl;
    exit(0);
  }
  //}
  m_picture_frame.saveToBmpFile(output_file.c_str());

  return 0;
}

int Frame::reset() {
  m_picture_coded_type = H264_PICTURE_CODED_TYPE_UNKNOWN;
  m_picture_coded_type_marked_as_refrence = H264_PICTURE_CODED_TYPE_UNKNOWN;

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
  reference_marked_type = H264_PICTURE_MARKED_AS_unkown;
  m_is_decode_finished = 0;
  m_is_in_use = 1; // 正在使用状态

  return 0;
}
