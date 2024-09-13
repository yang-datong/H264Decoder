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

  /* TODO: 同时也需要当前使用的SPS、PPS，因为header、Data内的SPS,PPS是不允许对外提供的，相当于Slice是一个对外类，header、data是Slice的内部类，只能由Slice操作 */

  Slice();

  SliceHeader slice_header;
  SliceData slice_data;
};

#endif /* end of include guard: SLICE_CPP_BRS58Q9D */
