#ifndef SLICE_HPP_BRS58Q9D
#define SLICE_HPP_BRS58Q9D

#include "MacroBlock.hpp"
#include "SliceBody.hpp"
#include "SliceHeader.hpp"
#include <memory>
#include <vector>

class PictureBase;
class Nalu;
class Frame;

class Slice {
 private:
  std::vector<std::shared_ptr<MacroBlock>> _macroblocks;

 public:
  void addMacroblock(std::shared_ptr<MacroBlock> macroblock);

  int encode();
  int decode(BitStream &bitStream, Frame *(&dpb)[16], SPS &sps, PPS &pps,
             Frame *frame);

  SPS m_sps;
  PPS m_pps;
  IDR m_idr;
  Slice();

  SliceHeader slice_header;
  SliceBody slice_body;

  /* SliceHeader,SliceBody的sps,pps只能由Slice进行修改! */
  void setSPS(SPS &sps) {
    this->m_sps = sps;
    this->slice_header.setSPS(sps);
    this->slice_body.setSPS(sps);
  };

  void setPPS(PPS &pps) {
    this->m_pps = pps;
    this->slice_header.setPPS(pps);
    this->slice_body.setPPS(pps);
  };
};

#endif /* end of include guard: SLICE_CPP_BRS58Q9D */
