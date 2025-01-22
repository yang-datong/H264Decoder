#include "PictureBase.hpp"
#include "Constants.hpp"
#include "Frame.hpp"
#include "MacroBlock.hpp"
#include "SliceHeader.hpp"
#include "Type.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>

extern int32_t g_PicNumCnt;

PictureBase::~PictureBase() { unInit(); }

int PictureBase::reset() {
  if (m_mbs) {
    for (int i = 0; i < PicSizeInMbs; ++i)
      m_mbs[i] = MacroBlock();
  }

  if (m_picture_coded_type == FRAME) {
    if (m_pic_buff_luma)
      memset(m_pic_buff_luma, 0, PicWidthInSamplesL * PicHeightInSamplesL);
    if (m_pic_buff_cb)
      memset(m_pic_buff_cb, 0, PicWidthInSamplesC * PicHeightInSamplesC);
    if (m_pic_buff_cr)
      memset(m_pic_buff_cr, 0, PicWidthInSamplesC * PicHeightInSamplesC);
  }

  CurrMbAddr = 0;
  Chroma_Format = 0;
  mb_x = 0, mb_y = 0;
  m_slice_cnt = 0, mb_cnt = 0;
  TopFieldOrderCnt = 0, BottomFieldOrderCnt = 0;
  m_pic_coded_width_pixels = 0, m_pic_coded_height_pixels = 0;
  MbWidthL = 0, MbHeightL = 0, MbWidthC = 0, MbHeightC = 0;
  PicWidthInMbs = 0, PicHeightInMbs = 0, PicSizeInMbs = 0;
  PicOrderCnt = 0, PicOrderCntMsb = 0, PicOrderCntLsb = 0;
  FrameNumOffset = 0, absFrameNum = 0, frameNumInPicOrderCntCycle = 0;
  picOrderCntCycleCnt = 0, expectedPicOrderCnt = 0;
  FrameNum = 0, FrameNumWrap = 0;
  PicNum = 0, LongTermPicNum = 0, LongTermFrameIdx = 0;
  FieldNum = NA, MaxLongTermFrameIdx = NA;
  mmco_5_flag = 0, mmco_6_flag = 0;
  reference_marked_type = UNKOWN;
  m_picture_coded_type = UNKNOWN, m_picture_type = SLICE_UNKNOWN;
  m_RefPicList0Length = 0, m_RefPicList1Length = 0;
  m_PicNumCnt = 0;
  m_parent = nullptr;

  memset(m_dpb, 0, sizeof(Frame *) * MAX_DPB);
  memset(m_RefPicList0, 0, sizeof(Frame *) * MAX_REF_PIC_LIST_COUNT);
  memset(m_RefPicList1, 0, sizeof(Frame *) * MAX_REF_PIC_LIST_COUNT);
  return 0;
}

int PictureBase::init(Slice *slice) {
  this->m_slice = slice;
  const SliceHeader *header = m_slice->slice_header;
  const SPS *sps = header->m_sps;

  Chroma_Format = sps->Chroma_Format;

  MbWidthL = MB_WIDTH, MbHeightL = MB_HEIGHT;
  MbWidthC = sps->MbWidthC, MbHeightC = sps->MbHeightC;

  PicWidthInMbs = header->PicWidthInMbs,
  PicHeightInMbs = header->PicHeightInMbs;
  PicSizeInMbs = PicWidthInMbs * PicHeightInMbs;

  PicWidthInSamplesL = PicWidthInMbs * 16,
  PicHeightInSamplesL = PicHeightInMbs * 16;

  PicWidthInSamplesC = PicWidthInMbs * MbWidthC,
  PicHeightInSamplesC = PicHeightInMbs * MbHeightC;

  m_pic_coded_width_pixels = PicWidthInMbs * MbWidthL,
  m_pic_coded_height_pixels = PicHeightInMbs * MbHeightL;

  if (m_is_malloc_mem_by_myself == 1) return 0;

  if (m_picture_coded_type == FRAME) {
    m_mbs = new MacroBlock[PicSizeInMbs];
    for (int i = 0; i < PicSizeInMbs; ++i)
      m_mbs[i] = MacroBlock();

    //-----------YUV420P-----------------
    int sizeY = PicWidthInSamplesL * PicHeightInSamplesL;
    int sizeU = PicWidthInSamplesC * PicHeightInSamplesC;
    int sizeV = PicWidthInSamplesC * PicHeightInSamplesC;

    uint8_t *pic_buff = new uint8_t[sizeY + sizeU + sizeV]{0};
    m_pic_buff_luma = pic_buff;
    m_pic_buff_cb = m_pic_buff_luma + sizeY;
    m_pic_buff_cr = m_pic_buff_cb + sizeU;
    m_is_malloc_mem_by_myself = 1;
  } else {
    // 因为top_filed顶场帧和bottom底场帧，都是共享frame帧的大部分数据信息，所以frame帧必须先初始化过了才行
    RET(this->m_parent->m_picture_frame.m_is_malloc_mem_by_myself != 1);
    H264_PICTURE_CODED_TYPE picture_coded_type = m_picture_coded_type;

    int32_t copyMbsDataFlag = 0;
    copyData2(this->m_parent->m_picture_frame, copyMbsDataFlag);

    m_picture_coded_type = picture_coded_type;

    //----------重新计算filed帧的高度--------------------
    Chroma_Format = sps->Chroma_Format;
    MbWidthL = MB_WIDTH, MbHeightL = MB_HEIGHT;
    MbWidthC = sps->MbWidthC, MbHeightC = sps->MbHeightC;

    PicWidthInMbs = sps->PicWidthInMbs;
    PicHeightInMbs = header->PicHeightInMbs / 2;
    PicSizeInMbs = PicWidthInMbs * PicHeightInMbs;

    // filed场帧像素的宽度是frame帧宽度的2倍（即两个相邻奇数行或两个相邻偶数行的间距）
    PicWidthInSamplesL = PicWidthInMbs * 16 * 2;
    PicWidthInSamplesC = PicWidthInMbs * MbWidthC * 2;

    PicHeightInSamplesL = PicHeightInMbs * 16;
    PicHeightInSamplesC = PicHeightInMbs * MbHeightC;

    m_pic_coded_width_pixels = PicWidthInMbs * MbWidthL;
    m_pic_coded_height_pixels = PicHeightInMbs * MbHeightL;

    // 底场帧被定义为图片的偶数行，像素地址从第二行开始计算
    if (m_picture_coded_type == BOTTOM_FIELD) {
      m_pic_buff_luma += PicWidthInMbs * 16;
      m_pic_buff_cb += PicWidthInMbs * MbWidthC;
      m_pic_buff_cr += PicWidthInMbs * MbWidthC;
    }
    m_is_malloc_mem_by_myself = 0;
  }

  return 0;
}

int PictureBase::unInit() {
  if (m_is_malloc_mem_by_myself == 1) {
    DELETE(m_mbs);
    DELETE(m_pic_buff_luma);
  } else {
    m_mbs = NULL;
    m_pic_buff_luma = NULL, m_pic_buff_cb = NULL, m_pic_buff_cr = NULL;
  }
  m_is_malloc_mem_by_myself = 0;
  return 0;
}

PictureBase &PictureBase::operator=(const PictureBase &src) {
  bool isMallocAndCopyData = false;
  copyData(src, isMallocAndCopyData);
  // 重载的等号运算符，默认不拷贝YUV数据，主要是为了RefPicListX[]排序时，只操作YUV数据的内存指针
  return *this;
}

int PictureBase::copyData(const PictureBase &src, bool isMallocAndCopyData) {
  RET(unInit());
  memcpy(this, &src, sizeof(PictureBase));
  m_is_malloc_mem_by_myself = 0;
  if (isMallocAndCopyData) {
    RET(init(src.m_slice));
    copy(src.m_mbs, src.m_mbs + PicSizeInMbs, m_mbs);
    memcpy(m_pic_buff_luma, src.m_pic_buff_luma,
           PicWidthInSamplesL * PicHeightInSamplesL);
    memcpy(m_pic_buff_cb, src.m_pic_buff_cb,
           PicWidthInSamplesC * PicHeightInSamplesC);
    memcpy(m_pic_buff_cr, src.m_pic_buff_cr,
           PicWidthInSamplesC * PicHeightInSamplesC);
  } else {
    SliceHeader *slice_header = src.m_slice->slice_header;
    MbWidthL = MB_WIDTH, MbHeightL = MB_HEIGHT;
    MbWidthC = m_slice->slice_header->m_sps->MbWidthC;
    MbHeightC = m_slice->slice_header->m_sps->MbHeightC;
    Chroma_Format = m_slice->slice_header->m_sps->Chroma_Format;

    PicWidthInMbs = m_slice->slice_header->m_sps->PicWidthInMbs;
    PicHeightInMbs = slice_header->PicHeightInMbs;
    PicSizeInMbs = PicWidthInMbs * PicHeightInMbs;

    PicWidthInSamplesL = PicWidthInMbs * 16;
    PicWidthInSamplesC = PicWidthInMbs * MbWidthC;

    PicHeightInSamplesL = PicHeightInMbs * 16;
    PicHeightInSamplesC = PicHeightInMbs * MbHeightC;

    m_pic_coded_width_pixels = PicWidthInMbs * MbWidthL;
    m_pic_coded_height_pixels = PicHeightInMbs * MbHeightL;
  }

  return 0;
}

int PictureBase::copyData2(const PictureBase &src, int32_t copyMbsDataFlag) {
  mb_x = src.mb_x, mb_y = src.mb_y;

  m_pic_coded_width_pixels = src.m_pic_coded_width_pixels,
  m_pic_coded_height_pixels = src.m_pic_coded_height_pixels;

  PicWidthInMbs = src.PicWidthInMbs, PicHeightInMbs = src.PicHeightInMbs,
  PicSizeInMbs = src.PicSizeInMbs;

  MbWidthL = src.MbWidthL, MbHeightL = src.MbHeightL;
  MbWidthC = src.MbWidthC, MbHeightC = src.MbHeightC;

  PicWidthInSamplesL = src.PicWidthInSamplesL,
  PicWidthInSamplesC = src.PicWidthInSamplesC;
  PicHeightInSamplesL = src.PicHeightInSamplesL,
  PicHeightInSamplesC = src.PicHeightInSamplesC;

  Chroma_Format = src.Chroma_Format;

  mb_cnt = src.mb_cnt, CurrMbAddr = src.CurrMbAddr;

  m_pic_buff_luma = src.m_pic_buff_luma;
  m_pic_buff_cb = src.m_pic_buff_cb, m_pic_buff_cr = src.m_pic_buff_cr;

  TopFieldOrderCnt = src.TopFieldOrderCnt,
  BottomFieldOrderCnt = src.BottomFieldOrderCnt;

  PicOrderCntMsb = src.PicOrderCntMsb, PicOrderCntLsb = src.PicOrderCntLsb;

  FrameNumOffset = src.FrameNumOffset, absFrameNum = src.absFrameNum;

  picOrderCntCycleCnt = src.picOrderCntCycleCnt;
  frameNumInPicOrderCntCycle = src.frameNumInPicOrderCntCycle;
  expectedPicOrderCnt = src.expectedPicOrderCnt;

  PicOrderCnt = src.PicOrderCnt;
  FrameNum = src.FrameNum, FrameNumWrap = src.FrameNumWrap,
  LongTermFrameIdx = src.LongTermFrameIdx;

  PicNum = src.PicNum;
  LongTermPicNum = src.LongTermPicNum;
  FieldNum = src.FieldNum;
  MaxLongTermFrameIdx = src.MaxLongTermFrameIdx;

  mmco_5_flag = src.mmco_5_flag, mmco_6_flag = src.mmco_6_flag;

  reference_marked_type = src.reference_marked_type;

  if (copyMbsDataFlag == 0) {
    m_mbs = src.m_mbs;
    m_is_malloc_mem_by_myself = 0;
  } else if (copyMbsDataFlag == 1) {
    copy(src.m_mbs, src.m_mbs + PicSizeInMbs, m_mbs);
    m_is_malloc_mem_by_myself = 1;
  } else
    m_is_malloc_mem_by_myself = 0;

  memcpy(LevelScale4x4, src.LevelScale4x4, sizeof(int32_t) * 6 * 4 * 4);
  memcpy(LevelScale8x8, src.LevelScale8x8, sizeof(int32_t) * 6 * 4 * 4);

  m_picture_coded_type = src.m_picture_coded_type;
  m_picture_type = src.m_picture_type;
  m_slice_cnt = src.m_slice_cnt;

  memcpy(m_dpb, src.m_dpb, sizeof(Frame *) * 16);
  m_parent = src.m_parent;

  memcpy(m_RefPicList0, src.m_RefPicList0, sizeof(Frame *) * 16);
  memcpy(m_RefPicList1, src.m_RefPicList1, sizeof(Frame *) * 16);
  m_RefPicList0Length = src.m_RefPicList0Length;
  m_RefPicList1Length = src.m_RefPicList1Length;

  m_PicNumCnt = src.m_PicNumCnt;

  return 0;
}

/* 重写高性能 Copy函数？ */
int PictureBase::copyDataPicOrderCnt(const PictureBase &src) {
  TopFieldOrderCnt = src.TopFieldOrderCnt;
  BottomFieldOrderCnt = src.BottomFieldOrderCnt;
  PicOrderCntMsb = src.PicOrderCntMsb;
  PicOrderCntLsb = src.PicOrderCntLsb;
  FrameNumOffset = src.FrameNumOffset;
  absFrameNum = src.absFrameNum;
  picOrderCntCycleCnt = src.picOrderCntCycleCnt;
  frameNumInPicOrderCntCycle = src.frameNumInPicOrderCntCycle;
  expectedPicOrderCnt = src.expectedPicOrderCnt;
  PicOrderCnt = src.PicOrderCnt;
  FrameNum = src.FrameNum;
  FrameNumWrap = src.FrameNumWrap;
  LongTermFrameIdx = src.LongTermFrameIdx;
  PicNum = src.PicNum;
  LongTermPicNum = src.LongTermPicNum;
  FieldNum = src.FieldNum;
  MaxLongTermFrameIdx = src.MaxLongTermFrameIdx;

  m_slice_cnt = src.m_slice_cnt;

  memcpy(m_RefPicList0, src.m_RefPicList0, sizeof(Frame *) * 16);
  memcpy(m_RefPicList1, src.m_RefPicList1, sizeof(Frame *) * 16);
  m_RefPicList0Length = src.m_RefPicList0Length;
  m_RefPicList1Length = src.m_RefPicList1Length;
  m_PicNumCnt = src.m_PicNumCnt;

  return 0;
}

int PictureBase::getOneFrameFromDPB(Frame *&pic) {
  for (int i = 0; i < MAX_DPB; i++) {
    // 本帧数据未使用，即处于闲置状态, 重复利用被释放了的参考帧
    if (m_dpb[i] != this->m_parent &&
        m_dpb[i]->reference_marked_type != SHORT_REF &&
        m_dpb[i]->reference_marked_type != LONG_REF &&
        m_dpb[i]->m_is_in_use == false) {
      pic = m_dpb[i];
      break;
    }
  }
  return 0;
}

int PictureBase::getEmptyFrameFromDPB(Frame *&emptyPic) {
  if (emptyPic) {
    emptyPic = nullptr;
    std::cout << "Warring emptyPic point is non-NULL !!!" << std::endl;
  }
  if (m_picture_coded_type == FRAME || m_picture_coded_type == BOTTOM_FIELD)
    this->m_parent->m_is_decode_finished = true;
  // 如果当前帧是非参考帧，则处理前面的已解码帧标记
  if (m_slice->slice_header->nal_ref_idc != 0) {
    // 处理解码后的参考图片标记
    RET(decoded_reference_picture_marking(m_dpb));
    if (mmco_5_flag)
      TopFieldOrderCnt -= PicOrderCnt, BottomFieldOrderCnt -= PicOrderCnt;
  }

  // 重置变量的值，用于重复利用帧内存
  getOneFrameFromDPB(emptyPic);
  RET(emptyPic == nullptr);
  emptyPic->reset();
  emptyPic->m_picture_frame.reset();
  emptyPic->m_picture_top_filed.reset();
  emptyPic->m_picture_bottom_filed.reset();
  emptyPic->m_picture_previous = this;

  if (reference_marked_type == SHORT_REF || reference_marked_type == LONG_REF)
    emptyPic->m_picture_previous_ref = this;
  else
    emptyPic->m_picture_previous_ref = this->m_parent->m_picture_previous_ref;

  g_PicNumCnt++;
  emptyPic->m_picture_frame.m_PicNumCnt = g_PicNumCnt;
  emptyPic->m_picture_top_filed.m_PicNumCnt = g_PicNumCnt;
  emptyPic->m_picture_bottom_filed.m_PicNumCnt = g_PicNumCnt;
  return 0;
}

// 6.4.1 Inverse macroblock scanning process
/* 输入: 宏块地址mbAddr。
 * 输出: 地址为 mbAddr 的宏块的左上角亮度样本相对于图片左上角样本的位置 ( x, y )。*/
int PictureBase::inverse_mb_scanning_process(int32_t MbaffFrameFlag,
                                             int32_t mbAddr,
                                             int32_t mb_field_decoding_flag,
                                             int32_t &x, int32_t &y) {
  if (MbaffFrameFlag == 0) {
    //光栅扫描顺序是从左到右、从上到下逐行扫描的顺序。
    x = InverseRasterScan(mbAddr, 16, 16, PicWidthInSamplesL, 0);
    y = InverseRasterScan(mbAddr, 16, 16, PicWidthInSamplesL, 1);
    /* 比如一个720x624分辨率的Slice被分为了45x39=1755（宽45个，高39个）个宏块，那么按照逐行扫描的方式，第46个宏块应该是在第二行的第一个宏块，对于亮度样本而言，此时的x,y = (16,16)
     * 第48个宏块应该是在第二行的第三个宏块，对于亮度样本而言，此时的x,y = (16*3,16)
     * 最后一个宏块即x,y=(720-16,624-16)，x,y以宏块的左上角坐标为准
     * 第一个宏块即x,y=(0,0)*/
  } else {
    x = InverseRasterScan(mbAddr / 2, 16, 32, PicWidthInSamplesL, 0);
    y = InverseRasterScan(mbAddr / 2, 16, 32, PicWidthInSamplesL, 1);
    if (mb_field_decoding_flag == 0)
      y += (mbAddr % 2) * 16;
    else
      y += (mbAddr % 2);
  }
  return 0;
}

// 6.4.2.2 Inverse sub-macroblock partition scanning process
//int PictureBase::inverse_sub_macroblock_partition_scanning_process(
//    H264_MB_TYPE m_name_of_mb_type, int32_t mbPartIdx, int32_t subMbPartIdx,
//    int32_t &x, int32_t &y) {
//  /* TODO YangJing  <24-10-09 20:29:27> */
//  return 0;
//}
