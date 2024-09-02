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

 private:
  /* 私有化SliceBody，不提供给外界，只能通过Slice来访问本类 */
  SliceData(SPS &sps, PPS &pps) : m_sps(sps), m_pps(pps) {}

 public:
  /* 允许Slice类访问 */
  friend class Slice;
  void setSPS(SPS &sps) { this->m_sps = sps; }
  void setPPS(PPS &pps) { this->m_pps = pps; }

 public:
  /* 这个id是解码器自己维护的，每次解码一帧则+1 */
  uint32_t slice_id = 0;
  /* 这个编号是解码器自己维护的，每次解码一帧则+1，与slice id同理 */
  uint32_t slice_number = 0;

  /* 由CABAC单独解码而来的重要控制变量 */
  int32_t mb_skip_flag = 0;
  int32_t mb_field_decoding_flag = 0;
  uint32_t CurrMbAddr = 0;

 private:
  uint32_t mb_skip_run = 0;
  int32_t mb_skip_flag_next_mb = 0;

 private:
  int parseSliceData(BitStream &bitStream, PictureBase &picture);
  /* 由外部(parseSliceData)传进来的指针，不是Slice Data的一部分，随着parseSliceData后一起消灭 */
  SliceHeader *header = nullptr;
  /* 由外部(parseSliceData)初始化，不是Slice Data的一部分，随着parseSliceData后一起消灭 */
  CH264Cabac *cabac = nullptr;
  /* 由外部(parseSliceData)传进来的指针，不是Slice Data的一部分，随着parseSliceData后一起消灭 */
  BitStream *bs = nullptr;

  bool is_need_skip_read_mb_field_decoding_flag = false;
  int slice_decoding_process(PictureBase &picture);
  int decoding_macroblock_to_slice_group_map();
  int setMapUnitToSliceGroupMap();
  int setMbToSliceGroupMap();

  /* process表示处理字段，具体处理手段有推流或解码操作 */
  int process_mb_skip_run(PictureBase &picture, int32_t &prevMbSkipped,
                          const bool &is_cabac);
  int process_mb_skip_flag(PictureBase &picture, const int32_t prevMbSkipped);
  int process_mb_field_decoding_flag(PictureBase &picture,
                                     const bool entropy_coding_mode_flag);
  int process_end_of_slice_flag(int32_t &end_of_slice_flag);

  /* derivation表示推断字段（根据其他内容进行猜测） */
  int derivation_for_mb_field_decoding_flag(PictureBase &picture);

  int do_macroblock_layer(PictureBase &picture);

  int initCABAC();

  void printFrameReorderPriorityInfo(PictureBase &picture);

 private:
  void updatesLocationOfCurrentMacroblock(PictureBase &picture,
                                          const bool MbaffFrameFlag);

 private:
  /* 用于更新mapUnitToSliceGroupMap的值 */
  int interleaved_slice_group_map_type(int32_t *&mapUnitToSliceGroupMap);
  int dispersed_slice_group_map_type(int32_t *&mapUnitToSliceGroupMap);
  int foreground_with_left_over_slice_group_ma_type(
      int32_t *&mapUnitToSliceGroupMap);
  int box_out_slice_group_map_types(int32_t *&mapUnitToSliceGroupMap,
                                    const int &MapUnitsInSliceGroup0);
  int raster_scan_slice_group_map_types(int32_t *&mapUnitToSliceGroupMap,
                                        const int &MapUnitsInSliceGroup0);
  int wipe_slice_group_map_types(int32_t *&mapUnitToSliceGroupMap,
                                 const int &MapUnitsInSliceGroup0);
  int explicit_slice_group_map_type(int32_t *&mapUnitToSliceGroupMap);
};

int NextMbAddress(int n, SliceHeader *slice_header);

#endif /* end of include guard: SLICEBODY_HPP_OVHTPIZQ */
