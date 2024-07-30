#ifndef __H264_SLICE_DATA_H__
#define __H264_SLICE_DATA_H__

#include "BitStream.hpp"
#include "H264Cabac.hpp"
#include <cstdint>

class PictureBase;
class Picture;

/*
 * T-REC-H.264-201704-S!!PDF-E.pdf
 * Page 55/77/812
 * 7.3.4 Slice data syntax
 */
class CH264SliceData {
 public:
  int32_t cabac_alignment_one_bit; // 2 f(1)
  int32_t mb_skip_run;             // 2 ue(v)
  int32_t mb_skip_flag;            // 2 ae(v)
  int32_t end_of_slice_flag;       // 2 ae(v)
  int32_t mb_field_decoding_flag;  // 2 u(1) | ae(v)    0-a frame macroblock
                                   // pair; 1-a field macroblock pair;

  int32_t slice_id;
  int32_t slice_number;
  int32_t CurrMbAddr;
  int32_t syntax_element_categories; // 2 | 3 | 4

 public:
  CH264SliceData();
  ~CH264SliceData();

  int printInfo();

  int init();

  int slice_data(BitStream &bs, PictureBase &picture, int32_t _slice_id);
  // int NextMbAddress(const H264SliceHeader &slice_header, int32_t n);
};

#endif //__H264_SLICE_DATA_H__
