#ifndef SLICE_HPP_BRS58Q9D
#define SLICE_HPP_BRS58Q9D

#include "MacroBlock.hpp"
#include "SliceData.hpp"
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

  /* 同时也需要当前使用的SPS、PPS，因为header、Data内的SPS,PPS是不允许对外提供的，相当于Slice是一个对外类，header、data是Slice的内部类，只能由Slice操作 */
  SPS m_sps;
  PPS m_pps;
  /* Slice需要一组SPS、PPS因为若存在多个SPS，PPS时会在Slice Header中指明当前的PPS，进而推导出SPS */
  /* TODO YangJing  <24-09-14 00:28:39> */
  //SPS m_spss[MAX_SPS_COUNT]; // sps[32]
  //PPS m_ppss[MAX_PPS_COUNT]; // pps[256]

  Slice();

  SliceHeader slice_header;
  SliceData slice_data;

  /* SliceHeader,SliceBody的sps,pps只能由Slice进行修改! */
  void setSPS(SPS &sps) {
    this->m_sps = sps;
    this->slice_header.setSPS(sps);
    this->slice_data.setSPS(sps);
  };

  void setPPS(PPS &pps) {
    this->m_pps = pps;
    this->slice_header.setPPS(pps);
    this->slice_data.setPPS(pps);
  };
};

#endif /* end of include guard: SLICE_CPP_BRS58Q9D */
