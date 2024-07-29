#ifndef NALU_HPP_YDI8RPRP
#define NALU_HPP_YDI8RPRP

#include "BitStream.hpp"
#include "Common.hpp"
#include "EBSP.hpp"
#include "IDR.hpp"
#include "PPS.hpp"
#include "PictureBase.hpp"
#include "RBSP.hpp"
#include "SPS.hpp"
#include "Slice.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

// 用于存放264中每一个单个Nalu数据
class Nalu {
 public:
  Nalu();
  /* 拷贝构造函数 */
  Nalu(const Nalu &nalu);
  ~Nalu();

  int _startCodeLenth = 0;
  uint8_t *_buffer = nullptr;
  int _len = 0;

  int setBuffer(uint8_t *buf, int len);
  // 用于给外界传输buf进来

 public:
  int parseEBSP(EBSP &ebsp);
  int parseRBSP(EBSP &ebsp, RBSP &rbsp);

  char forbidden_zero_bit = 0;
  char nal_ref_idc = 0;
  char nal_unit_type = 0;

  int extractSPSparameters(RBSP &rbsp);
  int extractPPSparameters(RBSP &rbsp);
  int extractSEIparameters(RBSP &rbsp);
  int extractSliceparameters(RBSP &rbsp);
  int extractIDRparameters(RBSP &rbsp);

  int GetNaluType();

  /* 开始解码图像 */
  int decode(RBSP &rbsp);

 private:
  int parseNALHeader(EBSP &rbsp);
  void scaling_list(BitStream &bitStream, uint32_t *scalingList,
                    uint32_t sizeOfScalingList,
                    uint32_t &useDefaultScalingMatrixFlag);

  void vui_parameters(BitStream &bitStream);

  void hrd_parameters(BitStream &bitStream);

  /* SPS 参数 */
  SPS sps;

  /* PPS 参数 */
  PPS pps;

  /* SEI */
  void sei_message(BitStream &bitStream);
  void sei_payload(BitStream &bitStream, long payloadType, long payloadSize);
  bool byte_aligned(BitStream &bitStream);

  /* Slice */
  Slice slice;
  int parseSliceHeader(BitStream &bitStream, RBSP &rbsp);
  int parseSliceData(BitStream &bitStream, RBSP &rbsp, PictureBase &picture);

  void ref_pic_list_mvc_modification(BitStream &bitStream);
  void ref_pic_list_modification(BitStream &bitStream);

  void pred_weight_table(BitStream &bitStream);
  void dec_ref_pic_marking(BitStream &bitStream);

  int setMapUnitToSliceGroupMap();
  int setMbToSliceGroupMap();
  int set_scaling_lists_values();
  int set_mb_skip_flag(int32_t &mb_skip_flag, PictureBase &picture,
                       BitStream &bitStream);

  /* IDR */
  IDR idr;
  int NextMbAddress(int n);
  int macroblock_layer(BitStream &bs, PictureBase &picture);

 public:
  /* NOTE 以下均来自Picture */
  PictureBase m_picture_frame;
  PictureBase m_picture_top_filed;
  PictureBase m_picture_bottom_filed;
  PictureBase *
      m_current_picture_ptr; // 指向m_picture_frame或者m_picture_top_filed或者m_picture_bottom_filed
  PictureBase *m_picture_previous_ref; // 前一个已解码的参考图像
  PictureBase *m_picture_previous;     // 前一个已解码的图像
  H264_PICTURE_CODED_TYPE m_picture_coded_type; // H264_PICTURE_CODED_TYPE_FRAME
  H264_PICTURE_CODED_TYPE
  m_picture_coded_type_marked_as_refrence; // 整个帧或哪一场被标记为参考帧或参考场

  int32_t TopFieldOrderCnt;
  int32_t BottomFieldOrderCnt;
  int32_t PicOrderCntMsb;
  int32_t PicOrderCntLsb;
  int32_t FrameNumOffset;
  int32_t absFrameNum;
  int32_t picOrderCntCycleCnt;
  int32_t frameNumInPicOrderCntCycle;
  int32_t expectedPicOrderCnt;
  int32_t PicOrderCnt;
  int32_t PicNum; // To each short-term reference picture 短期参考图像
  int32_t LongTermPicNum; // To each long-term reference picture 长期参考图像
  H264_PICTURE_MARKED_AS reference_marked_type; // I,P作为参考帧的mark状态
  int32_t m_is_decode_finished;                 // 本帧是否解码完毕;
                                // 0-未解码完毕，1-已经解码完毕
  int32_t m_is_in_use; // 本帧数据是否正在使用; 0-未使用，1-正在使用
};

#endif /* end of include guard: NALU_HPP_YDI8RPRP */
