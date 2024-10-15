#include "PictureBase.hpp"
#include "Bitmap.hpp"
#include "Frame.hpp"
#include "MacroBlock.hpp"
#include "SliceHeader.hpp"
#include "Type.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>

extern int32_t g_PicNumCnt;

PictureBase::PictureBase() {
  m_mbs = NULL;
  m_pic_buff_luma = NULL;
  m_pic_buff_cb = NULL;
  m_pic_buff_cr = NULL;
  m_is_malloc_mem_by_myself = 0;

  reset();
}

PictureBase::~PictureBase() { unInit(); }

int PictureBase::reset() {
  //----------------------
  if (m_mbs) {
    memset(m_mbs, 0, sizeof(MacroBlock) * PicSizeInMbs);
    //fill(m_mbs, m_mbs + PicSizeInMbs, MacroBlock());
  }

  if (m_picture_coded_type == PICTURE_CODED_TYPE_FRAME) {
    if (m_pic_buff_luma) {
      memset(m_pic_buff_luma, 0,
             sizeof(uint8_t) * PicWidthInSamplesL * PicHeightInSamplesL);
    }

    if (m_pic_buff_cb) {
      memset(m_pic_buff_cb, 0,
             sizeof(uint8_t) * PicWidthInSamplesC * PicHeightInSamplesC);
    }

    if (m_pic_buff_cr) {
      memset(m_pic_buff_cr, 0,
             sizeof(uint8_t) * PicWidthInSamplesC * PicHeightInSamplesC);
    }
  }

  //----------------------
  mb_x = 0;
  mb_y = 0;
  m_pic_coded_width_pixels = 0;
  m_pic_coded_height_pixels = 0;
  MbWidthL = 0;
  MbHeightL = 0;
  MbWidthC = 0;
  MbHeightC = 0;
  Chroma_Format = 0;
  mb_cnt = 0;
  CurrMbAddr = 0;
  PicWidthInMbs = 0;
  PicHeightInMbs = 0;
  PicSizeInMbs = 0;
  //    m_mbs = NULL;
  //    m_pic_buff_luma = NULL;
  //    m_pic_buff_cb = NULL;
  //    m_pic_buff_cr = NULL;
  TopFieldOrderCnt = 0;
  BottomFieldOrderCnt = 0;
  PicOrderCntMsb = 0;
  PicOrderCntLsb = 0;
  FrameNumOffset = 0;
  absFrameNum = 0;
  picOrderCntCycleCnt = 0;
  frameNumInPicOrderCntCycle = 0;
  expectedPicOrderCnt = 0;
  PicOrderCnt = 0;
  FrameNum = 0;
  FrameNumWrap = 0;
  LongTermFrameIdx = 0;
  PicNum = 0;
  LongTermPicNum = 0;
  FieldNum = NA;
  MaxLongTermFrameIdx = NA;
  memory_management_control_operation_5_flag = 0;
  memory_management_control_operation_6_flag = 0;
  reference_marked_type = PICTURE_MARKED_AS_unkown;
  m_picture_coded_type = PICTURE_CODED_TYPE_UNKNOWN;
  m_picture_type = H264_PICTURE_TYPE_UNKNOWN;
  m_is_decode_finished = 0;
  m_parent = NULL;
  m_slice_cnt = 0;
  memset(m_dpb, 0, sizeof(Frame *) * 16);
  memset(m_RefPicList0, 0, sizeof(Frame *) * 16);
  memset(m_RefPicList1, 0, sizeof(Frame *) * 16);
  m_RefPicList0Length = 0;
  m_RefPicList1Length = 0;
  m_PicNumCnt = 0;

  return 0;
}

int PictureBase::init(Slice *slice) {
  this->m_slice = slice;

  MbWidthL = MB_WIDTH;   // 16
  MbHeightL = MB_HEIGHT; // 16
  MbWidthC = m_slice->slice_header->m_sps->MbWidthC;
  MbHeightC = m_slice->slice_header->m_sps->MbHeightC;
  Chroma_Format = m_slice->slice_header->m_sps->Chroma_Format;

  PicWidthInMbs = m_slice->slice_header->m_sps->PicWidthInMbs;
  PicHeightInMbs = m_slice->slice_header->PicHeightInMbs;
  PicSizeInMbs = PicWidthInMbs * PicHeightInMbs;

  PicWidthInSamplesL = PicWidthInMbs * 16;
  PicWidthInSamplesC = PicWidthInMbs * MbWidthC;

  PicHeightInSamplesL = PicHeightInMbs * 16;
  PicHeightInSamplesC = PicHeightInMbs * MbHeightC;

  m_pic_coded_width_pixels = PicWidthInMbs * MbWidthL;
  m_pic_coded_height_pixels = PicHeightInMbs * MbHeightL;

  //-----------------------
  if (m_is_malloc_mem_by_myself == 1) {
    return 0;
  }

  //----------------------------
  if (m_picture_coded_type == PICTURE_CODED_TYPE_FRAME) {
    m_mbs = (MacroBlock *)malloc(sizeof(MacroBlock) * PicSizeInMbs);
    // 因为MacroBlock构造函数中，有对变量初始化，可以考虑使用C++/new申请内存，此处使用C/my_malloc
    RETURN_IF_FAILED(m_mbs == NULL, -1);
    memset(m_mbs, 0, sizeof(MacroBlock) * PicSizeInMbs);

    //-----------YUV420P-----------------
    int sizeY = PicWidthInSamplesL * PicHeightInSamplesL;
    int sizeU = PicWidthInSamplesC * PicHeightInSamplesC;
    int sizeV = PicWidthInSamplesC * PicHeightInSamplesC;

    int totalSzie = sizeY + sizeU + sizeV;

    uint8_t *pic_buff = (uint8_t *)malloc(
        sizeof(uint8_t) *
        totalSzie); // Y,U,V 这3个通道数据存储在一块连续的内存中
    RETURN_IF_FAILED(pic_buff == NULL, -1);
    memset(pic_buff, 0, sizeof(uint8_t) * totalSzie);

    m_pic_buff_luma = pic_buff;
    m_pic_buff_cb = m_pic_buff_luma + sizeY;
    m_pic_buff_cr = m_pic_buff_cb + sizeU;

    m_is_malloc_mem_by_myself = 1;
  } else {
    // 因为top_filed顶场帧和bottom底场帧，都是共享frame帧的大部分数据信息，所以frame帧必须先初始化过了才行
    RETURN_IF_FAILED(
        this->m_parent->m_picture_frame.m_is_malloc_mem_by_myself != 1, -1);

    H264_PICTURE_CODED_TYPE picture_coded_type = m_picture_coded_type;

    // memcpy(this, &(this->m_parent->m_picture_frame),
    // sizeof(PictureBase)); //先整体拷贝一份

    int32_t copyMbsDataFlag = 0;
    copyData2(this->m_parent->m_picture_frame, copyMbsDataFlag);

    m_picture_coded_type = picture_coded_type;

    //----------重新计算filed帧的高度--------------------
    MbWidthL = MB_WIDTH;   // 16
    MbHeightL = MB_HEIGHT; // 16
    MbWidthC = m_slice->slice_header->m_sps->MbWidthC;
    MbHeightC = m_slice->slice_header->m_sps->MbHeightC;
    Chroma_Format = m_slice->slice_header->m_sps->Chroma_Format;

    PicWidthInMbs = m_slice->slice_header->m_sps->PicWidthInMbs;
    PicHeightInMbs = m_slice->slice_header->PicHeightInMbs /
                     2; // filed场帧的高度是frame帧高度的一半
    PicSizeInMbs = PicWidthInMbs * PicHeightInMbs;

    PicWidthInSamplesL =
        PicWidthInMbs * 16 *
        2; // filed场帧像素的宽度是frame帧宽度的2倍（即两个相邻奇数行或两个相邻偶数行的间距）
    PicWidthInSamplesC = PicWidthInMbs * MbWidthC * 2;

    PicHeightInSamplesL = PicHeightInMbs * 16;
    PicHeightInSamplesC = PicHeightInMbs * MbHeightC;

    m_pic_coded_width_pixels = PicWidthInMbs * MbWidthL;
    m_pic_coded_height_pixels = PicHeightInMbs * MbHeightL;

    if (m_picture_coded_type == PICTURE_CODED_TYPE_TOP_FIELD) {
      //
    } else // if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD)
    {
      // 因为bottom底场帧被定义为图片的偶数行，所以像素地址从第二行开始计算
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
    FREE(m_mbs);
    FREE(m_pic_buff_luma);
  } else {
    m_mbs = NULL;
    m_pic_buff_luma = NULL;
    m_pic_buff_cb = NULL;
    m_pic_buff_cr = NULL;
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
  int ret = 0;

  ret = unInit();
  RETURN_IF_FAILED(ret != 0, ret);

  memcpy(this, &src, sizeof(PictureBase));

  m_is_malloc_mem_by_myself = 0;

  if (isMallocAndCopyData) {
    ret = init(src.m_slice);
    RETURN_IF_FAILED(ret, -1);

    //memcpy(m_mbs, src.m_mbs, sizeof(MacroBlock) * PicSizeInMbs);
    copy(src.m_mbs, src.m_mbs + PicSizeInMbs, m_mbs);

    memcpy(m_pic_buff_luma, src.m_pic_buff_luma,
           sizeof(uint8_t) * PicWidthInSamplesL * PicHeightInSamplesL);
    memcpy(m_pic_buff_cb, src.m_pic_buff_cb,
           sizeof(uint8_t) * PicWidthInSamplesC * PicHeightInSamplesC);
    memcpy(m_pic_buff_cr, src.m_pic_buff_cr,
           sizeof(uint8_t) * PicWidthInSamplesC * PicHeightInSamplesC);
  } else {
    SliceHeader *slice_header = src.m_slice->slice_header;

    MbWidthL = MB_WIDTH;   // 16
    MbHeightL = MB_HEIGHT; // 16
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

  return ret;
}

int PictureBase::copyData2(const PictureBase &src, int32_t copyMbsDataFlag) {
  int ret = 0;

  mb_x = src.mb_x;
  mb_y = src.mb_y;
  m_pic_coded_width_pixels = src.m_pic_coded_width_pixels;
  m_pic_coded_height_pixels = src.m_pic_coded_height_pixels;
  PicWidthInMbs = src.PicWidthInMbs;
  PicHeightInMbs = src.PicHeightInMbs;
  PicSizeInMbs = src.PicSizeInMbs;
  MbWidthL = src.MbWidthL;
  MbHeightL = src.MbHeightL;
  MbWidthC = src.MbWidthC;
  MbHeightC = src.MbHeightC;
  PicWidthInSamplesL = src.PicWidthInSamplesL;
  PicWidthInSamplesC = src.PicWidthInSamplesC;
  PicHeightInSamplesL = src.PicHeightInSamplesL;
  PicHeightInSamplesC = src.PicHeightInSamplesC;
  Chroma_Format = src.Chroma_Format;
  mb_cnt = src.mb_cnt;
  CurrMbAddr = src.CurrMbAddr;
  m_pic_buff_luma = src.m_pic_buff_luma;
  m_pic_buff_cb = src.m_pic_buff_cb;
  m_pic_buff_cr = src.m_pic_buff_cr;
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
  memory_management_control_operation_5_flag =
      src.memory_management_control_operation_5_flag;
  memory_management_control_operation_6_flag =
      src.memory_management_control_operation_6_flag;
  reference_marked_type = src.reference_marked_type;

  //m_h264_slice_header = src.m_h264_slice_header;
  //m_h264_slice_data = src.m_h264_slice_data;

  if (copyMbsDataFlag == 0) {
    m_mbs = src.m_mbs;
    m_is_malloc_mem_by_myself = 0; // src.m_is_malloc_mem_by_myself;
  } else if (copyMbsDataFlag == 1) {
    //memcpy(m_mbs, src.m_mbs, sizeof(MacroBlock) * PicSizeInMbs);
    copy(src.m_mbs, src.m_mbs + PicSizeInMbs, m_mbs);
    m_is_malloc_mem_by_myself = 1;
  } else {
    // do nothing
    m_is_malloc_mem_by_myself = 0;
  }

  memcpy(LevelScale4x4, src.LevelScale4x4, sizeof(int32_t) * 6 * 4 * 4);
  memcpy(LevelScale8x8, src.LevelScale8x8, sizeof(int32_t) * 6 * 4 * 4);

  m_picture_coded_type = src.m_picture_coded_type;
  m_picture_type = src.m_picture_type;
  m_is_decode_finished = src.m_is_decode_finished;
  m_slice_cnt = src.m_slice_cnt;

  memcpy(m_dpb, src.m_dpb, sizeof(Frame *) * 16);
  m_parent = src.m_parent;
  memcpy(m_RefPicList0, src.m_RefPicList0, sizeof(Frame *) * 16);
  memcpy(m_RefPicList1, src.m_RefPicList1, sizeof(Frame *) * 16);
  m_RefPicList0Length = src.m_RefPicList0Length;
  m_RefPicList1Length = src.m_RefPicList1Length;
  m_PicNumCnt = src.m_PicNumCnt;

  return ret;
}

/* 重写高性能 Copy函数？ */
int PictureBase::copyDataPicOrderCnt(const PictureBase &src) {
  int ret = 0;

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

  m_is_decode_finished = src.m_is_decode_finished;
  m_slice_cnt = src.m_slice_cnt;

  memcpy(m_RefPicList0, src.m_RefPicList0, sizeof(Frame *) * 16);
  memcpy(m_RefPicList1, src.m_RefPicList1, sizeof(Frame *) * 16);
  m_RefPicList0Length = src.m_RefPicList0Length;
  m_RefPicList1Length = src.m_RefPicList1Length;
  m_PicNumCnt = src.m_PicNumCnt;

  return ret;
}

int PictureBase::convertYuv420pToBgr24(uint32_t width, uint32_t height,
                                       const uint8_t *yuv420p, uint8_t *bgr24,
                                       uint32_t widthBytesBgr24) {
  int32_t W = width, H = height, channels = 3;

  //------------- YUV420P to BGR24 --------------------
  // m_slice->slice_header->m_sps->frame_crop_[left,right,top,bottom]_offset
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      unsigned char Y = yuv420p[y * W + x];
      unsigned char U = yuv420p[H * W + (y / 2) * (W / 2) + x / 2];
      unsigned char V = yuv420p[H * W + H * W / 4 + (y / 2) * (W / 2) + x / 2];

      int b = (1164 * (Y - 16) + 2018 * (U - 128)) / 1000;
      int g = (1164 * (Y - 16) - 813 * (V - 128) - 391 * (U - 128)) / 1000;
      int r = (1164 * (Y - 16) + 1596 * (V - 128)) / 1000;

      bgr24[y * widthBytesBgr24 + x * channels + 0] = CLIP3(0, 255, b);
      bgr24[y * widthBytesBgr24 + x * channels + 1] = CLIP3(0, 255, g);
      bgr24[y * widthBytesBgr24 + x * channels + 2] = CLIP3(0, 255, r);
    }
  }

  return 0;
}

int PictureBase::convertYuv420pToBgr24FlipLines(uint32_t width, uint32_t height,
                                                const uint8_t *yuv420p,
                                                uint8_t *bgr24,
                                                uint32_t widthBytesBgr24) {
  int32_t W = width, H = height, channels = 3;

  //------------- YUV420P to BGR24 --------------------
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      unsigned char Y = yuv420p[y * W + x];
      unsigned char U = yuv420p[H * W + (y / 2) * (W / 2) + x / 2];
      unsigned char V = yuv420p[H * W + H * W / 4 + (y / 2) * (W / 2) + x / 2];

      int b = (1164 * (Y - 16) + 2018 * (U - 128)) / 1000;
      int g = (1164 * (Y - 16) - 813 * (V - 128) - 391 * (U - 128)) / 1000;
      int r = (1164 * (Y - 16) + 1596 * (V - 128)) / 1000;

      bgr24[(H - 1 - y) * widthBytesBgr24 + x * channels + 0] =
          CLIP3(0, 255, b);
      bgr24[(H - 1 - y) * widthBytesBgr24 + x * channels + 1] =
          CLIP3(0, 255, g);
      bgr24[(H - 1 - y) * widthBytesBgr24 + x * channels + 2] =
          CLIP3(0, 255, r);
    }
  }

  return 0;
}

int PictureBase::createEmptyImage(MY_BITMAP &bitmap, int32_t width,
                                  int32_t height, int32_t bmBitsPixel) {
  bitmap.bmWidth = width;
  bitmap.bmHeight = height;
  bitmap.bmType = 0;
  bitmap.bmPlanes = 1;
  bitmap.bmBitsPixel = bmBitsPixel; // 32

  bitmap.bmWidthBytes = (width * bmBitsPixel / 8 + 3) / 4 * 4;

  uint8_t *pBits = (uint8_t *)malloc(bitmap.bmHeight * bitmap.bmWidthBytes);
  if (pBits == nullptr) RET(-1);
  // 初始化为黑色背景
  memset(pBits, 0, sizeof(uint8_t) * bitmap.bmHeight * bitmap.bmWidthBytes);
  bitmap.bmBits = pBits;
  return 0;
}

int PictureBase::saveToBmpFile(const char *filename) {
  //----------------yuv420p到brg24的格式转换-------------------------
  int32_t W = PicWidthInSamplesL;
  int32_t H = PicHeightInSamplesL;
  MY_BITMAP bitmap;
  RET(createEmptyImage(bitmap, W, H, 24));
  RET(convertYuv420pToBgr24(W, H, m_pic_buff_luma, (uint8_t *)bitmap.bmBits,
                            bitmap.bmWidthBytes));
  RET(saveBmp(filename, &bitmap));
  free(bitmap.bmBits);
  bitmap.bmBits = nullptr;
  return 0;
}

int PictureBase::saveBmp(const char *filename, MY_BITMAP *pBitmap) {
  MY_BitmapFileHeader bmpFileHeader;
  MY_BitmapInfoHeader bmpInfoHeader;
  // unsigned char pixVal = '\0';
  MY_RgbQuad quad[256] = {{0}};

  FILE *fp = fopen(filename, "wb");
  if (!fp) return -1;

  unsigned short fileType = 0x4D42;
  fwrite(&fileType, sizeof(unsigned short), 1, fp);

  // 24位，通道，彩图
  if (pBitmap->bmBitsPixel == 24 || pBitmap->bmBitsPixel == 32) {
    int rowbytes = pBitmap->bmWidthBytes;

    bmpFileHeader.bfSize = pBitmap->bmHeight * rowbytes + 54;
    bmpFileHeader.bfReserved1 = 0;
    bmpFileHeader.bfReserved2 = 0;
    bmpFileHeader.bfOffBits = 54;
    fwrite(&bmpFileHeader, sizeof(MY_BitmapFileHeader), 1, fp);

    bmpInfoHeader.biSize = 40;
    bmpInfoHeader.biWidth = pBitmap->bmWidth;
    bmpInfoHeader.biHeight = pBitmap->bmHeight;
    bmpInfoHeader.biPlanes = 1;
    bmpInfoHeader.biBitCount = pBitmap->bmBitsPixel; // 24|32
    bmpInfoHeader.biCompression = 0;
    bmpInfoHeader.biSizeImage = pBitmap->bmHeight * rowbytes;
    bmpInfoHeader.biXPelsPerMeter = 0;
    bmpInfoHeader.biYPelsPerMeter = 0;
    bmpInfoHeader.biClrUsed = 0;
    bmpInfoHeader.biClrImportant = 0;
    fwrite(&bmpInfoHeader, sizeof(MY_BitmapInfoHeader), 1, fp);

    // int channels = pBitmap->bmBitsPixel / 8;
    unsigned char *pBits = (unsigned char *)(pBitmap->bmBits);

    for (int i = pBitmap->bmHeight - 1; i > -1; i--)
      fwrite(pBits + i * rowbytes, rowbytes, 1, fp);
  }
  // 8位，单通道，灰度图
  else if (pBitmap->bmBitsPixel == 8) {
    int rowbytes = pBitmap->bmWidthBytes;

    bmpFileHeader.bfSize = pBitmap->bmHeight * rowbytes + 54 + 256 * 4;
    bmpFileHeader.bfReserved1 = 0;
    bmpFileHeader.bfReserved2 = 0;
    bmpFileHeader.bfOffBits = 54 + 256 * 4;
    fwrite(&bmpFileHeader, sizeof(MY_BitmapFileHeader), 1, fp);

    bmpInfoHeader.biSize = 40;
    bmpInfoHeader.biWidth = pBitmap->bmWidth;
    bmpInfoHeader.biHeight = pBitmap->bmHeight;
    bmpInfoHeader.biPlanes = 1;
    bmpInfoHeader.biBitCount = 8;
    bmpInfoHeader.biCompression = 0;
    bmpInfoHeader.biSizeImage = pBitmap->bmHeight * rowbytes;
    bmpInfoHeader.biXPelsPerMeter = 0;
    bmpInfoHeader.biYPelsPerMeter = 0;
    bmpInfoHeader.biClrUsed = 256;
    bmpInfoHeader.biClrImportant = 256;
    fwrite(&bmpInfoHeader, sizeof(MY_BitmapInfoHeader), 1, fp);

    for (int i = 0; i < 256; i++) {
      quad[i].rgbBlue = i;
      quad[i].rgbGreen = i;
      quad[i].rgbRed = i;
      quad[i].rgbReserved = 0;
    }

    fwrite(quad, sizeof(MY_RgbQuad), 256, fp);

    // int channels = pBitmap->bmBitsPixel / 8;
    unsigned char *pBits = (unsigned char *)(pBitmap->bmBits);

    for (int i = pBitmap->bmHeight - 1; i > -1; i--)
      fwrite(pBits + i * rowbytes, rowbytes, 1, fp);
  }

  fclose(fp);

  return 0;
}

#include <fstream>

/* 所有解码的帧写入到一个文件 */
int PictureBase::writeYUV(const char *filename) {
  static bool isFrist = false;
  if (isFrist == false) {
    std::ifstream f(filename);
    if (f.good()) remove(filename);
    isFrist = true;
  }

  FILE *fp = fopen(filename, "ab+");
  if (fp == NULL) return -1;

  fwrite(m_pic_buff_luma, PicWidthInSamplesL * PicHeightInSamplesL, 1, fp);
  fwrite(m_pic_buff_cb, PicWidthInSamplesC * PicHeightInSamplesC, 1, fp);
  fwrite(m_pic_buff_cr, PicWidthInSamplesC * PicHeightInSamplesC, 1, fp);

  fclose(fp);
  return 0;
}

int PictureBase::getOneEmptyPicture(Frame *&pic) {
  int32_t size_dpb = H264_MAX_DECODED_PICTURE_BUFFER_COUNT;

  for (int i = 0; i < size_dpb; i++) {
    // 本帧数据未使用，即处于闲置状态, 重复利用被释放了的参考帧
    if (m_dpb[i] != this->m_parent &&
        m_dpb[i]->reference_marked_type != PICTURE_MARKED_AS_used_short_ref &&
        m_dpb[i]->reference_marked_type != PICTURE_MARKED_AS_used_long_ref &&
        m_dpb[i]->m_is_in_use == 0) {
      pic = m_dpb[i];
      RET(pic == nullptr);
      return 0;
    }
  }

  return -1;
}

int PictureBase::end_decode_the_picture_and_get_a_new_empty_picture(
    Frame *&newEmptyPicture) {

  this->m_is_decode_finished = 1;
  if (m_picture_coded_type == PICTURE_CODED_TYPE_FRAME ||
      m_picture_coded_type == PICTURE_CODED_TYPE_BOTTOM_FIELD) {
    this->m_parent->m_is_decode_finished = 1;
  }

  //--------标记图像参考列表------------
  if (m_slice->slice_header->nal_ref_idc != 0) {
    /* TODO YangJing 这里函数要认真看 <24-10-14 05:44:27> */
    RET(decoded_reference_picture_marking(m_dpb));
    if (memory_management_control_operation_5_flag) {
      int32_t tempPicOrderCnt = PicOrderCnt; // PicOrderCntFunc(this);
      TopFieldOrderCnt = TopFieldOrderCnt - tempPicOrderCnt;
      BottomFieldOrderCnt = BottomFieldOrderCnt - tempPicOrderCnt;
    }
  }

  Frame *emptyPic = nullptr;
  RET(getOneEmptyPicture(emptyPic));

  int ret = 0;
  ret = emptyPic->reset();                        // 重置各个变量的值
  ret = emptyPic->m_picture_frame.reset();        // 重置各个变量的值
  ret = emptyPic->m_picture_top_filed.reset();    // 重置各个变量的值
  ret = emptyPic->m_picture_bottom_filed.reset(); // 重置各个变量的值
  RET(ret);

  emptyPic->m_picture_previous = this;

  if (reference_marked_type == PICTURE_MARKED_AS_used_short_ref ||
      reference_marked_type == PICTURE_MARKED_AS_used_long_ref)
    emptyPic->m_picture_previous_ref = this;
  else
    emptyPic->m_picture_previous_ref = this->m_parent->m_picture_previous_ref;

  g_PicNumCnt++;

  emptyPic->m_picture_frame.m_PicNumCnt = g_PicNumCnt;
  emptyPic->m_picture_top_filed.m_PicNumCnt = g_PicNumCnt;
  emptyPic->m_picture_bottom_filed.m_PicNumCnt = g_PicNumCnt;

  newEmptyPicture = emptyPic;
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

//// 6.4.2.2 Inverse sub-macroblock partition scanning process
//int PictureBase::inverse_sub_macroblock_partition_scanning_process(
//    H264_MB_TYPE m_name_of_mb_type, int32_t mbPartIdx, int32_t subMbPartIdx,
//    int32_t &x, int32_t &y) {
//  /* TODO YangJing  <24-10-09 20:29:27> */
//  return 0;
//}
