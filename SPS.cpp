#include "SPS.hpp"

SPS::SPS() {
  MaxPicOrderCntLsb = h264_power2(log2_max_pic_order_cnt_lsb_minus4 + 4);
  if (pic_order_cnt_type == 1) {
    ExpectedDeltaPerPicOrderCntCycle = 0;
    for (int i = 0; i < (int32_t)num_ref_frames_in_pic_order_cnt_cycle; i++) {
      ExpectedDeltaPerPicOrderCntCycle += offset_for_ref_frame[i];
    }
  }
}

SPS::~SPS() {}
