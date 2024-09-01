#ifndef SLICEBODY_HPP_OVHTPIZQ
#define SLICEBODY_HPP_OVHTPIZQ
#include "BitStream.hpp"
#include "H264Cabac.hpp"
#include "SliceHeader.hpp"
#include <cstdint>

class PictureBase;
class SliceData {
 private:
  SPS m_sps;
  PPS m_pps;
  IDR m_idr;

 private:
  /* 私有化SliceBody，不提供给外界，只能通过Slice来访问本类 */
  SliceData(SPS &sps, PPS &pps) : m_sps(sps), m_pps(pps) {}

 public:
  /* 允许Slice类访问 */
  friend class Slice;
  void setSPS(SPS &sps) { this->m_sps = sps; }
  void setPPS(PPS &pps) { this->m_pps = pps; }

  /* NOTE: 默认值应该设置为0,因为有时候就是需要使用到默认值为0的情况，
   * 如果将变量放在for中那么默认值为其他就会发生不可预知的错误 */

 public:
  /* 这个id是解码器自己维护的，每次解码一帧则+1 */
  uint32_t slice_id = 0;
  /* 这个编号是解码器自己维护的，每次解码一帧则+1，与slice id同理 */
  uint32_t slice_number = 0;

  /* 由CABAC单独解码而来的重要控制变量 */
  int32_t mb_skip_flag = 0;
  uint32_t mb_skip_run = 0;
  int32_t mb_skip_flag_next_mb = 0;
  int32_t mb_field_decoding_flag = 0;
  int32_t end_of_slice_flag = 0; // 2 ae(v)

  uint32_t CurrMbAddr = 0;

  int parseSliceData(BitStream &bitStream, PictureBase &picture);

 private:
  int do_decoding_picture_order_count(PictureBase &picture,
                                      const SliceHeader &header);
  int process_mb_skip_run();
  int process_mb_skip_flag(PictureBase &picture, const SliceHeader &header,
                           CH264Cabac &cabac, const int32_t prevMbSkipped);
  int process_mb_field_decoding_flag();
  int process_end_of_slice_flag(CH264Cabac &cabac);
  int do_macroblock_layer(PictureBase &picture, BitStream &bs,
                          CH264Cabac &cabac, const SliceHeader &header);

  int NextMbAddress(int n, SliceHeader &slice_header);
  int initCABAC(CH264Cabac &cabac, BitStream &bs, SliceHeader &slice_header);

  void printFrameReorderPriorityInfo(PictureBase &picture);
};
#endif /* end of include guard: SLICEBODY_HPP_OVHTPIZQ */
