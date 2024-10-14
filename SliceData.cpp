#include "SliceData.hpp"
#include "Frame.hpp"
#include "GOP.hpp"
#include "PictureBase.hpp"
#include "Type.hpp"
#include <cstdint>
#include <cstdlib>

/* 7.3.4 Slice data syntax */
int SliceData::parseSliceData(BitStream &bitStream, PictureBase &picture,
                              SPS &sps, PPS &pps) {
  /* 初始化类中的指针 */
  pic = &picture;
  header = pic->m_slice->slice_header;
  bs = &bitStream;
  m_sps = &sps;
  m_pps = &pps;
  MbaffFrameFlag = header->MbaffFrameFlag;

  /* 1. 对于Slice的首个熵解码，则需要初始化CABAC模型 */
  initCABAC();

  /* 如果当前帧不使用MBAFF编码模式。所有宏块都作为帧/场宏块进行编码，而不需要进行动态切换帧、场编码 */
  if (MbaffFrameFlag == 0) mb_field_decoding_flag = header->field_pic_flag;

  /* 宏块的初始地址（或宏块索引），在A Frame = A Slice的情况下，初始地址应该为0 */
  pic->CurrMbAddr = CurrMbAddr =
      header->first_mb_in_slice * (1 + MbaffFrameFlag);

  /* 8.2 Slice decoding process */
  slice_decoding_process();

  //----------------------- 开始对Slice分割为MacroBlock进行处理 ----------------------------
  // TODO: 如果是单帧=单Slice属于同一个Slice Gruop的情况，那么这里就是遍历Slice的每个宏块，反之，不清楚，没遇到过这种情况
  bool moreDataFlag = true;
  int32_t prevMbSkipped = 0;
  do {
    /* 1. 对于P,B帧，先解码出mb_skip_flag，判断是否跳过对MacroBlock的处理（P、B帧中存在不需要显式解码类型的宏块） */
    if (header->slice_type != SLICE_I && header->slice_type != SLICE_SI) {
      if (m_pps->entropy_coding_mode_flag == 0) {
        /* CAVLC熵解码：连续跳过的宏块数量，同时在宏块层处理跳过的宏块，这里包括了解码跳过的宏块 */
        process_mb_skip_run(prevMbSkipped);
        if (mb_skip_run > 0) moreDataFlag = bs->more_rbsp_data();
      } else {
        /* CABAC熵解码：单个宏块是否被跳过，同时在宏块层处理跳过的宏块，这里包括了解码跳过的宏块 */
        process_mb_skip_flag(prevMbSkipped);
        moreDataFlag = !mb_skip_flag;
      }
    }

    /* 2. 如果当前宏块未执行跳过处理，则进一步对MacroBlock的处理 */
    if (moreDataFlag) {
      /* 1. 在MBAFF下，且当前宏块为宏块对中的首宏块，则读取是否在宏块级别对帧进行场间隔解码;
         2. 在MBAFF下，且当前宏块为宏块对中的次宏块，且上一个宏块被跳过，则读取是否在宏块级别对帧进行场间隔解码;
      解释一下2，前宏块（首宏块）被标记为“跳过”，即编码器认为该宏块的内容与之前的宏块或参考帧的相应部分相似，因此不对该宏块进行"显式编码"，次宏块仍需要通过引用首宏块的信息来重构内容。*/
      if (MbaffFrameFlag &&
          (CurrMbAddr % 2 == 0 || (CurrMbAddr % 2 == 1 && prevMbSkipped)))
        process_mb_field_decoding_flag(m_pps->entropy_coding_mode_flag);
      //NOTE: 次宏块没有被标记跳过，实际上该次宏块已经在上次迭代中，通过首宏块一并处理过了

      /* 在宏块层解码帧内、帧间所需要的必须信息 */
      do_macroblock_layer();
      /* 帧内预测、帧间预测、解码宏块、去块滤波 */
      decoding_process();
    }

    //更新和管理循环条件
    if (!m_pps->entropy_coding_mode_flag)
      moreDataFlag = bs->more_rbsp_data();
    else {
      /* 对于P帧和B帧，更新prevMbSkipped为当前宏块的跳过标志Mb_skip_flag，以便下一个宏块处理时参考 */
      if (header->slice_type != SLICE_I && header->slice_type != SLICE_SI)
        prevMbSkipped = mb_skip_flag;

      /* MBAFF模式下，若当前是首宏块，当在奇数Slice中，最后一行作为首宏块处理，即使已经到达了Slice末尾，也需要与一个虚拟的底宏块组成宏块对进行处理 */
      if (MbaffFrameFlag && CurrMbAddr % 2 == 0)
        moreDataFlag = true;
      else {
        /* 判断是否到达Slice的末尾 */
        process_end_of_slice_flag(end_of_slice_flag);
        moreDataFlag = !end_of_slice_flag;
      }
    }
    /* 计算下一个宏块的地址 */
    CurrMbAddr = NextMbAddress(CurrMbAddr, header);
  } while (moreDataFlag);
  slice_number++;
  return 0;
}

/* 9.3.1 Initialization process */
int SliceData::initCABAC() {
  /* CABAC编码 */
  cabac = new Cabac(*bs, *pic);

  if (!m_pps->entropy_coding_mode_flag) return -1;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
  /* CABAC(上下文自适应二进制算术编码) */
  while (!bs->byte_aligned())
    uint8_t cabac_alignment_one_bit = bs->readU1();
#pragma GCC diagnostic pop

  // CABAC初始化环境变量
  cabac->init_of_context_variables((H264_SLICE_TYPE)header->slice_type,
                                   header->cabac_init_idc, header->SliceQPY);
  // CABAC初始化解码引擎
  cabac->init_of_decoding_engine();
  return 0;
}

int SliceData::slice_decoding_process() {
  /* 在场编码时可能存在多个slice data，只需要对首个slice data进行定位，同时在下面的操作中，只需要在首次进入Slice时才需要执行的 */
  if (pic->m_slice_cnt == 0) {
    /* 解码参考帧重排序(POC) */
    // 8.2.1 Decoding process for picture order count
    pic->decoding_picture_order_count(m_sps->pic_order_cnt_type);
    if (m_sps->frame_mbs_only_flag == 0) {
      /* 存在场宏块 */
      pic->m_parent->m_picture_top_filed.copyDataPicOrderCnt(*pic);
      //顶（底）场帧有可能被选为参考帧，在解码P/B帧时，会用到PicOrderCnt字段，所以需要在此处复制一份
      pic->m_parent->m_picture_bottom_filed.copyDataPicOrderCnt(*pic);
    }

    /* 8.2.2 Decoding process for macroblock to slice group map */
    decoding_macroblock_to_slice_group_map();

    // 8.2.4 Decoding process for reference picture lists construction
    if (header->slice_type == SLICE_P || header->slice_type == SLICE_SP ||
        header->slice_type == SLICE_B) {
      /* 当前帧需要参考帧预测，则需要进行参考帧重排序。在每个 P、SP 或 B Slice的解码过程开始时调用 */
      pic->decoding_ref_picture_lists_construction(
          pic->m_dpb, pic->m_RefPicList0, pic->m_RefPicList1);

      /* (m_RefPicList0,m_RefPicList1为m_dpb排序后的前后参考列表）打印帧重排序先后信息 */
      printFrameReorderPriorityInfo();
    }
  }
  pic->m_slice_cnt++;
  return 0;
}

// 8.2.2 Decoding process for macroblock to slice group map
/* 输入:活动图像参数集和要解码的Slice header。  
 * 输出:宏块到Slice Group映射MbToSliceGroupMap。 */
//该过程在每个Slice开始时调用（如果是单帧由单个Slice组成的情况，那么这里几乎没有逻辑）
inline int SliceData::decoding_macroblock_to_slice_group_map() {
  //输出为：mapUnitToSliceGroupMap
  mapUnitToSliceGroupMap();
  //输入为：mapUnitToSliceGroupMap
  mbToSliceGroupMap();
  return 0;
}

//8.2.2.1 - 8.2.2.7  Specification for interleaved slice group map type
inline int SliceData::mapUnitToSliceGroupMap() {
  /* 输入 */
  const int &MapUnitsInSliceGroup0 = header->MapUnitsInSliceGroup0;
  /* 输出 */
  int32_t *&mapUnitToSliceGroupMap = header->mapUnitToSliceGroupMap;

  /* mapUnitToSliceGroupMap 数组的推导如下：
   * – 如果 num_slice_groups_minus1 等于 0，则为范围从 0 到 PicSizeInMapUnits − 1（含）的所有 i 生成Slice Group映射的映射单元，如 mapUnitToSliceGroupMap[ i ] = 0 */
  /* 整个图像只被分为一个 slice group */
  if (m_pps->num_slice_groups_minus1 == 0) {
    /* 这里按照一个宏块或宏块对（当为MBAFF时，遍历大小减小一半）处理 */
    for (int i = 0; i < (int)m_sps->PicSizeInMapUnits; i++)
      /* 确保在只有一个Slice组的情况下，整个图像的所有宏块都被正确地映射到这个唯一的Slice组上，简化处理逻辑这里赋值不一定非要为0,只要保持映射单元内都是同一个值就行了 */
      mapUnitToSliceGroupMap[i] = 0;
    /* TODO YangJing 有问题，如果是场编码，那么这里实际上只处理了一半映射 <24-09-16 00:07:20> */
    return 0;
  }

  /* TODO YangJing 这里还没测过，怎么造一个多Slice文件？ <24-09-15 23:25:12> */

  /* — 否则（num_slice_groups_minus1 不等于0），mapUnitToSliceGroupMap 的推导如下： 
       * — 如果slice_group_map_type 等于0，则应用第8.2.2.1 节中指定的mapUnitToSliceGroupMap 的推导。  
       * — 否则，如果slice_group_map_type等于1，则应用第8.2.2.2节中指定的mapUnitToSliceGroupMap的推导。  
       * — 否则，如果slice_group_map_type等于2，则应用第8.2.2.3节中指定的mapUnitToSliceGroupMap的推导。
       * – 否则，如果slice_group_map_type等于3，则应用第8.2.2.4节中指定的mapUnitToSliceGroupMap的推导。  
       * — 否则，如果slice_group_map_type等于4，则应用第8.2.2.5节中指定的mapUnitToSliceGroupMap的推导。  
       * – 否则，如果slice_group_map_type等于5，则应用第8.2.2.6节中指定的mapUnitToSliceGroupMap的推导。  
       * — 否则（slice_group_map_type 等于 6），应用第 8.2.2.7 节中指定的 mapUnitToSliceGroupMap 的推导。*/

  switch (m_pps->slice_group_map_type) {
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

// 8.2.2.8 Specification for conversion of map unit to slice group map to macroblock to slice group map
/* 宏块（Macroblock）的位置映射到Slice（Slice）的过程*/
inline int SliceData::mbToSliceGroupMap() {
  /* 输入：存储每个宏块单元对应的Slice Group索引，在A Frame = A Slice的情况下，这里均为0 */
  const int32_t *mapUnitToSliceGroupMap = header->mapUnitToSliceGroupMap;

  /* 输出：存储映射后每个宏块对应的Slice Group索引 */
  int32_t *&MbToSliceGroupMap = header->MbToSliceGroupMap;

  /* 对于Slice中的每个宏块（若是场编码，顶、底宏块也需要单独遍历），宏块到Slice Group映射指定如下： */
  for (int mbIndex = 0; mbIndex < header->PicSizeInMbs; mbIndex++) {
    if (m_sps->frame_mbs_only_flag || header->field_pic_flag)
      /* 对于一个全帧或全场（即不存在混合帧、场），每个宏块独立对应一个映射单位 */
      MbToSliceGroupMap[mbIndex] = mapUnitToSliceGroupMap[mbIndex];
    else if (MbaffFrameFlag)
      /* 映射基于宏块对，一个宏块对（2个宏块）共享一个映射单位 */
      MbToSliceGroupMap[mbIndex] = mapUnitToSliceGroupMap[mbIndex / 2];
    else {
      /* 场编码的交错模式，每个宏块对跨越两行，每一对宏块（通常包括顶部场和底部场的宏块）被当作一个单元处理 */
      /*   +-----------+        +-----------+
           |           |        | top filed |
           |           |        | btm filed |
           | A  Slice  |   -->  | top filed | 
           |           |        | btm filed |
           |           |        | top filed |
           |           |        | btm filed |
           +-----------+        +-----------+*/
      /* 用宏块索引对图像宽度取模，得到该宏块在其行中的列位置 */
      uint32_t x = mbIndex % m_sps->PicWidthInMbs;
      /* 因为每个宏块对占据两行（每行一个宏块），所以用宽度的两倍来计算行号，得到顶宏块的行数 */
      uint32_t y = mbIndex / (2 * m_sps->PicWidthInMbs);
      MbToSliceGroupMap[mbIndex] =
          mapUnitToSliceGroupMap[y * m_sps->PicWidthInMbs + x];
    }
  }

  return 0;
}

/* mb_skip_run指定连续跳过的宏块的数量，在解码P或SPSlice时，MB_type应被推断为P_Skip并且宏块类型统称为P宏块类型，或者在解码BSlice时， MB_type应被推断为B_Skip，并且宏块类型统称为B宏块类型 */
int SliceData::process_mb_skip_run(int32_t &prevMbSkipped) {
  mb_skip_run = bs->readUE();

  /* 当mb_skip_run > 0时，实际上在接下来的宏块解析中，将要跳过处理，所以能够表示prevMbSkipped = 1*/
  prevMbSkipped = mb_skip_run > 0;

  /* 对于每个跳过的宏块，不同于CABAC的处理，这里是处理多个宏块，而CABAC是处理单个宏块 */
  for (uint32_t i = 0; i < mb_skip_run; i++) {
    /* 1. 计算当前宏块的位置 */
    updatesLocationOfCurrentMacroblock(MbaffFrameFlag);

    /* 2. 对于跳过的宏块同样需要增加实际的宏块计数 */
    pic->mb_cnt++;

    /* 3. 对于MBAFF模式下，顶宏块需要先确定当前是否为帧宏块还是场宏块，以确定下面的帧间预测中如何处理宏块对 */
    if (MbaffFrameFlag && CurrMbAddr % 2 == 0)
      //这里是推导（根据前面已解码的宏块进行推导当前值），而不是解码
      derivation_for_mb_field_decoding_flag();

    /* 4. 宏块层也需要将该宏块该宏块的类型设置“Skip类型” */
    pic->m_mbs[pic->CurrMbAddr].decode_skip(*pic, *this, *cabac);

    /* 5. 由于跳过的宏块只能是P,B Slice中的宏块，那么就只需要调用帧间预测 */
    pic->inter_prediction_process();

    /* 6. 外层循环也需要更新宏块跳过地址 */
    CurrMbAddr = NextMbAddress(CurrMbAddr, header);
  }
  return 0;
}

/* 更新当前宏快位置 */
/* 输入：PicWidthInMbs,MbaffFrameFlag
 * 输出：mb_x, mb_y, CurrMbAddr*/
void SliceData::updatesLocationOfCurrentMacroblock(const bool MbaffFrameFlag) {
  const uint32_t h = MbaffFrameFlag ? 2 : 1;
  const uint32_t w = pic->PicWidthInMbs * h; //当为宏块对模式时，实际上$W = 2*W$
  pic->mb_x = (CurrMbAddr % w) / h;
  pic->mb_y = (CurrMbAddr / w * h) + ((CurrMbAddr % w) % h);
  pic->CurrMbAddr = CurrMbAddr;
}

/* 如果当前宏块的运动矢量与参考帧中的预测块非常接近，且残差（即当前块与预测块的差异）非常小或为零，编码器可能会选择跳过该宏块的编码。
 * 在这种情况下，解码器可以通过运动矢量预测和参考帧直接重建宏块，而无需传输额外的残差信息。
 * 一般来说，在I帧中，不会出现宏块跳过处理。这是因为I帧中的宏块是使用帧内预测进行编码的，而不是基于参考帧的帧间预测 */
int SliceData::process_mb_skip_flag(const int32_t prevMbSkipped) {
  /* 1. 计算当前宏块的位置 */
  updatesLocationOfCurrentMacroblock(MbaffFrameFlag);

  MacroBlock &curr_mb = pic->m_mbs[CurrMbAddr];
  MacroBlock &next_mb = pic->m_mbs[CurrMbAddr + 1];

  /* 2. 设置当前宏块的Slice编号 */
  curr_mb.slice_number = slice_number;

  /* 3. 当前帧是MBAFF帧，且顶宏块和底宏块的场解码标志都未设置，则推导出mb_field_decoding_flag的值 */
  if (MbaffFrameFlag) {
    /* 当 mb_field_decoding_flag 未设置，需要推导其初始值，根据相邻“宏块对”来推导当前宏块类型，若为首个宏块(0,0)则设置为帧宏块。（不能理解为"当宏块对作为帧宏块处理时"）*/
    if (CurrMbAddr % 2 == 0 &&
        (!curr_mb.mb_field_decoding_flag && !next_mb.mb_field_decoding_flag))
      derivation_for_mb_field_decoding_flag();

    curr_mb.mb_field_decoding_flag = mb_field_decoding_flag;
  }

  /* 4. 在当前宏块对中，若首宏块跳过处理，则次宏块跳过标记设置为下个宏块对的首宏块一致。NOTE:这里是CABAC的一个特殊规则，为了减少比特流中需要传输的标志位数量 */
  if (MbaffFrameFlag && CurrMbAddr % 2 == 1 && prevMbSkipped)
    mb_skip_flag = mb_skip_flag_next_mb;

  /* 若非MBAFF模式，则直接解码获取是否需要跳过该宏块*/
  else
    cabac->decode_mb_skip_flag(CurrMbAddr, mb_skip_flag);

  /* 5. 宏块需要进行跳过处理时：由于宏块跳过时，一般来说没有残差数据，则需要运动矢量预测和参考帧直接重建宏块（帧间预测,P,B Slice） */
  if (mb_skip_flag && header->slice_type != SLICE_I &&
      header->slice_type != SLICE_SI) {
    pic->mb_cnt++;

    /* 若为MBAFF模式，则宏块对需要一并处理 */
    if (MbaffFrameFlag && CurrMbAddr % 2 == 0) {
      curr_mb.mb_skip_flag = mb_skip_flag;

      // 次宏块与首宏块的宏块对是相同的（要么都是帧宏块，要么都是场宏块），进行宏块对的同步
      next_mb.slice_number = slice_number;
      next_mb.mb_field_decoding_flag = mb_field_decoding_flag;

      /* 解码次宏块是否需要跳过宏块处理（由于宏块对是一起操作的，所以这里允许先解码下一个宏块）*/
      cabac->decode_mb_skip_flag(CurrMbAddr + 1, mb_skip_flag_next_mb);

      /* 当首宏块跳过处理，但次宏块需要解码 */
      if (mb_skip_flag_next_mb == 0) {
        cabac->decode_mb_field_decoding_flag(mb_field_decoding_flag);
        is_mb_field_decoding_flag_prcessed = true;
      }
      /* 当首、次宏块均跳过处理时，需根据标准规定，要再次推导当前首宏块的宏块对解码类型，确保宏块对类型正确（与上面一次调用不同的是，CABAC解码需要根据最新的上下文重新推导
       * NOTE: 这里有一个情况是当一个Slice被分为奇数行宏块时，那么在MBAFF模式下，宏块对的划分后，最后一行宏块会被当作首宏块处理，且需要与一个虚拟的底宏块组成宏块对进行处理，虚拟的底宏块会被视为跳过的宏块，不包含任何数据。 */
      else
        derivation_for_mb_field_decoding_flag();
    }

    /* 宏块层对应的处理 */
    curr_mb.decode_skip(*pic, *this, *cabac);

    /* 由于跳过的宏块只能是P,B Slice中的宏块，那么就只需要调用帧间预测 */
    pic->inter_prediction_process(); // 帧间预测
  }

  return 0;
}

//解码mb_field_decoding_flag: 表示本宏块对是帧宏块对，还是场宏块对
int SliceData::process_mb_field_decoding_flag(bool entropy_coding_mode_flag) {
  int ret = 0;
  if (is_mb_field_decoding_flag_prcessed) {
    is_mb_field_decoding_flag_prcessed = false;
    return 1;
  }

  if (entropy_coding_mode_flag)
    ret = cabac->decode_mb_field_decoding_flag(mb_field_decoding_flag);
  else
    mb_field_decoding_flag = bs->readU1();

  RET(ret);
  return 0;
}

int SliceData::process_end_of_slice_flag(int32_t &end_of_slice_flag) {
  cabac->decode_end_of_slice_flag(end_of_slice_flag);
  return 0;
}

/* mb_field_decoding_flag. ->  Rec. ITU-T H.264 (08/2021) 98*/
//该函数只适用于MBAFF模式下调用，根据相邻“宏块对”的状态来推断当前“宏块对”的 mb_field_decoding_flag 值
int SliceData::derivation_for_mb_field_decoding_flag() {
  /* NOTE:在同一Slice内，编码模式只能是一种，若存在左邻宏块或上邻宏块则编码模式必然一致（只能通过左、上方向推导、这是扫描顺序决定的） */
  /* 同一片中当前宏块对的左侧或上方不存在相邻宏块对，即默认为帧宏块 */
  mb_field_decoding_flag = 0;
  if (pic->mb_x > 0) {
    /* 存在左侧的相邻宏块对，且属于同一Slice group，则直接copy */
    auto &left_mb = pic->m_mbs[CurrMbAddr - 2];
    if (left_mb.slice_number == slice_number)
      mb_field_decoding_flag = left_mb.mb_field_decoding_flag;
  } else if (pic->mb_y > 0) {
    /* 存在上侧的相邻宏块对，且属于同一Slice group，则直接copy */
    auto &top_mb = pic->m_mbs[CurrMbAddr - 2 * pic->PicWidthInMbs];
    if (top_mb.slice_number == slice_number)
      mb_field_decoding_flag = top_mb.mb_field_decoding_flag;
  }
  return 0;
}

int SliceData::do_macroblock_layer() {
  /* 1. 计算当前宏块的位置 */
  updatesLocationOfCurrentMacroblock(MbaffFrameFlag);
  /* 2. 在宏块层中对每个宏块处理或解码得到对应帧内、帧间解码所需要的信息*/
  pic->m_mbs[pic->CurrMbAddr].decode(*bs, *pic, *this, *cabac);
  pic->mb_cnt++;
  return 0;
}

int SliceData::decoding_process() {
  /* ------------------ 设置别名 ------------------ */
  const int32_t &picWidthInSamplesL = pic->PicWidthInSamplesL;
  const int32_t &picWidthInSamplesC = pic->PicWidthInSamplesC;
  const int32_t &BitDepth = pic->m_slice->slice_header->m_sps->BitDepthY;

  uint8_t *&pic_buff_luma = pic->m_pic_buff_luma;
  uint8_t *&pic_buff_cb = pic->m_pic_buff_cb;
  uint8_t *&pic_buff_cr = pic->m_pic_buff_cr;

  MacroBlock &mb = pic->m_mbs[pic->CurrMbAddr];
  /* ------------------  End ------------------ */

  //8.5 Transform coefficient decoding process and picture construction process prior to deblocking filter process（根据不同类型的预测模式，进行去块滤波处理之前的变换系数解码处理和图片构造处理 ）
  bool isNeedIntraPrediction = true;
  //----------------------------------- 帧内预测 -----------------------------------
  if (mb.m_mb_pred_mode == Intra_4x4) //分区预测，处理最为复杂的高纹理区域
    pic->transform_decoding_for_4x4_luma_residual_blocks(
        0, 0, BitDepth, picWidthInSamplesL, pic_buff_luma);
  else if (mb.m_mb_pred_mode == Intra_8x8) //分区预测
    pic->transform_decoding_for_8x8_luma_residual_blocks(
        0, 0, BitDepth, picWidthInSamplesL, mb.LumaLevel8x8, pic_buff_luma);
  else if (mb.m_mb_pred_mode == Intra_16x16) //整块预测，处理较为简单的区域
    pic->transform_decoding_for_luma_samples_of_16x16(
        0, BitDepth, mb.QP1Y, picWidthInSamplesL, mb.Intra16x16DCLevel,
        mb.Intra16x16ACLevel, pic_buff_luma);
  //----------------------------------- 原始数据 -----------------------------------
  else if (mb.m_name_of_mb_type == I_PCM) {
    pic->sample_construction_for_I_PCM();
    goto eof;
  }
  //----------------------------------- 帧间预测 -----------------------------------
  else {
    // 对于帧间预测而言，过程中不需要调用帧内预测
    isNeedIntraPrediction = false;
    pic->inter_prediction_process();
    /* 选择 4x4 或 8x8 的残差块解码函数来处理亮度残差块 */
    if (mb.transform_size_8x8_flag)
      pic->transform_decoding_for_8x8_luma_residual_blocks(
          0, 0, BitDepth, picWidthInSamplesL, mb.LumaLevel8x8, pic_buff_luma,
          false);
    else
      pic->transform_decoding_for_4x4_luma_residual_blocks(
          0, 0, BitDepth, picWidthInSamplesL, pic_buff_luma, false);
  }

  /* 帧内、帧间预测都调用，当存在色度采样时，即YUV420,YUV422,YUV444进行色度解码; 反之，则不进行色度解码 */
  if (m_sps->ChromaArrayType) {
    pic->transform_decoding_for_chroma_samples(
        1, picWidthInSamplesC, pic_buff_cb, isNeedIntraPrediction);
    pic->transform_decoding_for_chroma_samples(
        0, picWidthInSamplesC, pic_buff_cr, isNeedIntraPrediction);
  }

  /* 至此该宏块的原始数据完成全部解码工作，输出的pic_buff_luma,pic_buff_cb,pic_buff_cr即为解码的原始数据 */
eof:
  return 0;
}

// 第 8.2.2.8 节的规定导出宏块到Slice Group映射后，该函数导出NextMbAddress的值
/* 跳过不属于当前Slice Group的宏块，找到与当前宏块位于同一Slice Group中的下一个宏块 */
int NextMbAddress(int currMbAddr, SliceHeader *header) {
  int nextMbAddr = currMbAddr + 1;

  /* 宏块索引不应该为负数或大于该Slice的总宏块数 + 1 */
  while (nextMbAddr < header->PicSizeInMbs &&
         header->MbToSliceGroupMap[nextMbAddr] !=
             header->MbToSliceGroupMap[currMbAddr]) {
    /* 下一个宏块是否与当前宏块位于同一个Slice Group中。如果不在同一个Slice Group中，则继续增加 nextMbAddr 直到找到属于同一个Slice Group的宏块。*/
    nextMbAddr++;
  }

  if (nextMbAddr < 0)
    cerr << "An error occurred CurrMbAddr:" << nextMbAddr << " on "
         << __FUNCTION__ << "():" << __LINE__ << endl;
  return nextMbAddr;
}

void SliceData::printFrameReorderPriorityInfo() {
  string sliceType = "UNKNOWN";
  cout << "\tGOP[" << pic->m_PicNumCnt + 1 << "] -> {" << endl;
  for (int i = 0; i < GOP_SIZE; ++i) {
    const auto &refPic = pic->m_dpb[i];
    if (refPic) {
      auto &frame = refPic->m_picture_frame;
      auto &sliceHeader = frame.m_slice->slice_header;
      if (frame.m_slice == nullptr) continue;
      if (sliceHeader->slice_type != SLICE_I && frame.PicOrderCnt == 0 &&
          frame.PicNum == 0 && frame.m_PicNumCnt == 0)
        continue;
      sliceType = H264_SLIECE_TYPE_TO_STR(sliceHeader->slice_type);
      if (pic->PicOrderCnt == frame.PicOrderCnt)
        cout << "\t\t* DPB[" << i << "]: ";
      else
        cout << "\t\t  DPB[" << i << "]: ";
      cout << sliceType << "; POC(显示顺序)=" << frame.PicOrderCnt
           << "; frame_num(帧编号，编码顺序)="
           << frame.m_slice->slice_header->frame_num << ";\n";
    }
  }
  cout << "\t}" << endl;

  if (header->slice_type == SLICE_P || header->slice_type == SLICE_SP)
    cout << "\t当前帧所参考帧列表(按frame_num排序) -> {" << endl;
  else if (header->slice_type == SLICE_B)
    cout << "\t当前帧所参考帧列表(按POC排序) -> {" << endl;

  for (uint32_t i = 0; i < pic->m_RefPicList0Length; ++i) {
    const auto &refPic = pic->m_RefPicList0[i];
    if (refPic &&
        (refPic->reference_marked_type == PICTURE_MARKED_AS_used_short_ref ||
         refPic->reference_marked_type == PICTURE_MARKED_AS_used_long_ref)) {
      auto &frame = refPic->m_picture_frame;
      auto &sliceHeader = frame.m_slice->slice_header;

      sliceType = H264_SLIECE_TYPE_TO_STR(sliceHeader->slice_type);
      cout << "\t\t(前参考)RefPicList0[" << i << "]: " << sliceType
           << "; POC(显示顺序)=" << frame.PicOrderCnt
           << "; frame_num(帧编号，编码顺序)=" << frame.PicNum << ";\n";
    }
  }

  for (uint32_t i = 0; i < pic->m_RefPicList1Length; ++i) {
    const auto &refPic = pic->m_RefPicList1[i];
    if (refPic &&
        (refPic->reference_marked_type == PICTURE_MARKED_AS_used_short_ref ||
         refPic->reference_marked_type == PICTURE_MARKED_AS_used_long_ref)) {
      auto &frame = refPic->m_picture_frame;
      auto &sliceHeader = frame.m_slice->slice_header;

      sliceType = H264_SLIECE_TYPE_TO_STR(sliceHeader->slice_type);
      cout << "\t\t(后参考)RefPicList1[" << i << "]: " << sliceType
           << "; POC(显示顺序)=" << frame.PicOrderCnt
           << "; frame_num(帧编号，编码顺序)=" << frame.PicNum << ";\n";
    }
  }
  cout << "\t}" << endl;
}

//8.2.2.1 Specification for interleaved slice group map type
int SliceData::interleaved_slice_group_map_type(
    int32_t *&mapUnitToSliceGroupMap) {
  uint32_t i = 0;
  do {
    for (uint32_t iGroup = 0; iGroup <= m_pps->num_slice_groups_minus1 &&
                              i < m_sps->PicSizeInMapUnits;
         i += m_pps->run_length_minus1[iGroup++] + 1) {
      for (uint32_t j = 0; j <= m_pps->run_length_minus1[iGroup] &&
                           i + j < m_sps->PicSizeInMapUnits;
           j++) {
        mapUnitToSliceGroupMap[i + j] = iGroup;
      }
    }
  } while (i < m_sps->PicSizeInMapUnits);
  return 0;
}
//8.2.2.2 Specification for dispersed slice group map type
int SliceData::dispersed_slice_group_map_type(
    int32_t *&mapUnitToSliceGroupMap) {
  for (int i = 0; i < (int)m_sps->PicSizeInMapUnits; i++) {
    mapUnitToSliceGroupMap[i] =
        ((i % m_sps->PicWidthInMbs) +
         (((i / m_sps->PicWidthInMbs) * (m_pps->num_slice_groups_minus1 + 1)) /
          2)) %
        (m_pps->num_slice_groups_minus1 + 1);
  }
  return 0;
}
//8.2.2.3 Specification for foreground with left-over slice group map type
int SliceData::foreground_with_left_over_slice_group_ma_type(
    int32_t *&mapUnitToSliceGroupMap) {
  for (int i = 0; i < (int)m_sps->PicSizeInMapUnits; i++)
    mapUnitToSliceGroupMap[i] = m_pps->num_slice_groups_minus1;

  for (int iGroup = m_pps->num_slice_groups_minus1 - 1; iGroup >= 0; iGroup--) {
    int32_t yTopLeft = m_pps->top_left[iGroup] / m_sps->PicWidthInMbs;
    int32_t xTopLeft = m_pps->top_left[iGroup] % m_sps->PicWidthInMbs;
    int32_t yBottomRight = m_pps->bottom_right[iGroup] / m_sps->PicWidthInMbs;
    int32_t xBottomRight = m_pps->bottom_right[iGroup] % m_sps->PicWidthInMbs;
    for (int y = yTopLeft; y <= yBottomRight; y++) {
      for (int x = xTopLeft; x <= xBottomRight; x++) {
        mapUnitToSliceGroupMap[y * m_sps->PicWidthInMbs + x] = iGroup;
      }
    }
  }
  return 0;
}

//8.2.2.4 Specification for box-out slice group map types
int SliceData::box_out_slice_group_map_types(int32_t *&mapUnitToSliceGroupMap,
                                             const int &MapUnitsInSliceGroup0) {

  for (int i = 0; i < (int)m_sps->PicSizeInMapUnits; i++)
    mapUnitToSliceGroupMap[i] = 1;

  int x = (m_sps->PicWidthInMbs - m_pps->slice_group_change_direction_flag) / 2;
  int y =
      (m_sps->PicHeightInMapUnits - m_pps->slice_group_change_direction_flag) /
      2;

  int32_t leftBound = x;
  int32_t topBound = y;
  int32_t rightBound = x;
  int32_t bottomBound = y;
  int32_t xDir = m_pps->slice_group_change_direction_flag - 1;
  int32_t yDir = m_pps->slice_group_change_direction_flag;
  int32_t mapUnitVacant = 0;

  for (int k = 0; k < MapUnitsInSliceGroup0; k += mapUnitVacant) {
    mapUnitVacant = (mapUnitToSliceGroupMap[y * m_sps->PicWidthInMbs + x] == 1);
    if (mapUnitVacant) {
      mapUnitToSliceGroupMap[y * m_sps->PicWidthInMbs + x] = 0;
    }
    if (xDir == -1 && x == leftBound) {
      leftBound = fmax(leftBound - 1, 0);
      x = leftBound;
      xDir = 0;
      yDir = 2 * m_pps->slice_group_change_direction_flag - 1;
    } else if (xDir == 1 && x == rightBound) {
      rightBound = MIN(rightBound + 1, m_sps->PicWidthInMbs - 1);
      x = rightBound;
      xDir = 0;
      yDir = 1 - 2 * m_pps->slice_group_change_direction_flag;
    } else if (yDir == -1 && y == topBound) {
      topBound = MAX(topBound - 1, 0);
      y = topBound;
      xDir = 1 - 2 * m_pps->slice_group_change_direction_flag;
      yDir = 0;
    } else if (yDir == 1 && y == bottomBound) {
      bottomBound = MIN(bottomBound + 1, m_sps->PicHeightInMapUnits - 1);
      y = bottomBound;
      xDir = 2 * m_pps->slice_group_change_direction_flag - 1;
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
  if (m_pps->num_slice_groups_minus1 == 1) {
    sizeOfUpperLeftGroup =
        (m_pps->slice_group_change_direction_flag
             ? (m_sps->PicSizeInMapUnits - MapUnitsInSliceGroup0)
             : MapUnitsInSliceGroup0);
  }

  for (int i = 0; i < (int)m_sps->PicSizeInMapUnits; i++) {
    if (i < sizeOfUpperLeftGroup)
      mapUnitToSliceGroupMap[i] = m_pps->slice_group_change_direction_flag;
    else
      mapUnitToSliceGroupMap[i] = 1 - m_pps->slice_group_change_direction_flag;
  }
  return 0;
}
//8.2.2.6 Specification for wipe slice group map types
int SliceData::wipe_slice_group_map_types(int32_t *&mapUnitToSliceGroupMap,
                                          const int &MapUnitsInSliceGroup0) {
  int32_t sizeOfUpperLeftGroup = 0;
  if (m_pps->num_slice_groups_minus1 == 1) {
    sizeOfUpperLeftGroup =
        (m_pps->slice_group_change_direction_flag
             ? (m_sps->PicSizeInMapUnits - MapUnitsInSliceGroup0)
             : MapUnitsInSliceGroup0);
  }

  int k = 0;
  for (int j = 0; j < (int)m_sps->PicWidthInMbs; j++) {
    for (int i = 0; i < (int)m_sps->PicHeightInMapUnits; i++) {
      if (k++ < sizeOfUpperLeftGroup) {
        mapUnitToSliceGroupMap[i * m_sps->PicWidthInMbs + j] =
            m_pps->slice_group_change_direction_flag;
      } else {
        mapUnitToSliceGroupMap[i * m_sps->PicWidthInMbs + j] =
            1 - m_pps->slice_group_change_direction_flag;
      }
    }
  }
  return 0;
}

//8.2.2.7 Specification for explicit slice group map type
int SliceData::explicit_slice_group_map_type(int32_t *&mapUnitToSliceGroupMap) {
  for (int i = 0; i < (int)m_sps->PicSizeInMapUnits; i++)
    mapUnitToSliceGroupMap[i] = m_pps->slice_group_id[i];
  return 0;
}
