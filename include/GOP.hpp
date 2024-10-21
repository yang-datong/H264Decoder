#ifndef GOP_HPP_PUYEPJDM
#define GOP_HPP_PUYEPJDM

#include "Common.hpp"
#include "PPS.hpp"
#include "SPS.hpp"
#include "Type.hpp"
#include "VPS.hpp"

class Frame;
class GOP {
 public:
  GOP();
  ~GOP();

 public:
  VPS m_vpss[MAX_VPS_COUNT]; // sps[32]
  uint32_t last_vps_id = 0;
  //7.4.1.2.1 Order of sequence and picture parameter set RBSPs and their activation
  SPS m_spss[MAX_SPS_COUNT]; // sps[32]
  /* 最新得到的SPS ID */
  uint32_t last_sps_id = 0;
  /* 当前Slcie使用的SPS ID */
  uint32_t curr_sps_id = 0;
  // SPSExt m_sps_ext;

  PPS m_ppss[MAX_PPS_COUNT]; // pps[256]
  /* 最新得到的PPS ID */
  uint32_t last_pps_id = 0;
  /* 当前Slcie使用的PPS ID */
  uint32_t curr_pps_id = 0;

  // DPB: decoded picture buffer
  Frame *m_dpb[MAX_DPB];
  // 因为含有B帧的视频帧的显示顺序和解码顺序是不一样的，已经解码完的P/B帧不能立即输出给用户，需要先缓存一下，一般来说大小为2,按照最大申请
  Frame *m_dpb_for_output[MAX_DPB];
  // 最大重排序帧数有B帧一般是2
  int32_t m_max_num_reorder_frames = 0;
  int32_t m_dpb_for_output_length = 0;

 public:
  int outputOneFrame(Frame *newDecodedPic, Frame *&outPic);
  int flush();
};

#endif /* end of include guard: GOP_HPP_PUYEPJDM */
