#include "Nalu.hpp"
#include "BitStream.hpp"
#include "PictureBase.hpp"
#include "Slice.hpp"
#include <cstdint>

Nalu::~Nalu() {
  if (buffer != nullptr) {
    free(buffer);
    buffer = nullptr;
  }
}

Nalu::EBSP::~EBSP() {
  if (buf) delete[] buf;
}

Nalu::RBSP::~RBSP() {
  if (buf) delete[] buf;
}

int Nalu::setBuffer(uint8_t *buf, int len) {
  if (buffer != nullptr) {
    free(buffer);
    buffer = nullptr;
  }
  uint8_t *tmpBuf = (uint8_t *)malloc(len);
  memcpy(tmpBuf, buf, len);
  buffer = tmpBuf;
  this->len = len;
  return 0;
}

int Nalu::parseEBSP(EBSP &ebsp) {
  ebsp.len = len - startCodeLenth;
  uint8_t *ebspBuffer = new uint8_t[ebsp.len];
  memcpy(ebspBuffer, buffer + startCodeLenth, ebsp.len);
  ebsp.buf = ebspBuffer;
  return 0;
}

//7.3.1 NAL unit syntax
/* 注意，这里解析出来的RBSP是不包括RBSP head的一个字节的 */
int Nalu::parseRBSP(EBSP &ebsp, RBSP &rbsp) {
  BitStream *bs = new BitStream(ebsp.buf, ebsp.len);
  parseNALHeader(ebsp, bs); // RBSP的头也是EBSP的头

  // H.265 nalUnitHeaderBytes的默认head大小为2字节
  uint8_t nalUnitHeaderBytes = 2;

  // 去掉RBSP head (2 byte)
  uint8_t *rbspBuffer = new uint8_t[ebsp.len - nalUnitHeaderBytes]{0};
  int32_t NumBytesInRBSP = 0;
  // 从RBSP body开始
  rbspBuffer[NumBytesInRBSP++] = ebsp.buf[nalUnitHeaderBytes];
  rbspBuffer[NumBytesInRBSP++] = ebsp.buf[nalUnitHeaderBytes + 1];
  // 不包括RBSP head
  rbsp.len = ebsp.len - nalUnitHeaderBytes;
  for (int i = nalUnitHeaderBytes + 2; i < ebsp.len; i++) {
    if (ebsp.buf[i] == 3 && ebsp.buf[i - 1] == 0 && ebsp.buf[i - 2] == 0) {
      if (ebsp.buf[i + 1] == 0 || ebsp.buf[i + 1] == 1 ||
          ebsp.buf[i + 1] == 2 || ebsp.buf[i + 1] == 3)
        // 满足0030, 0031, 0032, 0033的特征，故一定是防竞争字节序
        rbsp.len--;
    } else
      rbspBuffer[NumBytesInRBSP++] = ebsp.buf[i];
    // 如果不是防竞争字节序就依次放入到rbspbuff
  }
  rbsp.buf = rbspBuffer;
  return 0;
}

int Nalu::parseNALHeader(EBSP &ebsp) {
  uint8_t firstByte = ebsp.buf[0];
  nal_unit_type = firstByte & 0b00011111;
  /* 取低5bit，即0-4 bytes */
  nal_ref_idc = (firstByte & 0b01100000) >> 5;
  /* 取5-6 bytes */
  forbidden_zero_bit = firstByte >> 7;
  /* 取最高位，即7 byte */
  return 0;
}

int Nalu::parseNALHeader(EBSP &ebsp, BitStream *bs) {
  forbidden_zero_bit = bs->readU1();
  nal_unit_type = bs->readUn(6);
  nuh_layer_id = bs->readUn(6);
  nuh_temporal_id_plus1 = bs->readUn(3);
  TemporalId = nuh_temporal_id_plus1 - 1;
  return 0;
}

int Nalu::extractVPSparameters(RBSP &rbsp, VPS vpss[MAX_SPS_COUNT],
                               uint32_t &curr_vps_id) {
  /* 初始化bit处理器，填充sps的数据 */
  BitStream bitStream(rbsp.buf, rbsp.len);
  VPS *vps = new VPS();
  vps->extractParameters(bitStream);
  vpss[vps->vps_video_parameter_set_id] = *vps;
  curr_vps_id = vps->vps_video_parameter_set_id;
  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -43页 */
int Nalu::extractSPSparameters(RBSP &rbsp, SPS spss[MAX_SPS_COUNT],
                               uint32_t &curr_sps_id, VPS vpss[MAX_SPS_COUNT]) {
  /* 初始化bit处理器，填充sps的数据 */
  BitStream bitStream(rbsp.buf, rbsp.len);
  SPS *sps = new SPS();
  sps->extractParameters(bitStream, vpss);
  spss[sps->seq_parameter_set_id] = *sps;
  curr_sps_id = sps->seq_parameter_set_id;
  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -47页 */
int Nalu::extractPPSparameters(RBSP &rbsp, PPS ppss[MAX_PPS_COUNT],
                               uint32_t &curr_pps_id,
                               uint32_t chroma_format_idc,
                               SPS spss[MAX_SPS_COUNT]) {
  BitStream bitStream(rbsp.buf, rbsp.len);
  PPS *pps = new PPS();
  pps->extractParameters(bitStream, chroma_format_idc, spss);
  ppss[pps->pic_parameter_set_id] = *pps;
  curr_pps_id = pps->pic_parameter_set_id;
  return 0;
}

/* 在T-REC-H.264-202108-I!!PDF-E.pdf -48页 */
int Nalu::extractSEIparameters(RBSP &rbsp, SEI &sei, SPS &sps) {
  sei._buf = rbsp.buf;
  sei._len = rbsp.len;
  sei.extractParameters(sps);
  return 0;
}

int Nalu::extractSliceparameters(BitStream &bitStream, GOP &gop, Frame &frame) {
  Slice *slice = new Slice(this);
  slice->slice_header->slice_segment_header(bitStream, gop);
  frame.slice = slice;
  return 0;
}

int Nalu::extractIDRparameters(BitStream &bitStream, GOP &gop, Frame &frame) {
  extractSliceparameters(bitStream, gop, frame);
  return 0;
}
