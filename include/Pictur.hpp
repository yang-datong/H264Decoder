#ifndef PICTUR_HPP_OD59YHZW
#define PICTUR_HPP_OD59YHZW
#include "PictureBase.hpp"

class Picture {
 public:
  PictureBase m_picture_frame;
  PictureBase m_picture_top_filed;
  PictureBase m_picture_bottom_filed;
  PictureBase *
      m_current_picture_ptr; //指向m_picture_frame或者m_picture_top_filed或者m_picture_bottom_filed
  PictureBase *m_picture_previous_ref; //前一个已解码的参考图像
  PictureBase *m_picture_previous;     //前一个已解码的图像
  H264_PICTURE_CODED_TYPE m_picture_coded_type; // H264_PICTURE_CODED_TYPE_FRAME
  H264_PICTURE_CODED_TYPE
  m_picture_coded_type_marked_as_refrence; //整个帧或哪一场被标记为参考帧或参考场

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
  int32_t m_is_decode_finished; //本帧是否解码完毕; 0-未解码完毕，1-已经解码完毕
  int32_t m_is_in_use; //本帧数据是否正在使用; 0-未使用，1-正在使用

 public:
  Picture();
  ~Picture();

  int init();
  int reset();

  //  int decode_one_slice(SliceHeader &slice_header, CBitstream &bs,
  //                       Picture *(&dpb)[16]);
  Picture &operator=(const Picture &src);
  int copyPicture(const Picture &src);
};

#endif /* end of include guard: PICTUR_HPP_OD59YHZW */
