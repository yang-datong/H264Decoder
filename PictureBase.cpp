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

// 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
int PictureBase::inverse_scanning_for_4x4_transform_coeff_and_scaling_lists(
    const int32_t values[16], int32_t (&c)[4][4], int32_t field_scan_flag) {
  // Table 8-13 – Specification of mapping of idx to cij for zig-zag and field scan
  if (field_scan_flag == 0) {
    // zig-zag scan
    c[0][0] = values[0];
    c[0][1] = values[1];
    c[1][0] = values[2];
    c[2][0] = values[3];

    c[1][1] = values[4];
    c[0][2] = values[5];
    c[0][3] = values[6];
    c[1][2] = values[7];

    c[2][1] = values[8];
    c[3][0] = values[9];
    c[3][1] = values[10];
    c[2][2] = values[11];

    c[1][3] = values[12];
    c[2][3] = values[13];
    c[3][2] = values[14];
    c[3][3] = values[15];
  } else {
    // field scan
    c[0][0] = values[0];
    c[1][0] = values[1];
    c[0][1] = values[2];
    c[2][0] = values[3];

    c[3][0] = values[4];
    c[1][1] = values[5];
    c[2][1] = values[6];
    c[3][1] = values[7];

    c[0][2] = values[8];
    c[1][2] = values[9];
    c[2][2] = values[10];
    c[3][2] = values[11];

    c[0][3] = values[12];
    c[1][3] = values[13];
    c[2][3] = values[14];
    c[3][3] = values[15];
  }

  return 0;
}

// 8.5.7 Inverse scanning process for 8x8 transform coefficients and scaling lists
int PictureBase::inverse_scanning_for_8x8_transform_coeff_and_scaling_lists(
    int32_t values[64], int32_t (&c)[8][8], int32_t field_scan_flag) {
  // Table 8-14 – Specification of mapping of idx to cij for 8x8 zig-zag and 8x8 field scan
  if (field_scan_flag == 0) {
    // 8x8 zig-zag scan
    c[0][0] = values[0];
    c[0][1] = values[1];
    c[1][0] = values[2];
    c[2][0] = values[3];
    c[1][1] = values[4];
    c[0][2] = values[5];
    c[0][3] = values[6];
    c[1][2] = values[7];
    c[2][1] = values[8];
    c[3][0] = values[9];
    c[4][0] = values[10];
    c[3][1] = values[11];
    c[2][2] = values[12];
    c[1][3] = values[13];
    c[0][4] = values[14];
    c[0][5] = values[15];

    c[1][4] = values[16];
    c[2][3] = values[17];
    c[3][2] = values[18];
    c[4][1] = values[19];
    c[5][0] = values[20];
    c[6][0] = values[21];
    c[5][1] = values[22];
    c[4][2] = values[23];
    c[3][3] = values[24];
    c[2][4] = values[25];
    c[1][5] = values[26];
    c[0][6] = values[27];
    c[0][7] = values[28];
    c[1][6] = values[29];
    c[2][5] = values[30];
    c[3][4] = values[31];

    c[4][3] = values[32];
    c[5][2] = values[33];
    c[6][1] = values[34];
    c[7][0] = values[35];
    c[7][1] = values[36];
    c[6][2] = values[37];
    c[5][3] = values[38];
    c[4][4] = values[39];
    c[3][5] = values[40];
    c[2][6] = values[41];
    c[1][7] = values[42];
    c[2][7] = values[43];
    c[3][6] = values[44];
    c[4][5] = values[45];
    c[5][4] = values[46];
    c[6][3] = values[47];

    c[7][2] = values[48];
    c[7][3] = values[49];
    c[6][4] = values[50];
    c[5][5] = values[51];
    c[4][6] = values[52];
    c[3][7] = values[53];
    c[4][7] = values[54];
    c[5][6] = values[55];
    c[6][5] = values[56];
    c[7][4] = values[57];
    c[7][5] = values[58];
    c[6][6] = values[59];
    c[5][7] = values[60];
    c[6][7] = values[61];
    c[7][6] = values[62];
    c[7][7] = values[63];
  } else {
    // 8x8 field scan
    c[0][0] = values[0];
    c[1][0] = values[1];
    c[2][0] = values[2];
    c[0][1] = values[3];
    c[1][1] = values[4];
    c[3][0] = values[5];
    c[4][0] = values[6];
    c[2][1] = values[7];
    c[0][2] = values[8];
    c[3][1] = values[9];
    c[5][0] = values[10];
    c[6][0] = values[11];
    c[7][0] = values[12];
    c[4][1] = values[13];
    c[1][2] = values[14];
    c[0][3] = values[15];

    c[2][2] = values[16];
    c[5][1] = values[17];
    c[6][1] = values[18];
    c[7][1] = values[19];
    c[3][2] = values[20];
    c[1][3] = values[21];
    c[0][4] = values[22];
    c[2][3] = values[23];
    c[4][2] = values[24];
    c[5][2] = values[25];
    c[6][2] = values[26];
    c[7][2] = values[27];
    c[3][3] = values[28];
    c[1][4] = values[29];
    c[0][5] = values[30];
    c[2][4] = values[31];

    c[4][3] = values[32];
    c[5][3] = values[33];
    c[6][3] = values[34];
    c[7][3] = values[35];
    c[3][4] = values[36];
    c[1][5] = values[37];
    c[0][6] = values[38];
    c[2][5] = values[39];
    c[4][4] = values[40];
    c[5][4] = values[41];
    c[6][4] = values[42];
    c[7][4] = values[43];
    c[3][5] = values[44];
    c[1][6] = values[45];
    c[2][6] = values[46];
    c[4][5] = values[47];

    c[5][5] = values[48];
    c[6][5] = values[49];
    c[7][5] = values[50];
    c[3][6] = values[51];
    c[0][7] = values[52];
    c[1][7] = values[53];
    c[4][6] = values[54];
    c[5][6] = values[55];
    c[6][6] = values[56];
    c[7][6] = values[57];
    c[2][7] = values[58];
    c[3][7] = values[59];
    c[4][7] = values[60];
    c[5][7] = values[61];
    c[6][7] = values[62];
    c[7][7] = values[63];
  }

  return 0;
}

// 6.4.2.2 Inverse sub-macroblock partition scanning process
int PictureBase::inverse_sub_macroblock_partition_scanning_process(
    H264_MB_TYPE m_name_of_mb_type, int32_t mbPartIdx, int32_t subMbPartIdx,
    int32_t &x, int32_t &y) {
  /* TODO YangJing  <24-10-09 20:29:27> */
  return 0;
}

// 6.4.11.1 Derivation process for neighbouring macroblocks
/* 该过程的输出为： 
 * – mbAddrA：当前宏块左侧宏块的地址及其可用性状态， 
 * – mbAddrB：当前宏块上方宏块的地址及其可用性状态。*/
int PictureBase::derivation_for_neighbouring_macroblocks(int32_t MbaffFrameFlag,
                                                         int32_t currMbAddr,
                                                         int32_t &mbAddrA,
                                                         int32_t &mbAddrB,
                                                         int32_t isChroma) {

  int32_t xW = 0, yW = 0;

  /* mbAddrA：当前宏块左侧宏块的地址及其可用性状态 */
  MB_ADDR_TYPE mbAddrA_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma4x4BlkIdxA = 0, luma8x8BlkIdxA = 0;
  int32_t xA = -1, yA = 0;
  RET(derivation_for_neighbouring_locations(
      MbaffFrameFlag, xA, yA, currMbAddr, mbAddrA_type, mbAddrA, luma4x4BlkIdxA,
      luma8x8BlkIdxA, xW, yW, isChroma));

  /* mbAddrB：当前宏块上方宏块的地址及其可用性状态 */
  MB_ADDR_TYPE mbAddrB_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma4x4BlkIdxB = 0, luma8x8BlkIdxB = 0;
  int32_t xB = 0, yB = -1;
  RET(derivation_for_neighbouring_locations(
      MbaffFrameFlag, xB, yB, currMbAddr, mbAddrB_type, mbAddrB, luma4x4BlkIdxB,
      luma8x8BlkIdxB, xW, yW, isChroma));

  return 0;
}

// 6.4.11.2 Derivation process for neighbouring 8x8 luma block
int PictureBase::derivation_for_neighbouring_8x8_luma_block(
    int32_t luma8x8BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
    int32_t &luma8x8BlkIdxA, int32_t &luma8x8BlkIdxB, int32_t isChroma) {

  int32_t xW = 0, yW = 0;

  //---------------mbAddrA---------------------
  MB_ADDR_TYPE mbAddrA_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma4x4BlkIdxA = 0;
  int32_t xA = (luma8x8BlkIdx % 2) * 8 - 1;
  int32_t yA = (luma8x8BlkIdx / 2) * 8 + 0;

  // 6.4.12 Derivation process for neighbouring locations
  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xA, yA, CurrMbAddr, mbAddrA_type,
      mbAddrA, luma4x4BlkIdxA, luma8x8BlkIdxA, xW, yW, isChroma));

  if (mbAddrA < 0) {
    luma8x8BlkIdxA = -2; // marked as not available
  } else {
    // 6.4.13.3 Derivation process for 8x8 luma block indices
    luma8x8BlkIdxA = 2 * (yW / 8) + (xW / 8);
  }

  //---------------mbAddrB---------------------
  MB_ADDR_TYPE mbAddrB_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma4x4BlkIdxB = 0;
  int32_t xB = (luma8x8BlkIdx % 2) * 8 - 0;
  int32_t yB = (luma8x8BlkIdx / 2) * 8 - 1;

  // 6.4.12 Derivation process for neighbouring locations
  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xB, yB, CurrMbAddr, mbAddrB_type,
      mbAddrB, luma4x4BlkIdxB, luma8x8BlkIdxB, xW, yW, isChroma));

  if (mbAddrB < 0) {
    luma8x8BlkIdxB = -2; // marked as not available
  } else {
    // 6.4.13.3 Derivation process for 8x8 luma block indices
    luma8x8BlkIdxB = 2 * (yW / 8) + (xW / 8);
  }

  return 0;
}

// 6.4.11.3 Derivation process for neighbouring 8x8 chroma blocks for ChromaArrayType equal to 3
int PictureBase::derivation_for_neighbouring_8x8_chroma_blocks_for_YUV444(
    int32_t chroma8x8BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
    int32_t &chroma8x8BlkIdxA, int32_t &chroma8x8BlkIdxB) {
  return derivation_for_neighbouring_8x8_luma_block(
      chroma8x8BlkIdx, mbAddrA, mbAddrB, chroma8x8BlkIdxA, chroma8x8BlkIdxB, 1);
}

// 6.4.11.4 Derivation process for neighbouring 4x4 luma blocks
int PictureBase::derivation_for_neighbouring_4x4_luma_blocks(
    int32_t luma4x4BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
    int32_t &luma4x4BlkIdxA, int32_t &luma4x4BlkIdxB, int32_t isChroma) {

  int32_t xW = 0, yW = 0;
  int32_t x = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
              InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
  int32_t y = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
              InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

  //---------------mbAddrA---------------------
  MB_ADDR_TYPE mbAddrA_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxA = 0;

  int32_t xA = x - 1, yA = y + 0;

  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xA, yA, CurrMbAddr, mbAddrA_type,
      mbAddrA, luma4x4BlkIdxA, luma8x8BlkIdxA, xW, yW, isChroma));

  if (mbAddrA < 0) {
    luma4x4BlkIdxA = -2; // marked as not available
  } else {
    // 6.4.13.1 Derivation process for 4x4 luma block indices
    luma4x4BlkIdxA =
        8 * (yW / 8) + 4 * (xW / 8) + 2 * ((yW % 8) / 4) + ((xW % 8) / 4);
  }

  //---------------mbAddrB---------------------
  MB_ADDR_TYPE mbAddrB_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxB = 0;
  int32_t xB = x + 0, yB = y - 1;

  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xB, yB, CurrMbAddr, mbAddrB_type,
      mbAddrB, luma4x4BlkIdxB, luma8x8BlkIdxB, xW, yW, isChroma));

  if (mbAddrB < 0)
    luma4x4BlkIdxB = -2; // marked as not available
  else
    luma4x4BlkIdxB =
        8 * (yW / 8) + 4 * (xW / 8) + 2 * ((yW % 8) / 4) + ((xW % 8) / 4);

  return 0;
}

// 6.4.11.5 Derivation process for neighbouring 4x4 chroma blocks
int PictureBase::derivation_for_neighbouring_4x4_chroma_blocks(
    int32_t chroma4x4BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
    int32_t &chroma4x4BlkIdxA, int32_t &chroma4x4BlkIdxB) {

  int32_t xW = 0, yW = 0;
  int32_t x = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 0);
  int32_t y = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 1);

  //---------------mbAddrA---------------------
  MB_ADDR_TYPE mbAddrA_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxA = 0;
  int32_t xA = x - 1, yA = y + 0;

  // 6.4.12 Derivation process for neighbouring locations
  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xA, yA, CurrMbAddr, mbAddrA_type,
      mbAddrA, chroma4x4BlkIdxA, luma8x8BlkIdxA, xW, yW, 1));

  if (mbAddrA < 0) {
    chroma4x4BlkIdxA = -2; // marked as not available
  } else {
    // 6.4.13.2 Derivation process for 4x4 chroma block indices
    // ret = Derivation_process_for_4x4_chroma_block_indices(xW, yW, (uint8_t &)chroma4x4BlkIdxA);
    chroma4x4BlkIdxA = 2 * (yW / 4) + (xW / 4);
  }

  //---------------mbAddrB---------------------
  MB_ADDR_TYPE mbAddrB_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxB = 0;
  int32_t xB = x + 0, yB = y - 1;
  RET(derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xB, yB, CurrMbAddr, mbAddrB_type,
      mbAddrB, chroma4x4BlkIdxB, luma8x8BlkIdxB, xW, yW, 1));

  if (mbAddrB < 0)
    chroma4x4BlkIdxB = -2; // marked as not available
  else
    // 6.4.13.2 Derivation process for 4x4 chroma block indices
    // ret = Derivation_process_for_4x4_chroma_block_indices(xW, yW, (uint8_t &)chroma4x4BlkIdxB);
    chroma4x4BlkIdxB = 2 * (yW / 4) + (xW / 4);

  return 0;
}

// 6.4.12 Derivation process for neighbouring locations
/* 输入: 相对于当前宏块左上角表示的亮度或色度位置 ( xN, yN )
 * 输出：
 * – mbAddrN：等于 CurrMbAddr 或等于包含 (xN, yN) 及其可用性状态的相邻宏块的地址，
 * – ( xW, yW )：相对于当前宏块表示的位置 (xN, yN) mbAddrN 宏块的左上角（而不是相对于当前宏块的左上角）。
 * */
int PictureBase::derivation_for_neighbouring_locations(
    int32_t MbaffFrameFlag, int32_t xN, int32_t yN, int32_t currMbAddr,
    MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN, int32_t &b4x4BlkIdxN,
    int32_t &b8x8BlkIdxN, int32_t &xW, int32_t &yW, int32_t isChroma) {
  /* 邻近的亮,色度位置调用此过程 */
  int32_t maxW = (isChroma) ? MbWidthC : 16, maxH = (isChroma) ? MbHeightC : 16;

  if (MbaffFrameFlag == 0) {
    RET(neighbouring_locations_non_MBAFF(xN, yN, maxW, maxH, currMbAddr,
                                         mbAddrN_type, mbAddrN, b4x4BlkIdxN,
                                         b8x8BlkIdxN, xW, yW, isChroma));
  } else
    RET(neighbouring_locations_MBAFF(xN, yN, maxW, maxH, currMbAddr,
                                     mbAddrN_type, mbAddrN, b4x4BlkIdxN,
                                     b8x8BlkIdxN, xW, yW, isChroma));
  return 0;
}

// 6.4.8 Derivation process of the availability for macroblock addresses
int PictureBase::derivation_of_availability_for_macroblock_addresses(
    int32_t mbAddr, int32_t &is_mbAddr_available) {
  is_mbAddr_available = 1;
  if (mbAddr < 0 || mbAddr > CurrMbAddr ||
      m_mbs[mbAddr].slice_number != m_mbs[CurrMbAddr].slice_number)
    is_mbAddr_available = 0;
  return 0;
}

/* 6.4.8 Derivation process of the availability for macroblock addresses */
int PictureBase::derivation_of_availability_macroblock_addresses(
    int32_t _mbAddr, int32_t CurrMbAddr, MB_ADDR_TYPE &mbAddrN_type,
    int32_t &mbAddrN) {

  //宏块被标记为不可用
  if (_mbAddr < 0 || _mbAddr > CurrMbAddr ||
      m_mbs[_mbAddr].slice_number != m_mbs[CurrMbAddr].slice_number)
    mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -1;
  //宏块被标记为可用
  else
    mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = _mbAddr;
  return 0;
}

/* 6.4.9 Derivation process for neighbouring macroblock addresses and their availability */
/* 该过程的输出为：
 * – mbAddrA：当前宏块左侧宏块的地址和可用性状态，
 * – mbAddrB：当前宏块上方宏块的地址和可用性状态，
 * – mbAddrC：地址和可用性状态
 * – mbAddrD：当前宏块左上宏块的地址和可用状态。

 Figure 6-12 – Neighbouring macroblocks for a given macroblock
 +-----+-----+-----+
 |  D  |  B  |  C  |
 +-----+-----+-----+
 |  A  | Addr|     |
 +-----+-----+-----+
 |     |     |     |
 +-----+-----+-----+
 */
int PictureBase::derivation_for_neighbouring_macroblock_addr_availability(
    int32_t xN, int32_t yN, int32_t maxW, int32_t maxH, int32_t CurrMbAddr,
    MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN) {

  mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
  mbAddrN = -1;

  int32_t mbAddrA = CurrMbAddr - 1;
  int32_t mbAddrB = CurrMbAddr - PicWidthInMbs;
  int32_t mbAddrC = CurrMbAddr - PicWidthInMbs + 1;
  int32_t mbAddrD = CurrMbAddr - PicWidthInMbs - 1;

  /* Table 6-3 – Specification of mbAddrN */
  /* 左宏快 */
  if (xN < 0 && (yN >= 0 && yN <= maxH - 1)) {
    /* 第 6.4.8 节中的过程的输入是 mbAddrA = CurrMbAddr − 1，输出是宏块 mbAddrA 是否可用。此外，当 CurrMbAddr % PicWidthInMbs 等于 0 时，mbAddrA 被标记为不可用。*/
    derivation_of_availability_macroblock_addresses(mbAddrA, CurrMbAddr,
                                                    mbAddrN_type, mbAddrN);
    if (CurrMbAddr % PicWidthInMbs == 0)
      mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -1;
    else
      mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA;
  }
  /* 上宏快 */
  else if ((xN >= 0 && xN <= maxW - 1) && yN < 0) {
    /* 第 6.4.8 节中的过程的输入是 mbAddrB = CurrMbAddr - PicWidthInMbs，输出是宏块 mbAddrB 是否可用。 */
    derivation_of_availability_macroblock_addresses(mbAddrB, CurrMbAddr,
                                                    mbAddrN_type, mbAddrN);
  }
  /* 右上宏快 */
  else if (xN > maxW - 1 && yN < 0) {
    /* 第 6.4.8 节中的过程的输入是 mbAddrC = CurrMbAddr − PicWidthInMbs + 1，输出是宏块 mbAddrC 是否可用。此外，当 (CurrMbAddr + 1) % PicWidthInMbs 等于 0 时，mbAddrC 被标记为不可用 */
    derivation_of_availability_macroblock_addresses(mbAddrC, CurrMbAddr,
                                                    mbAddrN_type, mbAddrN);
    if ((CurrMbAddr + 1) % PicWidthInMbs == 0)
      mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -1;
    else
      mbAddrN_type = MB_ADDR_TYPE_mbAddrC, mbAddrN = mbAddrC;
  }
  /* 左上宏快 */
  else if (xN < 0 && yN < 0) {
    /* 第 6.4.8 节中的过程的输入是 mbAddrD = CurrMbAddr − PicWidthInMbs − 1，输出是宏块 mbAddrD 是否可用。此外，当 CurrMbAddr % PicWidthInMbs 等于 0 时，mbAddrD 被标记为不可用 */
    derivation_of_availability_macroblock_addresses(mbAddrD, CurrMbAddr,
                                                    mbAddrN_type, mbAddrN);
    if (CurrMbAddr % PicWidthInMbs == 0)
      mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -1;
    else
      mbAddrN_type = MB_ADDR_TYPE_mbAddrD, mbAddrN = mbAddrD;
  } else if ((xN >= 0 && xN <= maxW - 1) && (yN >= 0 && yN <= maxH - 1))
    /* 当前宏块 */
    mbAddrN_type = MB_ADDR_TYPE_CurrMbAddr, mbAddrN = CurrMbAddr;
  else if ((xN > maxW - 1) && (yN >= 0 && yN <= maxH - 1)) {
    /* not available */
  } else if (yN > maxH - 1) {
    /* not available */
  }
  return 0;
}

/* 6.4.10 Derivation process for neighbouring macroblock addresses and their availability in MBAFF frames */
/* 该过程的输出为：
 * – mbAddrA：当前宏块对左侧宏块对顶部宏块的地址和可用性状态，
 * – mbAddrB：当前宏块对上方宏块对顶部宏块的地址和可用性状态当前宏块对，
 * – mbAddrC：当前宏块对右上方宏块对的顶部宏块的地址和可用性状态，
 * – mbAddrD：当前宏块对左上方宏块对的顶部宏块的地址和可用性状态当前宏块对。

 Figure 6-13 – Neighbouring macroblocks for a given macroblock in MBAFF frames
 +-----+-----+-----+
 |  D  |  B  |  C  |
 +-----+-----+-----+
 |     |     |     |
 +-----+-----+-----+
 |  A  | Addr|     |
 +-----+-----+-----+
 |     | Addr|     |
 +-----+-----+-----+
 */
int PictureBase::
    derivation_for_neighbouring_macroblock_addr_availability_in_MBAFF(
        int32_t &mbAddrA, int32_t &mbAddrB, int32_t &mbAddrC,
        int32_t &mbAddrD) {

  /* 第 6.4.8 节中的过程的输入是 mbAddrA = 2 * ( CurrMbAddr / 2 − 1 )，输出是宏块 mbAddrA 是否可用。此外，当 (CurrMbAddr / 2) % PicWidthInMbs 等于 0 时，mbAddrA 被标记为不可用。 */
  mbAddrA = 2 * (CurrMbAddr / 2 - 1);
  mbAddrB = 2 * (CurrMbAddr / 2 - PicWidthInMbs);
  mbAddrC = 2 * (CurrMbAddr / 2 - PicWidthInMbs + 1);
  mbAddrD = 2 * (CurrMbAddr / 2 - PicWidthInMbs - 1);

  if (mbAddrA < 0 || mbAddrA > CurrMbAddr ||
      m_mbs[mbAddrA].slice_number != m_mbs[CurrMbAddr].slice_number ||
      (CurrMbAddr / 2) % PicWidthInMbs == 0)
    mbAddrA = -2;

  if (mbAddrB < 0 || mbAddrB > CurrMbAddr ||
      m_mbs[mbAddrB].slice_number != m_mbs[CurrMbAddr].slice_number)
    mbAddrB = -2;

  if (mbAddrC < 0 || mbAddrC > CurrMbAddr ||
      m_mbs[mbAddrC].slice_number != m_mbs[CurrMbAddr].slice_number ||
      (CurrMbAddr / 2 + 1) % PicWidthInMbs == 0)
    mbAddrC = -2;

  if (mbAddrD < 0 || mbAddrD > CurrMbAddr ||
      m_mbs[mbAddrD].slice_number != m_mbs[CurrMbAddr].slice_number ||
      (CurrMbAddr / 2) % PicWidthInMbs == 0)
    mbAddrD = -2;

  return 0;
}

// 8.5.8 Derivation process for chroma quantisation parameters (色度量化参数的推导过程)
/* 输出： – QPC：每个色度分量 Cb 和 Cr 的色度量化参数，
 * – QSC：解码 SP 和 SI 切片所需的每个色度分量 Cb 和 Cr 的附加色度量化参数（如果适用）*/
/* TODO YangJing 记得看 <24-10-03 21:02:25> */
int PictureBase::derivation_chroma_quantisation_parameters(int32_t isChromaCb) {
  const SliceHeader *header = m_slice->slice_header;
  MacroBlock &mb = m_mbs[CurrMbAddr];

  int32_t qPOffset = header->m_pps->second_chroma_qp_index_offset;
  if (isChromaCb == 1) qPOffset = header->m_pps->chroma_qp_index_offset;

  int32_t qPI =
      CLIP3(-((int32_t)header->m_sps->QpBdOffsetC), 51, mb.QPY + qPOffset);

  // Table 8-15 – Specification of QPC as a function of qPI
  int32_t QPC = qPI;
  if (qPI >= 30) {
    const int32_t QPCs[] = {29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36,
                            36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 39};
    int32_t index = qPI - 30;
    QPC = QPCs[index];
  }

  int32_t QP1C = QPC + header->m_sps->QpBdOffsetC;
  if (isChromaCb == 1)
    mb.QPCb = QPC, mb.QP1Cb = QP1C;
  else
    mb.QPCr = QPC, mb.QP1Cr = QP1C;

  if (header->slice_type == SLICE_SP || header->slice_type == SLICE_SI ||
      header->slice_type == SLICE_SP2 || header->slice_type == SLICE_SI2) {
    mb.QSY = mb.QPY;
    if (isChromaCb == 1)
      mb.QSCb = mb.QPCb, mb.QS1Cb = mb.QP1Cb;
    else
      mb.QSCr = mb.QPCr, mb.QS1Cr = mb.QP1Cr;
  }

  return 0;
}

// 8.5.8 Derivation process for chroma quantisation parameters
int PictureBase::get_chroma_quantisation_parameters2(int32_t QPY,
                                                     int32_t isChromaCb,
                                                     int32_t &QPC) {
  int32_t qPOffset = 0;
  if (isChromaCb == 1)
    qPOffset = m_slice->slice_header->m_pps->chroma_qp_index_offset;
  else
    qPOffset = m_slice->slice_header->m_pps->second_chroma_qp_index_offset;

  int32_t qPI = CLIP3(-(int32_t)m_slice->slice_header->m_sps->QpBdOffsetC, 51,
                      QPY + qPOffset);

  // Table 8-15 – Specification of QPC as a function of qPI
  QPC = qPI;
  if (qPI >= 30) {
    int32_t QPCs[] = {29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36,
                      36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 39};
    QPC = QPCs[qPI - 30];
  }

  return 0;
}

// 6.4.12.2 Specification for neighbouring locations in MBAFF frames
// Table 6-4 – Specification of mbAddrN and yM
int PictureBase::neighbouring_locations_MBAFF(
    int32_t xN, int32_t yN, int32_t maxW, int32_t maxH, int32_t CurrMbAddr,
    MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN, int32_t &b4x4BlkIdxN,
    int32_t &b8x8BlkIdxN, int32_t &xW, int32_t &yW, int32_t isChroma) {

  int32_t yM = 0;
  mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -1;

  /* 第 6.4.10 节中相邻宏块地址及其可用性的推导过程是通过 mbAddrA、mbAddrB、mbAddrC 和 mbAddrD 以及它们的可用性状态作为输出来调用的。 */
  int32_t mbAddrA, mbAddrB, mbAddrC, mbAddrD;
  /* Table 6-4 – Specification of mbAddrN and yM */
  derivation_for_neighbouring_macroblock_addr_availability_in_MBAFF(
      mbAddrA, mbAddrB, mbAddrC, mbAddrD);

  bool currMbFrameFlag = m_mbs[CurrMbAddr].mb_field_decoding_flag == 0;
  bool mbIsTopMbFlag = CurrMbAddr % 2 == 0;

  /* Table 6-4 – Specification of mbAddrN and yM */
  int32_t mbAddrX = -1, mbAddrXFrameFlag = 0;
  if (xN < 0 && yN < 0) {
    if (currMbFrameFlag) {
      if (mbIsTopMbFlag)
        mbAddrX = mbAddrD, mbAddrN_type = MB_ADDR_TYPE_mbAddrD_add_1,
        mbAddrN = mbAddrD + 1, yM = yN;
      else if (mbIsTopMbFlag == false) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA, yM = yN;
          else
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
            yM = (yN + maxH) >> 1;
        }
      }
    } else if (currMbFrameFlag == false) {
      if (mbIsTopMbFlag) {
        mbAddrX = mbAddrD;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrD_add_1, mbAddrN = mbAddrD + 1,
            yM = 2 * yN;
          else
            mbAddrN_type = MB_ADDR_TYPE_mbAddrD, mbAddrN = mbAddrD, yM = yN;
        }
      } else if (mbIsTopMbFlag == false) {
        mbAddrX = mbAddrD, mbAddrN_type = MB_ADDR_TYPE_mbAddrD_add_1,
        mbAddrN = mbAddrD + 1, yM = yN;
      }
    }
  } else if (xN < 0 && (yN >= 0 && yN <= maxH - 1)) {
    if (currMbFrameFlag) {
      if (mbIsTopMbFlag) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA, yM = yN;
          else {
            if (yN % 2 == 0)
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA,
              yM = yN >> 1;
            else if (yN % 2 != 0)
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
              yM = yN >> 1;
          }
        }
      } else if (mbIsTopMbFlag == false) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag == 1) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
            yM = yN;
          } else {
            if (yN % 2 == 0)
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA,
              yM = (yN + maxH) >> 1;
            else if (yN % 2 != 0)
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
              yM = (yN + maxH) >> 1;
          }
        }
      }
    } else if (currMbFrameFlag == false) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag) {
            if (yN < (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA,
              yM = yN << 1;
            } else if (yN >= (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
              yM = (yN << 1) - maxH;
            }
          } else
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA, yM = yN;
        }
      } else if (mbIsTopMbFlag == false) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag) {
            if (yN < (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA, mbAddrN = mbAddrA,
              yM = (yN << 1) + 1;
            } else if (yN >= (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
              yM = (yN << 1) + 1 - maxH;
            }
          } else
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1, mbAddrN = mbAddrA + 1,
            yM = yN;
        }
      }
    }
  } else if ((xN >= 0 && xN <= maxW - 1) && yN < 0) {
    if (currMbFrameFlag) {
      if (mbIsTopMbFlag) {
        mbAddrX = mbAddrB, mbAddrN_type = MB_ADDR_TYPE_mbAddrB_add_1,
        mbAddrN = mbAddrB + 1, yM = yN;
      } else if (mbIsTopMbFlag == false) {
        mbAddrX = CurrMbAddr, mbAddrN_type = MB_ADDR_TYPE_CurrMbAddr_minus_1,
        mbAddrN = CurrMbAddr - 1, yM = yN;
      }
    } else if (currMbFrameFlag == false) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrB;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrB_add_1, mbAddrN = mbAddrB + 1,
            yM = 2 * yN;
          else if (mbAddrXFrameFlag == 0)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrB, mbAddrN = mbAddrB, yM = yN;
        }
      } else
        mbAddrX = mbAddrB, mbAddrN_type = MB_ADDR_TYPE_mbAddrB_add_1,
        mbAddrN = mbAddrB + 1, yM = yN;
    }
  } else if ((xN >= 0 && xN <= maxW - 1) && (yN >= 0 && yN <= maxH - 1)) {
    mbAddrX = CurrMbAddr, mbAddrN_type = MB_ADDR_TYPE_CurrMbAddr,
    mbAddrN = CurrMbAddr, yM = yN;
  } else if (xN > maxW - 1 && yN < 0) {
    if (currMbFrameFlag) {
      if (mbIsTopMbFlag)
        mbAddrX = mbAddrC, mbAddrN_type = MB_ADDR_TYPE_mbAddrC_add_1,
        mbAddrN = mbAddrC + 1, yM = yN;
      else if (mbIsTopMbFlag == 0)
        mbAddrX = -2, mbAddrN_type = MB_ADDR_TYPE_UNKOWN, mbAddrN = -2, yM = NA;
    } else {
      if (mbIsTopMbFlag) {
        mbAddrX = mbAddrC;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag = (m_mbs[mbAddrX].mb_field_decoding_flag) ? 0 : 1;
          if (mbAddrXFrameFlag)
            mbAddrN_type = MB_ADDR_TYPE_mbAddrC_add_1, mbAddrN = mbAddrC + 1,
            yM = 2 * yN;
          else
            mbAddrN_type = MB_ADDR_TYPE_mbAddrC, mbAddrN = mbAddrC, yM = yN;
        }
      } else if (mbIsTopMbFlag == 0) {
        mbAddrX = mbAddrC, mbAddrN_type = MB_ADDR_TYPE_mbAddrC_add_1,
        mbAddrN = mbAddrC + 1, yM = yN;
      }
    }
  } else {
    // not available
  }
  if (mbAddrN < 0) mbAddrN_type = MB_ADDR_TYPE_UNKOWN;

  if (mbAddrN_type != MB_ADDR_TYPE_UNKOWN) {
    xW = (xN + maxW) % maxW;
    yW = (yM + maxH) % maxH;

    // 6.4.13.2 Derivation process for 4x4 chroma block indices
    if (isChroma == 1) b4x4BlkIdxN = 2 * (yW / 4) + (xW / 4);
    // 6.4.13.1 Derivation process for 4x4 luma block indices
    else
      b4x4BlkIdxN =
          8 * (yW / 8) + 4 * (xW / 8) + 2 * ((yW % 8) / 4) + ((xW % 8) / 4);

    b8x8BlkIdxN = 2 * (yW / 8) + (xW / 8);
  } else
    b4x4BlkIdxN = b8x8BlkIdxN = NA;

  return 0;
}

// 6.4.12.1 Specification for neighbouring locations in fields and non-MBAFF frames
int PictureBase::neighbouring_locations_non_MBAFF(
    int32_t xN, int32_t yN, int32_t maxW, int32_t maxH, int32_t CurrMbAddr,
    MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN, int32_t &b4x4BlkIdx,
    int32_t &b8x8BlkIdxN, int32_t &xW, int32_t &yW, int32_t isChroma) {

  mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
  mbAddrN = -1;

  /* 第 6.4.9 节中相邻宏块地址及其可用性的推导过程是通过 mbAddrA、mbAddrB、mbAddrC 和 mbAddrD 以及它们的可用性状态作为输出来调用的。
   * Table 6-3 specifies mbAddrN depending on ( xN, yN ). */
  derivation_for_neighbouring_macroblock_addr_availability(
      xN, yN, maxW, maxH, CurrMbAddr, mbAddrN_type, mbAddrN);

  if (mbAddrN_type != MB_ADDR_TYPE_UNKOWN) {
    /* 相对于宏块 mbAddrN 左上角的相邻位置 ( xW, yW )  */
    xW = (xN + maxW) % maxW;
    yW = (yN + maxH) % maxH;

    /* For 4x4 Block */
    if (!isChroma)
      // 6.4.13.1 Derivation process for 4x4 luma block indices
      b4x4BlkIdx =
          8 * (yW / 8) + 4 * (xW / 8) + 2 * ((yW % 8) / 4) + ((xW % 8) / 4);
    else
      // 6.4.13.2 Derivation process for 4x4 chroma block indices
      b4x4BlkIdx = 2 * (yW / 4) + (xW / 4);

    /* For 8x8 Block */
    // 6.4.13.3 Derivation process for 8x8 luma block indices
    b8x8BlkIdxN = 2 * (yW / 8) + (xW / 8);
  } else
    b4x4BlkIdx = b8x8BlkIdxN = -1;

  return 0;
}

// 8.5.11 Scaling and transformation process for chroma DC transform coefficients
int PictureBase::scaling_and_transform_for_chroma_DC(int32_t isChromaCb,
                                                     int32_t c[4][2],
                                                     int32_t nW, int32_t nH,
                                                     int32_t (&dcC)[4][2]) {
  // 8.5.8 Derivation process for chroma quantisation parameters
  RET(derivation_chroma_quantisation_parameters(isChromaCb));
  int32_t qP =
      (isChromaCb == 1) ? m_mbs[CurrMbAddr].QP1Cb : m_mbs[CurrMbAddr].QP1Cr;

  if (m_mbs[CurrMbAddr].TransformBypassModeFlag) {
    for (int32_t i = 0; i < MbWidthC / 4; i++)
      for (int32_t j = 0; j < MbHeightC / 4; j++)
        dcC[i][j] = c[i][j];
  } else {
    //YUV420 的逆变换与反量化
    if (nW == 2 && nH == 2) {
      // 8.5.11.1 Transformation process for chroma DC transform coefficients
      int32_t f[2][2] = {{0}};
      int32_t e00 = c[0][0] + c[1][0];
      int32_t e01 = c[0][1] + c[1][1];
      int32_t e10 = c[0][0] - c[1][0];
      int32_t e11 = c[0][1] - c[1][1];
      f[0][0] = e00 + e01;
      f[0][1] = e00 - e01;
      f[1][0] = e10 + e11;
      f[1][1] = e10 - e11;

      // 8.5.11.2 Scaling process for chroma DC transform coefficientsu
      for (int32_t i = 0; i < 2; i++)
        for (int32_t j = 0; j < 2; j++)
          dcC[i][j] =
              ((f[i][j] * LevelScale4x4[qP % 6][0][0]) << (qP / 6)) >> 5;
    }

    //YUV422 的逆变换与反量化
    else if (nW == 2 && nH == 4) {
      // 8.5.11.1 Transformation process for chroma DC transform coefficients
      int32_t f[4][2] = {{0}};
      int32_t e00 = c[0][0] + c[1][0] + c[2][0] + c[3][0];
      int32_t e01 = c[0][1] + c[1][1] + c[2][1] + c[3][1];
      int32_t e10 = c[0][0] + c[1][0] - c[2][0] - c[3][0];
      int32_t e11 = c[0][1] + c[1][1] - c[2][1] - c[3][1];
      int32_t e20 = c[0][0] - c[1][0] - c[2][0] + c[3][0];
      int32_t e21 = c[0][1] - c[1][1] - c[2][1] + c[3][1];
      int32_t e30 = c[0][0] - c[1][0] + c[2][0] - c[3][0];
      int32_t e31 = c[0][1] - c[1][1] + c[2][1] - c[3][1];
      f[0][0] = e00 + e01;
      f[0][1] = e00 - e01;
      f[1][0] = e10 + e11;
      f[1][1] = e10 - e11;
      f[2][0] = e20 + e21;
      f[2][1] = e20 - e21;
      f[3][0] = e30 + e31;
      f[3][1] = e30 - e31;

      // 8.5.11.2 Scaling process for chroma DC transform coefficients
      int32_t qP_DC = qP + 3;
      for (int32_t i = 0; i < 4; i++)
        for (int32_t j = 0; j < 2; j++)
          if (qP_DC >= 36)
            dcC[i][j] = (f[i][j] * LevelScale4x4[qP_DC % 6][0][0])
                        << (qP_DC / 6 - 6);
          else
            dcC[i][j] = (f[i][j] * LevelScale4x4[qP_DC % 6][0][0] +
                         h264_power2(5 - qP_DC / 6)) >>
                        (6 - qP / 6);
    }
  }

  return 0;
}

// 8.5.12 Scaling and transformation process for residual 4x4 blocks
/* 输入: 具有元素cij的4x4数组c，cij是与亮度分量的残差块相关的数组或与色度分量的残差块相关的数组。
 * 输出: 剩余样本值，为 4x4 数组 r，元素为 rij。*/
/* 该函数包括了反量化、反整数变换（类IDCT变换）操作 */
int PictureBase::scaling_and_transform_for_residual_4x4_blocks(
    int32_t c[4][4], int32_t (&r)[4][4], int32_t isChroma, int32_t isChromaCb) {

  const uint32_t slice_type = m_slice->slice_header->slice_type % 5;
  const MacroBlock &mb = m_mbs[CurrMbAddr];

  /* 场景切换标志，需要特殊处理量化值 */
  bool sMbFlag = false;
  if (slice_type == SLICE_SI ||
      (slice_type == SLICE_SP && IS_INTER_Prediction_Mode(mb.m_mb_pred_mode)))
    sMbFlag = true;

  /* 为色差块计算量化参数 */
  RET(derivation_chroma_quantisation_parameters(isChromaCb));

  int32_t qP = 0;
  //对于亮度块，根据是否为场景切换，使用对应的量化值
  if (isChroma == 0 && sMbFlag == false)
    qP = mb.QP1Y;
  else if (isChroma == 0 && sMbFlag)
    qP = mb.QSY;
  //对于色度块，根据是否为场景切换，使用对应的量化值
  else if (isChroma && sMbFlag == false)
    qP = isChromaCb ? mb.QP1Cb : mb.QP1Cr;
  else if (isChroma && sMbFlag)
    qP = isChromaCb ? mb.QSCb : mb.QSCr;

  //变换旁路模式，即不进行任何变换或缩放处理
  if (mb.TransformBypassModeFlag) {
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        r[i][j] = c[i][j];
  } else {
    // 8.5.12.1 Scaling process for residual 4x4 blocks (反量化)
    int32_t d[4][4] = {{0}};
    scaling_for_residual_4x4_blocks(d, c, isChroma, mb.m_mb_pred_mode, qP);

    // 8.5.12.2 Transformation process for residual 4x4 blocks （反整数变换）
    transform_decoding_for_residual_4x4_blocks(d, r);
    /* TODO YangJing 量化和变换需要再了解下理论知识 <24-10-03 22:32:49> */
  }

  return 0;
}

// 8.5.10 Scaling and transformation process for DC transform coefficients for Intra_16x16 macroblock type
// 对于DC系数矩阵需要进行第二层的逆变换、反量化
int PictureBase::scaling_and_transform_for_DC_Intra16x16(int32_t bitDepth,
                                                         int32_t qP,
                                                         int32_t c[4][4],
                                                         int32_t (&dcY)[4][4]) {

  if (m_mbs[CurrMbAddr].TransformBypassModeFlag) {
    for (int32_t i = 0; i < 4; i++)
      for (int32_t j = 0; j < 4; j++)
        dcY[i][j] = c[i][j];
  } else {
    int32_t f[4][4] = {{0}}, g[4][4] = {{0}};
    /* 行变换 */
    for (int32_t i = 0; i < 4; ++i) {
      g[0][i] = c[0][i] + c[1][i] + c[2][i] + c[3][i];
      g[1][i] = c[0][i] + c[1][i] - c[2][i] - c[3][i];
      g[2][i] = c[0][i] - c[1][i] - c[2][i] + c[3][i];
      g[3][i] = c[0][i] - c[1][i] + c[2][i] - c[3][i];
    }

    /* 列变换：同理行变换 */
    for (int32_t j = 0; j < 4; ++j) {
      f[j][0] = g[j][0] + g[j][1] + g[j][2] + g[j][3];
      f[j][1] = g[j][0] + g[j][1] - g[j][2] - g[j][3];
      f[j][2] = g[j][0] - g[j][1] - g[j][2] + g[j][3];
      f[j][3] = g[j][0] - g[j][1] + g[j][2] - g[j][3];
    }

    /* 反量化 */
    for (int32_t i = 0; i < 4; i++)
      for (int32_t j = 0; j < 4; j++) {
        if (qP >= 36)
          dcY[i][j] = (f[i][j] * LevelScale4x4[qP % 6][0][0]) << (qP / 6 - 6);
        else
          dcY[i][j] =
              (f[i][j] * LevelScale4x4[qP % 6][0][0] + (1 << (5 - qP / 6))) >>
              (6 - qP / 6);
      }
  }

  return 0;
}

//8.5.12.1 Scaling process for residual 4x4 blocks
/* 输入：
 * – 变量 bitDepth 和 qP
 * – 具有元素 cij 的 4x4 数组 c，它是与亮度分量的残差块相关的数组或与色度分量的残差块相关的数组。
 * 输出: 缩放变换系数 d 的 4x4 数组，其中元素为 dij。 */
// 对残差宏块进行反量化操作
int PictureBase::scaling_for_residual_4x4_blocks(
    int32_t d[4][4], int32_t c[4][4], int32_t isChroma,
    const H264_MB_PART_PRED_MODE &m_mb_pred_mode, int32_t qP) {

  /* 比特流不应包含导致 c 的任何元素 cij 的数据，其中 i, j = 0..3 超出从 −2(7 + bitDepth) 到 2(7 + bitDepth) − 1（含）的整数值范围。 */
  for (int32_t i = 0; i < 4; i++) {
    for (int32_t j = 0; j < 4; j++) {
      //对于Intra_16x16模式下的亮度残差块（DC系数已经经过了反量化处理），色度残差块的4x4残差块的首个样本（DC系数已经经过了反量化处理）直接进行复制
      if (i == 0 && j == 0 &&
          ((isChroma == 0 && m_mb_pred_mode == Intra_16x16) || isChroma))
        d[0][0] = c[0][0];
      else {
        if (qP >= 24)
          d[i][j] = (c[i][j] * LevelScale4x4[qP % 6][i][j]) << (qP / 6 - 4);
        else
          d[i][j] = (c[i][j] * LevelScale4x4[qP % 6][i][j] +
                     h264_power2(3 - qP / 6)) >>
                    (4 - qP / 6);
      }
    }
  }
  return 0;
}

int PictureBase::scaling_for_residual_8x8_blocks(
    int32_t d[8][8], int32_t c[8][8], int32_t isChroma,
    const H264_MB_PART_PRED_MODE &m_mb_pred_mode, int32_t qP) {
  for (int32_t i = 0; i < 8; i++) {
    for (int32_t j = 0; j < 8; j++) {
      if (qP >= 36)
        d[i][j] = (c[i][j] * LevelScale8x8[qP % 6][i][j]) << (qP / 6 - 6);
      else
        d[i][j] =
            (c[i][j] * LevelScale8x8[qP % 6][i][j] + h264_power2(5 - qP / 6)) >>
            (6 - qP / 6);
    }
  }
  return 0;
}

// 8.5.13 Scaling and transformation process for residual 8x8 blocks
int PictureBase::scaling_and_transform_for_residual_8x8_blocks(
    int32_t c[8][8], int32_t (&r)[8][8], int32_t isChroma, int32_t isChromaCb) {

  const MacroBlock &mb = m_mbs[CurrMbAddr];

  //对于亮度块，使用对应的量化值
  int32_t qP = 0;
  if (isChroma == 0) qP = mb.QP1Y;
  //对于色度块，使用对应的量化值
  else {
    if (isChromaCb == 1)
      qP = mb.QP1Cb;
    else if (isChromaCb == 0)
      qP = mb.QP1Cr;
  }

  //变换旁路模式，即不进行任何变换或缩放处理
  if (mb.TransformBypassModeFlag) {
    for (int32_t i = 0; i < 8; i++)
      for (int32_t j = 0; j < 8; j++)
        r[i][j] = c[i][j];
  } else {
    // 8.5.13.1 Scaling process for residual 8x8 blocks (反量化)
    int32_t d[8][8] = {{0}};
    scaling_for_residual_8x8_blocks(d, c, isChroma, mb.m_mb_pred_mode, qP);

    // 8.5.13.2 Transformation process for residual 8x8 blocks
    transform_decoding_for_residual_8x8_blocks(d, r);
  }

  return 0;
}

// 8.5.9 Derivation process for scaling functions
/*  输出为：
 *  – LevelScale4x4：4x4 块变换亮度或色度系数级别的缩放因子;
 *  – LevelScale8x8：8x8 块变换亮度或色度系数级别的缩放因子; */
int PictureBase::scaling_functions(int32_t isChroma, int32_t isChromaCb) {

  /* -------------- 设置别名，初始化变量 -------------- */
  MacroBlock &mb = m_mbs[CurrMbAddr];
  bool mbIsInterFlag = !IS_INTRA_Prediction_Mode(mb.m_mb_pred_mode);

  int32_t iYCbCr = (!isChroma) ? 0 : (isChroma == 1 && isChromaCb == 1) ? 1 : 2;
  //YUV444
  if (m_slice->slice_header->m_sps->separate_colour_plane_flag)
    iYCbCr = m_slice->slice_header->colour_plane_id;

  const uint32_t *ScalingList4x4 =
      m_slice->slice_header->ScalingList4x4[iYCbCr + (mbIsInterFlag ? 3 : 0)];
  const uint32_t *ScalingList8x8 =
      m_slice->slice_header->ScalingList8x8[2 * iYCbCr + mbIsInterFlag];
  /* ------------------  End ------------------ */

  //------------------------ 4x4 缩放矩阵 ----------------------------
  int32_t weightScale4x4[4][4] = {{0}};
  RET(inverse_scanning_for_4x4_transform_coeff_and_scaling_lists(
      (int32_t *)ScalingList4x4, weightScale4x4,
      mb.field_pic_flag | mb.mb_field_decoding_flag));

  /* 其中 v 的第一个和第二个下标分别是矩阵的行索引和列索引 */
  int32_t v4x4[6][3] = {{10, 16, 13}, {11, 18, 14}, {13, 20, 16},
                        {14, 23, 18}, {16, 25, 20}, {18, 29, 23}};
  /* m[0-5]分别表示帧内、帧间预测三个分量 */
  for (int m = 0; m < 6; m++)
    for (int m = 0; m < 6; m++)
      for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
          if (i % 2 == 0 && j % 2 == 0)
            LevelScale4x4[m][i][j] = weightScale4x4[i][j] * v4x4[m][0];
          else if (i % 2 == 1 && j % 2 == 1)
            LevelScale4x4[m][i][j] = weightScale4x4[i][j] * v4x4[m][1];
          else
            LevelScale4x4[m][i][j] = weightScale4x4[i][j] * v4x4[m][2];
        }

  //------------------------ 8x8 缩放矩阵 ----------------------------
  int32_t weightScale8x8[8][8] = {{0}};
  RET(inverse_scanning_for_8x8_transform_coeff_and_scaling_lists(
      (int32_t *)ScalingList8x8, weightScale8x8,
      mb.field_pic_flag | mb.mb_field_decoding_flag));

  /* 其中 v 的第一个和第二个下标分别是矩阵的行索引和列索引 */
  int32_t v8x8[6][6] = {
      {20, 18, 32, 19, 25, 24}, {22, 19, 35, 21, 28, 26},
      {26, 23, 42, 24, 33, 31}, {28, 25, 45, 26, 35, 33},
      {32, 28, 51, 30, 40, 38}, {36, 32, 58, 34, 46, 43},
  };
  /* m[0-5]分别表示帧内、帧间预测三个分量 */
  for (int m = 0; m < 6; m++)
    for (int i = 0; i < 8; i++)
      for (int j = 0; j < 8; j++) {
        if (i % 4 == 0 && j % 4 == 0)
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][0];
        else if (i % 2 == 1 && j % 2 == 1)
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][1];
        else if (i % 4 == 2 && j % 4 == 2)
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][2];
        else if ((i % 4 == 0 && j % 2 == 1) || (i % 2 == 1 && j % 4 == 0))
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][3];
        else if ((i % 4 == 0 && j % 4 == 2) || (i % 4 == 2 && j % 4 == 0))
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][4];
        else
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][5];
      }

  return 0;
}

// 8.5.14 Picture construction process prior to deblocking filter process (去块过滤过程之前的图片构造过程)
// 重建图像数据，写入最终数据到pic_buff，主要针对帧、场编码的不同情况
/* 输入：– 包含元素 uij 的样本数组 u，它是 16x16 亮度块或 (MbWidthC)x(MbHeightC) 色度块或 4x4 亮度块或 4x4 色度块或 8x8 亮度块，或者，当 ChromaArrayType等于 3，8x8 色度块，
 * – 当 u 不是 16x16 亮度块或 (MbWidthC)x(MbHeightC) 色度块时，块索引 luma4x4BlkIdx 或 chroma4x4BlkIdx 或 luma8x8BlkIdx 或 cb4x4BlkIdx 或 cr4x4BlkIdx 或 cb8x8BlkIdx 或idx。*/
int PictureBase::picture_construction_process_prior_to_deblocking_filter(
    int32_t *u, int32_t nW, int32_t nH, int32_t BlkIdx, int32_t isChroma,
    int32_t PicWidthInSamples, uint8_t *pic_buff) {

  /* ------------------ 设置别名 ------------------ */
  const int32_t mb_field_decoding_flag =
      m_mbs[CurrMbAddr].mb_field_decoding_flag;

  const SliceHeader *header = m_slice->slice_header;
  const bool MbaffFrameFlag = header->MbaffFrameFlag;
  const uint32_t ChromaArrayType = header->m_sps->ChromaArrayType;
  const int32_t SubWidthC = header->m_sps->SubWidthC;
  const int32_t SubHeightC = header->m_sps->SubHeightC;
  bool isMbAff = header->MbaffFrameFlag && mb_field_decoding_flag;
  /* ------------------  End ------------------ */

  int32_t xP = 0, yP = 0;
  inverse_mb_scanning_process(MbaffFrameFlag, CurrMbAddr,
                              mb_field_decoding_flag, xP, yP);

  /* 当 u 是亮度块时，对于亮度块的每个样本 uij，指定以下有序步骤：*/
  if (isChroma == 0) {
    int32_t xO = 0, yO = 0, nE = 16;
    if (nW == 16 && nH == 16) {
    } else if (nW == 4 && nH == 4) {
      // 6.4.3 Inverse 4x4 luma block scanning process luma4x4BlkIdx
      xO = InverseRasterScan(BlkIdx / 4, 8, 8, 16, 0) +
           InverseRasterScan(BlkIdx % 4, 4, 4, 8, 0);
      yO = InverseRasterScan(BlkIdx / 4, 8, 8, 16, 1) +
           InverseRasterScan(BlkIdx % 4, 4, 4, 8, 1);
      nE = 4;
    } else {
      // 6.4.5 Inverse 8x8 luma block scanning process
      xO = InverseRasterScan(BlkIdx, 8, 8, 16, 0);
      yO = InverseRasterScan(BlkIdx, 8, 8, 16, 1);
      nE = 8;
    }

    int32_t n = (isMbAff) ? 2 : 1;
    for (int32_t i = 0; i < nE; i++)
      for (int32_t j = 0; j < nE; j++) {
        int32_t y = yP + n * (yO + i);
        int32_t x = xP + xO + j;
        pic_buff[y * PicWidthInSamples + x] = u[i * nE + j];
      }
  }

  /* 当 u 是色度块时，对于色度块的每个样本 uij，指定以下有序步骤：*/
  else if (isChroma) {
    int32_t xO = 0, yO = 0;
    if (nW == MbWidthC && nH == MbHeightC) {
    } else if (nW == 4 && nH == 4) {
      if (ChromaArrayType == 1 || ChromaArrayType == 2) {
        // 6.4.7 Inverse 4x4 chroma block scanning process chroma4x4BlkIdx
        xO = InverseRasterScan(BlkIdx, 4, 4, 8, 0);
        yO = InverseRasterScan(BlkIdx, 4, 4, 8, 1);
      } else {
        // 6.4.3 Inverse 4x4 luma block scanning process
        xO = InverseRasterScan(BlkIdx / 4, 8, 8, 16, 0) +
             InverseRasterScan(BlkIdx % 4, 4, 4, 8, 0);
        yO = InverseRasterScan(BlkIdx / 4, 8, 8, 16, 1) +
             InverseRasterScan(BlkIdx % 4, 4, 4, 8, 1);
      }
    } else if (ChromaArrayType == 3 && nW == 8 && nH == 8) {
      // 6.4.5 Inverse 8x8 luma block scanning process luma8x8BlkIdx
      xO = InverseRasterScan(BlkIdx, 8, 8, 16, 0);
      yO = InverseRasterScan(BlkIdx, 8, 8, 16, 1);
    }

    for (int32_t i = 0; i < nH; i++)
      for (int32_t j = 0; j < nW; j++) {
        int32_t x = (xP / SubWidthC) + xO + j;

        int32_t y;
        if (isMbAff)
          y = ((yP + SubHeightC - 1) / SubHeightC) + 2 * (yO + i);
        else
          y = (yP / SubHeightC) + yO + i;

        pic_buff[y * PicWidthInSamples + x] = u[i * nW + j];
      }
  }

  return 0;
}
