#include "Nalu.hpp"
#include "BitStream.hpp"
#include "MacroBlock.hpp"
#include "PictureBase.hpp"
#include "RBSP.hpp"
#include <cmath>
#include <cstdint>

Nalu::Nalu() {}

Nalu::~Nalu() {
  if (_buffer != nullptr) {
    free(_buffer);
    _buffer = nullptr;
  }
}

int Nalu::setBuffer(uint8_t *buf, int len) {
  if (_buffer != nullptr) {
    free(_buffer);
    _buffer = nullptr;
  }
  uint8_t *tmpBuf = (uint8_t *)malloc(len);
  memcpy(tmpBuf, buf, len);
  _buffer = tmpBuf;
  _len = len;
  return 0;
}

int Nalu::parseEBSP(EBSP &ebsp) {
  ebsp._len = _len - _startCodeLenth;
  uint8_t *ebspBuffer = new uint8_t[ebsp._len];
  memcpy(ebspBuffer, _buffer + _startCodeLenth, ebsp._len);
  ebsp._buf = ebspBuffer;
  return 0;
}

/* 注意，这里解析出来的RBSP是不包括RBSP head的一个字节的 */
int Nalu::parseRBSP(EBSP &ebsp, RBSP &rbsp) {
  parseNALHeader(ebsp); // RBSP的头也是EBSP的头

  bool NumBytesInRBSP = 0;
  bool nalUnitHeaderBytes = 1; // nalUnitHeaderBytes的默认head大小为1字节

  if (nal_unit_type == 14 || nal_unit_type == 20 || nal_unit_type == 21) {
    bool svc_extension_flag = 0, avc_3d_extension_flag = 0;

    if (nal_unit_type != 21)
      svc_extension_flag = ebsp._buf[1] >> 7;
    else
      avc_3d_extension_flag = ebsp._buf[1] >> 7;

    if (svc_extension_flag)
      // nal_unit_header_svc_extension();
      nalUnitHeaderBytes += 3;
    else if (avc_3d_extension_flag)
      // nal_unit_header_3davc_extension()
      /* specified in Annex J */
      nalUnitHeaderBytes += 2;
    else
      // nal_unit_header_mvc_extension()
      /* specified in Annex H */
      nalUnitHeaderBytes += 3;
  }
  if (nalUnitHeaderBytes != 1) {
    std::cerr << "\033[31m未实现NAL head 为多字节的情况~\033[0m" << std::endl;
    return -1;
  }

  uint8_t *rbspBuffer = new uint8_t[ebsp._len - 1]{0}; // 去掉RBSP head (1 byte)
  int index = 0;
  rbspBuffer[index++] = ebsp._buf[1]; // 从RBSP body开始
  rbspBuffer[index++] = ebsp._buf[2];
  rbsp._len = ebsp._len - 1; // 不包括RBSP head
  for (int i = 3; i < ebsp._len; i++) {
    if (ebsp._buf[i] == 3 && ebsp._buf[i - 1] == 0 && ebsp._buf[i - 2] == 0) {
      if (ebsp._buf[i + 1] == 0 || ebsp._buf[i + 1] == 1 ||
          ebsp._buf[i + 1] == 2 || ebsp._buf[i + 1] == 3)
        // 满足0030, 0031, 0032, 0033的特征，故一定是防竞争字节序
        rbsp._len--;
    } else
      rbspBuffer[index++] = ebsp._buf[i];
    // 如果不是防竞争字节序就依次放入到rbspbuff
  }
  rbsp._buf = rbspBuffer;
  return 0;
}

int Nalu::parseNALHeader(EBSP &ebsp) {
  uint8_t firstByte = ebsp._buf[0];
  nal_unit_type = firstByte & 0b00011111;
  /* 取低5bit，即0-4 bytes */
  nal_ref_idc = (firstByte & 0b01100000) >> 5;
  /* 取5-6 bytes */
  forbidden_zero_bit = firstByte >> 7;
  /* 取最高位，即7 byte */
  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -43页 */
int Nalu::extractSPSparameters(RBSP &rbsp) {
  sps._buf = rbsp._buf;
  sps._len = rbsp._len;
  sps.extractParameters();
  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -47页 */
int Nalu::extractPPSparameters(RBSP &rbsp) {
  pps._buf = rbsp._buf;
  pps._len = rbsp._len;
  pps.sps = this->sps;
  pps.extractParameters();
  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -48页 */
int Nalu::extractSEIparameters(RBSP &rbsp) {
  sei._buf = rbsp._buf;
  sei._len = rbsp._len;
  sei.pps = this->pps;
  sei.extractParameters();
  return 0;
}

int Nalu::extractSliceparameters(RBSP &rbsp,GOP &gop) {
  /* 初始化bit处理器，填充slice的数据 */
  BitStream bitStream(rbsp._buf, rbsp._len);
  slice_header.m_sps = sps;
  slice_header.m_pps = pps;
  slice_header.m_idr = idr;
  slice_header.nal_unit_type = nal_unit_type;
  slice_header.nal_ref_idc = nal_ref_idc;
  slice_header.parseSliceHeader(bitStream, this);
  decode(bitStream,gop.m_DecodedPictureBuffer);
  return 0;
}

int Nalu::extractIDRparameters(RBSP &rbsp,GOP &gop) {
  /* 初始化bit处理器，填充idr的数据 */
  BitStream bitStream(rbsp._buf, rbsp._len);
  slice_header.m_sps = sps;
  slice_header.m_pps = pps;
  slice_header.m_idr = idr;
  slice_header.nal_unit_type = nal_unit_type;
  slice_header.nal_ref_idc = nal_ref_idc;
  slice_header.parseSliceHeader(bitStream, this);
  decode(bitStream,gop.m_DecodedPictureBuffer);
  return 0;
}


int Nalu::decode(BitStream &bitStream,Nalu *(&dpb)[GOP_SIZE]) {

  //----------------帧----------------------------------
  m_picture_coded_type = H264_PICTURE_CODED_TYPE_FRAME;
  m_picture_frame.m_picture_coded_type = H264_PICTURE_CODED_TYPE_FRAME;
  m_picture_frame.m_parent = this;
  memcpy(m_picture_frame.m_dpb, dpb, sizeof(Nalu *) * GOP_SIZE);
  m_current_picture_ptr = &m_picture_frame;
  m_picture_frame.init(slice_header);

  if (slice_header.field_pic_flag == 0) // 帧
    std::cout << "\t帧编码" << std::endl;
  else { // 场编码->顶场，底场
    std::cout << "\t场编码(暂不处理)" << std::endl;
    return -1;
  }

  slice_body.slice_header = this->slice_header;
  slice_body.m_sps = this->sps;
  slice_body.m_pps = this->pps;
  slice_body.m_idr = this->idr;
  slice_body.parseSliceData(bitStream, m_picture_frame);
  // NOTE:已经可以正确解码I帧
  // m_picture_frame.saveToBmpFile("output.bmp");
  return 0;
}

int Nalu::reset() {
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
