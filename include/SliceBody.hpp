#ifndef SLICEBODY_HPP_OVHTPIZQ
#define SLICEBODY_HPP_OVHTPIZQ

#include <cstdint>
class SliceBody {
 public:
  // Slice();
  //~Slice();

  /* NOTE: 默认值应该设置为0,因为有时候就是需要使用到默认值为0的情况，
   * 如果将变量放在for中那么默认值为其他就会发生不可预知的错误 */
 public:
  uint32_t cabac_alignment_one_bit = 0;
  uint32_t mb_field_decoding_flag = 0;
  uint32_t CurrMbAddr = 0;
  uint32_t prevMbSkipped = 0;
  bool moreDataFlag = 1;
};
#endif /* end of include guard: SLICEBODY_HPP_OVHTPIZQ */
