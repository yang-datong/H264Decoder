#include "SliceData.hpp"
#include "BitStream.hpp"
#include "Frame.hpp"
#include "PictureBase.hpp"
#include "SliceHeader.hpp"
#include <cstdint>
#include <cstring>

/* 7.3.4 Slice data syntax */
int SliceData::parseSliceData(BitStream &bs, PictureBase &picture) {
  SliceHeader &header = picture.m_slice.slice_header;

  /* CABAC编码 */
  CH264Cabac cabac(bs, picture);
  /* 1. 对于Slice的首个熵解码，则需要初始化CABAC模型 */
  initCABAC(cabac, bs, header);

  if (header.MbaffFrameFlag == 0)
    /* 如果当前帧不使用MBAFF编码模式。所有宏块都作为帧宏块进行编码 */
    mb_field_decoding_flag = header.field_pic_flag;

  picture.CurrMbAddr = CurrMbAddr =
      header.first_mb_in_slice * (1 + header.MbaffFrameFlag);

  /* 更新参考帧列表0,1的预测图像编号 */
  header.picNumL0Pred = header.picNumL1Pred = header.CurrPicNum;

  /* 处理帧间控制变量（主要是针对帧间解码情况） */
  /* 8.2.1 Decoding process for picture order count */
  do_decoding_picture_order_count(picture, header);

  /* 8.2.5 Decoded reference picture marking process ? */
  decoding_macroblock_to_slice_group_map(header);

  bool moreDataFlag = 1;
  int32_t prevMbSkipped = 0;

  do {
    /* 1. 对于非I帧，先解码出mb_skip_flag，判断是否跳过对MacroBlock的处理 */
    if (header.slice_type != SLICE_I && header.slice_type != SLICE_SI) {
      if (m_pps.entropy_coding_mode_flag == 0) {
        /* CAVLC熵编码，暂时不处理 */
        process_mb_skip_run();
        prevMbSkipped = (mb_skip_run > 0);
        for (int i = 0; i < mb_skip_run; i++)
          CurrMbAddr = NextMbAddress(CurrMbAddr, header);
        if (mb_skip_run > 0) moreDataFlag = bs.more_rbsp_data();
      } else {
        /* CABAC熵编码 */
        process_mb_skip_flag(picture, header, cabac, prevMbSkipped);
        moreDataFlag = !mb_skip_flag;
      }
    }

    /* 2. 对MacroBlock的处理 */
    if (moreDataFlag) {
      if (header.MbaffFrameFlag &&
          (CurrMbAddr % 2 == 0 || (CurrMbAddr % 2 == 1 && prevMbSkipped)))
        /* 表示本宏块是属于一个宏块对中的一个 */
        process_mb_field_decoding_flag();

      /* 这里包括了后续的，帧内预测、帧间预测、解码宏块、去块滤波 */
      do_macroblock_layer(picture, bs, cabac, header);
    }

    if (!m_pps.entropy_coding_mode_flag)
      moreDataFlag = bs.more_rbsp_data();
    else {
      if (header.slice_type != SLICE_I && header.slice_type != SLICE_SI)
        prevMbSkipped = mb_skip_flag;
      if (header.MbaffFrameFlag && CurrMbAddr % 2 == 0)
        moreDataFlag = 1;
      else {
        process_end_of_slice_flag(cabac);
        moreDataFlag = !end_of_slice_flag;
      }
    }
    CurrMbAddr = NextMbAddress(CurrMbAddr, header);
  } while (moreDataFlag);

  if (picture.mb_cnt == picture.PicSizeInMbs) picture.m_is_decode_finished = 1;

  slice_id++;
  slice_number++;
  return 0;
}

/* 9.3.1 Initialization process */
int SliceData::initCABAC(CH264Cabac &cabac, BitStream &bs,
                         SliceHeader &header) {
  if (!m_pps.entropy_coding_mode_flag) return -1;

  /* CABAC(上下文自适应二进制算术编码) */
  while (!bs.byte_aligned())
    bs.readU1(); //cabac_alignment_one_bit

  // cabac初始化环境变量
  cabac.init_of_context_variables((H264_SLICE_TYPE)header.slice_type,
                                  header.cabac_init_idc, header.SliceQPY);
  // cabac初始化解码引擎
  cabac.init_of_decoding_engine();
  return 0;
}

int SliceData::do_decoding_picture_order_count(PictureBase &picture,
                                               const SliceHeader &header) {
  /* 设为0是防止在场编码时可能存在多个slice data，那么就只需要对首个slice data进行定位，防止对附属的slice data进行再次解码工作 */
  if (picture.m_slice_cnt == 0) {
    /* 解码参考帧重排序(POC) */
    // 8.2.1 Decoding process for picture order count
    picture.decoding_picture_order_count();
    if (m_sps.frame_mbs_only_flag == 0) {
      /* 场编码 */
      std::cout << "\033[33m Into -> " << __LINE__ << "()\033[0m" << std::endl;
      exit(0);
    }

    /* 在每个 P、SP 或 B 切片的解码过程开始时调用此过程。 */
    if (header.slice_type == SLICE_P || header.slice_type == SLICE_SP ||
        header.slice_type == SLICE_B) {
      /* 当前帧需要参考帧预测，则需要进行参考帧重排序 */
      // 8.2.4 Decoding process for reference picture lists construction
      picture.decoding_reference_picture_lists_construction(
          picture.m_dpb, picture.m_RefPicList0, picture.m_RefPicList1);
      /* (m_RefPicList0,m_RefPicList1为m_dpb排序后的前后参考列表）打印帧重排序先后信息 */
      printFrameReorderPriorityInfo(picture);
    }
  }
  picture.m_slice_cnt++;
  return 0;
}

// 8.2.2 Decoding process for macroblock to slice group map
/* 输入:活动图像参数集和要解码的切片的切片头。  
 * 输出:宏块到切片组映射MbToSliceGroupMap。 */
//该过程在每个切片开始时调用。
int SliceData::decoding_macroblock_to_slice_group_map(SliceHeader &header) {
  setMapUnitToSliceGroupMap(header);
  setMbToSliceGroupMap(header);
  return 0;
}

//8.2.2.1 - 8.2.2.7  Specification for interleaved slice group map type
int SliceData::setMapUnitToSliceGroupMap(SliceHeader &header) {

  /* 别名 */
  const int &MapUnitsInSliceGroup0 = header.MapUnitsInSliceGroup0;
  int32_t *&mapUnitToSliceGroupMap = header.mapUnitToSliceGroupMap;

  /* mapUnitToSliceGroupMap 数组的推导如下：
   * – 如果 num_slice_groups_minus1 等于 0，则为范围从 0 到 PicSizeInMapUnits − 1（含）的所有 i 生成切片组映射的映射单元，如 mapUnitToSliceGroupMap[ i ] = 0 */
  if (m_pps.num_slice_groups_minus1 == 0) {
    for (int i = 0; i < m_sps.PicSizeInMapUnits; i++)
      mapUnitToSliceGroupMap[i] = 0;
    return 0;
  }

  /* — 否则（num_slice_groups_minus1 不等于0），mapUnitToSliceGroupMap 的推导如下： 
       * — 如果slice_group_map_type 等于0，则应用第8.2.2.1 节中指定的mapUnitToSliceGroupMap 的推导。  
       * — 否则，如果slice_group_map_type等于1，则应用第8.2.2.2节中指定的mapUnitToSliceGroupMap的推导。  
       * — 否则，如果slice_group_map_type等于2，则应用第8.2.2.3节中指定的mapUnitToSliceGroupMap的推导。
       * – 否则，如果slice_group_map_type等于3，则应用第8.2.2.4节中指定的mapUnitToSliceGroupMap的推导。  
       * — 否则，如果slice_group_map_type等于4，则应用第8.2.2.5节中指定的mapUnitToSliceGroupMap的推导。  
       * – 否则，如果slice_group_map_type等于5，则应用第8.2.2.6节中指定的mapUnitToSliceGroupMap的推导。  
       * — 否则（slice_group_map_type 等于 6），应用第 8.2.2.7 节中指定的 mapUnitToSliceGroupMap 的推导。*/

  switch (m_pps.slice_group_map_type) {
  case 0:
    interleaved_slice_group_map_type(mapUnitToSliceGroupMap);
    break;
  case 1:
    dispersed_slice_group_map_type(mapUnitToSliceGroupMap);
    break;
  case 2:
    foreground_with_left_over_slice_group_ma_type(mapUnitToSliceGroupMap);
    break;
  case 3:
    box_out_slice_group_map_types(mapUnitToSliceGroupMap,
                                  MapUnitsInSliceGroup0);
    break;
  case 4:
    raster_scan_slice_group_map_types(mapUnitToSliceGroupMap,
                                      MapUnitsInSliceGroup0);
    break;
  case 5:
    wipe_slice_group_map_types(mapUnitToSliceGroupMap, MapUnitsInSliceGroup0);
    break;
  default:
    explicit_slice_group_map_type(mapUnitToSliceGroupMap);
    break;
  }
  return 0;
}

//8.2.2.1 Specification for interleaved slice group map type
int SliceData::interleaved_slice_group_map_type(
    int32_t *&mapUnitToSliceGroupMap) {
  int i = 0;
  do {
    for (int iGroup = 0;
         iGroup <= m_pps.num_slice_groups_minus1 && i < m_sps.PicSizeInMapUnits;
         i += m_pps.run_length_minus1[iGroup++] + 1) {
      for (int j = 0; j <= m_pps.run_length_minus1[iGroup] &&
                      i + j < m_sps.PicSizeInMapUnits;
           j++) {
        mapUnitToSliceGroupMap[i + j] = iGroup;
      }
    }
  } while (i < m_sps.PicSizeInMapUnits);
  return 0;
}
//8.2.2.2 Specification for dispersed slice group map type
int SliceData::dispersed_slice_group_map_type(
    int32_t *&mapUnitToSliceGroupMap) {
  for (int i = 0; i < m_sps.PicSizeInMapUnits; i++) {
    mapUnitToSliceGroupMap[i] =
        ((i % m_sps.PicWidthInMbs) +
         (((i / m_sps.PicWidthInMbs) * (m_pps.num_slice_groups_minus1 + 1)) /
          2)) %
        (m_pps.num_slice_groups_minus1 + 1);
  }
  return 0;
}
//8.2.2.3 Specification for foreground with left-over slice group map type
int SliceData::foreground_with_left_over_slice_group_ma_type(
    int32_t *&mapUnitToSliceGroupMap) {
  for (int i = 0; i < m_sps.PicSizeInMapUnits; i++) {
    mapUnitToSliceGroupMap[i] = m_pps.num_slice_groups_minus1;
  }
  for (int iGroup = m_pps.num_slice_groups_minus1 - 1; iGroup >= 0; iGroup--) {
    int32_t yTopLeft = m_pps.top_left[iGroup] / m_sps.PicWidthInMbs;
    int32_t xTopLeft = m_pps.top_left[iGroup] % m_sps.PicWidthInMbs;
    int32_t yBottomRight = m_pps.bottom_right[iGroup] / m_sps.PicWidthInMbs;
    int32_t xBottomRight = m_pps.bottom_right[iGroup] % m_sps.PicWidthInMbs;
    for (int y = yTopLeft; y <= yBottomRight; y++) {
      for (int x = xTopLeft; x <= xBottomRight; x++) {
        mapUnitToSliceGroupMap[y * m_sps.PicWidthInMbs + x] = iGroup;
      }
    }
  }
  return 0;
}
//8.2.2.4 Specification for box-out slice group map types
int SliceData::box_out_slice_group_map_types(int32_t *&mapUnitToSliceGroupMap,
                                             const int &MapUnitsInSliceGroup0) {

  for (int i = 0; i < m_sps.PicSizeInMapUnits; i++) {
    mapUnitToSliceGroupMap[i] = 1;
  }
  int x = (m_sps.PicWidthInMbs - m_pps.slice_group_change_direction_flag) / 2;
  int y =
      (m_sps.PicHeightInMapUnits - m_pps.slice_group_change_direction_flag) / 2;

  int32_t leftBound = x;
  int32_t topBound = y;
  int32_t rightBound = x;
  int32_t bottomBound = y;
  int32_t xDir = m_pps.slice_group_change_direction_flag - 1;
  int32_t yDir = m_pps.slice_group_change_direction_flag;
  int32_t mapUnitVacant = 0;

  for (int k = 0; k < MapUnitsInSliceGroup0; k += mapUnitVacant) {
    mapUnitVacant = (mapUnitToSliceGroupMap[y * m_sps.PicWidthInMbs + x] == 1);
    if (mapUnitVacant) {
      mapUnitToSliceGroupMap[y * m_sps.PicWidthInMbs + x] = 0;
    }
    if (xDir == -1 && x == leftBound) {
      leftBound = fmax(leftBound - 1, 0);
      x = leftBound;
      xDir = 0;
      yDir = 2 * m_pps.slice_group_change_direction_flag - 1;
    } else if (xDir == 1 && x == rightBound) {
      rightBound = MIN(rightBound + 1, m_sps.PicWidthInMbs - 1);
      x = rightBound;
      xDir = 0;
      yDir = 1 - 2 * m_pps.slice_group_change_direction_flag;
    } else if (yDir == -1 && y == topBound) {
      topBound = MAX(topBound - 1, 0);
      y = topBound;
      xDir = 1 - 2 * m_pps.slice_group_change_direction_flag;
      yDir = 0;
    } else if (yDir == 1 && y == bottomBound) {
      bottomBound = MIN(bottomBound + 1, m_sps.PicHeightInMapUnits - 1);
      y = bottomBound;
      xDir = 2 * m_pps.slice_group_change_direction_flag - 1;
      yDir = 0;
    } else {
      //(x, y) = (x + xDir, y + yDir);
    }
  }
  return 0;
}
//8.2.2.5 Specification for raster scan slice group map types
int SliceData::raster_scan_slice_group_map_types(
    int32_t *&mapUnitToSliceGroupMap, const int &MapUnitsInSliceGroup0) {
  // 8.2.2.5 Specification for raster scan slice group map types
  // 栅格扫描型 slice 组映射类型的描述
  int32_t sizeOfUpperLeftGroup = 0;
  if (m_pps.num_slice_groups_minus1 == 1) {
    sizeOfUpperLeftGroup =
        (m_pps.slice_group_change_direction_flag
             ? (m_sps.PicSizeInMapUnits - MapUnitsInSliceGroup0)
             : MapUnitsInSliceGroup0);
  }

  for (int i = 0; i < m_sps.PicSizeInMapUnits; i++) {
    if (i < sizeOfUpperLeftGroup) {
      mapUnitToSliceGroupMap[i] = m_pps.slice_group_change_direction_flag;
    } else {
      mapUnitToSliceGroupMap[i] = 1 - m_pps.slice_group_change_direction_flag;
    }
  }
  return 0;
}
//8.2.2.6 Specification for wipe slice group map types
int SliceData::wipe_slice_group_map_types(int32_t *&mapUnitToSliceGroupMap,
                                          const int &MapUnitsInSliceGroup0) {
  int32_t sizeOfUpperLeftGroup = 0;
  if (m_pps.num_slice_groups_minus1 == 1) {
    sizeOfUpperLeftGroup =
        (m_pps.slice_group_change_direction_flag
             ? (m_sps.PicSizeInMapUnits - MapUnitsInSliceGroup0)
             : MapUnitsInSliceGroup0);
  }

  int k = 0;
  for (int j = 0; j < m_sps.PicWidthInMbs; j++) {
    for (int i = 0; i < m_sps.PicHeightInMapUnits; i++) {
      if (k++ < sizeOfUpperLeftGroup) {
        mapUnitToSliceGroupMap[i * m_sps.PicWidthInMbs + j] =
            m_pps.slice_group_change_direction_flag;
      } else {
        mapUnitToSliceGroupMap[i * m_sps.PicWidthInMbs + j] =
            1 - m_pps.slice_group_change_direction_flag;
      }
    }
  }
  return 0;
}

//8.2.2.7 Specification for explicit slice group map type
int SliceData::explicit_slice_group_map_type(int32_t *&mapUnitToSliceGroupMap) {
  for (int i = 0; i < m_sps.PicSizeInMapUnits; i++)
    mapUnitToSliceGroupMap[i] = m_pps.slice_group_id[i];
  return 0;
}

// 8.2.2.8 Specification for conversion of map unit to slice group map to macroblock to slice group map
int SliceData::setMbToSliceGroupMap(SliceHeader &header) {
  int32_t *&MbToSliceGroupMap = header.MbToSliceGroupMap;
  int32_t *&mapUnitToSliceGroupMap = header.mapUnitToSliceGroupMap;

  /* 对于范围从 0 到 PicSizeInMbs - 1（含）的每个 i 值，宏块到切片组映射指定如下： */
  for (int i = 0; i < header.PicSizeInMbs; i++) {
    if (m_sps.frame_mbs_only_flag == 1 || header.field_pic_flag == 1)
      MbToSliceGroupMap[i] = mapUnitToSliceGroupMap[i];
    else if (header.MbaffFrameFlag == 1)
      MbToSliceGroupMap[i] = mapUnitToSliceGroupMap[i / 2];
    else
      MbToSliceGroupMap[i] =
          mapUnitToSliceGroupMap[(i / (2 * m_sps.PicWidthInMbs)) *
                                     m_sps.PicWidthInMbs +
                                 (i % m_sps.PicWidthInMbs)];
  }

  return 0;
}

int SliceData::process_mb_skip_run() {
  std::cout << "\033[33m Into -> " << __LINE__ << "()\033[0m" << std::endl;
  exit(0);
  return 0;
}

/* 更新当前宏快位置 */
/* 输入：PicWidthInMbs,MbaffFrameFlag
 * 输出：mb_x, mb_y, CurrMbAddr*/
void SliceData::updatesLocationOfCurrentMacroblock(PictureBase &picture,
                                                   const bool MbaffFrameFlag) {

  const int32_t m = (picture.PicWidthInMbs * (1 + MbaffFrameFlag));

  picture.mb_x = (CurrMbAddr % m) / (1 + MbaffFrameFlag);
  picture.mb_y = (CurrMbAddr / m * (1 + MbaffFrameFlag)) +
                 ((CurrMbAddr % m) % (1 + MbaffFrameFlag));

  picture.CurrMbAddr = CurrMbAddr;
}

int SliceData::process_mb_skip_flag(PictureBase &picture,
                                    const SliceHeader &header,
                                    CH264Cabac &cabac,
                                    const int32_t prevMbSkipped) {
  /* 1. 计算当前宏块的位置 */
  updatesLocationOfCurrentMacroblock(picture, header.MbaffFrameFlag);

  //-------------解码mb_skip_flag-----------------------
  /* mb_skip_flag等于1指定对于当前宏块，在解码P或SP切片时，mb_type应推断为P_Skip，宏块类型统称为P宏块类型，或者在解码B切片时，mb_type应推断为B_Skip，该宏块类型统称为B宏块类型。 mb_skip_flag等于0表示不跳过当前宏块 */

  // //因为解码mb_skip_flag需要事先知道MbaffFrameFlag的值
  /* 2. 设置当前宏块的切片编号 */
  picture.m_mbs[picture.CurrMbAddr].slice_number = slice_number;
  // 因为解码mb_skip_flag需要事先知道slice_id的值（从0开始）

  if (header.MbaffFrameFlag) {
    /* 当前帧使用MBAFF编码模式。在这种模式下，每个宏块对（MB pair）可以独立地选择是作为帧宏块对还是场宏块对进行编码。 */
    std::cout << "\033[33m Into -> " << __LINE__ << "()\033[0m" << std::endl;
    exit(0);
  }

  /* 3. 处理宏块跳过标志位 */
  if (header.MbaffFrameFlag && CurrMbAddr % 2 == 1 && prevMbSkipped) {
    /* 当前帧使用MBAFF编码模式。在这种模式下，每个宏块对（MB pair）可以独立地选择是作为帧宏块对还是场宏块对进行编码。 */
    std::cout << "\033[33m Into -> " << __LINE__ << "()\033[0m" << std::endl;
    exit(0);
  } else
    cabac.decode_mb_skip_flag(CurrMbAddr, mb_skip_flag);

  /* 4. 处理跳过的宏块 */
  if (mb_skip_flag == 1) {
    // 表示本宏块没有残差数据，相应的像素值只需要利用之前已经解码的I/P帧来预测获得
    // 首个IDR帧不会进这里，紧跟其后的P帧会进这里（可能会进）
    picture.mb_cnt++;
    if (header.MbaffFrameFlag) {
      if (CurrMbAddr % 2 == 0) {
        // 只需要处理top field macroblock
        picture.m_mbs[picture.CurrMbAddr].mb_skip_flag = mb_skip_flag;
        // 因为解码mb_skip_flag_next_mb需要事先知道前面顶场宏块的mb_skip_flag值
        picture.m_mbs[picture.CurrMbAddr + 1].slice_number = slice_number;
        // 因为解码mb_skip_flag需要事先知道slice_id的值
        picture.m_mbs[picture.CurrMbAddr + 1].mb_field_decoding_flag =
            mb_field_decoding_flag;
        // 特别注意：底场宏块和顶场宏块的mb_field_decoding_flag值是相同的

        cabac.decode_mb_skip_flag(CurrMbAddr + 1, mb_skip_flag_next_mb);
        // 2 ae(v) 先读取底场宏块的mb_skip_flag

        if (mb_skip_flag_next_mb == 0) {
          // 如果底场宏块mb_skip_flag=0
          cabac.decode_mb_field_decoding_flag(mb_field_decoding_flag);
          // 2 u(1) | ae(v)
          // 再读取底场宏块的mb_field_decoding_flag

          //is_need_skip_read_mb_field_decoding_flag = true;
        } else // if (mb_skip_flag_next_mb == 1)
        {
          // When MbaffFrameFlag is equal to 1 and mb_field_decoding_flag
          // is not present for both the top and the bottom macroblock of
          // a macroblock pair
          if (picture.mb_x > 0 &&
              picture.m_mbs[CurrMbAddr - 2].slice_number ==
                  slice_number) // the left of the current macroblock pair
                                // in the same slice
          {
            mb_field_decoding_flag =
                picture.m_mbs[CurrMbAddr - 2].mb_field_decoding_flag;
          } else if (picture.mb_y > 0 &&
                     picture.m_mbs[CurrMbAddr - 2 * picture.PicWidthInMbs]
                             .slice_number ==
                         slice_number) // above the current macroblock
                                       // pair in the same slice
          {
            mb_field_decoding_flag =
                picture.m_mbs[CurrMbAddr - 2 * picture.PicWidthInMbs]
                    .mb_field_decoding_flag;
          } else {
            mb_field_decoding_flag = 0; // is inferred to be equal to 0
          }
        }
      }
    }

    //-----------------------------------------------------------------
    picture.m_mbs[picture.CurrMbAddr].macroblock_layer_mb_skip(
        picture, *this, cabac); // 2 | 3 | 4

    // The inter prediction process for P and B macroblocks is specified
    // in clause 8.4 with inter prediction samples being the output.
    picture.Inter_prediction_process(); // 帧间预测
  }

  return 0;
}

int SliceData::process_mb_field_decoding_flag() {
  std::cout << "hi~" << __LINE__ << std::endl;
  exit(0);
  return 0;
}

int SliceData::process_end_of_slice_flag(CH264Cabac &cabac) {
  cabac.decode_end_of_slice_flag(end_of_slice_flag);
  return 0;
}

int SliceData::do_macroblock_layer(PictureBase &picture, BitStream &bs,
                                   CH264Cabac &cabac,
                                   const SliceHeader &header) {

  updatesLocationOfCurrentMacroblock(picture, header.MbaffFrameFlag);
  picture.mb_cnt++;

  //--------熵解码------------
  picture.m_mbs[picture.CurrMbAddr].macroblock_layer(bs, picture, *this, cabac);

  //--------帧内/间预测------------
  //--------反量化------------
  //--------反变换------------

  int32_t isChroma = 0;
  int32_t isChromaCb = 0;
  int32_t BitDepth = 0;

  int32_t picWidthInSamplesL = picture.PicWidthInSamplesL;
  int32_t picWidthInSamplesC = picture.PicWidthInSamplesC;

  uint8_t *pic_buff_luma = picture.m_pic_buff_luma;
  uint8_t *pic_buff_cb = picture.m_pic_buff_cb;
  uint8_t *pic_buff_cr = picture.m_pic_buff_cr;
  // 帧内预测
  if (picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode == Intra_4x4) {
    isChroma = 0;
    isChromaCb = 0;
    BitDepth = picture.m_slice.m_sps.BitDepthY;

    picture.transform_decoding_process_for_4x4_luma_residual_blocks(
        isChroma, isChromaCb, BitDepth, picWidthInSamplesL, pic_buff_luma);

    isChromaCb = 1;
    picture.transform_decoding_process_for_chroma_samples(
        isChromaCb, picWidthInSamplesC, pic_buff_cb);

    isChromaCb = 0;
    picture.transform_decoding_process_for_chroma_samples(
        isChromaCb, picWidthInSamplesC, pic_buff_cr);

  } else if (picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode == Intra_8x8) {
    isChroma = 0;
    isChromaCb = 0;
    BitDepth = picture.m_slice.m_sps.BitDepthY;

    picture.transform_decoding_process_for_8x8_luma_residual_blocks(
        isChroma, isChromaCb, BitDepth, picWidthInSamplesL,
        picture.m_mbs[picture.CurrMbAddr].LumaLevel8x8, pic_buff_luma);

    isChromaCb = 1;
    picture.transform_decoding_process_for_chroma_samples(
        isChromaCb, picWidthInSamplesC, pic_buff_cb);

    isChromaCb = 0;
    picture.transform_decoding_process_for_chroma_samples(
        isChromaCb, picWidthInSamplesC, pic_buff_cr);

  } else if (picture.m_mbs[picture.CurrMbAddr].m_mb_pred_mode == Intra_16x16) {
    isChroma = 0;
    isChromaCb = 0;
    BitDepth = picture.m_slice.m_sps.BitDepthY;
    int32_t QP1 = picture.m_mbs[picture.CurrMbAddr].QP1Y;

    picture
        .transform_decoding_process_for_luma_samples_of_Intra_16x16_macroblock_prediction_mode(
            isChroma, BitDepth, QP1, picWidthInSamplesL,
            picture.m_mbs[picture.CurrMbAddr].Intra16x16DCLevel,
            picture.m_mbs[picture.CurrMbAddr].Intra16x16ACLevel, pic_buff_luma);
    isChromaCb = 1;
    picture.transform_decoding_process_for_chroma_samples(
        isChromaCb, picWidthInSamplesC, pic_buff_cb);

    isChromaCb = 0;
    picture.transform_decoding_process_for_chroma_samples(
        isChromaCb, picWidthInSamplesC, pic_buff_cr);
  } else if (picture.m_mbs[picture.CurrMbAddr].m_name_of_mb_type == I_PCM)
    // 说明该宏块没有残差，也没有预测值，码流中的数据直接为原始像素值
    exit(0);
  else {

    // P,B帧，I帧不会进这里
    picture.Inter_prediction_process(); // 帧间预测

    BitDepth = picture.m_slice.m_sps.BitDepthY;

    //-------残差-----------
    if (picture.m_mbs[picture.CurrMbAddr].transform_size_8x8_flag == 0) {
      picture.transform_decoding_process_for_4x4_luma_residual_blocks_inter(
          isChroma, isChromaCb, BitDepth, picWidthInSamplesL, pic_buff_luma);
    } else {
      picture.transform_decoding_process_for_8x8_luma_residual_blocks_inter(
          isChroma, isChromaCb, BitDepth, picWidthInSamplesL,
          picture.m_mbs[picture.CurrMbAddr].LumaLevel8x8, pic_buff_luma);
    }

    isChromaCb = 1;
    picture.transform_decoding_process_for_chroma_samples_inter(
        isChromaCb, picWidthInSamplesC, pic_buff_cb);

    isChromaCb = 0;
    picture.transform_decoding_process_for_chroma_samples_inter(
        isChromaCb, picWidthInSamplesC, pic_buff_cr);
  }

  return 0;
}

/* 在按照第 8.2.2.8 节的规定导出宏块到切片组映射之后，函数 NextMbAddress( n ) 被定义为由以下伪代码指定导出的变量 nextMbAddress 的值： */
int NextMbAddress(int currMbAddr, SliceHeader &header) {
  int nextMbAddr = currMbAddr + 1;
  while (nextMbAddr < header.PicSizeInMbs &&
         header.MbToSliceGroupMap[nextMbAddr] !=
             header.MbToSliceGroupMap[currMbAddr])
    nextMbAddr++;
  return nextMbAddr;
}

void SliceData::printFrameReorderPriorityInfo(PictureBase &picture) {
  string sliceType = "UNKNOWN";
  std::cout << "\tGOP[" << picture.m_PicNumCnt + 1 << "] -> {" << std::endl;
  for (int i = 0; i < picture.m_PicNumCnt + 1; ++i) {
    const auto &refPic = picture.m_dpb[i];
    if (refPic) {
      auto &frame = refPic->m_picture_frame;
      auto &sliceHeader = frame.m_slice.slice_header;

      sliceType = H264_SLIECE_TYPE_TO_STR(sliceHeader.slice_type);
      std::cout << "\t\t m_RefPicList0[" << i << "]: " << sliceType
                << "; PicOrderCnt(显示顺序)=" << frame.PicOrderCnt
                << "; PicNum(帧编号)=" << frame.PicNum
                << "; PicNumCnt(位置)=" << frame.m_PicNumCnt << ";\n";
    }
  }
  std::cout << "\t}" << std::endl;

  std::cout << "\t当前帧所参考帧列表(已排序) -> {" << std::endl;
  for (int i = 0; i < picture.m_RefPicList0Length; ++i) {
    const auto &refPic = picture.m_RefPicList0[i];
    if (refPic) {
      auto &frame = refPic->m_picture_frame;
      auto &sliceHeader = frame.m_slice.slice_header;

      sliceType = H264_SLIECE_TYPE_TO_STR(sliceHeader.slice_type);
      std::cout << "\t\t(前参考)m_RefPicList0[" << i << "]: " << sliceType
                << "; PicOrderCnt(参考帧显示顺序)=" << frame.PicOrderCnt
                << "; PicNum(参考帧编号)=" << frame.PicNum
                << "; PicNumCnt(原参考列表中的位置)=" << frame.m_PicNumCnt
                << ";\n";
    }
  }

  /* TODO YangJing m_RefPicList1好像并不是后参考帧，后面再确认一下 <24-08-31 18:19:22> */
  for (int i = 0; i < picture.m_RefPicList1Length; ++i) {
    const auto &refPic = picture.m_RefPicList1[i];
    if (refPic) {
      auto &frame = refPic->m_picture_frame;
      auto &sliceHeader = frame.m_slice.slice_header;

      sliceType = H264_SLIECE_TYPE_TO_STR(sliceHeader.slice_type);
      std::cout << "\t\t(后参考)m_RefPicList1[" << i << "]: " << sliceType
                << "; PicOrderCnt(参考帧显示顺序)=" << frame.PicOrderCnt
                << "; PicNum(参考帧编号)=" << frame.PicNum
                << "; PicNumCnt(原参考列表中的位置)=" << frame.m_PicNumCnt
                << ";\n";
    }
  }
  std::cout << "\t}" << std::endl;
}
