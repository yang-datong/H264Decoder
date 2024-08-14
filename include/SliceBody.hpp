#ifndef SLICEBODY_HPP_OVHTPIZQ
#define SLICEBODY_HPP_OVHTPIZQ
#include "BitStream.hpp"
#include "SliceHeader.hpp"

#include <cstdint>
class PictureBase;
class SliceBody {
 public:
  // Slice();
  //~Slice();

  SliceHeader slice_header;
  SPS m_sps;
  PPS m_pps;
  IDR m_idr;

  /* NOTE: 默认值应该设置为0,因为有时候就是需要使用到默认值为0的情况，
   * 如果将变量放在for中那么默认值为其他就会发生不可预知的错误 */
 public:
  /* TODO YangJing 这里可能有点问题id由1开始？ <24-07-31 16:41:24> */
  uint32_t slice_id = 0;
  uint32_t slice_number = 0;

  uint32_t mb_skip_run = 0;
  int32_t mb_skip_flag = 0;
  int32_t end_of_slice_flag = 0; // 2 ae(v)
  int32_t mb_skip_flag_next_mb = 0;

  uint32_t cabac_alignment_one_bit = 0;
  int32_t mb_field_decoding_flag = 0;
  uint32_t CurrMbAddr = 0;
  uint32_t prevMbSkipped = 0;
  bool moreDataFlag = 1;

  int parseSliceData(BitStream &bitStream, PictureBase &picture);

  int NextMbAddress(int n);
};
#endif /* end of include guard: SLICEBODY_HPP_OVHTPIZQ */
