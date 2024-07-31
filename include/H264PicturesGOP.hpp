#ifndef H264PICTURESGOP_HPP_4UBCQ63F
#define H264PICTURESGOP_HPP_4UBCQ63F

#include "Nalu.hpp"
#include "PPS.hpp"
#include "SPS.hpp"

class CH264PicturesGOP {
 public:
  SPS m_spss[H264_MAX_SPS_COUNT]; // sps[32]
  // SPSExt m_sps_ext;
  PPS m_ppss[H264_MAX_PPS_COUNT]; // pps[256]
  SEI m_sei;

  Nalu *m_DecodedPictureBuffer
      [H264_MAX_DECODED_PICTURE_BUFFER_COUNT]; // DPB: decoded picture buffer
  Nalu *m_dpb_for_output
      [H264_MAX_DECODED_PICTURE_BUFFER_COUNT]; // 因为含有B帧的视频帧的显示顺序和解码顺序是不一样的，已经解码完的P/B帧不能立即输出给用户，需要先缓存一下，
  int32_t
      m_dpb_for_output_length; // m_dpb_index_for_output[]数组的真实大小，此值是动态变化的，取值范围[0,
                               // max_num_reorder_frames-1]
  int32_t
      max_num_reorder_frames; // m_dpb_index_for_output[]数组的最大大小，来源于m_spss[i].m_vui.max_num_reorder_frames，对于含B帧的视频，此值一般等于2

  Nalu m_picture_previous_reference;
  Nalu m_picture_previous;

  int32_t m_gop_size;

 public:
  CH264PicturesGOP();
  ~CH264PicturesGOP();

  int init();
  int unInit();

  int getOneEmptyPicture(Nalu *&pic);
  int getOneOutPicture(Nalu *newDecodedPic, Nalu *&outPic);
};

#endif /* end of include guard: H264PICTURESGOP_HPP_4UBCQ63F */
