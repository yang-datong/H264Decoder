#include "Frame.hpp"

int Frame::decode(BitStream &bitStream, Frame *(&dpb)[GOP_SIZE], SPS &sps,
                  PPS &pps) {
  static int index = 0;

  //----------------帧----------------------------------
  m_picture_coded_type = H264_PICTURE_CODED_TYPE_FRAME;
  m_picture_frame.m_picture_coded_type = H264_PICTURE_CODED_TYPE_FRAME;
  m_picture_frame.m_parent = this;
  memcpy(m_picture_frame.m_dpb, dpb, sizeof(Nalu *) * GOP_SIZE);
  m_current_picture_ptr = &m_picture_frame;
  m_picture_frame.init(slice_header);

  //----------------顶场-------------------------------
  m_picture_top_filed.m_picture_coded_type = H264_PICTURE_CODED_TYPE_TOP_FIELD;
  m_picture_top_filed.m_parent = this;

  m_picture_top_filed.init(slice_header);

  //----------------底场-------------------------------
  m_picture_bottom_filed.m_picture_coded_type =
      H264_PICTURE_CODED_TYPE_BOTTOM_FIELD;
  m_picture_bottom_filed.m_parent = this;

  m_picture_bottom_filed.init(slice_header);

  if (slice_header.field_pic_flag == 0) // 帧
    std::cout << "\t帧编码" << std::endl;
  else { // 场编码->顶场，底场
    std::cout << "\t场编码(暂不处理)" << std::endl;
    return -1;
  }

  slice_body.m_sps = sps;
  slice_body.m_pps = pps;
  slice_body.m_idr = idr;
  slice_body.parseSliceData(bitStream, m_picture_frame);

  // NOTE: 已经可以正常解码IPPPP的H264视频了，测试使用，后续删除
  string output_file;
  if (slice_header.slice_type == SLICE_I)
    output_file = "output_I_" + to_string(index++) + ".bmp";
  else if (slice_header.slice_type == SLICE_P)
    output_file = "output_P_" + to_string(index++) + ".bmp";
  else if (slice_header.slice_type == SLICE_B)
    output_file = "output_B_" + to_string(index++) + ".bmp";
  else {
    std::cerr << "未知帧" << std::endl;
    exit(0);
  }
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
