#include "Slice.hpp"
#include "Frame.hpp"
#include "Nalu.hpp"
#include "SliceData.hpp"

#include "SliceHeader.hpp"
#include "Type.hpp"

Slice::Slice(Nalu *nalu) : mNalu(nalu) {
  slice_header = new SliceHeader(mNalu->nal_unit_type, mNalu->nal_ref_idc);
  slice_data = new SliceData();
};

Slice::~Slice() {
  if (slice_header) {
    delete slice_header;
    slice_header = nullptr;
  }
  if (slice_data) {
    delete slice_data;
    slice_data = nullptr;
  }
}

void Slice::addMacroblock(std::shared_ptr<MacroBlock> macroblock) {
  _macroblocks.push_back(macroblock);
}

int Slice::encode() {
  // Implement slice encoding logic
  //for (auto &mb : _macroblocks) {
  //mb->encode();
  //}
  return 0;
}

int Slice::decode(BitStream &bs, Frame *(&dpb)[16], SPS &sps, PPS &pps,
                  Frame *frame) {
  //----------------帧----------------------------------
  frame->m_picture_coded_type = FRAME;
  frame->m_picture_frame.m_picture_coded_type = FRAME;
  frame->m_picture_frame.m_parent = frame;
  memcpy(frame->m_picture_frame.m_dpb, dpb, sizeof(Frame *) * MAX_DPB);
  frame->m_current_picture_ptr = &(frame->m_picture_frame);
  frame->m_picture_frame.init(this);

  slice_data->slice_segment_data(bs, frame->m_picture_frame, sps, pps);
  return 0;
}
