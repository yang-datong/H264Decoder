#include "Nalu.hpp"
#include "BitStream.hpp"
#include "PictureBase.hpp"
#include <cmath>
#include <cstdint>

Nalu::Nalu() {}

Nalu::~Nalu() {
  if (_buffer != nullptr) {
    free(_buffer);
    _buffer = nullptr;
  }
}

Nalu::EBSP::EBSP() {}

Nalu::EBSP::~EBSP() {
  if (_buf) delete[] _buf;
}

Nalu::RBSP::RBSP() {}

Nalu::RBSP::~RBSP() {
  if (_buf) delete[] _buf;
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
int Nalu::extractSPSparameters(RBSP &rbsp, SPS &sps) {
  sps._buf = rbsp._buf;
  sps._len = rbsp._len;
  sps.extractParameters();
  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -47页 */
int Nalu::extractPPSparameters(RBSP &rbsp, PPS &pps,
                               uint32_t chroma_format_idc) {
  pps._buf = rbsp._buf;
  pps._len = rbsp._len;
  pps.extractParameters(chroma_format_idc);
  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -48页 */
int Nalu::extractSEIparameters(RBSP &rbsp, SEI &sei, SPS &sps) {
  sei._buf = rbsp._buf;
  sei._len = rbsp._len;
  sei.extractParameters(sps);
  return 0;
}

int Nalu::extractSliceparameters(BitStream &bitStream, GOP &gop, Frame &frame) {
  frame.slice_header.m_sps = gop.m_spss[0];
  frame.slice_header.m_pps = gop.m_ppss[0];
  // frame.slice_header.m_idr = idr;
  frame.slice_header.nal_unit_type = nal_unit_type;
  frame.slice_header.nal_ref_idc = nal_ref_idc;
  frame.slice_header.parseSliceHeader(bitStream, this);

  frame.slice_body.m_sps = frame.slice_header.m_sps;
  frame.slice_body.m_pps = frame.slice_header.m_pps;
  // frame.slice_body.m_idr = frame.slice_header.m_idr;
  frame.decode(bitStream, gop.m_DecodedPictureBuffer, gop.m_spss[0],
               gop.m_ppss[0]);
  return 0;
}

int Nalu::extractIDRparameters(BitStream &bitStream, GOP &gop, Frame &frame) {
  frame.slice_header.m_sps = gop.m_spss[0];
  frame.slice_header.m_pps = gop.m_ppss[0];
  // frame.slice_header.m_idr = idr;
  frame.slice_header.nal_unit_type = nal_unit_type;
  frame.slice_header.nal_ref_idc = nal_ref_idc;
  frame.slice_header.parseSliceHeader(bitStream, this);

  frame.slice_body.m_sps = frame.slice_header.m_sps;
  frame.slice_body.m_pps = frame.slice_header.m_pps;
  // frame.slice_body.m_idr = frame.slice_header.m_idr;
  frame.decode(bitStream, gop.m_DecodedPictureBuffer, gop.m_spss[0],
               gop.m_ppss[0]);
  return 0;
}
