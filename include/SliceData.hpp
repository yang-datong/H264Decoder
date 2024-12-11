#ifndef SLICEBODY_HPP_OVHTPIZQ
#define SLICEBODY_HPP_OVHTPIZQ
#include "BitStream.hpp"
#include "Cabac.hpp"
#include "SliceHeader.hpp"
#include <cstdint>
#include <cstring>

class PictureBase;
class SliceData {
 private:
  //允许Slice类访问
  friend class Slice;
  SliceData(){};
  ~SliceData() {
    header = nullptr;
    cabac = nullptr;
    bs = nullptr;
    pic = nullptr;
    m_sps = nullptr;
    m_pps = nullptr;
  }

  int coding_tree_unit();
  int sao(int32_t rx, int32_t ry);
  int coding_quadtree(int x0, int y0, int log2CbSize, int cqtDepth);
  int coding_unit(int x0, int y0, int log2CbSize);
  int prediction_unit(int x0, int y0, int nPbW, int nPbH);
  int pcm_sample(int x0, int y0, int log2CbSize);
  int transform_tree(int x0, int y0, int xBase, int yBase, int log2TrafoSize,
                     int trafoDepth, int blkIdx);
  int mvd_coding(int x0, int y0, int refList);
  int transform_unit(int x0, int y0, int xBase, int yBase, int log2TrafoSize,
                     int trafoDepth, int blkIdx);
  int residual_coding(int x0, int y0, int log2TrafoSize, int cIdx);
  int cross_comp_pred(int x0, int y0, int c);
  int palette_coding(int x0, int y0, int nCbS);
  int delta_qp();
  int chroma_qp_offset();
  int CtbAddrInTs = 0;
  int CtbAddrInRs = 0;

  int tab_slice_address[32] = {0};
  int first_qp_group = 0;
  int end_of_tiles_x = 0;
  void hls_decode_neighbour(int x_ctb, int y_ctb, int ctb_addr_ts);

  int cabac_init(int ctb_addr_ts);

  int ff_init_cabac_decoder();
  int cabac_init_state();
  int load_states();
  int Z_scan_order_array_initialization();
  int derivation_z_scan_order_block_availability(int xCurr, int yCurr, int xNbY,
                                                 int yNbY);

  uint8_t cabac_state[HEVC_CONTEXTS] = {0};
  int StatCoeff[4] = {0};

  int parseSaoMerge();

 public:
  /* 这个编号是解码器自己维护的，每次解码一帧则++ */
  uint32_t slice_number = 0;
  uint32_t CurrMbAddr = 0;

  /* 由CABAC单独解码而来的重要控制变量 */
 public:
  /* 当前宏块是否跳过解码标志 */
  int32_t mb_skip_flag = 0;
  /* 下一宏块是否跳过解码标志 */
  int32_t mb_skip_flag_next_mb = 0;
  uint32_t mb_skip_run = 0;
  int32_t mb_field_decoding_flag = 0;
  int32_t end_of_slice_flag = -1;

 private:
  /* 引用自Slice Header，不能在SliceData中进行二次修改，但是为了代码设计，这里并没有设置为const模式 */
  bool MbaffFrameFlag = 0;
  bool is_mb_field_decoding_flag_prcessed = false;

 private:
  /* 由外部(parseSliceData)传进来的指针，不是Slice Data的一部分，随着parseSliceData后一起消灭 */
  SPS *m_sps = nullptr;
  PPS *m_pps = nullptr;
  /* 由外部(parseSliceData)传进来的指针，不是Slice Data的一部分，随着parseSliceData后一起消灭 */
  SliceHeader *header = nullptr;
  /* 由外部(parseSliceData)初始化，不是Slice Data的一部分，随着parseSliceData后一起消灭 */
  Cabac *cabac = nullptr;
  /* 由外部(parseSliceData)传进来的指针，不是Slice Data的一部分，随着parseSliceData后一起消灭 */
  BitStream *bs = nullptr;
  PictureBase *pic = nullptr;

  int slice_segment_data(BitStream &bs, PictureBase &pic, SPS &sps, PPS &pps);

  int hls_coding_quadtree(int x0, int y0, int log2_cb_size, int cb_depth);
  int ff_hevc_split_coding_unit_flag_decode(int ct_depth, int x0, int y0);

  int ctb_left_flag = 0;
  int ctb_up_flag = 0;
  int ctb_up_right_flag = 0;
  int ctb_up_left_flag = 0;

  /* process表示处理字段，具体处理手段有推流或解码操作 */
  int process_mb_skip_run(int32_t &prevMbSkipped);
  int process_mb_skip_flag(int32_t prevMbSkipped);
  int process_mb_field_decoding_flag(bool entropy_coding_mode_flag);
  int process_end_of_slice_flag(int32_t &end_of_slice_flag);

  int slice_decoding_process();
  inline int decoding_macroblock_to_slice_group_map();
  inline int mapUnitToSliceGroupMap();
  inline int mbToSliceGroupMap();

  /* derivation表示推断字段（根据其他内容进行猜测） */
  int derivation_for_mb_field_decoding_flag();
  int do_macroblock_layer();
  int decoding_process();
  int initCABAC();
  void printFrameReorderPriorityInfo();

 private:
  inline void updatesLocationOfCurrentMacroblock(bool MbaffFrameFlag);

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
