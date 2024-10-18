#ifndef FRAME_HPP_9ZDDHOX0
#define FRAME_HPP_9ZDDHOX0
#include "GOP.hpp"
#include "PictureBase.hpp"

class Slice;

class Frame {
 public:
  //void addSlice(Slice *slice);

  virtual void encode();
  virtual void decode();

  /* 开始解码图像 */
  int decode(BitStream &bitStream, Frame *(&dpb)[16], GOP &gop);

  //protected:
  //std::vector<Slice *> slices;

 public:
  /* Slice */
  Slice *slice = nullptr;

  PictureBase m_picture_frame;
  PictureBase m_picture_top_filed;
  PictureBase m_picture_bottom_filed;

  PictureBase *m_current_picture_ptr = nullptr;
  // 指向m_picture_frame或者m_picture_top_filed或者m_picture_bottom_filed

  PictureBase *m_picture_previous_ref = nullptr; // 前一个已解码的参考图像
  PictureBase *m_picture_previous = nullptr;    // 前一个已解码的图像
  H264_PICTURE_CODED_TYPE m_picture_coded_type; // H264_PICTURE_CODED_TYPE_FRAME
  H264_PICTURE_CODED_TYPE
  m_pic_coded_type_marked_as_refrence; // 整个帧或哪一场被标记为参考帧或参考场

  int32_t TopFieldOrderCnt = 0;
  int32_t BottomFieldOrderCnt = 0;
  //private:
  //Field topField;
  //Field bottomField;

  int32_t PicOrderCntMsb = 0;
  int32_t PicOrderCntLsb = 0;
  int32_t FrameNumOffset = 0;
  int32_t absFrameNum = 0;
  int32_t picOrderCntCycleCnt = 0;
  int32_t frameNumInPicOrderCntCycle = 0;
  int32_t expectedPicOrderCnt = 0;
  int32_t PicOrderCnt = 0;
  // To each short-term reference picture 短期参考图像
  int32_t PicNum = 0;
  // To each long-term reference picture 长期参考图像
  int32_t LongTermPicNum = 0;
  PICTURE_MARKED_AS reference_marked_type; // I,P作为参考帧的mark状态
  int32_t m_is_decode_finished = 0; // 0-未解码完毕，1-已经解码完毕
  bool m_is_in_use = false; // 本帧数据是否正在使用; 0-未使用，1-正在使用

  int reset();
};

#endif /* end of include guard: FRAME_HPP_9ZDDHOX0 */
