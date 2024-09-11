#include "PictureBase.hpp"
#include "Bitmap.hpp"
#include "Frame.hpp"
#include "MacroBlock.hpp"
#include "SliceHeader.hpp"
#include "Type.hpp"
#include <cstdint>

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

int PictureBase::printInfo() {
  printf("---------Picture info------------\n");

  return 0;
}

int PictureBase::reset() {
  //----------------------
  if (m_mbs) {
    memset(m_mbs, 0, sizeof(MacroBlock) * PicSizeInMbs);
  }

  if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_FRAME) {
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
  reference_marked_type = H264_PICTURE_MARKED_AS_unkown;
  m_picture_coded_type = H264_PICTURE_CODED_TYPE_UNKNOWN;
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

int PictureBase::init(Slice &slice) {
  this->m_slice = slice;

  MbWidthL = MB_WIDTH;   // 16
  MbHeightL = MB_HEIGHT; // 16
  MbWidthC = m_slice.m_sps.MbWidthC;
  MbHeightC = m_slice.m_sps.MbHeightC;
  Chroma_Format = m_slice.m_sps.Chroma_Format;

  PicWidthInMbs = m_slice.m_sps.PicWidthInMbs;
  PicHeightInMbs = m_slice.slice_header.PicHeightInMbs;
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
  if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_FRAME) {
    m_mbs = (MacroBlock *)my_malloc(
        sizeof(MacroBlock) *
        PicSizeInMbs); // 因为MacroBlock构造函数中，有对变量初始化，可以考虑使用C++/new申请内存，此处使用C/my_malloc
    RETURN_IF_FAILED(m_mbs == NULL, -1);
    memset(m_mbs, 0, sizeof(MacroBlock) * PicSizeInMbs);

    //-----------YUV420P-----------------
    int sizeY = PicWidthInSamplesL * PicHeightInSamplesL;
    int sizeU = PicWidthInSamplesC * PicHeightInSamplesC;
    int sizeV = PicWidthInSamplesC * PicHeightInSamplesC;

    int totalSzie = sizeY + sizeU + sizeV;

    uint8_t *pic_buff = (uint8_t *)my_malloc(
        sizeof(uint8_t) *
        totalSzie); // Y,U,V 这3个通道数据存储在一块连续的内存中
    RETURN_IF_FAILED(pic_buff == NULL, -1);
    memset(pic_buff, 0, sizeof(uint8_t) * totalSzie);

    m_pic_buff_luma = pic_buff;
    m_pic_buff_cb = m_pic_buff_luma + sizeY;
    m_pic_buff_cr = m_pic_buff_cb + sizeU;

    m_is_malloc_mem_by_myself = 1;
  } else // if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD ||
         // m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD)
  {
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
    MbWidthC = m_slice.m_sps.MbWidthC;
    MbHeightC = m_slice.m_sps.MbHeightC;
    Chroma_Format = m_slice.m_sps.Chroma_Format;

    PicWidthInMbs = m_slice.m_sps.PicWidthInMbs;
    PicHeightInMbs = m_slice.slice_header.PicHeightInMbs /
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

    if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_TOP_FIELD) {
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
    SAFE_FREE(m_mbs);
    SAFE_FREE(m_pic_buff_luma);
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
    ret = init((Slice &)src.m_slice);
    RETURN_IF_FAILED(ret, -1);

    memcpy(m_mbs, src.m_mbs, sizeof(MacroBlock) * PicSizeInMbs);

    memcpy(m_pic_buff_luma, src.m_pic_buff_luma,
           sizeof(uint8_t) * PicWidthInSamplesL * PicHeightInSamplesL);
    memcpy(m_pic_buff_cb, src.m_pic_buff_cb,
           sizeof(uint8_t) * PicWidthInSamplesC * PicHeightInSamplesC);
    memcpy(m_pic_buff_cr, src.m_pic_buff_cr,
           sizeof(uint8_t) * PicWidthInSamplesC * PicHeightInSamplesC);
  } else {
    SliceHeader &slice_header = (SliceHeader &)src.m_slice.slice_header;

    MbWidthL = MB_WIDTH;   // 16
    MbHeightL = MB_HEIGHT; // 16
    MbWidthC = m_slice.m_sps.MbWidthC;
    MbHeightC = m_slice.m_sps.MbHeightC;
    Chroma_Format = m_slice.m_sps.Chroma_Format;

    PicWidthInMbs = m_slice.m_sps.PicWidthInMbs;
    PicHeightInMbs = slice_header.PicHeightInMbs;
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
    memcpy(m_mbs, src.m_mbs, sizeof(MacroBlock) * PicSizeInMbs);
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
  int ret = 0;

  int32_t W = width;
  int32_t H = height;
  int32_t channels = 3;

  //------------- YUV420P to BGR24 --------------------
  for (int y = 0; y < H; ++y) // 可以在此处进行
  // m_slice.m_sps.frame_crop_[left,right,top,bottom]_offset
  {
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

  return ret;
}

int PictureBase::convertYuv420pToBgr24FlipLines(uint32_t width, uint32_t height,
                                                const uint8_t *yuv420p,
                                                uint8_t *bgr24,
                                                uint32_t widthBytesBgr24) {
  int ret = 0;

  int32_t W = width;
  int32_t H = height;
  int32_t channels = 3;

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

  return ret;
}

int PictureBase::createEmptyImage(MY_BITMAP &bitmap, int32_t width,
                                  int32_t height, int32_t bmBitsPixel) {
  bitmap.bmWidth = width;
  bitmap.bmHeight = height;
  bitmap.bmBitsPixel = bmBitsPixel; // 32

  bitmap.bmType = 0;
  bitmap.bmPlanes = 1;

  bitmap.bmWidthBytes = (width * bmBitsPixel / 8 + 3) / 4 * 4;

  uint8_t *pBits =
      (uint8_t *)my_malloc(bitmap.bmHeight * bitmap.bmWidthBytes); // 在堆上申请
  if (pBits == NULL) {
    printf("CreateEmptyImage: pBits == NULL\n");
    return -1;
  }
  memset(pBits, 0,
         sizeof(uint8_t) * bitmap.bmHeight *
             bitmap.bmWidthBytes); // 初始化为黑色背景

  bitmap.bmBits = pBits;

  return 0;
}

int PictureBase::saveToBmpFile(const char *filename) {
  int ret = 0;
  //----------------yuv420p到brg24的格式转换-------------------------
  int32_t W = PicWidthInSamplesL;
  int32_t H = PicHeightInSamplesL;

  MY_BITMAP bitmap;
  ret = createEmptyImage(bitmap, W, H, 24);

  ret = convertYuv420pToBgr24(W, H, m_pic_buff_luma, (uint8_t *)bitmap.bmBits,
                              bitmap.bmWidthBytes);
  if (ret != 0) {
    return -1;
  }

  ret = saveBmp(filename, &bitmap);
  if (ret != 0) {
    return -1;
  }

  my_free(bitmap.bmBits);
  bitmap.bmBits = NULL;

  return 0;
}

int PictureBase::saveBmp(const char *filename, MY_BITMAP *pBitmap) {

  int ret = 0;

  MY_BitmapFileHeader bmpFileHeader;
  MY_BitmapInfoHeader bmpInfoHeader;
  // unsigned char pixVal = '\0';
  MY_RgbQuad quad[256] = {{0}};

  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    return -1;
  }

  unsigned short fileType = 0x4D42;
  fwrite(&fileType, sizeof(unsigned short), 1, fp);

  if (pBitmap->bmBitsPixel == 24 ||
      pBitmap->bmBitsPixel == 32) // 24位，通道，彩图
  {
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

    for (int i = pBitmap->bmHeight - 1; i > -1; i--) {
      fwrite(pBits + i * rowbytes, rowbytes, 1, fp);
    }
  } else if (pBitmap->bmBitsPixel == 8) // 8位，单通道，灰度图
  {
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

    for (int i = pBitmap->bmHeight - 1; i > -1; i--) {
      fwrite(pBits + i * rowbytes, rowbytes, 1, fp);
    }
  }

  fclose(fp);

  return ret;
}

/* 所有解码的帧写入到一个文件 */
int PictureBase::writeYUV(const char *filename) {
  int ret = 0;

  FILE *fp = fopen(filename, "ab+");
  if (fp == NULL) {
    return -1;
  }

  fwrite(m_pic_buff_luma, PicWidthInSamplesL * PicHeightInSamplesL, 1, fp);
  fwrite(m_pic_buff_cb, PicWidthInSamplesC * PicHeightInSamplesC, 1, fp);
  fwrite(m_pic_buff_cr, PicWidthInSamplesC * PicHeightInSamplesC, 1, fp);

  fclose(fp);

  return ret;
}

int PictureBase::getOneEmptyPicture(Frame *&pic) {
  int32_t size_dpb = H264_MAX_DECODED_PICTURE_BUFFER_COUNT;

  for (int i = 0; i < size_dpb; i++) {
    if (m_dpb[i] != this->m_parent &&
        m_dpb[i]->reference_marked_type !=
            H264_PICTURE_MARKED_AS_used_for_short_term_reference &&
        m_dpb[i]->reference_marked_type !=
            H264_PICTURE_MARKED_AS_used_for_long_term_reference &&
        m_dpb[i]->m_is_in_use == 0 // 本帧数据未使用，即处于闲置状态
        )                          // 重复利用被释放了的参考帧
    {
      pic = m_dpb[i];
      RETURN_IF_FAILED(pic == NULL, -1);
      return 0;
    }
  }

  return -1;
}

int PictureBase::end_decode_the_picture_and_get_a_new_empty_picture(
    Frame *&newEmptyPicture) {
  int ret = 0;

  this->m_is_decode_finished = 1;
  if (m_picture_coded_type == H264_PICTURE_CODED_TYPE_FRAME ||
      m_picture_coded_type == H264_PICTURE_CODED_TYPE_BOTTOM_FIELD) {
    this->m_parent->m_is_decode_finished = 1;
  }

  //    char filename[600] = {0};
  //    sprintf(filename, "./out_%dx%d.%d.no_loop_filter.yuv",
  //    PicWidthInSamplesL, PicHeightInSamplesL, m_PicNumCnt); ret =
  //    writeYUV(filename); //将环路滤波前的数据保存到磁盘

  //--------环路滤波------------
  ret = this->Deblocking_filter_process();
  // RETURN_IF_FAILED(ret != 0, ret); //环路滤波在码流有问题时，基本上会返回失败

  //    sprintf(filename, "./out_%dx%d.%d.yuv", PicWidthInSamplesL,
  //    PicHeightInSamplesL, m_PicNumCnt); ret = writeYUV(filename);
  //    //将解码后的数据保存到磁盘

  //--------标记图像参考列表------------
  // When the current picture is a reference picture and after all slices of the
  // current picture have been decoded, the decoded reference picture marking
  // process in clause 8.2.5 specifies how the current picture is used in the
  // decoding process of inter prediction in later decoded pictures.
  if (m_slice.slice_header.nal_ref_idc != 0) {
    // 8.2.5 Decoded reference picture marking process
    ret = Decoded_reference_picture_marking_process(m_dpb);
    RETURN_IF_FAILED(ret != 0, ret);

    // When the current picture includes a memory_management_control_operation
    // equal to 5, after the decoding of the current picture, tempPicOrderCnt is
    // set equal to PicOrderCnt( CurrPic ), TopFieldOrderCnt of the current
    // picture (if any) is set equal to TopFieldOrderCnt − tempPicOrderCnt, and
    // BottomFieldOrderCnt of the current picture (if any) is set equal to
    // BottomFieldOrderCnt − tempPicOrderCnt.
    if (memory_management_control_operation_5_flag == 1) {
      int32_t tempPicOrderCnt = PicOrderCnt; // PicOrderCntFunc(this);
      TopFieldOrderCnt = TopFieldOrderCnt - tempPicOrderCnt;
      BottomFieldOrderCnt = BottomFieldOrderCnt - tempPicOrderCnt;
    }
  }

  //--------------------------------------
  Frame *emptyPic = NULL;
  ret = getOneEmptyPicture(emptyPic);
  RETURN_IF_FAILED(ret != 0, ret);

  ret = emptyPic->reset();                        // 重置各个变量的值
  ret = emptyPic->m_picture_frame.reset();        // 重置各个变量的值
  ret = emptyPic->m_picture_top_filed.reset();    // 重置各个变量的值
  ret = emptyPic->m_picture_bottom_filed.reset(); // 重置各个变量的值

  emptyPic->m_picture_previous = this;

  if (reference_marked_type ==
          H264_PICTURE_MARKED_AS_used_for_short_term_reference ||
      reference_marked_type ==
          H264_PICTURE_MARKED_AS_used_for_long_term_reference) {
    emptyPic->m_picture_previous_ref = this;
  } else {
    emptyPic->m_picture_previous_ref = this->m_parent->m_picture_previous_ref;
  }

  g_PicNumCnt++;

  emptyPic->m_picture_frame.m_PicNumCnt = g_PicNumCnt;
  emptyPic->m_picture_top_filed.m_PicNumCnt = g_PicNumCnt;
  emptyPic->m_picture_bottom_filed.m_PicNumCnt = g_PicNumCnt;

  //----------------------------
  newEmptyPicture = emptyPic;

  return ret;
}

//--------------帧内预测------------------------
// 8.3.1.1 Derivation process for Intra4x4PredMode (8.3.1 Intra_4x4 prediction
// process for luma samples)
int PictureBase::getIntra4x4PredMode(
    int32_t luma4x4BlkIdx,
    int32_t &Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr, int32_t isChroma) {
  int ret = 0;
  int32_t x = 0;
  int32_t y = 0;
  int32_t maxW = 0;
  int32_t maxH = 0;
  int32_t xW = 0;
  int32_t yW = 0;

  MB_ADDR_TYPE mbAddrN_type_A = MB_ADDR_TYPE_UNKOWN;
  MB_ADDR_TYPE mbAddrN_type_B = MB_ADDR_TYPE_UNKOWN;
  int32_t mbAddrN_A = -1;
  int32_t mbAddrN_B = -1;
  int32_t luma4x4BlkIdxN_A = 0;
  int32_t luma4x4BlkIdxN_B = 0;
  int32_t luma8x8BlkIdxN_A = 0;
  int32_t luma8x8BlkIdxN_B = 0;

  //SliceHeader &slice_header = m_h264_slice_header;

  // 6.4.11.4 Derivation process for neighbouring 4x4 luma blocks

  // 6.4.3 Inverse 4x4 luma block scanning process
  // InverseRasterScan = (a % (d / b) ) * b;    if e == 0;
  // InverseRasterScan = (a / (d / b) ) * c;    if e == 1;
  x = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
      InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
  y = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
      InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

  // 6.4.12 Derivation process for neighbouring locations
  if (isChroma == 0) {
    maxW = 16;
    maxH = 16;
  } else // if (isChroma == 1)
  {
    maxW = MbWidthC;
    maxH = MbHeightC;
  }

  if (m_slice.slice_header.MbaffFrameFlag == 0) {
    ret = neighbouring_locations_non_MBAFF(
        x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_type_A, mbAddrN_A,
        luma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma);
    RETURN_IF_FAILED(ret != 0, ret);

    ret = neighbouring_locations_non_MBAFF(
        x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_type_B, mbAddrN_B,
        luma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma);
    RETURN_IF_FAILED(ret != 0, ret);
  } else // if (slice_header.MbaffFrameFlag == 1) //6.4.12.2 Specification for
         // neighbouring locations in MBAFF frames
  {
    ret = neighbouring_locations_MBAFF(
        x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_type_A, mbAddrN_A,
        luma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma);
    RETURN_IF_FAILED(ret != 0, ret);

    ret = neighbouring_locations_MBAFF(
        x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_type_B, mbAddrN_B,
        luma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma);
    RETURN_IF_FAILED(ret != 0, ret);
  }

  //----------------------------------------
  int32_t dcPredModePredictedFlag = 0;

  if (mbAddrN_A < 0 || mbAddrN_B < 0 ||
      (mbAddrN_A >= 0 &&
       IS_INTER_Prediction_Mode(m_mbs[mbAddrN_A].m_mb_pred_mode) &&
       m_slice.m_pps.constrained_intra_pred_flag == 1) ||
      (mbAddrN_B >= 0 &&
       IS_INTER_Prediction_Mode(m_mbs[mbAddrN_B].m_mb_pred_mode) &&
       m_slice.m_pps.constrained_intra_pred_flag == 1)) {
    dcPredModePredictedFlag = 1;
  } else {
    dcPredModePredictedFlag = 0;
  }

  //----------------------------------------
  int32_t intraMxMPredModeA = 0;
  int32_t intraMxMPredModeB = 0;

  if (dcPredModePredictedFlag == 1 ||
      (mbAddrN_A >= 0 && m_mbs[mbAddrN_A].m_mb_pred_mode != Intra_4x4 &&
       m_mbs[mbAddrN_A].m_mb_pred_mode != Intra_8x8)) {
    intraMxMPredModeA =
        Prediction_Mode_Intra_4x4_DC; // Prediction_Mode_Intra_4x4_DC = 2;
  } else {
    if (mbAddrN_A >= 0 && m_mbs[mbAddrN_A].m_mb_pred_mode == Intra_4x4) {
      intraMxMPredModeA = m_mbs[mbAddrN_A].Intra4x4PredMode[luma4x4BlkIdxN_A];
    } else // if (mbAddrN_A >= 0 && m_mbs[mbAddrN_A].m_mb_pred_mode ==
           // Intra_8x8)
    {
      intraMxMPredModeA =
          m_mbs[mbAddrN_A].Intra8x8PredMode[luma4x4BlkIdxN_A >> 2];
    }
  }

  if (dcPredModePredictedFlag == 1 ||
      (mbAddrN_B >= 0 && m_mbs[mbAddrN_B].m_mb_pred_mode != Intra_4x4 &&
       m_mbs[mbAddrN_B].m_mb_pred_mode != Intra_8x8)) {
    intraMxMPredModeB =
        Prediction_Mode_Intra_4x4_DC; // Prediction_Mode_Intra_4x4_DC = 2;
  } else {
    if (mbAddrN_B >= 0 && m_mbs[mbAddrN_B].m_mb_pred_mode == Intra_4x4) {
      intraMxMPredModeB = m_mbs[mbAddrN_B].Intra4x4PredMode[luma4x4BlkIdxN_B];
    } else // if (mbAddrN_B >= 0 && m_mbs[mbAddrN_B].m_mb_pred_mode ==
           // Intra_8x8)
    {
      intraMxMPredModeB =
          m_mbs[mbAddrN_B].Intra8x8PredMode[luma4x4BlkIdxN_B >> 2];
    }
  }

  //-----------------------------
  int32_t predIntra4x4PredMode =
      MIN(intraMxMPredModeA, intraMxMPredModeB); // 取两个相邻宏块中的最小值

  if (m_mbs[CurrMbAddr].prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]) {
    m_mbs[CurrMbAddr].Intra4x4PredMode[luma4x4BlkIdx] =
        predIntra4x4PredMode; // 使用计算出来的值作为当前宏块的预测模式值
  } else {
    if (m_mbs[CurrMbAddr].rem_intra4x4_pred_mode[luma4x4BlkIdx] <
        predIntra4x4PredMode) {
      m_mbs[CurrMbAddr].Intra4x4PredMode[luma4x4BlkIdx] =
          m_mbs[CurrMbAddr].rem_intra4x4_pred_mode
              [luma4x4BlkIdx]; // 使用视频编码器事先设置好的值作为当前宏块的预测模式值
    } else {
      m_mbs[CurrMbAddr].Intra4x4PredMode[luma4x4BlkIdx] =
          m_mbs[CurrMbAddr].rem_intra4x4_pred_mode[luma4x4BlkIdx] +
          1; // 使用视频编码器事先设置好的值作为当前宏块的预测模式值
    }
  }

  Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr =
      m_mbs[CurrMbAddr].Intra4x4PredMode[luma4x4BlkIdx];

  return 0;
}

// 8.3.2.1 Derivation process for Intra8x8PredMode (8.3.2 Intra_8x8 prediction
// process for luma samples)
int PictureBase::getIntra8x8PredMode(
    int32_t luma8x8BlkIdx,
    int32_t &Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr, int32_t isChroma) {
  int ret = 0;
  int32_t x = 0;
  int32_t y = 0;
  int32_t maxW = 0;
  int32_t maxH = 0;
  int32_t xW = 0;
  int32_t yW = 0;

  MB_ADDR_TYPE mbAddrN_type_A = MB_ADDR_TYPE_UNKOWN;
  MB_ADDR_TYPE mbAddrN_type_B = MB_ADDR_TYPE_UNKOWN;
  int32_t mbAddrN_A = -1;
  int32_t mbAddrN_B = -1;
  int32_t luma4x4BlkIdxN_A = 0;
  int32_t luma4x4BlkIdxN_B = 0;
  int32_t luma8x8BlkIdxN_A = 0;
  int32_t luma8x8BlkIdxN_B = 0;

  //SliceHeader &slice_header = m_h264_slice_header;

  // 6.4.11.2 Derivation process for neighbouring 8x8 luma block

  // 6.4.12 Derivation process for neighbouring locations
  if (isChroma == 0) {
    maxW = 16;
    maxH = 16;
  } else // if (isChroma == 1)
  {
    maxW = MbWidthC;
    maxH = MbHeightC;
  }

  x = (luma8x8BlkIdx % 2) * 8;
  y = (luma8x8BlkIdx / 2) * 8;

  if (m_slice.slice_header.MbaffFrameFlag == 0) {
    ret = neighbouring_locations_non_MBAFF(
        x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_type_A, mbAddrN_A,
        luma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma);
    RETURN_IF_FAILED(ret != 0, ret);

    ret = neighbouring_locations_non_MBAFF(
        x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_type_B, mbAddrN_B,
        luma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma);
    RETURN_IF_FAILED(ret != 0, ret);
  } else // if (slice_header.MbaffFrameFlag == 1) //6.4.12.2 Specification for
         // neighbouring locations in MBAFF frames
  {
    ret = neighbouring_locations_MBAFF(
        x - 1, y + 0, maxW, maxH, CurrMbAddr, mbAddrN_type_A, mbAddrN_A,
        luma4x4BlkIdxN_A, luma8x8BlkIdxN_A, xW, yW, isChroma);
    RETURN_IF_FAILED(ret != 0, ret);

    ret = neighbouring_locations_MBAFF(
        x + 0, y - 1, maxW, maxH, CurrMbAddr, mbAddrN_type_B, mbAddrN_B,
        luma4x4BlkIdxN_B, luma8x8BlkIdxN_B, xW, yW, isChroma);
    RETURN_IF_FAILED(ret != 0, ret);
  }

  //----------------------------------------
  int32_t dcPredModePredictedFlag = 0;

  if (mbAddrN_A < 0 || mbAddrN_B < 0 ||
      (mbAddrN_A >= 0 &&
       IS_INTER_Prediction_Mode(m_mbs[mbAddrN_A].m_mb_pred_mode) &&
       m_slice.m_pps.constrained_intra_pred_flag == 1) ||
      (mbAddrN_B >= 0 &&
       IS_INTER_Prediction_Mode(m_mbs[mbAddrN_B].m_mb_pred_mode) &&
       m_slice.m_pps.constrained_intra_pred_flag == 1)) {
    dcPredModePredictedFlag = 1;
  } else {
    dcPredModePredictedFlag = 0;
  }

  //------------------------------------
  int32_t intraMxMPredModeA = 0;
  int32_t intraMxMPredModeB = 0;

  if (dcPredModePredictedFlag == 1 ||
      (mbAddrN_A >= 0 && m_mbs[mbAddrN_A].m_mb_pred_mode != Intra_4x4 &&
       m_mbs[mbAddrN_A].m_mb_pred_mode != Intra_8x8)) {
    intraMxMPredModeA =
        Prediction_Mode_Intra_8x8_DC; // Prediction_Mode_Intra_8x8_DC = 2;
  } else {
    if (mbAddrN_A >= 0 && m_mbs[mbAddrN_A].m_mb_pred_mode == Intra_8x8) {
      intraMxMPredModeA = m_mbs[mbAddrN_A].Intra8x8PredMode[luma8x8BlkIdxN_A];
    } else // if (mbAddrN_A >= 0 && m_mbs[mbAddrN_A].m_mb_pred_mode ==
           // Intra_4x4)
    {
      int32_t n = 0;

      if (m_slice.slice_header.MbaffFrameFlag == 1 &&
          m_mbs[CurrMbAddr].field_pic_flag == 0 &&
          m_mbs[mbAddrN_A].field_pic_flag == 1 && luma8x8BlkIdx == 2) {
        n = 3;
      } else {
        n = 1;
      }

      intraMxMPredModeA =
          m_mbs[mbAddrN_A].Intra4x4PredMode[luma8x8BlkIdxN_A * 4 + n];
    }
  }

  if (dcPredModePredictedFlag == 1 ||
      (mbAddrN_B >= 0 && m_mbs[mbAddrN_B].m_mb_pred_mode != Intra_4x4 &&
       m_mbs[mbAddrN_B].m_mb_pred_mode != Intra_8x8)) {
    intraMxMPredModeB =
        Prediction_Mode_Intra_8x8_DC; // Prediction_Mode_Intra_8x8_DC = 2;
  } else {
    if (mbAddrN_B >= 0 && m_mbs[mbAddrN_B].m_mb_pred_mode == Intra_8x8) {
      intraMxMPredModeB = m_mbs[mbAddrN_B].Intra8x8PredMode[luma8x8BlkIdxN_B];
    } else // if (mbAddrN_B >= 0 && m_mbs[mbAddrN_B].m_mb_pred_mode ==
           // Intra_4x4)
    {
      int32_t n = 2; // Otherwise (N is equal to B), n is set equal to 2
      intraMxMPredModeB =
          m_mbs[mbAddrN_B].Intra4x4PredMode[luma8x8BlkIdxN_B * 4 + n];
    }
  }

  //-----------------------------
  int32_t predIntra8x8PredMode = MIN(intraMxMPredModeA, intraMxMPredModeB);

  if (m_mbs[CurrMbAddr].prev_intra8x8_pred_mode_flag[luma8x8BlkIdx]) {
    m_mbs[CurrMbAddr].Intra8x8PredMode[luma8x8BlkIdx] = predIntra8x8PredMode;
  } else {
    if (m_mbs[CurrMbAddr].rem_intra8x8_pred_mode[luma8x8BlkIdx] <
        predIntra8x8PredMode) {
      m_mbs[CurrMbAddr].Intra8x8PredMode[luma8x8BlkIdx] =
          m_mbs[CurrMbAddr].rem_intra8x8_pred_mode[luma8x8BlkIdx];
    } else {
      m_mbs[CurrMbAddr].Intra8x8PredMode[luma8x8BlkIdx] =
          m_mbs[CurrMbAddr].rem_intra8x8_pred_mode[luma8x8BlkIdx] + 1;
    }
  }

  Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr =
      m_mbs[CurrMbAddr].Intra8x8PredMode[luma8x8BlkIdx];

  return 0;
}

// 8.3.1.2 Intra_4x4 sample prediction (8.3.1 Intra_4x4 prediction process for
// luma samples)
int PictureBase::Intra_4x4_sample_prediction(int32_t luma4x4BlkIdx,
                                             int32_t PicWidthInSamples,
                                             uint8_t *pic_buff_luma_pred,
                                             int32_t isChroma,
                                             int32_t BitDepth) {
  int ret = 0;
  int32_t xO = 0;
  int32_t yO = 0;
  int32_t xW = 0;
  int32_t yW = 0;
  int32_t maxW = 0;
  int32_t maxH = 0;

  MB_ADDR_TYPE mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
  int32_t mbAddrN = -1;
  int32_t luma4x4BlkIdxN = 0;
  int32_t luma8x8BlkIdxN = 0;

  //SliceHeader &slice_header = m_h264_slice_header;

  // The 13 neighbouring samples p[ x, y ] that are constructed luma samples
  // prior to the deblocking filter process,
  //  with x = -1, y = -1..3 and x = 0..7, y = -1
  int32_t neighbouring_samples_x[13] = {-1, -1, -1, -1, -1, 0, 1,
                                        2,  3,  4,  5,  6,  7};
  int32_t neighbouring_samples_y[13] = {-1, 0,  1,  2,  3,  -1, -1,
                                        -1, -1, -1, -1, -1, -1};

  int32_t p[5 * 9] = {-1}; // x范围[-1,7]，y范围[-1,3]，共5行9列，原点为pp[1][1]
  memset(p, -1, sizeof(int32_t) * 5 * 9);
#define P(x, y) p[((y) + 1) * 9 + ((x) + 1)]
// #define cSL(x, y)    pic_buff_luma_pred[(mb_y * 16 + (yO + (y))) *
// PicWidthInSamples + (mb_x * 16 + (xO + (x)))]
#define IsMbAff                                                                \
  ((m_slice.slice_header.MbaffFrameFlag == 1 &&                                \
    m_mbs[CurrMbAddr].mb_field_decoding_flag == 1)                             \
       ? 1                                                                     \
       : 0)
#define cSL(x, y)                                                              \
  pic_buff_luma_pred[(m_mbs[CurrMbAddr].m_mb_position_y +                      \
                      (yO + (y)) * (1 + IsMbAff)) *                            \
                         PicWidthInSamples +                                   \
                     (m_mbs[CurrMbAddr].m_mb_position_x + (xO + (x)))]

  // 6.4.11.4 Derivation process for neighbouring 4x4 luma blocks

  // 6.4.3 Inverse 4x4 luma block scanning process
  // InverseRasterScan = (a % (d / b) ) * b;    if e == 0;
  // InverseRasterScan = (a / (d / b) ) * c;    if e == 1;
  xO = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
       InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
  yO = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
       InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

  for (int32_t i = 0; i < 13; i++) {
    // 6.4.12 Derivation process for neighbouring locations
    if (isChroma == 0) {
      maxW = 16;
      maxH = 16;
    } else // if (isChroma == 1)
    {
      maxW = MbWidthC;
      maxH = MbHeightC;
    }

    int32_t x = neighbouring_samples_x[i];
    int32_t y = neighbouring_samples_y[i];

    if (m_slice.slice_header.MbaffFrameFlag == 0) {
      ret = neighbouring_locations_non_MBAFF(
          xO + x, yO + y, maxW, maxH, CurrMbAddr, mbAddrN_type, mbAddrN,
          luma4x4BlkIdxN, luma8x8BlkIdxN, xW, yW, isChroma);
      RETURN_IF_FAILED(ret != 0, ret);
    } else // if (slice_header.MbaffFrameFlag == 1) //6.4.12.2 Specification for
           // neighbouring locations in MBAFF frames
    {
      ret = neighbouring_locations_MBAFF(xO + x, yO + y, maxW, maxH, CurrMbAddr,
                                         mbAddrN_type, mbAddrN, luma4x4BlkIdxN,
                                         luma8x8BlkIdxN, xW, yW, isChroma);
      RETURN_IF_FAILED(ret != 0, ret);
    }

    //----------------
    if (mbAddrN < 0 // mbAddrN is not available
        || (IS_INTER_Prediction_Mode(m_mbs[mbAddrN].m_mb_pred_mode) &&
            m_mbs[mbAddrN].constrained_intra_pred_flag == 1) ||
        (m_mbs[mbAddrN].m_name_of_mb_type == SI &&
         m_mbs[mbAddrN].constrained_intra_pred_flag == 1 &&
         m_mbs[CurrMbAddr].m_name_of_mb_type != SI) ||
        (x > 3 && (luma4x4BlkIdx == 3 || luma4x4BlkIdx == 11))) {
      P(x, y) = -1; // the sample p[ x, y ] is marked as "not available for
                    // Intra_4x4 prediction"
    } else {
      int32_t xM = 0;
      int32_t yM = 0;

      // 6.4.1 Inverse macroblock scanning process
      ret = inverse_macroblock_scanning_process(
          m_slice.slice_header.MbaffFrameFlag, mbAddrN,
          m_mbs[mbAddrN].mb_field_decoding_flag, xM, yM);
      RETURN_IF_FAILED(ret != 0, ret);

      //--------------------------
      if (m_slice.slice_header.MbaffFrameFlag == 1 &&
          m_mbs[mbAddrN].mb_field_decoding_flag == 1) {
        P(x, y) = pic_buff_luma_pred[(yM + 2 * yW) * PicWidthInSamples +
                                     (xM + xW)]; // cSL[ xM + xW, yM + 2 * yW ];
      } else {
        P(x, y) = pic_buff_luma_pred[(yM + yW) * PicWidthInSamples +
                                     (xM + xW)]; // cSL[ xM + xW, yM + yW ];
      }
    }
  }

  //-----------------------------
  // When samples p[ x, -1 ], with x = 4..7, are marked as "not available for
  // Intra_4x4 prediction," and the sample p[ 3, -1 ] is marked as "available
  // for Intra_4x4 prediction," the sample value of p[ 3, -1 ] is substituted
  // for sample values p[ x, -1 ], with x = 4..7, and samples p[ x, -1 ], with x
  // = 4..7, are marked as "available for Intra_4x4 prediction".
  if (P(4, -1) < 0 && P(5, -1) < 0 && P(6, -1) < 0 && P(7, -1) < 0 &&
      P(3, -1) >= 0) {
    P(4, -1) = P(3, -1);
    P(5, -1) = P(3, -1);
    P(6, -1) = P(3, -1);
    P(7, -1) = P(3, -1);
  }

  //----------9种帧内4x4预测模式----------------
  int32_t Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr = -1;
  ret = getIntra4x4PredMode(
      luma4x4BlkIdx, Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr, isChroma);
  RETURN_IF_FAILED(ret != 0, ret);

  if (Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr ==
      0) // 8.3.1.2.1 Specification of Intra_4x4_Vertical prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0) {
      for (int32_t y = 0; y <= 3; y++) {
        for (int32_t x = 0; x <= 3; x++) {
          cSL(x, y) = P(x, -1); // pred4x4L[y * 4 + x] = P(x, -1);
        }
      }
    }
  } else if (Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr ==
             1) // 8.3.1.2.2 Specification of Intra_4x4_Horizontal prediction
                // mode
  {
    if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0) {
      for (int32_t y = 0; y <= 3; y++) {
        for (int32_t x = 0; x <= 3; x++) {
          cSL(x, y) = P(-1, y); // pred4x4L[y * 4 + x] = P(-1, y);
        }
      }
    }
  } else if (Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr ==
             2) // 8.3.1.2.3 Specification of Intra_4x4_DC prediction mode
  {
    int32_t mean_value = 0;

    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0) {
      mean_value = (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + P(-1, 0) +
                    P(-1, 1) + P(-1, 2) + P(-1, 3) + 4) >>
                   3;
    } else if ((P(0, -1) < 0 || P(1, -1) < 0 || P(2, -1) < 0 || P(3, -1) < 0) &&
               (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
                P(-1, 3) >= 0)) {
      mean_value = (P(-1, 0) + P(-1, 1) + P(-1, 2) + P(-1, 3) + 2) >> 2;
    } else if ((P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 &&
                P(3, -1) >= 0) &&
               (P(-1, 0) < 0 || P(-1, 1) < 0 || P(-1, 2) < 0 || P(-1, 3) < 0)) {
      mean_value = (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + 2) >> 2;
    } else // some samples p[ x, -1 ], with x = 0..3, and some samples p[ -1, y
           // ], with y = 0..3, are marked as "not available for Intra_4x4
           // prediction"
    {
      mean_value = 1 << (BitDepth - 1); // mean_value = 1 << (8 - 1) = 128;
    }

    //-----------------------------------
    for (int32_t y = 0; y <= 3; y++) {
      for (int32_t x = 0; x <= 3; x++) {
        cSL(x, y) = mean_value; // pred4x4L[y * 4 + x] = mean_value;
      }
    }
  } else if (Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr ==
             3) // 8.3.1.2.4 Specification of Intra_4x4_Diagonal_Down_Left
                // prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0) {
      for (int32_t y = 0; y <= 3; y++) {
        for (int32_t x = 0; x <= 3; x++) {
          if (x == 3 && y == 3) {
            cSL(x, y) =
                (P(6, -1) + 3 * P(7, -1) + 2) >> 2; // pred4x4L[y * 4 + x]
          } else {
            cSL(x, y) =
                (P(x + y, -1) + 2 * P(x + y + 1, -1) + P(x + y + 2, -1) + 2) >>
                2; // pred4x4L[y * 4 + x]
          }
        }
      }
    }
  } else if (Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr ==
             4) // 8.3.1.2.5 Specification of Intra_4x4_Diagonal_Down_Right
                // prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0) {
      for (int32_t y = 0; y <= 3; y++) {
        for (int32_t x = 0; x <= 3; x++) {
          if (x > y) {
            cSL(x, y) =
                (P(x - y - 2, -1) + 2 * P(x - y - 1, -1) + P(x - y, -1) + 2) >>
                2; // pred4x4L[y * 4 + x]
          } else if (x < y) {
            cSL(x, y) =
                (P(-1, y - x - 2) + 2 * P(-1, y - x - 1) + P(-1, y - x) + 2) >>
                2; // pred4x4L[y * 4 + x]
          } else   // if (x == y)
          {
            cSL(x, y) = (P(0, -1) + 2 * P(-1, -1) + P(-1, 0) + 2) >>
                        2; // pred4x4L[y * 4 + x]
          }
        }
      }
    }
  } else if (Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr ==
             5) // 8.3.1.2.6 Specification of Intra_4x4_Vertical_Right
                // prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0) {
      for (int32_t y = 0; y <= 3; y++) {
        for (int32_t x = 0; x <= 3; x++) {
          int32_t zVR = 2 * x - y;

          if (zVR == 0 || zVR == 2 || zVR == 4 || zVR == 6) {
            cSL(x, y) = (P(x - (y >> 1) - 1, -1) + P(x - (y >> 1), -1) + 1) >>
                        1; // pred4x4L[y * 4 + x]
          } else if (zVR == 1 || zVR == 3 || zVR == 5) {
            cSL(x, y) = (P(x - (y >> 1) - 2, -1) + 2 * P(x - (y >> 1) - 1, -1) +
                         P(x - (y >> 1), -1) + 2) >>
                        2; // pred4x4L[y * 4 + x]
          } else if (zVR == -1) {
            cSL(x, y) = (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >>
                        2; // pred4x4L[y * 4 + x]
          } else           // if (zVR == -2 || zVR == -3)
          {
            cSL(x, y) = (P(-1, y - 1) + 2 * P(-1, y - 2) + P(-1, y - 3) + 2) >>
                        2; // pred4x4L[y * 4 + x]
          }
        }
      }
    }
  } else if (Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr ==
             6) // 8.3.1.2.7 Specification of Intra_4x4_Horizontal_Down
                // prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0) {
      for (int32_t y = 0; y <= 3; y++) {
        for (int32_t x = 0; x <= 3; x++) {
          int32_t zHD = 2 * y - x;

          if (zHD == 0 || zHD == 2 || zHD == 4 || zHD == 6) {
            cSL(x, y) = (P(-1, y - (x >> 1) - 1) + P(-1, y - (x >> 1)) + 1) >>
                        1; // pred4x4L[y * 4 + x]
          } else if (zHD == 1 || zHD == 3 || zHD == 5) {
            cSL(x, y) = (P(-1, y - (x >> 1) - 2) + 2 * P(-1, y - (x >> 1) - 1) +
                         P(-1, y - (x >> 1)) + 2) >>
                        2; // pred4x4L[y * 4 + x]
          } else if (zHD == -1) {
            cSL(x, y) = (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >>
                        2; // pred4x4L[y * 4 + x]
          } else           // if (zHD == -2 || zHD == -3)
          {
            cSL(x, y) = (P(x - 1, -1) + 2 * P(x - 2, -1) + P(x - 3, -1) + 2) >>
                        2; // pred4x4L[y * 4 + x]
          }
        }
      }
    }
  } else if (Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr ==
             7) // 8.3.1.2.8 Specification of Intra_4x4_Vertical_Left prediction
                // mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0) {
      for (int32_t y = 0; y <= 3; y++) {
        for (int32_t x = 0; x <= 3; x++) {
          if (y == 0 || y == 2) {
            cSL(x, y) = (P(x + (y >> 1), -1) + P(x + (y >> 1) + 1, -1) + 1) >>
                        1; // pred4x4L[y * 4 + x]
          } else           // if (y == 1 || y == 3)
          {
            cSL(x, y) = (P(x + (y >> 1), -1) + 2 * P(x + (y >> 1) + 1, -1) +
                         P(x + (y >> 1) + 2, -1) + 2) >>
                        2; // pred4x4L[y * 4 + x]
          }
        }
      }
    }
  } else if (Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr ==
             8) // 8.3.1.2.9 Specification of Intra_4x4_Horizontal_Up prediction
                // mode
  {
    if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0) {
      for (int32_t y = 0; y <= 3; y++) {
        for (int32_t x = 0; x <= 3; x++) {
          int32_t zHU = x + 2 * y;

          if (zHU == 0 || zHU == 2 || zHU == 4) {
            cSL(x, y) = (P(-1, y + (x >> 1)) + P(-1, y + (x >> 1) + 1) + 1) >>
                        1; // pred4x4L[y * 4 + x]
          } else if (zHU == 1 || zHU == 3) {
            cSL(x, y) = (P(-1, y + (x >> 1)) + 2 * P(-1, y + (x >> 1) + 1) +
                         P(-1, y + (x >> 1) + 2) + 2) >>
                        2; // pred4x4L[y * 4 + x]
          } else if (zHU == 5) {
            cSL(x, y) =
                (P(-1, 2) + 3 * P(-1, 3) + 2) >> 2; // pred4x4L[y * 4 + x]
          } else                                    // if (zHU > 5)
          {
            cSL(x, y) = P(-1, 3); // pred4x4L[y * 4 + x]
          }
        }
      }
    }
  } else {
    printf("Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr(%d) must be [0,8]\n",
           Intra4x4PredMode_luma4x4BlkIdx_of_CurrMbAddr);
  }

  /*
      printf("%s(%d): start:\n", __FUNCTION__, __LINE__);
      for (int32_t y = 0; y <= 3; y++)
      {
          for (int32_t x = 0; x <= 3; x++)
          {
              printf(" 0x%02x(%d)", cSL(x, y), cSL(x, y));
          }
          printf("\n");
      }
      printf("end\n");
  */

#undef P
#undef cSL
#undef IsMbAff

  return 0;
}

// 8.3.2.2 Intra_8x8 sample prediction (8.3.2 Intra_8x8 prediction process for
// luma sampless)
int PictureBase::Intra_8x8_sample_prediction(int32_t luma8x8BlkIdx,
                                             int32_t PicWidthInSamples,
                                             uint8_t *pic_buff_luma_pred,
                                             int32_t isChroma,
                                             int32_t BitDepth) {
  int ret = 0;
  int32_t xO = 0;
  int32_t yO = 0;
  int32_t xW = 0;
  int32_t yW = 0;
  int32_t maxW = 0;
  int32_t maxH = 0;

  MB_ADDR_TYPE mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
  int32_t mbAddrN = -1;
  int32_t luma4x4BlkIdxN = 0;
  int32_t luma8x8BlkIdxN = 0;

  // The 25 neighbouring samples p[ x, y ] that are constructed luma samples
  // prior to the deblocking filter process, with x = -1, y = -1..7 and x =
  // 0..15, y = -1,
  int32_t neighbouring_samples_x[25] = {-1, -1, -1, -1, -1, -1, -1, -1, -1,
                                        0,  1,  2,  3,  4,  5,  6,  7,  8,
                                        9,  10, 11, 12, 13, 14, 15};
  int32_t neighbouring_samples_y[25] = {-1, 0,  1,  2,  3,  4,  5,  6,  7,
                                        -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                        -1, -1, -1, -1, -1, -1, -1};

  int32_t p[9 * 17] = {
      0}; // x范围[-1,15]，y范围[-1,7]，共9行17列，原点为pp[1][1]
  int32_t p1[9 * 17] = {
      0}; // x范围[-1,15]，y范围[-1,7]，共9行17列，原点为pp[1][1]
#define P(x, y) p[((y) + 1) * 17 + ((x) + 1)]
#define P1(x, y) p1[((y) + 1) * 17 + ((x) + 1)]
// #define cSL(x, y)    pic_buff_luma_pred[(mb_y * 16 + (yO + y)) *
// PicWidthInSamples + (mb_x * 16 + (xO + x))]
#define IsMbAff                                                                \
  ((m_slice.slice_header.MbaffFrameFlag == 1 &&                                \
    m_mbs[CurrMbAddr].mb_field_decoding_flag == 1)                             \
       ? 1                                                                     \
       : 0)
#define cSL(x, y)                                                              \
  pic_buff_luma_pred[(m_mbs[CurrMbAddr].m_mb_position_y +                      \
                      (yO + (y)) * (1 + IsMbAff)) *                            \
                         PicWidthInSamples +                                   \
                     (m_mbs[CurrMbAddr].m_mb_position_x + (xO + (x)))]

  memset(p, -1, sizeof(int32_t) * 9 * 17);
  memset(p1, -1, sizeof(int32_t) * 9 * 17);

  //SliceHeader &slice_header = m_h264_slice_header;

  // 6.4.5 Inverse 8x8 luma block scanning process
  // InverseRasterScan = (a % (d / b) ) * b;    if e == 0;
  // InverseRasterScan = (a / (d / b) ) * c;    if e == 1;
  xO = InverseRasterScan(luma8x8BlkIdx, 8, 8, 16, 0);
  yO = InverseRasterScan(luma8x8BlkIdx, 8, 8, 16, 1);

  for (int32_t i = 0; i < 25; i++) {
    // 6.4.12 Derivation process for neighbouring locations
    if (isChroma == 0) {
      maxW = 16;
      maxH = 16;
    } else // if (isChroma == 1)
    {
      maxW = MbWidthC;
      maxH = MbHeightC;
    }

    int32_t x = neighbouring_samples_x[i];
    int32_t y = neighbouring_samples_y[i];

    if (m_slice.slice_header.MbaffFrameFlag == 0) {
      ret = neighbouring_locations_non_MBAFF(
          xO + x, yO + y, maxW, maxH, CurrMbAddr, mbAddrN_type, mbAddrN,
          luma4x4BlkIdxN, luma8x8BlkIdxN, xW, yW, isChroma);
      RETURN_IF_FAILED(ret != 0, ret);
    } else // if (slice_header.MbaffFrameFlag == 1) //6.4.12.2 Specification for
           // neighbouring locations in MBAFF frames
    {
      ret = neighbouring_locations_MBAFF(xO + x, yO + y, maxW, maxH, CurrMbAddr,
                                         mbAddrN_type, mbAddrN, luma4x4BlkIdxN,
                                         luma8x8BlkIdxN, xW, yW, isChroma);
      RETURN_IF_FAILED(ret != 0, ret);
    }

    //----------------
    if (mbAddrN < 0 // mbAddrN is not available
        || (IS_INTER_Prediction_Mode(m_mbs[mbAddrN].m_mb_pred_mode) &&
            m_mbs[mbAddrN].constrained_intra_pred_flag == 1)) {
      P(x, y) = -1; // the sample p[ x, y ] is marked as "not available for
                    // Intra_8x8 prediction"
    } else {
      int32_t xM = 0;
      int32_t yM = 0;

      // 6.4.1 Inverse macroblock scanning process
      ret = inverse_macroblock_scanning_process(
          m_slice.slice_header.MbaffFrameFlag, mbAddrN,
          m_mbs[mbAddrN].mb_field_decoding_flag, xM, yM);
      RETURN_IF_FAILED(ret != 0, ret);

      //--------------------------
      if (m_slice.slice_header.MbaffFrameFlag == 1 &&
          m_mbs[mbAddrN].mb_field_decoding_flag == 1) {
        P(x, y) = pic_buff_luma_pred[(yM + 2 * yW) * PicWidthInSamples +
                                     (xM + xW)]; // cSL[ xM + xW, yM + 2 * yW ];
      } else {
        P(x, y) = pic_buff_luma_pred[(yM + yW) * PicWidthInSamples +
                                     (xM + xW)]; // cSL[ xM + xW, yM + yW ];
      }
    }
  }

  //-----------------------------
  // When samples p[ x, -1 ], with x = 8..15, are marked as "not available for
  // Intra_8x8 prediction," and the sample p[ 7, -1 ] is marked as "available
  // for Intra_8x8 prediction," the sample value of p[ 7, -1 ] is substituted
  // for sample values p[ x, -1 ], with x = 8..15, and samples p[ x, -1 ], with
  // x = 8..15, are marked as "available for Intra_8x8 prediction".
  if (P(8, -1) < 0 && P(9, -1) < 0 && P(10, -1) < 0 && P(11, -1) < 0 &&
      P(12, -1) < 0 && P(13, -1) < 0 && P(14, -1) < 0 && P(15, -1) < 0 &&
      P(7, -1) >= 0) {
    P(8, -1) = P(7, -1);
    P(9, -1) = P(7, -1);
    P(10, -1) = P(7, -1);
    P(11, -1) = P(7, -1);
    P(12, -1) = P(7, -1);
    P(13, -1) = P(7, -1);
    P(14, -1) = P(7, -1);
    P(15, -1) = P(7, -1);
  }

  //-------------------------------------------
  // 8.3.2.2.1 Reference sample filtering process for Intra_8x8 sample
  // prediction
  if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
      P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
      P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
      P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0) {
    if (P(-1, -1) >= 0) {
      P1(0, -1) = (P(-1, -1) + 2 * P(0, -1) + P(1, -1) + 2) >> 2;
    } else {
      P1(0, -1) = (3 * P(0, -1) + P(1, -1) + 2) >> 2;
    }

    for (int32_t x = 1; x <= 14; x++) {
      P1(x, -1) = (P(x - 1, -1) + 2 * P(x, -1) + P(x + 1, -1) + 2) >> 2;
    }

    P1(15, -1) = (P(14, -1) + 3 * P(15, -1) + 2) >> 2;
  }

  if (P(-1, -1) >= 0) {
    if (P(0, -1) < 0 || P(-1, 0) < 0) {
      if (P(0, -1) >= 0) {
        P1(-1, -1) = (3 * P(-1, -1) + P(0, -1) + 2) >> 2;
      } else if (P(0, -1) < 0 && P(-1, 0) >= 0) {
        P1(-1, -1) = (3 * P(-1, -1) + P(-1, 0) + 2) >> 2;
      } else // if (P(0, -1) < 0 && P(-1, 0) < 0)
      {
        P1(-1, -1) = P(-1, -1);
      }
    } else // if (P(0, -1) >= 0 || P(-1, 0) >= 0)
    {
      P1(-1, -1) = (P(0, -1) + 2 * P(-1, -1) + P(-1, 0) + 2) >> 2;
    }
  }

  if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
      P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0) {
    if (P(-1, -1) >= 0) {
      P1(-1, 0) = (P(-1, -1) + 2 * P(-1, 0) + P(-1, 1) + 2) >> 2;
    } else // if (P(-1, -1) < 0)
    {
      P1(-1, 0) = (3 * P(-1, 0) + P(-1, 1) + 2) >> 2;
    }

    for (int32_t y = 1; y <= 6; y++) {
      P1(-1, y) = (P(-1, y - 1) + 2 * P(-1, y) + P(-1, y + 1) + 2) >> 2;
    }

    P1(-1, 7) = (P(-1, 6) + 3 * P(-1, 7) + 2) >> 2;
  }

  memcpy(p, p1, sizeof(int32_t) * 9 * 17);

  //----------9种帧内8x8预测模式----------------
  int32_t Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr = -1;
  ret = getIntra8x8PredMode(
      luma8x8BlkIdx, Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr, isChroma);
  RETURN_IF_FAILED(ret != 0, ret);

  if (Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr ==
      0) // 8.3.2.2.2 Specification of Intra_8x8_Vertical prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0) {
      for (int32_t y = 0; y <= 7; y++) {
        for (int32_t x = 0; x <= 7; x++) {
          cSL(x, y) = P(x, -1); // pred8x8L[y * 8 + x ]
        }
      }
    }
  } else if (Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr ==
             1) // 8.3.2.2.3 Specification of Intra_8x8_Horizontal prediction
                // mode
  {
    if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0) {
      for (int32_t y = 0; y <= 7; y++) {
        for (int32_t x = 0; x <= 7; x++) {
          cSL(x, y) = P(-1, y); // pred8x8L[y * 8 + x ]
        }
      }
    }
  } else if (Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr ==
             2) // 8.3.2.2.4 Specification of Intra_8x8_DC prediction mode
  {
    int32_t mean_value = 0;

    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0) {
      mean_value =
          (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + P(4, -1) + P(5, -1) +
           P(6, -1) + P(7, -1) + P(-1, 0) + P(-1, 1) + P(-1, 2) + P(-1, 3) +
           P(-1, 4) + P(-1, 5) + P(-1, 6) + P(-1, 7) + 8) >>
          4;
    } else if ((P(0, -1) < 0 || P(1, -1) < 0 || P(2, -1) < 0 || P(3, -1) < 0 ||
                P(4, -1) < 0 || P(5, -1) < 0 || P(6, -1) < 0 || P(7, -1) < 0) &&
               (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
                P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 &&
                P(-1, 6) >= 0 && P(-1, 7) >= 0)) {
      mean_value = (P(-1, 0) + P(-1, 1) + P(-1, 2) + P(-1, 3) + P(-1, 4) +
                    P(-1, 5) + P(-1, 6) + P(-1, 7) + 4) >>
                   3;
    } else if ((P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 &&
                P(3, -1) >= 0 && P(4, -1) >= 0 && P(5, -1) >= 0 &&
                P(6, -1) >= 0 && P(7, -1) >= 0) &&
               (P(-1, 0) < 0 || P(-1, 1) < 0 || P(-1, 2) < 0 || P(-1, 3) < 0 ||
                P(-1, 4) < 0 || P(-1, 5) < 0 || P(-1, 6) < 0 || P(-1, 7) < 0)) {
      mean_value = (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + P(4, -1) +
                    P(5, -1) + P(6, -1) + P(7, -1) + 4) >>
                   3;
    } else // some samples p[ x, -1 ], with x = 0..7, and some samples p[ -1, y
           // ], with y = 0..7, are marked as "not available for Intra_8x8
           // prediction")
    {
      mean_value = 1 << (BitDepth - 1); // mean_value = 1 << (8 - 1) = 128;
    }

    for (int32_t y = 0; y <= 7; y++) {
      for (int32_t x = 0; x <= 7; x++) {
        cSL(x, y) = mean_value; // red8x8L[y * 8 + x]
      }
    }
  } else if (Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr ==
             3) // 8.3.2.2.5 Specification of Intra_8x8_Diagonal_Down_Left
                // prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
        P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0) {
      for (int32_t y = 0; y <= 7; y++) {
        for (int32_t x = 0; x <= 7; x++) {
          if (x == 7 && y == 7) {
            cSL(x, y) =
                (P(14, -1) + 3 * P(15, -1) + 2) >> 2; // red8x8L[y * 8 + x]
          } else                                      // if (x != 7 || y != 7)
          {
            cSL(x, y) =
                (P(x + y, -1) + 2 * P(x + y + 1, -1) + P(x + y + 2, -1) + 2) >>
                2; // red8x8L[y * 8 + x]
          }
        }
      }
    }
  } else if (Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr ==
             4) // 8.3.2.2.6 Specification of Intra_8x8_Diagonal_Down_Right
                // prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 &&
        P(-1, 7) >= 0) {
      for (int32_t y = 0; y <= 7; y++) {
        for (int32_t x = 0; x <= 7; x++) {
          if (x > y) {
            cSL(x, y) =
                (P(x - y - 2, -1) + 2 * P(x - y - 1, -1) + P(x - y, -1) + 2) >>
                2; // red8x8L[y * 8 + x]
          } else if (x < y) {
            cSL(x, y) =
                (P(-1, y - x - 2) + 2 * P(-1, y - x - 1) + P(-1, y - x) + 2) >>
                2; // red8x8L[y * 8 + x]
          } else   // if (x == y)
          {
            cSL(x, y) = (P(0, -1) + 2 * P(-1, -1) + P(-1, 0) + 2) >>
                        2; // red8x8L[y * 8 + x]
          }
        }
      }
    }
  } else if (Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr ==
             5) // 8.3.2.2.7 Specification of Intra_8x8_Vertical_Right
                // prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 &&
        P(-1, 7) >= 0) {
      for (int32_t y = 0; y <= 7; y++) {
        for (int32_t x = 0; x <= 7; x++) {
          int32_t zVR = 2 * x - y;

          if (zVR == 0 || zVR == 2 || zVR == 4 || zVR == 6 || zVR == 8 ||
              zVR == 10 || zVR == 12 || zVR == 14) {
            cSL(x, y) = (P(x - (y >> 1) - 1, -1) + P(x - (y >> 1), -1) + 1) >>
                        1; // red8x8L[y * 8 + x]
          } else if (zVR == 1 || zVR == 3 || zVR == 5 || zVR == 7 || zVR == 9 ||
                     zVR == 11 || zVR == 13) {
            cSL(x, y) = (P(x - (y >> 1) - 2, -1) + 2 * P(x - (y >> 1) - 1, -1) +
                         P(x - (y >> 1), -1) + 2) >>
                        2; // red8x8L[y * 8 + x]
          } else if (zVR == -1) {
            cSL(x, y) = (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >>
                        2; // red8x8L[y * 8 + x]
          } else // if (zVR == -2 || zVR == -3 || zVR == -4 || zVR == -5 || zVR
                 // == -6 || zVR == -7)
          {
            cSL(x, y) = (P(-1, y - 2 * x - 1) + 2 * P(-1, y - 2 * x - 2) +
                         P(-1, y - 2 * x - 3) + 2) >>
                        2; // red8x8L[y * 8 + x]
          }
        }
      }
    }
  } else if (Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr ==
             6) // 8.3.2.2.8 Specification of Intra_8x8_Horizontal_Down
                // prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(-1, -1) >= 0 && P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
        P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 &&
        P(-1, 7) >= 0) {
      for (int32_t y = 0; y <= 7; y++) {
        for (int32_t x = 0; x <= 7; x++) {
          int32_t zHD = 2 * y - x;

          if (zHD == 0 || zHD == 2 || zHD == 4 || zHD == 6 || zHD == 8 ||
              zHD == 10 || zHD == 12 || zHD == 14) {
            cSL(x, y) = (P(-1, y - (x >> 1) - 1) + P(-1, y - (x >> 1)) + 1) >>
                        1; // red8x8L[y * 8 + x]
          } else if (zHD == 1 || zHD == 3 || zHD == 5 || zHD == 7 || zHD == 9 ||
                     zHD == 11 || zHD == 13) {
            cSL(x, y) = (P(-1, y - (x >> 1) - 2) + 2 * P(-1, y - (x >> 1) - 1) +
                         P(-1, y - (x >> 1)) + 2) >>
                        2; // red8x8L[y * 8 + x]
          } else if (zHD == -1) {
            cSL(x, y) = (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >>
                        2; // red8x8L[y * 8 + x]
          } else // if (zHD == -2 || zHD == -3 || zHD == -4 || zHD == -5 || zHD
                 // == -6 || zHD == -7)
          {
            cSL(x, y) = (P(x - 2 * y - 1, -1) + 2 * P(x - 2 * y - 2, -1) +
                         P(x - 2 * y - 3, -1) + 2) >>
                        2; // red8x8L[y * 8 + x]
          }
        }
      }
    }
  } else if (Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr ==
             7) // 8.3.2.2.9 Specification of Intra_8x8_Vertical_Left prediction
                // mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
        P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0) {
      for (int32_t y = 0; y <= 7; y++) {
        for (int32_t x = 0; x <= 7; x++) {
          if (y == 0 || y == 2 || y == 4 || y == 6) {
            cSL(x, y) = (P(x + (y >> 1), -1) + P(x + (y >> 1) + 1, -1) + 1) >>
                        1; // red8x8L[y * 8 + x]
          } else           // if (y == 1 || y == 3 || y == 5 || y == 7)
          {
            cSL(x, y) = (P(x + (y >> 1), -1) + 2 * P(x + (y >> 1) + 1, -1) +
                         P(x + (y >> 1) + 2, -1) + 2) >>
                        2; // red8x8L[y * 8 + x]
          }
        }
      }
    }
  } else if (Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr ==
             8) // 8.3.2.2.10 Specification of Intra_8x8_Horizontal_Up
                // prediction mode
  {
    if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0) {
      for (int32_t y = 0; y <= 7; y++) {
        for (int32_t x = 0; x <= 7; x++) {
          int32_t zHU = x + 2 * y;

          if (zHU == 0 || zHU == 2 || zHU == 4 || zHU == 6 || zHU == 8 ||
              zHU == 10 || zHU == 12) {
            cSL(x, y) = (P(-1, y + (x >> 1)) + P(-1, y + (x >> 1) + 1) + 1) >>
                        1; // red8x8L[y * 8 + x]
          } else if (zHU == 1 || zHU == 3 || zHU == 5 || zHU == 7 || zHU == 9 ||
                     zHU == 11) {
            cSL(x, y) = (P(-1, y + (x >> 1)) + 2 * P(-1, y + (x >> 1) + 1) +
                         P(-1, y + (x >> 1) + 2) + 2) >>
                        2; // red8x8L[y * 8 + x]
          } else if (zHU == 13) {
            cSL(x, y) = (P(-1, 6) + 3 * P(-1, 7) + 2) >> 2; // red8x8L[y * 8 +
                                                            // x]
          } else                                            // if (zHU > 13)
          {
            cSL(x, y) = P(-1, 7); // red8x8L[y * 8 + x]
          }
        }
      }
    }
  } else {
    printf("Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr(%d) must be [0,8]\n",
           Intra8x8PredMode_luma8x8BlkIdx_of_CurrMbAddr);
  }

#undef P
#undef P1
#undef cSL
#undef IsMbAff

  return 0;
}

// 8.3.3 Intra_16x16 prediction process for luma samples
int PictureBase::Intra_16x16_sample_prediction(uint8_t *pic_buff_luma_pred,
                                               int32_t PicWidthInSamples,
                                               int32_t isChroma,
                                               int32_t BitDepth) {
  int ret = 0;
  int32_t xO = 0;
  int32_t yO = 0;
  int32_t xW = 0;
  int32_t yW = 0;
  int32_t maxW = 0;
  int32_t maxH = 0;

  MB_ADDR_TYPE mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
  int32_t mbAddrN = -1;
  int32_t luma4x4BlkIdxN = 0;
  int32_t luma8x8BlkIdxN = 0;

  // The 33 neighbouring samples p[ x, y ] that are constructed luma samples
  // prior to the deblocking filter process, with x = −1, y = −1..15 and with x
  // = 0..15, y = −1,
  int32_t neighbouring_samples_x[33] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15};
  int32_t neighbouring_samples_y[33] = {
      -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  int32_t p[17 * 17] = {
      -1}; // x范围[-1,15]，y范围[-1,15]，共17行17列，原点为pp[1][1]
#define P(x, y) p[((y) + 1) * 17 + ((x) + 1)]
// #define cSL(x, y)    pic_buff_luma_pred[(mb_y * 16 + (y)) * PicWidthInSamples
// + (mb_x * 16 + (x))]
#define IsMbAff                                                                \
  ((m_slice.slice_header.MbaffFrameFlag == 1 &&                                \
    m_mbs[CurrMbAddr].mb_field_decoding_flag == 1)                             \
       ? 1                                                                     \
       : 0)
#define cSL(x, y)                                                              \
  pic_buff_luma_pred[(m_mbs[CurrMbAddr].m_mb_position_y +                      \
                      (y) * (1 + IsMbAff)) *                                   \
                         PicWidthInSamples +                                   \
                     (m_mbs[CurrMbAddr].m_mb_position_x + (x))]

  //SliceHeader &slice_header = m_h264_slice_header;

  for (int32_t i = 0; i < 33; i++) {
    // 6.4.12 Derivation process for neighbouring locations
    if (isChroma == 0) {
      maxW = 16;
      maxH = 16;
    } else // if (isChroma == 1)
    {
      maxW = MbWidthC;
      maxH = MbHeightC;
    }

    int32_t x = neighbouring_samples_x[i];
    int32_t y = neighbouring_samples_y[i];

    if (m_slice.slice_header.MbaffFrameFlag == 0) {
      ret = neighbouring_locations_non_MBAFF(
          xO + x, yO + y, maxW, maxH, CurrMbAddr, mbAddrN_type, mbAddrN,
          luma4x4BlkIdxN, luma8x8BlkIdxN, xW, yW, isChroma);
      RETURN_IF_FAILED(ret != 0, ret);
    } else // if (slice_header.MbaffFrameFlag == 1) //6.4.12.2 Specification for
           // neighbouring locations in MBAFF frames
    {
      ret = neighbouring_locations_MBAFF(xO + x, yO + y, maxW, maxH, CurrMbAddr,
                                         mbAddrN_type, mbAddrN, luma4x4BlkIdxN,
                                         luma8x8BlkIdxN, xW, yW, isChroma);
      RETURN_IF_FAILED(ret != 0, ret);
    }

    //----------------
    if (mbAddrN < 0 // mbAddrN is not available
        || (IS_INTER_Prediction_Mode(m_mbs[mbAddrN].m_mb_pred_mode) &&
            m_mbs[mbAddrN].constrained_intra_pred_flag == 1) ||
        (m_mbs[mbAddrN].m_name_of_mb_type == SI &&
         m_mbs[mbAddrN].constrained_intra_pred_flag == 1)) {
      P(x, y) = -1; // the sample p[ x, y ] is marked as "not available for
                    // Intra_8x8 prediction"
    } else {
      int32_t xM = 0;
      int32_t yM = 0;

      // 6.4.1 Inverse macroblock scanning process
      ret = inverse_macroblock_scanning_process(
          m_slice.slice_header.MbaffFrameFlag, mbAddrN,
          m_mbs[mbAddrN].mb_field_decoding_flag, xM, yM);
      RETURN_IF_FAILED(ret != 0, ret);

      //--------------------------
      if (m_slice.slice_header.MbaffFrameFlag == 1 &&
          m_mbs[mbAddrN].mb_field_decoding_flag ==
              1) // If MbaffFrameFlag is equal to 1 and the macroblock mbAddrN
                 // is a field macroblock,
      {
        P(x, y) = pic_buff_luma_pred[(yM + 2 * yW) * PicWidthInSamples +
                                     (xM + xW)]; // cSL[ xM + xW, yM + 2 * yW ];
      } else {
        P(x, y) = pic_buff_luma_pred[(yM + yW) * PicWidthInSamples +
                                     (xM + xW)]; // cSL[ xM + xW, yM + yW ];
      }
    }
  }

  //----------4种帧内16x16预测模式----------------
  int32_t Intra16x16PredMode_of_CurrMbAddr =
      m_mbs[CurrMbAddr].Intra16x16PredMode;

  if (Intra16x16PredMode_of_CurrMbAddr ==
      0) // 8.3.3.1 Specification of Intra_16x16_Vertical prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
        P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0) {
      for (int32_t y = 0; y <= 15; y++) {
        for (int32_t x = 0; x <= 15; x++) {
          cSL(x, y) = P(x, -1); // pred16x16L[y * 16 + x ]
        }
      }
    }
  } else if (Intra16x16PredMode_of_CurrMbAddr ==
             1) // 8.3.3.2 Specification of Intra_16x16_Horizontal prediction
                // mode
  {
    if (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0 &&
        P(-1, 8) >= 0 && P(-1, 9) >= 0 && P(-1, 10) >= 0 && P(-1, 11) >= 0 &&
        P(-1, 12) >= 0 && P(-1, 13) >= 0 && P(-1, 14) >= 0 && P(-1, 15) >= 0) {
      for (int32_t y = 0; y <= 15; y++) {
        for (int32_t x = 0; x <= 15; x++) {
          cSL(x, y) = P(-1, y); // pred16x16L[y * 16 + x ]
        }
      }
    }
  } else if (Intra16x16PredMode_of_CurrMbAddr ==
             2) // 8.3.3.3 Specification of Intra_16x16_DC prediction mode
  {
    int32_t mean_value = 0;

    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
        P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0 &&
        P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0 &&
        P(-1, 8) >= 0 && P(-1, 9) >= 0 && P(-1, 10) >= 0 && P(-1, 11) >= 0 &&
        P(-1, 12) >= 0 && P(-1, 13) >= 0 && P(-1, 14) >= 0 && P(-1, 15) >= 0) {
      mean_value =
          (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + P(4, -1) + P(5, -1) +
           P(6, -1) + P(7, -1) + P(8, -1) + P(9, -1) + P(10, -1) + P(11, -1) +
           P(12, -1) + P(13, -1) + P(14, -1) + P(15, -1) + P(-1, 0) + P(-1, 1) +
           P(-1, 2) + P(-1, 3) + P(-1, 4) + P(-1, 5) + P(-1, 6) + P(-1, 7) +
           P(-1, 8) + P(-1, 9) + P(-1, 10) + P(-1, 11) + P(-1, 12) + P(-1, 13) +
           P(-1, 14) + P(-1, 15) + 16) >>
          5;
    } else if (!(P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 &&
                 P(3, -1) >= 0 && P(4, -1) >= 0 && P(5, -1) >= 0 &&
                 P(6, -1) >= 0 && P(7, -1) >= 0 && P(8, -1) >= 0 &&
                 P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
                 P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 &&
                 P(15, -1) >= 0) &&
               (P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
                P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 &&
                P(-1, 6) >= 0 && P(-1, 7) >= 0 && P(-1, 8) >= 0 &&
                P(-1, 9) >= 0 && P(-1, 10) >= 0 && P(-1, 11) >= 0 &&
                P(-1, 12) >= 0 && P(-1, 13) >= 0 && P(-1, 14) >= 0 &&
                P(-1, 15) >= 0)) {
      mean_value =
          (P(-1, 0) + P(-1, 1) + P(-1, 2) + P(-1, 3) + P(-1, 4) + P(-1, 5) +
           P(-1, 6) + P(-1, 7) + P(-1, 8) + P(-1, 9) + P(-1, 10) + P(-1, 11) +
           P(-1, 12) + P(-1, 13) + P(-1, 14) + P(-1, 15) + 8) >>
          4;
    } else if ((P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 &&
                P(3, -1) >= 0 && P(4, -1) >= 0 && P(5, -1) >= 0 &&
                P(6, -1) >= 0 && P(7, -1) >= 0 && P(8, -1) >= 0 &&
                P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
                P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 &&
                P(15, -1) >= 0) &&
               !(P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 &&
                 P(-1, 3) >= 0 && P(-1, 4) >= 0 && P(-1, 5) >= 0 &&
                 P(-1, 6) >= 0 && P(-1, 7) >= 0 && P(-1, 8) >= 0 &&
                 P(-1, 9) >= 0 && P(-1, 10) >= 0 && P(-1, 11) >= 0 &&
                 P(-1, 12) >= 0 && P(-1, 13) >= 0 && P(-1, 14) >= 0 &&
                 P(-1, 15) >= 0)) {
      mean_value =
          (P(0, -1) + P(1, -1) + P(2, -1) + P(3, -1) + P(4, -1) + P(5, -1) +
           P(6, -1) + P(7, -1) + P(8, -1) + P(9, -1) + P(10, -1) + P(11, -1) +
           P(12, -1) + P(13, -1) + P(14, -1) + P(15, -1) + 8) >>
          4;
    } else // some of the neighbouring samples p[ x, −1 ], with x = 0..15, and
           // some of the neighbouring samples p[ −1, y ], with y = 0..15, are
           // marked as "not available for Intra_16x16 prediction"
    {
      mean_value = (1 << (BitDepth - 1)); // mean_value = 1 << (8 - 1) = 128;
    }

    for (int32_t y = 0; y <= 15; y++) {
      for (int32_t x = 0; x <= 15; x++) {
        cSL(x, y) = mean_value; // pred16x16L[y * 16 + x ]
      }
    }
  } else if (Intra16x16PredMode_of_CurrMbAddr ==
             3) // 8.3.3.4 Specification of Intra_16x16_Plane prediction mode
  {
    if (P(0, -1) >= 0 && P(1, -1) >= 0 && P(2, -1) >= 0 && P(3, -1) >= 0 &&
        P(4, -1) >= 0 && P(5, -1) >= 0 && P(6, -1) >= 0 && P(7, -1) >= 0 &&
        P(8, -1) >= 0 && P(9, -1) >= 0 && P(10, -1) >= 0 && P(11, -1) >= 0 &&
        P(12, -1) >= 0 && P(13, -1) >= 0 && P(14, -1) >= 0 && P(15, -1) >= 0 &&
        P(-1, 0) >= 0 && P(-1, 1) >= 0 && P(-1, 2) >= 0 && P(-1, 3) >= 0 &&
        P(-1, 4) >= 0 && P(-1, 5) >= 0 && P(-1, 6) >= 0 && P(-1, 7) >= 0 &&
        P(-1, 8) >= 0 && P(-1, 9) >= 0 && P(-1, 10) >= 0 && P(-1, 11) >= 0 &&
        P(-1, 12) >= 0 && P(-1, 13) >= 0 && P(-1, 14) >= 0 && P(-1, 15) >= 0) {
      int32_t H = 0;
      int32_t V = 0;

      for (int32_t x = 0; x <= 7; x++) {
        H += (x + 1) * (P(8 + x, -1) - P(6 - x, -1));
      }

      for (int32_t y = 0; y <= 7; y++) {
        V += (y + 1) * (P(-1, 8 + y) - P(-1, 6 - y));
      }

      int32_t a = 16 * (P(-1, 15) + P(15, -1));
      int32_t b = (5 * H + 32) >> 6;
      int32_t c = (5 * V + 32) >> 6;

      for (int32_t y = 0; y <= 15; y++) {
        for (int32_t x = 0; x <= 15; x++) {
          cSL(x, y) = CLIP3(0, (1 << BitDepth) - 1,
                            (a + b * (x - 7) + c * (y - 7) + 16) >>
                                5); // //pred16x16L[y * 16 + x ] = Clip1Y( x ) =
                                    // Clip3( 0, ( 1 << BitDepthY ) − 1, x );
        }
      }
    }
  } else {
    printf("Intra16x16PredMode_of_CurrMbAddr(%d) must be [0,3]\n",
           Intra16x16PredMode_of_CurrMbAddr);
  }

#undef P
#undef cSL
#undef IsMbAff

  return 0;
}

// 8.3.4 Intra prediction process for chroma samples
int PictureBase::Intra_chroma_sample_prediction(uint8_t *pic_buff_chroma_pred,
                                                int32_t PicWidthInSamples) {
  if (m_slice.m_sps.ChromaArrayType == 3) // CHROMA_FORMAT_IDC_444
  {
    return Intra_chroma_sample_prediction_for_YUV444(pic_buff_chroma_pred,
                                                     PicWidthInSamples);
  } else // if (m_slice.m_sps.ChromaArrayType == 1 ||
         // m_slice.m_sps.ChromaArrayType == 2)
         // //CHROMA_FORMAT_IDC_420 || CHROMA_FORMAT_IDC_422
  {
    return Intra_chroma_sample_prediction_for_YUV420_or_YUV422(
        pic_buff_chroma_pred, PicWidthInSamples);
  }

  return 0;
}

// 8.3.4 Intra prediction process for chroma samples
int PictureBase::Intra_chroma_sample_prediction_for_YUV420_or_YUV422(
    uint8_t *pic_buff_chroma_pred, int32_t PicWidthInSamples) {
  int ret = 0;
  int32_t xO = 0;
  int32_t yO = 0;
  int32_t xW = 0;
  int32_t yW = 0;
  int32_t maxW = 0;
  int32_t maxH = 0;
  int32_t isChroma = 1;
  //    int32_t predC[256] = {0};

  MB_ADDR_TYPE mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
  int32_t mbAddrN = -1;
  int32_t luma4x4BlkIdxN = 0;
  int32_t luma8x8BlkIdxN = 0;

  //SliceHeader &slice_header = m_h264_slice_header;

  // int32_t SubWidthC = m_slice.m_sps.SubWidthC;
  // int32_t SubHeightC = m_slice.m_sps.SubHeightC;

  // The neighbouring samples p[ x, y ] that are constructed chroma samples
  // prior to the deblocking filter process, with x = −1,y = −1..MbHeightC − 1
  // and with x = 0..MbWidthC − 1, y = −1,
  // 因为MbWidthC和MbHeightC的最大值都为16，所以按照最大值来计算
  // int32_t neighbouring_samples_x[33] = {-1, -1, -1, -1, -1, -1, -1, -1, -1,
  // -1, -1, -1, -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
  // 11, 12, 13, 14, 15}; int32_t neighbouring_samples_y[33] = {-1,  0,  1,  2,
  // 3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1,
  // -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  int32_t neighbouring_samples_count = (MbHeightC + 1) + (MbWidthC + 0);

  //    int32_t * neighbouring_samples_x = (int32_t *)my_malloc(sizeof(int32_t)
  //    * neighbouring_samples_count); int32_t * neighbouring_samples_y =
  //    (int32_t *)my_malloc(sizeof(int32_t) * neighbouring_samples_count);

  int32_t neighbouring_samples_x[33] =
      {0}; // neighbouring_samples_count=17;此处在栈上按最大的尺寸申请,
           // MbHeightC=16; MbWidthC=16; 33=16+1+16;
  int32_t neighbouring_samples_y[33] = {0};

  memset(neighbouring_samples_x, -1,
         sizeof(int32_t) * neighbouring_samples_count);
  memset(neighbouring_samples_y, -1,
         sizeof(int32_t) * neighbouring_samples_count);

  for (int32_t i = -1; i <= (int32_t)MbHeightC - 1; i++) {
    neighbouring_samples_x[i + 1] = -1;
    neighbouring_samples_y[i + 1] = i;
  }

  for (int32_t i = 0; i <= (int32_t)MbWidthC - 1; i++) {
    neighbouring_samples_x[MbHeightC + 1 + i] = i;
    neighbouring_samples_y[MbHeightC + 1 + i] = -1;
  }

  //    int32_t * p = (int32_t *)my_malloc(sizeof(int32_t) * (MbHeightC + 1) *
  //    (MbWidthC + 1));
  //    //x范围[-1,15]，y范围[-1,15]，共17行17列，原点为pp[1][1]
  int32_t p[289]; // 81; 289=17*17;
  memset(p, -1, sizeof(int32_t) * (MbHeightC + 1) * (MbWidthC + 1));
#define P(x, y) p[((y) + 1) * (MbHeightC + 1) + ((x) + 1)]
// #define cSC(x, y)    pic_buff_chroma_pred[((mb_y) * MbHeightC + (y)) *
// PicWidthInSamples + ((mb_x) * MbWidthC + x)]
#define IsMbAff                                                                \
  ((m_slice.slice_header.MbaffFrameFlag == 1 &&                                \
    m_mbs[CurrMbAddr].mb_field_decoding_flag == 1)                             \
       ? 1                                                                     \
       : 0)
#define cSC(x, y)                                                              \
  pic_buff_chroma_pred                                                         \
      [(((m_mbs[CurrMbAddr].m_mb_position_y >> 4) * MbHeightC) +               \
        (m_mbs[CurrMbAddr].m_mb_position_y % 2) + (y) * (1 + IsMbAff)) *       \
           PicWidthInSamples +                                                 \
       ((m_mbs[CurrMbAddr].m_mb_position_x >> 4) * MbWidthC + (x))]

  for (int32_t i = 0; i < neighbouring_samples_count; i++) {
    // 6.4.12 Derivation process for neighbouring locations
    maxW = MbWidthC;
    maxH = MbHeightC;
    isChroma = 1;

    int32_t x = neighbouring_samples_x[i];
    int32_t y = neighbouring_samples_y[i];

    if (m_slice.slice_header.MbaffFrameFlag == 0) {
      ret = neighbouring_locations_non_MBAFF(
          xO + x, yO + y, maxW, maxH, CurrMbAddr, mbAddrN_type, mbAddrN,
          luma4x4BlkIdxN, luma8x8BlkIdxN, xW, yW, isChroma);
      RETURN_IF_FAILED(ret != 0, ret);
    } else // if (slice_header.MbaffFrameFlag == 1) //6.4.12.2 Specification for
           // neighbouring locations in MBAFF frames
    {
      ret = neighbouring_locations_MBAFF(xO + x, yO + y, maxW, maxH, CurrMbAddr,
                                         mbAddrN_type, mbAddrN, luma4x4BlkIdxN,
                                         luma8x8BlkIdxN, xW, yW, isChroma);
      RETURN_IF_FAILED(ret != 0, ret);
    }

    //----------------
    if (mbAddrN < 0 // mbAddrN is not available
        || (IS_INTER_Prediction_Mode(m_mbs[mbAddrN].m_mb_pred_mode) &&
            m_mbs[mbAddrN].constrained_intra_pred_flag == 1) ||
        (m_mbs[mbAddrN].m_name_of_mb_type == SI &&
         m_mbs[mbAddrN].constrained_intra_pred_flag == 1 &&
         m_mbs[CurrMbAddr].m_name_of_mb_type != SI)) {
      P(x, y) = -1; // the sample p[ x, y ] is marked as "not available for
                    // Intra chroma prediction"
    } else {
      int32_t xL = 0;
      int32_t yL = 0;
      int32_t xM = 0;
      int32_t yM = 0;

      // 6.4.1 Inverse macroblock scanning process
      ret = inverse_macroblock_scanning_process(
          m_slice.slice_header.MbaffFrameFlag, mbAddrN,
          m_mbs[mbAddrN].mb_field_decoding_flag, xL, yL);
      RETURN_IF_FAILED(ret != 0, ret);

      xM = (xL >> 4) * MbWidthC;
      yM = ((yL >> 4) * MbHeightC) + (yL % 2);

      //--------------------------
      if (m_slice.slice_header.MbaffFrameFlag == 1 &&
          m_mbs[mbAddrN].mb_field_decoding_flag == 1) {
        P(x, y) =
            pic_buff_chroma_pred[(yM + 2 * yW) * PicWidthInSamples +
                                 (xM + xW)]; // cSC[ xM + xW, yM + 2 * yW ];
      } else {
        P(x, y) = pic_buff_chroma_pred[(yM + yW) * PicWidthInSamples +
                                       (xM + xW)]; // cSC[ xM + xW, yM + yW ];
      }
    }
  }

  //----------4种帧内chroma预测模式----------------
  int32_t IntraChromaPredMode_of_CurrMbAddr =
      m_mbs[CurrMbAddr].intra_chroma_pred_mode;

  if (IntraChromaPredMode_of_CurrMbAddr ==
      0) // 8.3.4.1 Specification of Intra_Chroma_DC prediction mode
  {
    for (int32_t chroma4x4BlkIdx = 0;
         chroma4x4BlkIdx <= (1 << (m_slice.m_sps.ChromaArrayType + 1)) - 1;
         chroma4x4BlkIdx++) {
      // 6.4.7 Inverse 4x4 chroma block scanning process
      xO = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 0);
      yO = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 1);

      int32_t mean_value = 0;

      if ((xO == 0 && yO == 0) || (xO > 0 && yO > 0)) {
        if (P(0 + xO, -1) > 0 && P(1 + xO, -1) > 0 && P(2 + xO, -1) > 0 &&
            P(3 + xO, -1) > 0 && P(-1, 0 + yO) > 0 && P(-1, 1 + yO) > 0 &&
            P(-1, 2 + yO) > 0 && P(-1, 3 + yO) > 0) {
          mean_value = (P(0 + xO, -1) + P(1 + xO, -1) + P(2 + xO, -1) +
                        P(3 + xO, -1) + P(-1, 0 + yO) + P(-1, 1 + yO) +
                        P(-1, 2 + yO) + P(-1, 3 + yO) + 4) >>
                       3;
        } else if (!(P(0 + xO, -1) > 0 && P(1 + xO, -1) > 0 &&
                     P(2 + xO, -1) > 0 && P(3 + xO, -1) > 0) &&
                   (P(-1, 0 + yO) > 0 && P(-1, 1 + yO) > 0 &&
                    P(-1, 2 + yO) > 0 && P(-1, 3 + yO) > 0)) {
          mean_value = (P(-1, 0 + yO) + P(-1, 1 + yO) + P(-1, 2 + yO) +
                        P(-1, 3 + yO) + 2) >>
                       2;
        } else if ((P(0 + xO, -1) > 0 && P(1 + xO, -1) > 0 &&
                    P(2 + xO, -1) > 0 && P(3 + xO, -1) > 0) &&
                   !(P(-1, 0 + yO) > 0 && P(-1, 1 + yO) > 0 &&
                     P(-1, 2 + yO) > 0 && P(-1, 3 + yO) > 0)) {
          mean_value = (P(0 + xO, -1) + P(1 + xO, -1) + P(2 + xO, -1) +
                        P(3 + xO, -1) + 2) >>
                       2;
        } else // some samples p[ x + xO, −1 ], with x = 0..3, and some samples
               // p[ −1, y +yO ], with y = 0..3, are marked as "not available
               // for Intra chroma prediction"
        {
          mean_value = (1 << (m_slice.m_sps.BitDepthC - 1));
        }
      } else if (xO > 0 && yO == 0) {
        if (P(0 + xO, -1) > 0 && P(1 + xO, -1) > 0 && P(2 + xO, -1) > 0 &&
            P(3 + xO, -1) > 0) {
          mean_value = (P(0 + xO, -1) + P(1 + xO, -1) + P(2 + xO, -1) +
                        P(3 + xO, -1) + 2) >>
                       2;
        } else if (P(-1, 0 + yO) > 0 && P(-1, 1 + yO) > 0 &&
                   P(-1, 2 + yO) > 0 && P(-1, 3 + yO) > 0) {
          mean_value = (P(-1, 0 + yO) + P(-1, 1 + yO) + P(-1, 2 + yO) +
                        P(-1, 3 + yO) + 2) >>
                       2;
        } else // some samples p[ x + xO, −1 ], with x = 0..3, and some samples
               // p[ −1, y +yO ], with y = 0..3, are marked as "not available
               // for Intra chroma prediction"
        {
          mean_value = (1 << (m_slice.m_sps.BitDepthC - 1));
        }
      } else if (xO == 0 && yO > 0) {
        if (P(-1, 0 + yO) > 0 && P(-1, 1 + yO) > 0 && P(-1, 2 + yO) > 0 &&
            P(-1, 3 + yO) > 0) {
          mean_value = (P(-1, 0 + yO) + P(-1, 1 + yO) + P(-1, 2 + yO) +
                        P(-1, 3 + yO) + 2) >>
                       2;
        } else if (P(0 + xO, -1) > 0 && P(1 + xO, -1) > 0 &&
                   P(2 + xO, -1) > 0 && P(3 + xO, -1) > 0) {
          mean_value = (P(0 + xO, -1) + P(1 + xO, -1) + P(2 + xO, -1) +
                        P(3 + xO, -1) + 2) >>
                       2;
        } else // some samples p[ x + xO, −1 ], with x = 0..3, and some samples
               // p[ −1, y +yO ], with y = 0..3, are marked as "not available
               // for Intra chroma prediction"
        {
          mean_value = (1 << (m_slice.m_sps.BitDepthC - 1));
        }
      }

      for (int32_t y = 0; y <= 3; y++) {
        for (int32_t x = 0; x <= 3; x++) {
          cSC(x + xO, y + yO) =
              mean_value; // predC[(y + yO) * MbWidthC + (x + xO)]
        }
      }
    }
  } else if (IntraChromaPredMode_of_CurrMbAddr ==
             1) // 8.3.4.2 Specification of Intra_Chroma_Horizontal prediction
                // mode
  {
    int flag = 0;
    for (int32_t y = 0; y <= (int32_t)MbHeightC - 1; y++) {
      if (P(-1, y) < 0) {
        flag = 1;
        break;
      }
    }

    // This mode shall be used only when the samples p[ −1, y ] with y =
    // 0..MbHeightC − 1 are marked as "available for Intra chroma prediction".
    if (flag == 0) {
      for (int32_t y = 0; y <= (int32_t)MbHeightC - 1; y++) {
        for (int32_t x = 0; x <= (int32_t)MbWidthC - 1; x++) {
          cSC(x, y) = P(-1, y); // predC[y * MbWidthC + x]
        }
      }
    }
  } else if (IntraChromaPredMode_of_CurrMbAddr ==
             2) // 8.3.4.3 Specification of Intra_Chroma_Vertical prediction
                // mode
  {
    int flag = 0;
    for (int32_t x = 0; x <= (int32_t)MbWidthC - 1; x++) {
      if (P(x, -1) < 0) {
        flag = 1;
        break;
      }
    }

    // This mode shall be used only when the samples p[ x, −1 ] with x =
    // 0..MbWidthC − 1 are marked as "available for Intra chroma prediction".
    if (flag == 0) {
      for (int32_t y = 0; y <= (int32_t)MbHeightC - 1; y++) {
        for (int32_t x = 0; x <= (int32_t)MbWidthC - 1; x++) {
          cSC(x, y) = P(x, -1); // predC[y * MbWidthC + x]
        }
      }
    }
  } else if (IntraChromaPredMode_of_CurrMbAddr ==
             3) // 8.3.4.4 Specification of Intra_Chroma_Plane prediction mode
  {
    int flag = 0;
    for (int32_t x = 0; x <= (int32_t)MbWidthC - 1; x++) {
      if (P(x, -1) < 0) {
        flag = 1;
        break;
      }
    }

    for (int32_t y = -1; y <= (int32_t)MbHeightC - 1; y++) {
      if (P(-1, y) < 0) {
        flag = 1;
        break;
      }
    }

    // This mode shall be used only when the samples p[ x, −1 ], with x =
    // 0..MbWidthC − 1 and p[ −1, y ], with y = −1..MbHeightC − 1 are marked as
    // "available for Intra chroma prediction".
    if (flag == 0) {
      int32_t xCF = ((m_slice.m_sps.ChromaArrayType == 3) ? 4 : 0);
      int32_t yCF = ((m_slice.m_sps.ChromaArrayType != 1) ? 4 : 0);

      int32_t H = 0;
      int32_t V = 0;

      for (int32_t x1 = 0; x1 <= 3 + xCF; x1++) {
        H += (x1 + 1) * (P(4 + xCF + x1, -1) - P(2 + xCF - x1, -1));
      }

      for (int32_t y1 = 0; y1 <= 3 + yCF; y1++) {
        V += (y1 + 1) * (P(-1, 4 + yCF + y1) - P(-1, 2 + yCF - y1));
      }

      int32_t a = 16 * (P(-1, MbHeightC - 1) + P(MbWidthC - 1, -1));
      int32_t b =
          ((34 - 29 * (m_slice.m_sps.ChromaArrayType == 3)) * H + 32) >> 6;
      int32_t c =
          ((34 - 29 * (m_slice.m_sps.ChromaArrayType != 1)) * V + 32) >> 6;

      for (int32_t y = 0; y <= (int32_t)MbHeightC - 1; y++) {
        for (int32_t x = 0; x <= (int32_t)MbWidthC - 1; x++) {
          // predC[y * MbWidthC + x] = Clip1C( ( a + b * ( x - 3 - xCF ) + c * (
          // y - 3 - yCF ) + 16 ) >> 5 );
          cSC(x, y) =
              CLIP3(0, (1 << m_slice.m_sps.BitDepthC) - 1,
                    (a + b * (x - 3 - xCF) + c * (y - 3 - yCF) + 16) >> 5);
        }
      }
    }
  } else {
    printf("IntraChromaPredMode_of_CurrMbAddr(%d) must be [0,3]\n",
           IntraChromaPredMode_of_CurrMbAddr);
  }

#undef P
#undef cSC
#undef IsMbAff

  return 0;
}

// 8.3.4 Intra prediction process for chroma samples
// 8.3.4.5 Intra prediction for chroma samples with ChromaArrayType equal to 3
int PictureBase::Intra_chroma_sample_prediction_for_YUV444(
    uint8_t *pic_buff_chroma_pred, int32_t PicWidthInSamples) {
  int ret = 0;
  int32_t isChroma = 1;
  int32_t BitDepth = m_slice.m_sps.BitDepthC;

  if (m_mbs[CurrMbAddr].m_mb_pred_mode == Intra_4x4) {
    // The same process described in clause 8.3.1 is also applied to Cb or Cr
    // samples, substituting luma with Cb or Cr, substituting luma4x4BlkIdx with
    // cb4x4BlkIdx or cr4x4BlkIdx, substituting pred4x4L with pred4x4Cb or
    // pred4x4Cr, and substituting BitDepthY with BitDepthC.

    // The output variable Intra4x4PredMode[luma4x4BlkIdx] from the process
    // described in clause 8.3.1.1 is also used for the 4x4 Cb or 4x4 Cr blocks
    // with index luma4x4BlkIdx equal to index cb4x4BlkIdx or cr4x4BlkIdx.

    // The process to derive prediction Cb or Cr samples is identical to the
    // process described in clause 8.3.1.2 and its subsequent subclauses when
    // substituting luma with Cb or Cr, substituting pred4x4L with pred4x4Cb or
    // pred4x4Cr, and substituting BitDepthY with BitDepthC.
    for (int32_t chroma4x4BlkIdx = 0; chroma4x4BlkIdx < 16; chroma4x4BlkIdx++) {
      ret =
          Intra_4x4_sample_prediction(chroma4x4BlkIdx, PicWidthInSamples,
                                      pic_buff_chroma_pred, isChroma, BitDepth);
      RETURN_IF_FAILED(ret != 0, ret);
    }
  } else if (m_mbs[CurrMbAddr].m_mb_pred_mode == Intra_8x8) {
    // The same process described in clause 8.3.2 is also applied to Cb or Cr
    // samples, substituting luma with Cb or Cr, substituting luma8x8BlkIdx with
    // cb8x8BlkIdx or cr8x8BlkIdx, substituting pred8x8L with pred8x8Cb or
    // pred8x8Cr, and substituting BitDepthY with BitDepthC.

    // The output variable Intra8x8PredMode[luma8x8BlkIdx] from the process
    // described in clause 8.3.2.1 is used for the 8x8 Cb or 8x8 Cr blocks with
    // index luma8x8BlkIdx equal to index cb8x8BlkIdx or cr8x8BlkIdx.

    // The process to derive prediction Cb or Cr samples is identical to the
    // process described in clause 8.3.2.2 and its subsequent subclauses when
    // substituting luma with Cb or Cr, substituting pred8x8L with pred8x8Cb or
    // pred8x8Cr, and substituting BitDepthY with BitDepthC.

    for (int32_t chroma4x4BlkIdx = 0; chroma4x4BlkIdx < 4; chroma4x4BlkIdx++) {
      ret =
          Intra_8x8_sample_prediction(chroma4x4BlkIdx, PicWidthInSamples,
                                      pic_buff_chroma_pred, isChroma, BitDepth);
      RETURN_IF_FAILED(ret != 0, ret);
    }
  } else if (m_mbs[CurrMbAddr].m_mb_pred_mode == Intra_16x16) {
    // the same process described in clause 8.3.3 and its subsequent subclauses
    // is also applied to Cb or Cr samples, substituting luma with Cb or Cr,
    // substituting predL with predCb or predCr, and substituting BitDepthY with
    // BitDepthC.

    ret = Intra_16x16_sample_prediction(pic_buff_chroma_pred, PicWidthInSamples,
                                        isChroma, BitDepth);
    RETURN_IF_FAILED(ret != 0, ret);
  } else {
    printf("m_mbs[CurrMbAddr].m_mb_pred_mode = (%d) must be Intra_4x4(%d), "
           "Intra_8x8(%d) or Intra_16x16(%d).\n",
           m_mbs[CurrMbAddr].m_mb_pred_mode, Intra_4x4, Intra_8x8, Intra_16x16);
    return -1;
  }

  return 0;
}

// 8.3.5 Sample construction process for I_PCM macroblocks
// This process is invoked when mb_type is equal to I_PCM.
int PictureBase::Sample_construction_process_for_I_PCM_macroblocks() {
  int ret = 0;

  int32_t i = 0;
  int32_t dy = 0;

  //SliceHeader &slice_header = m_h264_slice_header;

  if (m_slice.slice_header.MbaffFrameFlag == 1 &&
      m_mbs[CurrMbAddr].mb_field_decoding_flag == 1) {
    dy = 2;
  } else // MbaffFrameFlag is equal to 0 or the current macroblock is a frame
         // macroblock
  {
    dy = 1;
  }

  int32_t xP = 0;
  int32_t yP = 0;

  // 6.4.1 Inverse macroblock scanning processy
  ret = inverse_macroblock_scanning_process(
      m_slice.slice_header.MbaffFrameFlag, CurrMbAddr,
      m_mbs[CurrMbAddr].mb_field_decoding_flag, xP, yP);
  RETURN_IF_FAILED(ret != 0, ret);

  //--------------------------------------
  for (i = 0; i < 256; ++i) {
    // S′L[ xP + ( i % 16 ), yP + dy * ( i / 16 ) ) ] = pcm_sample_luma[ i ];
    m_pic_buff_luma[(yP + dy * (i / 16)) * PicWidthInSamplesL +
                    (xP + (i % 16))] = m_mbs[CurrMbAddr].pcm_sample_luma[i];
  }

  if (m_slice.m_sps.ChromaArrayType != 0) {
    int32_t &SubWidthC = m_slice.m_sps.SubWidthC;
    int32_t &SubHeightC = m_slice.m_sps.SubHeightC;

    for (i = 0; i < (int32_t)(MbWidthC * MbHeightC); ++i) {
      // S′Cb[ ( xP / SubWidthC ) + ( i % MbWidthC ), ( ( yP + SubHeightC − 1 )
      // / SubHeightC ) + dy * ( i / MbWidthC ) ] = pcm_sample_chroma[ i ]
      m_pic_buff_cb[(((yP + SubHeightC - 1) / SubHeightC) +
                     dy * (i / MbWidthC)) *
                        PicWidthInSamplesC +
                    ((xP / SubWidthC) + (i % MbWidthC))] =
          m_mbs[CurrMbAddr].pcm_sample_chroma[i];

      // S′Cr[ ( xP / SubWidthC ) + ( i % MbWidthC ), ( ( yP + SubHeightC − 1 )
      // / SubHeightC ) + dy * ( i / MbWidthC ) ] = pcm_sample_chroma[ i +
      // MbWidthC * MbHeightC ]
      m_pic_buff_cr[(((yP + SubHeightC - 1) / SubHeightC) +
                     dy * (i / MbWidthC)) *
                        PicWidthInSamplesC +
                    ((xP / SubWidthC) + (i % MbWidthC))] =
          m_mbs[CurrMbAddr].pcm_sample_chroma[i + MbWidthC * MbHeightC];
    }
  }

  return 0;
}

// 6.4.1 Inverse macroblock scanning process
/* 输入: 宏块地址mbAddr。  
 * 输出: 地址为 mbAddr 的宏块的左上角亮度样本相对于图片左上角样本的位置 ( x, y )。*/
int PictureBase::inverse_macroblock_scanning_process(
    int32_t MbaffFrameFlag, int32_t mbAddr, int32_t mb_field_decoding_flag,
    int32_t &x, int32_t &y) {

  /* 逆宏块扫描过程具体如下： */
  if (MbaffFrameFlag == 0) {
    //光栅扫描顺序是从左到右、从上到下逐行扫描的顺序。
    x = InverseRasterScan(mbAddr, 16, 16, PicWidthInSamplesL, 0);
    y = InverseRasterScan(mbAddr, 16, 16, PicWidthInSamplesL, 1);
  } else {
    int32_t xO = InverseRasterScan(mbAddr / 2, 16, 32, PicWidthInSamplesL, 0);
    int32_t yO = InverseRasterScan(mbAddr / 2, 16, 32, PicWidthInSamplesL, 1);

    x = xO;
    if (mb_field_decoding_flag == 0)
      y = yO + (mbAddr % 2) * 16;
    else
      y = yO + (mbAddr % 2);
  }
  return 0;
}

// 6.4.2.2 Inverse sub-macroblock partition scanning process
int PictureBase::Inverse_sub_macroblock_partition_scanning_process(
    H264_MB_TYPE m_name_of_mb_type, int32_t mbPartIdx, int32_t subMbPartIdx,
    int32_t &x, int32_t &y) {

  if (m_name_of_mb_type == P_8x8 || m_name_of_mb_type == P_8x8ref0 ||
      m_name_of_mb_type == B_8x8) // FIXME:
  {                               /*
                                     ret = MacroBlock::SubMbPredModeFunc(m_h264_slice_header.slice_type,
                                     mb.sub_mb_type[ mbPartIdx ],                               NumSubMbPart,
                                     SubMbPredMode, SubMbPartWidth,                               SubMbPartHeight);                               RETURN_IF_FAILED(ret !=
                                     0, ret);
                              
                                     NumSubMbPart = mb.NumSubMbPart[mbPartIdx];
                                     int32_t partWidth = mb.SubMbPartWidth[mbPartIdx];
                                     int32_t partHeight = mb.SubMbPartHeight[mbPartIdx];
                              
                                     x = InverseRasterScan( subMbPartIdx, SubMbPartWidth( sub_mb_type[
                                     mbPartIdx ] ),SubMbPartHeight( sub_mb_type[ mbPartIdx ] ), 8, 0 );                               y =
                                     InverseRasterScan( subMbPartIdx, SubMbPartWidth( sub_mb_type[ mbPartIdx ]
                                     ), SubMbPartHeight( sub_mb_type[ mbPartIdx ] ), 8, 1 );*/
  } else                          // if (slice_header.MbaffFrameFlag == 1)
  {
    x = InverseRasterScan(subMbPartIdx, 4, 4, 8, 0);
    y = InverseRasterScan(subMbPartIdx, 4, 4, 8, 1);
  }

  return 0;
}

// 6.4.8 Derivation process of the availability for macroblock addresses
int PictureBase::
    Derivation_process_of_the_availability_for_macroblock_addresses(
        int32_t mbAddr, int32_t &is_mbAddr_available) {
  if (mbAddr < 0 || mbAddr > CurrMbAddr ||
      m_mbs[mbAddr].slice_number != m_mbs[CurrMbAddr].slice_number) {
    is_mbAddr_available = 0;
  } else {
    is_mbAddr_available = 1;
  }

  return 0;
}

// 6.4.11.2 Derivation process for neighbouring 8x8 luma block
int PictureBase::Derivation_process_for_neighbouring_8x8_luma_block(
    int32_t luma8x8BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
    int32_t &luma8x8BlkIdxA, int32_t &luma8x8BlkIdxB, int32_t isChroma) {
  int ret = 0;

  int32_t xW = 0;
  int32_t yW = 0;

  // 1. The difference of luma location ( xD, yD ) is set according to Table
  // 6-2.

  // 2. The luma location ( xN, yN ) is specified by
  // xN = ( luma8x8BlkIdx % 2 ) * 8 + xD;
  // yN = ( luma8x8BlkIdx / 2 ) * 8 + yD;

  // 3. The derivation process for neighbouring locations as specified in
  // clause 6.4.12 is invoked for luma locations with ( xN, yN ) as the input
  // and the output is assigned to mbAddrN and ( xW, yW ).

  //---------------mbAddrA---------------------
  MB_ADDR_TYPE mbAddrA_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma4x4BlkIdxA = 0;
  int32_t xA = (luma8x8BlkIdx % 2) * 8 - 1;
  int32_t yA = (luma8x8BlkIdx / 2) * 8 + 0;

  // 6.4.12 Derivation process for neighbouring locations
  ret = derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xA, yA, CurrMbAddr, mbAddrA_type,
      mbAddrA, luma4x4BlkIdxA, luma8x8BlkIdxA, xW, yW, isChroma);
  RETURN_IF_FAILED(ret != 0, ret);

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
  ret = derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xB, yB, CurrMbAddr, mbAddrB_type,
      mbAddrB, luma4x4BlkIdxB, luma8x8BlkIdxB, xW, yW, isChroma);
  RETURN_IF_FAILED(ret != 0, ret);

  if (mbAddrB < 0) {
    luma8x8BlkIdxB = -2; // marked as not available
  } else {
    // 6.4.13.3 Derivation process for 8x8 luma block indices
    luma8x8BlkIdxB = 2 * (yW / 8) + (xW / 8);
  }

  return 0;
}

// 6.4.11.3 Derivation process for neighbouring 8x8 chroma blocks for
// ChromaArrayType equal to 3
int PictureBase::
    Derivation_process_for_neighbouring_8x8_chroma_blocks_for_ChromaArrayType_equal_to_3(
        int32_t chroma8x8BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
        int32_t &chroma8x8BlkIdxA, int32_t &chroma8x8BlkIdxB) {
  int ret = 0;

  int32_t isChroma = 1;

  // 6.4.11.2 Derivation process for neighbouring 8x8 luma block
  ret = Derivation_process_for_neighbouring_8x8_luma_block(
      chroma8x8BlkIdx, mbAddrA, mbAddrB, chroma8x8BlkIdxA, chroma8x8BlkIdxB,
      isChroma);
  RETURN_IF_FAILED(ret != 0, -1);

  return 0;
}

// 6.4.11.4 Derivation process for neighbouring 4x4 luma blocks
int PictureBase::Derivation_process_for_neighbouring_4x4_luma_blocks(
    int32_t luma4x4BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
    int32_t &luma4x4BlkIdxA, int32_t &luma4x4BlkIdxB, int32_t isChroma) {
  int ret = 0;

  int32_t xW = 0;
  int32_t yW = 0;

  // 1. The difference of luma location ( xD, yD ) is set according to Table
  // 6-2.

  // 2. The inverse 4x4 luma block scanning process as specified in clause 6.4.3
  // is invoked with luma4x4BlkIdx as the input and ( x, y ) as the output.

  // 6.4.3 Inverse 4x4 luma block scanning process
  int32_t x = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
              InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
  int32_t y = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
              InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

  // 3. The luma location ( xN, yN ) is specified by:
  // xN = x + xD (6-25)
  // yN = y + yD (6-26)

  // 4. The derivation process for neighbouring locations as specified in
  // clause 6.4.12 is invoked for luma locations with ( xN, yN ) as the input
  // and the output is assigned to mbAddrN and ( xW, yW ).

  //---------------mbAddrA---------------------
  MB_ADDR_TYPE mbAddrA_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxA = 0;

  int32_t xA = x - 1;
  int32_t yA = y + 0;

  // 6.4.12 Derivation process for neighbouring locations
  ret = derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xA, yA, CurrMbAddr, mbAddrA_type,
      mbAddrA, luma4x4BlkIdxA, luma8x8BlkIdxA, xW, yW, isChroma);
  RETURN_IF_FAILED(ret != 0, ret);

  if (mbAddrA < 0) {
    luma4x4BlkIdxA = -2; // marked as not available
  } else {
    // 6.4.13.1 Derivation process for 4x4 luma block indices
    // ret = Derivation_process_for_4x4_luma_block_indices(xW, yW, (uint8_t
    // &)luma4x4BlkIdxA);
    luma4x4BlkIdxA =
        8 * (yW / 8) + 4 * (xW / 8) + 2 * ((yW % 8) / 4) + ((xW % 8) / 4);
  }

  //---------------mbAddrB---------------------
  MB_ADDR_TYPE mbAddrB_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxB = 0;

  int32_t xB = x + 0;
  int32_t yB = y - 1;

  // 6.4.12 Derivation process for neighbouring locations
  ret = derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xB, yB, CurrMbAddr, mbAddrB_type,
      mbAddrB, luma4x4BlkIdxB, luma8x8BlkIdxB, xW, yW, isChroma);
  RETURN_IF_FAILED(ret != 0, ret);

  if (mbAddrB < 0) {
    luma4x4BlkIdxB = -2; // marked as not available
  } else {
    // 6.4.13.1 Derivation process for 4x4 luma block indices
    // ret = Derivation_process_for_4x4_luma_block_indices(xW, yW, (uint8_t
    // &)luma4x4BlkIdxB);
    luma4x4BlkIdxB =
        8 * (yW / 8) + 4 * (xW / 8) + 2 * ((yW % 8) / 4) + ((xW % 8) / 4);
  }

  return 0;
}

// 6.4.11.5 Derivation process for neighbouring 4x4 chroma blocks
// This clause is only invoked when ChromaArrayType is equal to 1 or 2.
int PictureBase::Derivation_process_for_neighbouring_4x4_chroma_blocks(
    int32_t chroma4x4BlkIdx, int32_t &mbAddrA, int32_t &mbAddrB,
    int32_t &chroma4x4BlkIdxA, int32_t &chroma4x4BlkIdxB) {
  int ret = 0;

  int32_t isChroma = 1;
  int32_t xW = 0;
  int32_t yW = 0;

  // 1. The difference of chroma location ( xD, yD ) is set according to Table
  // 6-2.

  // 2. The inverse 4x4 chroma block scanning process as specified in
  // clause 6.4.7 is
  //    invoked with chroma4x4BlkIdx as the input and ( x, y ) as the output

  // 6.4.7 Inverse 4x4 chroma block scanning process
  int32_t x = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 0);
  int32_t y = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 1);

  // 3. The chroma location ( xN, yN ) is specified by
  // xN = x + xD
  // yN = y + yD

  // 4. The derivation process for neighbouring locations as specified in
  // clause 6.4.12 is invoked for chroma locations with ( xN, yN ) as the input
  // and the output is assigned to mbAddrN and ( xW, yW ).

  //---------------mbAddrA---------------------
  MB_ADDR_TYPE mbAddrA_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxA = 0;

  int32_t xA = x - 1;
  int32_t yA = y + 0;

  // 6.4.12 Derivation process for neighbouring locations
  ret = derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xA, yA, CurrMbAddr, mbAddrA_type,
      mbAddrA, chroma4x4BlkIdxA, luma8x8BlkIdxA, xW, yW, isChroma);
  RETURN_IF_FAILED(ret != 0, ret);

  if (mbAddrA < 0) {
    chroma4x4BlkIdxA = -2; // marked as not available
  } else {
    // 6.4.13.2 Derivation process for 4x4 chroma block indices
    // ret = Derivation_process_for_4x4_chroma_block_indices(xW, yW, (uint8_t
    // &)chroma4x4BlkIdxA);
    chroma4x4BlkIdxA = 2 * (yW / 4) + (xW / 4);
  }

  //---------------mbAddrB---------------------
  MB_ADDR_TYPE mbAddrB_type = MB_ADDR_TYPE_UNKOWN;
  int32_t luma8x8BlkIdxB = 0;

  int32_t xB = x + 0;
  int32_t yB = y - 1;

  // 6.4.12 Derivation process for neighbouring locations
  ret = derivation_for_neighbouring_locations(
      m_mbs[CurrMbAddr].MbaffFrameFlag, xB, yB, CurrMbAddr, mbAddrB_type,
      mbAddrB, chroma4x4BlkIdxB, luma8x8BlkIdxB, xW, yW, isChroma);
  RETURN_IF_FAILED(ret != 0, ret);

  if (mbAddrB < 0) {
    chroma4x4BlkIdxB = -2; // marked as not available
  } else {
    // 6.4.13.2 Derivation process for 4x4 chroma block indices
    // ret = Derivation_process_for_4x4_chroma_block_indices(xW, yW, (uint8_t
    // &)chroma4x4BlkIdxB);
    chroma4x4BlkIdxB = 2 * (yW / 4) + (xW / 4);
  }

  return 0;
}

// 6.4.12 Derivation process for neighbouring locations
/* 输入: 相对于当前宏块左上角表示的亮度或色度位置 ( xN, yN )
 * 输出： 
 * – mbAddrN：等于 CurrMbAddr 或等于包含 (xN, yN) 及其可用性状态的相邻宏块的地址， 
 * – ( xW, yW )：相对于当前宏块表示的位置 (xN, yN) mbAddrN 宏块的左上角（而不是相对于当前宏块的左上角）。
 * */
int PictureBase::derivation_for_neighbouring_locations(
    const int32_t MbaffFrameFlag, const int32_t xN, const int32_t yN,
    const int32_t currMbAddr, MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN,
    int32_t &b4x4BlkIdxN, int32_t &b8x8BlkIdxN, int32_t &xW, int32_t &yW,
    const int32_t isChroma) {

  /* maxW 和 maxH 分别为指定位置分量 xN、xW 和 yN、yW 的最大值的变量 */
  int32_t maxW = 0, maxH = 0;

  /* 邻近的亮度位置调用此过程 */
  if (isChroma == 0)
    maxW = maxH = 16;
  else {
    /* 相邻色度位置调用此过程 */
    maxW = MbWidthC;
    maxH = MbHeightC;
  }

  int ret = -1;

  if (MbaffFrameFlag == 0)
    /* 用第 6.4.12.1 节中描述的字段和非 MBAFF 帧中相邻位置的规范*/
    ret = neighbouring_locations_non_MBAFF(xN, yN, maxW, maxH, currMbAddr,
                                           mbAddrN_type, mbAddrN, b4x4BlkIdxN,
                                           b8x8BlkIdxN, xW, yW, isChroma);
  else
    /* 反之，用第 6.4.12.2 节中描述的 MBAFF 帧中相邻位置的规范 */
    ret = neighbouring_locations_MBAFF(xN, yN, maxW, maxH, currMbAddr,
                                       mbAddrN_type, mbAddrN, b4x4BlkIdxN,
                                       b8x8BlkIdxN, xW, yW, isChroma);
  if (ret != 0) {
    std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
              << std::endl;
    return ret;
  }

  return 0;
}

/* 6.4.8 Derivation process of the availability for macroblock addresses */
inline int PictureBase::derivation_of_availability_macroblock_addresses(
    int32_t _mbAddr, int32_t CurrMbAddr, MB_ADDR_TYPE &mbAddrN_type,
    int32_t &mbAddrN) {

  if (_mbAddr < 0 || _mbAddr > CurrMbAddr ||
      m_mbs[_mbAddr].slice_number != m_mbs[CurrMbAddr].slice_number) {
    //宏块被标记为不可用
    mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
    mbAddrN = -1;
  } else {
    //宏块被标记为可用
    mbAddrN_type = MB_ADDR_TYPE_mbAddrA;
    mbAddrN = _mbAddr;
  }
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
    const int32_t xN, const int32_t yN, const int32_t maxW, const int32_t maxH,
    const int32_t CurrMbAddr, MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN) {

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
    if (CurrMbAddr % PicWidthInMbs == 0) {
      mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
      mbAddrN = -1;
    } else {
      mbAddrN_type = MB_ADDR_TYPE_mbAddrA;
      mbAddrN = mbAddrA;
    }
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
    if ((CurrMbAddr + 1) % PicWidthInMbs == 0) {
      mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
      mbAddrN = -1;
    } else {
      mbAddrN_type = MB_ADDR_TYPE_mbAddrC;
      mbAddrN = mbAddrC;
    }
  }
  /* 左上宏快 */
  else if (xN < 0 && yN < 0) {
    /* 第 6.4.8 节中的过程的输入是 mbAddrD = CurrMbAddr − PicWidthInMbs − 1，输出是宏块 mbAddrD 是否可用。此外，当 CurrMbAddr % PicWidthInMbs 等于 0 时，mbAddrD 被标记为不可用 */
    derivation_of_availability_macroblock_addresses(mbAddrD, CurrMbAddr,
                                                    mbAddrN_type, mbAddrN);
    if (CurrMbAddr % PicWidthInMbs == 0) {
      mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
      mbAddrN = -1;
    } else {
      mbAddrN_type = MB_ADDR_TYPE_mbAddrD;
      mbAddrN = mbAddrD;
    }
  } else if ((xN >= 0 && xN <= maxW - 1) && (yN >= 0 && yN <= maxH - 1)) {
    /* 当前宏块 */
    mbAddrN_type = MB_ADDR_TYPE_CurrMbAddr;
    mbAddrN = CurrMbAddr;
  } else if ((xN > maxW - 1) && (yN >= 0 && yN <= maxH - 1)) {
    /* not available */
  } else if (yN > maxH - 1) {
    /* not available */
  }
  return 0;
}

// 6.4.12.1 Specification for neighbouring locations in fields and non-MBAFF frames
int PictureBase::neighbouring_locations_non_MBAFF(
    const int32_t xN, const int32_t yN, const int32_t maxW, const int32_t maxH,
    const int32_t CurrMbAddr, MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN,
    int32_t &b4x4BlkIdx, int32_t &b8x8BlkIdxN, int32_t &xW, int32_t &yW,
    const int32_t isChroma) {

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

// 6.4.12.2 Specification for neighbouring locations in MBAFF frames
// Table 6-4 – Specification of mbAddrN and yM
int PictureBase::neighbouring_locations_MBAFF(
    const int32_t xN, const int32_t yN, const int32_t maxW, const int32_t maxH,
    const int32_t CurrMbAddr, MB_ADDR_TYPE &mbAddrN_type, int32_t &mbAddrN,
    int32_t &b4x4BlkIdxN, int32_t &b8x8BlkIdxN, int32_t &xW, int32_t &yW,
    const int32_t isChroma) {

  int32_t yM = 0;
  mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
  mbAddrN = -1;

  /* 第 6.4.10 节中相邻宏块地址及其可用性的推导过程是通过 mbAddrA、mbAddrB、mbAddrC 和 mbAddrD 以及它们的可用性状态作为输出来调用的。 */
  int32_t mbAddrA, mbAddrB, mbAddrC, mbAddrD;
  /* Table 6-4 – Specification of mbAddrN and yM */
  derivation_for_neighbouring_macroblock_addr_availability_in_MBAFF(
      mbAddrA, mbAddrB, mbAddrC, mbAddrD);

  /* 变量 currMbFrameFlag 的推导如下： 
   * – 如果地址为 CurrMbAddr 的宏块是帧宏块，则 currMbFrameFlag 设置为等于 1。 
   * – 否则（地址为 CurrMbAddr 的宏块是场宏块），则 currMbFrameFlag 设置为等于 0 */
  int32_t currMbFrameFlag =
      m_mbs[CurrMbAddr].mb_field_decoding_flag == 0 ? 1 : 0;

  /* 变量 mbIsTopMbFlag 的推导如下： 
   * – 如果地址为 CurrMbAddr 的宏块是顶部宏块（即 CurrMbAddr % 2 等于 0），则 mbIsTopMbFlag 设置为等于 1。 
   * – 否则（地址为 CurrMbAddr 的宏块是底部宏块） ，即 CurrMbAddr % 2 等于 1)，mbIsTopMbFlag 设置为等于 0。 */
  int32_t mbIsTopMbFlag = CurrMbAddr % 2 == 0 ? 1 : 0;

  /* Table 6-4 – Specification of mbAddrN and yM */
  int32_t mbAddrX = -1, mbAddrXFrameFlag = 0;
  if (xN < 0 && yN < 0) {
    if (currMbFrameFlag == 1) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrD;
        mbAddrN_type = MB_ADDR_TYPE_mbAddrD_add_1;
        mbAddrN = mbAddrD + 1;
        yM = yN;
      } else if (mbIsTopMbFlag == 0) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag =
              (m_mbs[mbAddrX].mb_field_decoding_flag == 1) ? 0 : 1;
          if (mbAddrXFrameFlag == 1) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA;
            mbAddrN = mbAddrA;
            yM = yN;
          } else if (mbAddrXFrameFlag == 0) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1;
            mbAddrN = mbAddrA + 1;
            yM = (yN + maxH) >> 1;
          }
        }
      }
    } else if (currMbFrameFlag == 0) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrD;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag =
              (m_mbs[mbAddrX].mb_field_decoding_flag == 1) ? 0 : 1;
          if (mbAddrXFrameFlag == 1) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrD_add_1;
            mbAddrN = mbAddrD + 1;
            yM = 2 * yN;
          } else if (mbAddrXFrameFlag == 0) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrD;
            mbAddrN = mbAddrD;
            yM = yN;
          }
        }
      } else if (mbIsTopMbFlag == 0) {
        mbAddrX = mbAddrD;
        mbAddrN_type = MB_ADDR_TYPE_mbAddrD_add_1;
        mbAddrN = mbAddrD + 1;
        yM = yN;
      }
    }
  } else if (xN < 0 && (yN >= 0 && yN <= maxH - 1)) {
    if (currMbFrameFlag == 1) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag =
              (m_mbs[mbAddrX].mb_field_decoding_flag == 1) ? 0 : 1;
          if (mbAddrXFrameFlag == 1) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA;
            mbAddrN = mbAddrA;
            yM = yN;
          } else if (mbAddrXFrameFlag == 0) {
            if (yN % 2 == 0) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA;
              mbAddrN = mbAddrA;
              yM = yN >> 1;
            } else if (yN % 2 != 0) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1;
              mbAddrN = mbAddrA + 1;
              yM = yN >> 1;
            }
          }
        }
      } else if (mbIsTopMbFlag == 0) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag =
              (m_mbs[mbAddrX].mb_field_decoding_flag == 1) ? 0 : 1;
          if (mbAddrXFrameFlag == 1) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1;
            mbAddrN = mbAddrA + 1;
            yM = yN;
          } else if (mbAddrXFrameFlag == 0) {
            if (yN % 2 == 0) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA;
              mbAddrN = mbAddrA;
              yM = (yN + maxH) >> 1;
            } else if (yN % 2 != 0) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1;
              mbAddrN = mbAddrA + 1;
              yM = (yN + maxH) >> 1;
            }
          }
        }
      }
    } else if (currMbFrameFlag == 0) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag =
              (m_mbs[mbAddrX].mb_field_decoding_flag == 1) ? 0 : 1;
          if (mbAddrXFrameFlag == 1) {
            if (yN < (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA;
              mbAddrN = mbAddrA;
              yM = yN << 1;
            } else if (yN >= (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1;
              mbAddrN = mbAddrA + 1;
              yM = (yN << 1) - maxH;
            }
          } else if (mbAddrXFrameFlag == 0) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA;
            mbAddrN = mbAddrA;
            yM = yN;
          }
        }
      } else if (mbIsTopMbFlag == 0) {
        mbAddrX = mbAddrA;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag =
              (m_mbs[mbAddrX].mb_field_decoding_flag == 1) ? 0 : 1;
          if (mbAddrXFrameFlag == 1) {
            if (yN < (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA;
              mbAddrN = mbAddrA;
              yM = (yN << 1) + 1;
            } else if (yN >= (maxH / 2)) {
              mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1;
              mbAddrN = mbAddrA + 1;
              yM = (yN << 1) + 1 - maxH;
            }
          } else if (mbAddrXFrameFlag == 0) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrA_add_1;
            mbAddrN = mbAddrA + 1;
            yM = yN;
          }
        }
      }
    }
  } else if ((xN >= 0 && xN <= maxW - 1) && yN < 0) {
    if (currMbFrameFlag == 1) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrB;
        mbAddrN_type = MB_ADDR_TYPE_mbAddrB_add_1;
        mbAddrN = mbAddrB + 1;
        yM = yN;
      } else if (mbIsTopMbFlag == 0) {
        mbAddrX = CurrMbAddr;
        mbAddrN_type = MB_ADDR_TYPE_CurrMbAddr_minus_1;
        mbAddrN = CurrMbAddr - 1;
        yM = yN;
      }
    } else if (currMbFrameFlag == 0) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrB;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag =
              (m_mbs[mbAddrX].mb_field_decoding_flag == 1) ? 0 : 1;
          if (mbAddrXFrameFlag == 1) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrB_add_1;
            mbAddrN = mbAddrB + 1;
            yM = 2 * yN;
          } else if (mbAddrXFrameFlag == 0) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrB;
            mbAddrN = mbAddrB;
            yM = yN;
          }
        }
      } else if (mbIsTopMbFlag == 0) {
        mbAddrX = mbAddrB;
        mbAddrN_type = MB_ADDR_TYPE_mbAddrB_add_1;
        mbAddrN = mbAddrB + 1;
        yM = yN;
      }
    }
  } else if ((xN >= 0 && xN <= maxW - 1) && (yN >= 0 && yN <= maxH - 1)) {
    mbAddrX = CurrMbAddr;
    mbAddrN_type = MB_ADDR_TYPE_CurrMbAddr;
    mbAddrN = CurrMbAddr;
    yM = yN;
  } else if (xN > maxW - 1 && yN < 0) {
    if (currMbFrameFlag == 1) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrC;
        mbAddrN_type = MB_ADDR_TYPE_mbAddrC_add_1;
        mbAddrN = mbAddrC + 1;
        yM = yN;
      } else if (mbIsTopMbFlag == 0) {
        mbAddrX = -2;
        mbAddrN_type = MB_ADDR_TYPE_UNKOWN;
        mbAddrN = -2;
        yM = NA;
      }
    } else if (currMbFrameFlag == 0) {
      if (mbIsTopMbFlag == 1) {
        mbAddrX = mbAddrC;
        if (mbAddrX >= 0) {
          mbAddrXFrameFlag =
              (m_mbs[mbAddrX].mb_field_decoding_flag == 1) ? 0 : 1;
          if (mbAddrXFrameFlag == 1) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrC_add_1;
            mbAddrN = mbAddrC + 1;
            yM = 2 * yN;
          } else if (mbAddrXFrameFlag == 0) {
            mbAddrN_type = MB_ADDR_TYPE_mbAddrC;
            mbAddrN = mbAddrC;
            yM = yN;
          }
        }
      } else if (mbIsTopMbFlag == 0) {
        mbAddrX = mbAddrC;
        mbAddrN_type = MB_ADDR_TYPE_mbAddrC_add_1;
        mbAddrN = mbAddrC + 1;
        yM = yN;
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

// 8.5.1 Specification of transform decoding process for 4x4 luma residual
int PictureBase::transform_decoding_for_4x4_luma_residual_blocks(
    int32_t isChroma, int32_t isChromaCb, int32_t BitDepth,
    int32_t PicWidthInSamples, uint8_t *pic_buff) {
  int ret = 0;

  /* ------------------ 设置别名 ------------------ */
  MacroBlock &mb = m_mbs[CurrMbAddr];
  bool isMbAff =
      (m_slice.slice_header.MbaffFrameFlag && mb.mb_field_decoding_flag);
  /* ------------------  End ------------------ */

  /* 当当前宏块预测模式不等于Intra_16x16时，变量LumaLevel4x4包含亮度变换系数的级别。对于由 luma4x4BlkIdx = 0..15 索引的 4x4 亮度块，指定以下有序步骤： */
  if (mb.m_mb_pred_mode != Intra_16x16) {
    /* TODO YangJing 这里为什么有一个缩放举证？ <24-09-08 02:52:34> */
    ret = scaling_functions(isChroma, isChromaCb);
    RET(ret);
    for (int32_t luma4x4BlkIdx = 0; luma4x4BlkIdx <= 15; luma4x4BlkIdx++) {
      //1. 使用 LumaLevel4x4[ luma4x4BlkIdx ] 作为输入和二维数组 c 作为输出来调用第 8.5.6 节中指定的 4x4 变换系数和缩放列表的逆扫描过程。
      int32_t c[4][4] = {{0}};
      ret = inverse_scanning_for_4x4_transform_coefficients_and_scaling_lists(
          mb.LumaLevel4x4[luma4x4BlkIdx], c,
          mb.field_pic_flag | mb.mb_field_decoding_flag);
      RET(ret);

      //2. 使用 c 作为输入和 r 作为输出来调用第 8.5.12 节中指定的残差 4x4 块的缩放和变换过程。（反量化与反整数变换）
      int32_t r[4][4] = {{0}};
      ret = scaling_and_transformation_process_for_residual_4x4_blocks(
          c, r, isChroma, isChromaCb);
      RET(ret);

      //3. 当 TransformBypassModeFlag 等于 1，宏块预测模式等于 Intra_4x4，并且 Intra4x4PredMode[ luma4x4BlkIdx ] 等于 0 或 1 时，使用 nW 设置调用第 8.5.15 节中指定的帧内残差变换旁路解码过程。等于4，nH设置为等于4，horPredFlag设置为等于Intra4x4PredMode[luma4x4BlkIdx]，并且4x4数组r作为输入，并且输出是4x4数组r的修改版本。
      if (mb.TransformBypassModeFlag && mb.m_mb_pred_mode == Intra_4x4 &&
          (mb.Intra4x4PredMode[luma4x4BlkIdx] & ~1) == 0) {
        int32_t horPredFlag = mb.Intra4x4PredMode[luma4x4BlkIdx];
        // 8.5.15 Intra residual transform-bypass decoding process
        intra_residual_transform_bypass_decoding(4, 4, horPredFlag, &r[0][0]);
      }

      //4. 宏块内部具有索引 luma4x4BlkIdx 的 4x4 亮度块的左上角样本的位置是通过调用第 6.4.3 节中的逆 4x4 亮度块扫描过程来导出的，其中 luma4x4BlkIdx 作为输入，输出被分配给 ( xO ，yO ）
      // 6.4.3 Inverse 4x4 luma block scanning process
      /* 逆 4x4 亮度块扫描过程由下式指定: */
      int32_t xO = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
                   InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);
      int32_t yO = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
                   InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);

      //--------帧内预测------------
      ret = Intra_4x4_sample_prediction(luma4x4BlkIdx, PicWidthInSamples,
                                        pic_buff, isChroma, BitDepth);
      RET(ret);

      // 5.4x4 数组 u 的元素为 uij，i, j = 0..3 的推导如下：
      int32_t u[16] = {0};
      for (int32_t i = 0; i < 4; i++) {
        for (int32_t j = 0; j < 4; j++) {
          int32_t y = mb.m_mb_position_y + (yO + i) * (1 + isMbAff);
          int32_t x = mb.m_mb_position_x + (xO + j);
          u[i * 4 + j] =
              Clip1C(pic_buff[y * PicWidthInSamples + x] + r[i][j], BitDepth);
        }
      }

      //6. 使用 u 和 luma4x4BlkIdx 作为输入来调用第 8.5.14 节中的去块滤波器过程之前的图像构造过程。
      ret = picture_construction_process_prior_to_deblocking_filter(
          u, 4, 4, luma4x4BlkIdx, isChroma, PicWidthInSamples, pic_buff);
      RET(ret);
    }
  }

  return 0;
}

//8.5.15 Intra residual transform-bypass decoding process( 帧内残差变换-旁路解码过程)
/* 当TransformBypassModeFlag等于1，宏块预测模式等于Intra_4x4、Intra_8x8或Intra_16x16，并且适用的帧内预测模式等于垂直或水平模式时，调用该过程。 Cb 和 Cr 分量的处理方式与亮度（L 或 Y）分量的处理方式相同。  
 * 输入： – 两个变量 nW 和 nH， – 变量 horPredFlag， – 带有元素 rij 的 (nW)x(nH) 数组 r，它是与亮度分量的残差变换旁路块相关的数组，或者与 Cb 和 Cr 分量的残差变换旁路块相关的数组。  
 * 输出: (nW)x(nH) 数组 r 的修改版本，其中元素 rij 包含帧内残差变换旁路解码过程的结果。 */
int PictureBase::intra_residual_transform_bypass_decoding(int32_t nW,
                                                          int32_t nH,
                                                          int32_t horPredFlag,
                                                          int32_t *r) {
  /* 设 f 是一个临时 (nW)x(nH) 数组，其中元素 fij 是通过以下方式导出的： */
  int32_t f[4][4] = {{0}};
  for (int32_t i = 0; i <= nH - 1; i++)
    for (int32_t j = 0; j <= nW - 1; j++)
      f[i][j] = r[i * nW + j];

  /* 根据 horPredFlag，以下情况适用： 
   * – 如果 horPredFlag 等于 0，则修改后的数组 r 通过以下方式导出： */
  if (horPredFlag == 0) {
    for (int32_t i = 0; i <= nH - 1; i++)
      for (int32_t j = 0; j <= nW - 1; j++) {
        r[i * nH + j] = 0;
        for (int32_t k = 0; k <= i; k++)
          r[i * nW + j] += f[k][j];
      }
  } else {
    for (int32_t i = 0; i <= nH - 1; i++)
      for (int32_t j = 0; j <= nW - 1; j++) {
        r[i * nW + j] = 0;
        for (int32_t k = 0; k <= j; k++)
          r[i * nW + j] += f[i][k];
      }
  }
  return 0;
}

// 8.5.2 Specification of transform decoding process for luma samples of
// Intra_16x16 macroblock prediction mode
int PictureBase::
    transform_decoding_for_luma_samples_of_Intra_16x16_macroblock_prediction(
        int32_t isChroma, int32_t BitDepth, int32_t QP1,
        int32_t PicWidthInSamples, int32_t Intra16x16DCLevel[16],
        int32_t Intra16x16ACLevel[16][16], uint8_t *pic_buff) {
  int ret = 0;

  ret = scaling_functions(isChroma, 0);
  RETURN_IF_FAILED(ret != 0, ret);

  // The 4x4 luma DC transform coefficients of all 4x4 luma blocks of the
  // macroblock are decoded.
  int32_t c2[4][4] = {{0}};
  int32_t dcY[4][4] = {{0}};

  ret = inverse_scanning_for_4x4_transform_coefficients_and_scaling_lists(
      Intra16x16DCLevel, c2,
      m_mbs[CurrMbAddr].field_pic_flag |
          m_mbs[CurrMbAddr].mb_field_decoding_flag);
  RETURN_IF_FAILED(ret != 0, ret);

  ret =
      Scaling_and_transformation_process_for_DC_transform_coefficients_for_Intra_16x16_macroblock_type(
          BitDepth, QP1, c2, dcY);
  RETURN_IF_FAILED(ret != 0, ret);

  // raster scan
  int32_t dcY_to_luma_index[16] = {
      dcY[0][0], dcY[0][1], dcY[1][0], dcY[1][1], dcY[0][2], dcY[0][3],
      dcY[1][2], dcY[1][3], dcY[2][0], dcY[2][1], dcY[3][0], dcY[3][1],
      dcY[2][2], dcY[2][3], dcY[3][2], dcY[3][3],
  };

  int32_t rMb[16][16] = {{0}};

  int32_t isMbAff = (m_slice.slice_header.MbaffFrameFlag == 1 &&
                     m_mbs[CurrMbAddr].mb_field_decoding_flag == 1)
                        ? 1
                        : 0;

  for (int32_t _4x4BlkIdx = 0; _4x4BlkIdx <= 15;
       _4x4BlkIdx++) // luma4x4BlkIdx or cb4x4BlkIdx or cr4x4BlkIdx
  {
    int32_t lumaList[16]; // or CbList[16] or CrList[16]

    lumaList[0] = dcY_to_luma_index[_4x4BlkIdx];

    for (int32_t k = 1; k <= 15; k++) {
      lumaList[k] = Intra16x16ACLevel[_4x4BlkIdx][k - 1];
    }

    // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling
    // lists
    int32_t c[4][4] = {{0}};
    int32_t r[4][4] = {{0}};
    ret = inverse_scanning_for_4x4_transform_coefficients_and_scaling_lists(
        lumaList, c,
        m_mbs[CurrMbAddr].field_pic_flag |
            m_mbs[CurrMbAddr].mb_field_decoding_flag);
    RETURN_IF_FAILED(ret != 0, ret);

    int32_t isChroma = 0;
    int32_t isChromaCb = 0;
    ret = scaling_and_transformation_process_for_residual_4x4_blocks(
        c, r, isChroma, isChromaCb);
    RETURN_IF_FAILED(ret != 0, ret);

    // 6.4.3 Inverse 4x4 luma block scanning process
    // InverseRasterScan = (a % (d / b) ) * b;    if e == 0;
    // InverseRasterScan = (a / (d / b) ) * c;    if e == 1;
    int32_t xO = InverseRasterScan(_4x4BlkIdx / 4, 8, 8, 16, 0) +
                 InverseRasterScan(_4x4BlkIdx % 4, 4, 4, 8, 0);
    int32_t yO = InverseRasterScan(_4x4BlkIdx / 4, 8, 8, 16, 1) +
                 InverseRasterScan(_4x4BlkIdx % 4, 4, 4, 8, 1);

    for (int32_t i = 0; i <= 3; i++) {
      for (int32_t j = 0; j <= 3; j++) {
        rMb[yO + i][xO + j] = r[i][j];
      }
    }
  }

  //--------------------------------------------------
  if (m_mbs[CurrMbAddr].TransformBypassModeFlag == 1 &&
      (m_mbs[CurrMbAddr].Intra16x16PredMode == 0 ||
       m_mbs[CurrMbAddr].Intra16x16PredMode == 1)) {
    // 8.5.15 Intra residual transform-bypass decoding process
    int32_t nW = 16;
    int32_t nH = 16;
    int32_t horPredFlag = m_mbs[CurrMbAddr].Intra16x16PredMode;

    int32_t f[16][16];
    for (int32_t i = 0; i <= nH - 1; i++) {
      for (int32_t j = 0; j <= nW - 1; j++) {
        f[i][j] = rMb[i][j];
      }
    }

    if (horPredFlag == 0) {
      for (int32_t i = 0; i <= nH - 1; i++) {
        for (int32_t j = 0; j <= nW - 1; j++) {
          rMb[i][j] = 0;
          for (int32_t k = 0; k <= i; k++) {
            rMb[i][j] += f[k][j];
          }
        }
      }
    } else // if (horPredFlag == 1)
    {
      for (int32_t i = 0; i <= nH - 1; i++) {
        for (int32_t j = 0; j <= nW - 1; j++) {
          rMb[i][j] = 0;
          for (int32_t k = 0; k <= j; k++) {
            rMb[i][j] += f[i][k];
          }
        }
      }
    }
  }

  //--------------------------------------
  //--------帧内预测------------
  ret = Intra_16x16_sample_prediction(pic_buff, PicWidthInSamples, isChroma,
                                      BitDepth);
  RETURN_IF_FAILED(ret != 0, ret);

  int32_t u[16 * 16] = {0};

  for (int32_t i = 0; i <= 15; i++) {
    for (int32_t j = 0; j <= 15; j++) {
      // uij = Clip1Y( predL[ j, i ] + rMb[ j, i ] )  = Clip3( 0, ( 1 <<
      // BitDepthY ) − 1, x ); u[i * 16 + j] = CLIP3(0, (1 << BitDepth) - 1,
      // pic_buff[(mb_y * 16  + i) * PicWidthInSamples + (mb_x * 16 + j)] +
      // rMb[i][j]);
      u[i * 16 + j] =
          CLIP3(0, (1 << BitDepth) - 1,
                pic_buff[(m_mbs[CurrMbAddr].m_mb_position_y +
                          (0 + (i)) * (1 + isMbAff)) *
                             PicWidthInSamples +
                         (m_mbs[CurrMbAddr].m_mb_position_x + (0 + (j)))] +
                    rMb[i][j]);
    }
  }

  int32_t BlkIdx = 0;
  ret = picture_construction_process_prior_to_deblocking_filter(
      u, 16, 16, BlkIdx, isChroma, PicWidthInSamples, pic_buff);
  RETURN_IF_FAILED(ret != 0, ret);

  return 0;
}

// 8.5.3 Specification of transform decoding process for 8x8 luma residual
// blocks This specification applies when transform_size_8x8_flag is equal to 1.
int PictureBase::transform_decoding_for_8x8_luma_residual_blocks(
    int32_t isChroma, int32_t isChromaCb, int32_t BitDepth,
    int32_t PicWidthInSamples, int32_t Level8x8[4][64], uint8_t *pic_buff) {
  int ret = 0;

  ret = scaling_functions(isChroma, isChromaCb);

  int32_t isMbAff = (m_slice.slice_header.MbaffFrameFlag == 1 &&
                     m_mbs[CurrMbAddr].mb_field_decoding_flag == 1)
                        ? 1
                        : 0;

  for (int32_t luma8x8BlkIdx = 0; luma8x8BlkIdx <= 3;
       luma8x8BlkIdx++) // or cb8x8BlkIdx or cr8x8BlkIdx
  {
    int32_t c[8][8] = {{0}};
    int32_t r[8][8] = {{0}};

    ret =
        Inverse_scanning_process_for_8x8_transform_coefficients_and_scaling_lists(
            Level8x8[luma8x8BlkIdx], c,
            m_mbs[CurrMbAddr].field_pic_flag |
                m_mbs[CurrMbAddr].mb_field_decoding_flag);
    RETURN_IF_FAILED(ret != 0, ret);

    ret = Scaling_and_transformation_process_for_residual_8x8_blocks(
        c, r, isChroma, isChromaCb);
    RETURN_IF_FAILED(ret != 0, ret);

    if (m_mbs[CurrMbAddr].TransformBypassModeFlag == 1 &&
        m_mbs[CurrMbAddr].m_mb_pred_mode == Intra_8x8 &&
        (m_mbs[CurrMbAddr].Intra8x8PredMode[luma8x8BlkIdx] == 0 ||
         m_mbs[CurrMbAddr].Intra8x8PredMode[luma8x8BlkIdx] == 1)) {
      // 8.5.15 Intra residual transform-bypass decoding process
      int32_t nW = 8;
      int32_t nH = 8;
      int32_t horPredFlag = m_mbs[CurrMbAddr].Intra8x8PredMode[luma8x8BlkIdx];

      int32_t f[8][8];
      for (int32_t i = 0; i <= nH - 1; i++) {
        for (int32_t j = 0; j <= nW - 1; j++) {
          f[i][j] = r[i][j];
        }
      }

      if (horPredFlag == 0) {
        for (int32_t i = 0; i <= nH - 1; i++) {
          for (int32_t j = 0; j <= nW - 1; j++) {
            r[i][j] = 0;
            for (int32_t k = 0; k <= i; k++) {
              r[i][j] += f[k][j];
            }
          }
        }
      } else // if (horPredFlag == 1)
      {
        for (int32_t i = 0; i <= nH - 1; i++) {
          for (int32_t j = 0; j <= nW - 1; j++) {
            r[i][j] = 0;
            for (int32_t k = 0; k <= j; k++) {
              r[i][j] += f[i][k];
            }
          }
        }
      }
    }

    // 6.4.5 Inverse 8x8 luma block scanning process
    // InverseRasterScan = (a % (d / b) ) * b;    if e == 0;
    // InverseRasterScan = (a / (d / b) ) * c;    if e == 1;
    int32_t xO = InverseRasterScan(luma8x8BlkIdx, 8, 8, 16, 0);
    int32_t yO = InverseRasterScan(luma8x8BlkIdx, 8, 8, 16, 1);

    //--------帧内预测------------
    ret = Intra_8x8_sample_prediction(luma8x8BlkIdx, PicWidthInSamples,
                                      pic_buff, isChroma, BitDepth);
    RETURN_IF_FAILED(ret != 0, ret);

    int32_t u[64] = {0};

    for (int32_t i = 0; i <= 7; i++) {
      for (int32_t j = 0; j <= 7; j++) {
        // uij = Clip1Y( predL[ xO + j, yO + i ] + rij ) = Clip3( 0, ( 1 <<
        // BitDepthY ) − 1, x ); u[i * 8 + j] = CLIP3(0, (1 << BitDepth) - 1,
        // pic_buff[(mb_y * 16 + (yO + i)) * PicWidthInSamples + (mb_x * 16 +
        // (xO + j))] + r[i][j]);
        u[i * 8 + j] =
            CLIP3(0, (1 << BitDepth) - 1,
                  pic_buff[(m_mbs[CurrMbAddr].m_mb_position_y +
                            (yO + (i)) * (1 + isMbAff)) *
                               PicWidthInSamples +
                           (m_mbs[CurrMbAddr].m_mb_position_x + (xO + (j)))] +
                      r[i][j]);
      }
    }

    ret = picture_construction_process_prior_to_deblocking_filter(
        u, 8, 8, luma8x8BlkIdx, isChroma, PicWidthInSamples, pic_buff);
    RETURN_IF_FAILED(ret != 0, ret);
  }

  return 0;
}

// 8.5.4 Specification of transform decoding process for chroma samples
/* 当 ChromaArrayType 不等于 0 时，针对每个色度分量 Cb 和 Cr 分别调用此过程。*/
int PictureBase::transform_decoding_for_chroma_samples(
    int32_t isChromaCb, int32_t PicWidthInSamples, uint8_t *pic_buff) {
  int ret = 0;

  /* ------------------ 设置别名 ------------------ */
  MacroBlock &mb = m_mbs[CurrMbAddr];
  const uint32_t ChromaArrayType = m_slice.m_sps.ChromaArrayType;
  bool isMbAff =
      m_slice.slice_header.MbaffFrameFlag && mb.mb_field_decoding_flag;
  /* ------------------  End ------------------ */

  /* 根据 ChromaArrayType，以下情况适用： 
   * — 如果 ChromaArrayType 等于 3，则调用第8.5.5节中指定的 ChromaArrayType 等于 3 的色度样本的变换解码过程。*/
  if (ChromaArrayType == 3)
    // 8.5.5 Specification of transform decoding process for chroma samples with ChromaArrayType equal to 3
    ret = transform_decoding_for_chroma_samples_with_YUV444(
        isChromaCb, PicWidthInSamples, pic_buff);

  /* – 否则（ChromaArrayType不等于3），以下文本指定色度样本的变换解码过程：*/
  else {
    //对于每个色度分量，变量 ChromaDCLevel[ iCbCr ] 和 ChromaACLevel[ iCbCr ]（其中 Cb 的 iCbCr 设置为 0，Cr 的 iCbCr 设置为 1）包含色度变换系数的两个分量的级别。  将变量 numChroma4x4Blks 设置为等于 (MbWidthC / 4) * (MbHeightC / 4)。
    bool iCbCr = (isChromaCb != 1);
    int32_t numChroma4x4Blks = (MbWidthC / 4) * (MbHeightC / 4);

    /* 对于每个色度分量，变换解码按照以下有序步骤单独进行： 
     * 1. 按照以下有序步骤指定，对由宏块的 iCbCr 索引的分量的 4x4 色度块的 numChroma4x4Blks 色度 DC 变换系数进行解码： 
       * – 如果 ChromaArrayType 等于 1，则使用应用于 ChromaDCLevel 的逆光栅扫描过程导出 2x2 数组 c，如下所示: 
       * – 否则（ChromaArrayType 等于 2），将使用应用于 ChromaDCLevel 的逆光栅扫描过程导出 2x4 数组 c，如下所示：
     * 使用 c 作为输入和 dcC 作为输出来调用第 8.5.11 节中指定的色度 DC 变换系数的缩放和变换过程。*/
    int32_t dcC[4][2] = {{0}};

    int32_t w = 0, h = 0;
    int32_t c[4][2] = {{0}};
    /* NOTE:对于YUV420，会浪费c[2][2]的内存空间 */
    if (ChromaArrayType == 1) {
      // YUV420
      w = 2, h = 2;
      c[0][0] = mb.ChromaDCLevel[iCbCr][0];
      c[0][1] = mb.ChromaDCLevel[iCbCr][1];
      c[1][0] = mb.ChromaDCLevel[iCbCr][2];
      c[1][1] = mb.ChromaDCLevel[iCbCr][3];
    } else if (ChromaArrayType == 2) {
      // YUV422
      w = 2, h = 4;
      c[0][0] = mb.ChromaDCLevel[iCbCr][0];
      c[0][1] = mb.ChromaDCLevel[iCbCr][1];
      c[1][0] = mb.ChromaDCLevel[iCbCr][2];
      c[1][1] = mb.ChromaDCLevel[iCbCr][3];
      c[2][0] = mb.ChromaDCLevel[iCbCr][4];
      c[2][1] = mb.ChromaDCLevel[iCbCr][5];
      c[3][0] = mb.ChromaDCLevel[iCbCr][6];
      c[3][1] = mb.ChromaDCLevel[iCbCr][7];
    }
    // 8.5.11 Scaling and transformation process for chroma DC transform
    ret = scaling_and_transformation_for_chroma_DC_transform_coefficients(
        isChromaCb, c, w, h, dcC);
    RET(ret);

    /* 2. (MbWidthC)x(MbHeightC) 数组 rMb 是通过处理由 iCbCr 索引的分量的 chroma4x4BlkIdx = 0..numChroma4x4Blks − 1 索引的 4x4 色度块导出的，并且对于每个 4x4 色度块，指定以下有序步骤: */
    const int32_t dcC_to_chroma_index[8] = {
        dcC[0][0], dcC[0][1], dcC[1][0], dcC[1][1],
        dcC[2][0], dcC[2][1], dcC[3][0], dcC[3][1],
    };
    int32_t rMb[16][16] = {{0}};
    for (int chroma4x4BlkIdx = 0; chroma4x4BlkIdx < numChroma4x4Blks;
         chroma4x4BlkIdx++) {
      //* a. 导出变量 chromaList，它是一个包含 16 个条目的列表。 chromaList 的第一个条目是数组 dcC 中的相应值。图 8-7 显示了数组 dcC 的索引到 chroma4x4BlkIdx 的分配。小方块中的两个数字指的是 dcCij 中的索引 i 和 j，大方块中的数字指的是 chroma4x4BlkIdx。
      int32_t chromaList[16] = {0};

      //DC 系数
      chromaList[0] = dcC_to_chroma_index[chroma4x4BlkIdx];
      //AC 系数
      for (int k = 1; k <= 15; k++)
        chromaList[k] = mb.ChromaACLevel[iCbCr][chroma4x4BlkIdx][k - 1];

      //* b. 使用 chromaList 作为输入和二维数组 c 作为输出来调用第 8.5.6 节中指定的 4x4 变换系数和缩放列表的逆扫描过程。
      int32_t c[4][4] = {{0}};
      ret = inverse_scanning_for_4x4_transform_coefficients_and_scaling_lists(
          chromaList, c, mb.field_pic_flag | mb.mb_field_decoding_flag);
      RET(ret);

      //* c. 使用 c 作为输入和 r 作为输出来调用第 8.5.12 节中指定的残差 4x4 块的缩放和变换过程。
      int32_t isChroma = 1;
      int32_t r[4][4] = {{0}};
      // 8.5.12 Scaling and transformation process for residual 4x4 blocks (反量化、反整数变换)
      ret = scaling_and_transformation_process_for_residual_4x4_blocks(
          c, r, isChroma, isChromaCb);
      RET(ret);

      //* d. 当前宏块内索引为 chroma4x4BlkIdx 的 4x4 色度块左上角样本的位置是通过调用第 6.4.7 节中指定的逆 4x4 色度块扫描过程来导出的，其中 chroma4x4BlkIdx 作为输入，输出分配给 ( xO，yO）。
      // 6.4.7 Inverse 4x4 chroma block scanning process
      int32_t xO = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 0);
      int32_t yO = InverseRasterScan(chroma4x4BlkIdx, 4, 4, 8, 1);

      //* e. (MbWidthC)x(MbHeightC) 数组 rMb 的元素 rMb[ x, y ]（其中 x = xO..xO + 3 且 y = yO..yO + 3）通过以下公式导出：
      for (int32_t i = 0; i <= 3; i++)
        for (int32_t j = 0; j <= 3; j++)
          rMb[yO + i][xO + j] = r[i][j];
    }

    /* 3.当TransformBypassModeFlag等于1，宏块预测模式等于Intra_4x4、Intra_8x8或Intra_16x16，并且intra_chroma_pred_mode等于1或2时，调用第8.5.15节中指定的帧内残差变换旁路解码过程：
     * nW 设置等于 MbWidthC，nH 设置等于 MbHeightC，horPredFlag 设置等于 (2 − intra_chroma_pred_mode)，并且 (MbWidthC)x(MbHeightC) 数组 rMb 作为输入，输出是 (MbWidthC)x 的修改版本(MbHeightC) 数组 rMb。 */
    if (mb.TransformBypassModeFlag &&
        (mb.m_mb_pred_mode == Intra_4x4 || mb.m_mb_pred_mode == Intra_8x8 ||
         ((mb.m_mb_pred_mode == Intra_16x16 &&
           mb.intra_chroma_pred_mode == 1) ||
          mb.intra_chroma_pred_mode == 2))) {
      int32_t horPredFlag = 2 - mb.intra_chroma_pred_mode;
      // 8.5.15 Intra residual transform-bypass decoding process
      intra_residual_transform_bypass_decoding(MbWidthC, MbHeightC, horPredFlag,
                                               &rMb[0][0]);
    }

    //帧内预测
    ret = Intra_chroma_sample_prediction(pic_buff, PicWidthInSamples);
    RETURN_IF_FAILED(ret != 0, ret);

    /* 4. 对于 i = 0..MbHeightC − 1 和 j = 0..MbWidthC − 1，元素 uij 的 (MbWidthC)x(MbHeightC) 数组 u 的推导如下： */
    int32_t u[16 * 16] = {0};
    for (int32_t i = 0; i <= MbHeightC - 1; i++) {
      for (int32_t j = 0; j <= MbWidthC - 1; j++) {
        int32_t y = ((mb.m_mb_position_y >> 4) * MbHeightC) +
                    (mb.m_mb_position_y % 2) + i * (1 + isMbAff);
        int32_t x = (mb.m_mb_position_x >> 4) * MbWidthC + j;
        u[i * MbHeightC + j] =
            Clip1C(pic_buff[y * PicWidthInSamples + x] + rMb[i][j],
                   m_slice.m_sps.BitDepthC);
      }
    }

    /* 5.使用u作为输入来调用第8.5.14节中的去块滤波器过程之前的图像构建过程。 */
    int32_t BlkIdx = 0;
    int32_t isChroma = 1;
    ret = picture_construction_process_prior_to_deblocking_filter(
        u, MbWidthC, MbHeightC, BlkIdx, isChroma, PicWidthInSamples, pic_buff);
  }
  RET(ret);

  return 0;
}

// 8.5.5 Specification of transform decoding process for chroma samples with ChromaArrayType equal to 3
int PictureBase::transform_decoding_for_chroma_samples_with_YUV444(
    int32_t isChromaCb, int32_t PicWidthInSamples, uint8_t *pic_buff) {

  /* 根据宏块预测模式和 Transform_size_8x8_flag，适用以下内容：
   * – 如果宏块预测模式等于 Intra_16x16，则当用 Cb 或 Cr 代替亮度、用 CbIntra16x16DCLevel 或 CrIntra16x16DCLevel 代替 Intra16x16ACLevel 时，Cb 或 Cr 残余块的变换解码过程应与第 8.5.2 节中描述的过程相同用 CbIntra16x16ACLevel 或 CrIntra16x16ACLevel，并将 predL 替换为 predCb 或 predCr，将 luma4x4BlkIdx 替换为 cb4x4BlkIdx 或 cr4x4BlkIdx，将 lumaList 替换为 CbList 或 CrList，将 BitDepthY 替换为 BitDepthC，将 QP'Y 替换为 QP'C，以及 sub将Clip1Y与Clip1C组合。在第 8.5.12.1 节中指定的 4x4 块变换系数级别缩放期间（作为第 8.5.2 节中指定的过程的一部分调用），输入 4x4 数组 c 被视为与使用Intra_16x16 宏块预测模式。  
   * – 否则，如果transform_size_8x8_flag等于1，则8x8 Cb或8x8 Cr残差块的变换解码过程应与第8.5.3节中描述的过程相同，即用Cb或Cr替换亮度、用CbLevel8x8或CrLevel8x8替换LumaLevel8x8、替换将 predL 替换为 predCb 或 predCr，将 luma8x8BlkIdx 替换为 cb8x8BlkIdx 或 cr8x8BlkIdx，并将 Clip1Y 替换为 Clip1C。  
   * – 否则（宏块预测模式不等于Intra_16x16且transform_size_8x8_flag等于0），当用Cb或4x4 Cb或4x4 Cr替换亮度时，4x4 Cb或4x4 Cr残差块的变换解码过程应与第8.5.1节中描述的过程相同Cr，用CbLevel4x4或CrLevel4x4替代LumaLevel4x4，用predCb或predCr替代predL，用cb4x4BlkIdx或cr4x4BlkIdx替代luma4x4BlkIdx，以及用Clip1C替代Clip1Y。在第 8.5.12.1 节中指定的 4x4 块变换系数级别缩放期间（作为第 8.5.1 节中指定的过程的一部分调用），输入 4x4 数组 c 被视为与未使用以下方式编码的亮度残差块相关： Intra_16x16 宏块预测模式。*/

  const int32_t isChroma = 1;
  const int32_t BitDepth = m_slice.m_sps.BitDepthC;
  MacroBlock &mb = m_mbs[CurrMbAddr];

  int ret = 0;
  if (mb.m_mb_pred_mode == Intra_16x16) {
    if (isChromaCb)
      ret =
          transform_decoding_for_luma_samples_of_Intra_16x16_macroblock_prediction(
              isChroma, BitDepth, mb.QP1Cb, PicWidthInSamples,
              mb.CbIntra16x16DCLevel, mb.CbIntra16x16ACLevel, pic_buff);
    else
      ret =
          transform_decoding_for_luma_samples_of_Intra_16x16_macroblock_prediction(
              isChroma, BitDepth, mb.QP1Cr, PicWidthInSamples,
              mb.CrIntra16x16DCLevel, mb.CrIntra16x16ACLevel, pic_buff);
  } else if (mb.transform_size_8x8_flag) {
    if (isChromaCb)
      ret = transform_decoding_for_8x8_luma_residual_blocks(
          isChroma, isChromaCb, BitDepth, PicWidthInSamples, mb.CbLevel8x8,
          pic_buff);
    else
      ret = transform_decoding_for_8x8_luma_residual_blocks(
          isChroma, isChromaCb, BitDepth, PicWidthInSamples, mb.CbLevel8x8,
          pic_buff);
  } else {
    if (isChromaCb == 1)
      ret = transform_decoding_for_4x4_luma_residual_blocks(
          isChroma, isChromaCb, BitDepth, PicWidthInSamples, pic_buff);
    else
      ret = transform_decoding_for_4x4_luma_residual_blocks(
          isChroma, isChromaCb, BitDepth, PicWidthInSamples, pic_buff);
  }
  RET(ret);

  return 0;
}

// 8.5.11 Scaling and transformation process for chroma DC transform
// coefficients c[2][2] or c[4][2], dcC[nH][nW]
int PictureBase::
    scaling_and_transformation_for_chroma_DC_transform_coefficients(
        int32_t isChromaCb, int32_t c[4][2], int32_t nW, int32_t nH,
        int32_t (&dcC)[4][2]) {
  int ret = 0;
  // SliceHeader &slice_header = m_h264_slice_header;

  // 8.5.8 Derivation process for chroma quantisation parameters
  ret = derivation_chroma_quantisation_parameters(isChromaCb);
  RETURN_IF_FAILED(ret != 0, ret);

  // int32_t bitDepth = m_slice.m_sps.BitDepthC;
  int32_t qP =
      (isChromaCb == 1) ? m_mbs[CurrMbAddr].QP1Cb : m_mbs[CurrMbAddr].QP1Cr;

  if (m_mbs[CurrMbAddr].TransformBypassModeFlag == 1) {
    for (int32_t i = 0; i <= (MbWidthC / 4) - 1; i++) {
      for (int32_t j = 0; j <= (MbHeightC / 4) - 1; j++) {
        dcC[i][j] = c[i][j];
      }
    }
  } else // if (m_mbs[CurrMbAddr].TransformBypassModeFlag == 0)
  {
    // 8.5.11.1 Transformation process for chroma DC transform coefficients
    if (nW == 2 && nH == 2) // if (m_slice.m_sps.ChromaArrayType == 1) //YUV420
    {
      // the inverse transform for the 2x2 chroma DC transform coefficients
      // 2x2色度直流系数反变换
      //            | 1  1 |   | c00 c01 |   | 1  1 |   | c00 + c10    c01 + c11
      //            |   | 1  1 |
      //  f[2][2] = |      | * |         | * |      | = | | * |      |
      //            | 1 -1 |   | c10 c11 |   | 1 -1 |   | c00 - c10    c01 - c11
      //            |   | 1 -1 |

      int32_t f[2][2] = {{0}};

      int32_t e00 = c[0][0] + c[1][0];
      int32_t e01 = c[0][1] + c[1][1];
      int32_t e10 = c[0][0] - c[1][0];
      int32_t e11 = c[0][1] - c[1][1];

      f[0][0] = e00 + e01;
      f[0][1] = e00 - e01;
      f[1][0] = e10 + e11;
      f[1][1] = e10 - e11;

      //--------------------------
      // 8.5.11.2 Scaling process for chroma DC transform coefficients
      for (int32_t i = 0; i <= 1; i++) {
        for (int32_t j = 0; j <= 1; j++) {
          // dcCij = ( ( fij * LevelScale4x4(qP % 6, 0, 0 ) ) << ( qP/ 6) ) >>
          // 5;
          dcC[i][j] =
              ((f[i][j] * LevelScale4x4[qP % 6][0][0]) << (qP / 6)) >> 5;
        }
      }
    } else if (nW == 2 &&
               nH == 4) // if (m_slice.m_sps.ChromaArrayType == 2) //YUV422
    {
      // the inverse transform for the 2x2 chroma DC transform coefficients
      // 2x2色度直流系数反变换
      //            | 1  1  1  1 |   | c00 c01 |   | 1  1 |   | c00 + c10 + c20
      //            + c30    c01 + c11 + c21 + c31 |   | 1  1 |
      //  f[4][2] = | 1  1 -1 -1 | * | c10 c11 | * |      | = | c00 + c10 - c20
      //  - c30    c01 + c11 - c21 - c31 | * |      |
      //            | 1 -1 -1  1 |   | c20 c21 |   | 1 -1 |   | c00 - c10 - c20
      //            + c30    c01 - c11 - c21 + c31 |   | 1 -1 | | 1 -1  1 -1 |
      //            | c30 c31 |              | c00 - c10 + c20 - c30    c01 -
      //            c11 + c21 - c31 |

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

      //--------------------------
      // 8.5.11.2 Scaling process for chroma DC transform coefficients
      int32_t qPDC = qP + 3;

      if (qPDC >= 36) {
        for (int32_t i = 0; i <= 3; i++) {
          for (int32_t j = 0; j <= 1; j++) {
            // dcCij = ( fij *LevelScale4x4( qPDC %6, 0, 0 ) ) << ( qPDC / 6 - 6
            // );
            dcC[i][j] = (f[i][j] * LevelScale4x4[qPDC % 6][0][0])
                        << (qPDC / 6 - 6);
          }
        }
      } else // if (qPDC < 36)
      {
        for (int32_t i = 0; i <= 3; i++) {
          for (int32_t j = 0; j <= 1; j++) {
            // dcCij = ( fij * LevelScale4x4( qPDC % 6, 0, 0 ) + 2^(5 - qPDC /
            // 6) ) >> ( 6 - qPDC / 6 );
            dcC[i][j] = (f[i][j] * LevelScale4x4[qPDC % 6][0][0] +
                         h264_power2(5 - qPDC / 6)) >>
                        (6 - qP / 6);
          }
        }
      }
    }
  }

  return 0;
}

// 8.5.12 Scaling and transformation process for residual 4x4 blocks
/* 输入: 具有元素cij的4x4数组c，cij是与亮度分量的残差块相关的数组或与色度分量的残差块相关的数组。  
 * 输出: 剩余样本值，为 4x4 数组 r，元素为 rij。*/
/* 该函数包括了反量化、反整数变换（类IDCT变换）操作 */
int PictureBase::scaling_and_transformation_process_for_residual_4x4_blocks(
    int32_t c[4][4], int32_t (&r)[4][4], int32_t isChroma, int32_t isChromaCb) {

  const uint32_t slice_type = m_slice.slice_header.slice_type % 5;
  const MacroBlock &mb = m_mbs[CurrMbAddr];

  /* 变量 sMbFlag 的推导如下： 
   * – 如果 mb_type 等于 SI 或宏块预测模式等于 SP 切片中的 Inter，则 sMbFlag 设置为等于 1， 
   * – 否则（mb_type 不等于 SI 并且宏块预测模式为不等于 SP 切片中的 Inter），sMbFlag 设置为 0。 */
  int32_t sMbFlag = 0;
  if (slice_type == SLICE_SI ||
      (slice_type == SLICE_SP && IS_INTER_Prediction_Mode(mb.m_mb_pred_mode)))
    sMbFlag = 1;

  /* 先得到色度量化参数 */
  // 8.5.8 Derivation process for chroma quantisation parameters
  int ret = derivation_chroma_quantisation_parameters(isChromaCb);
  RET(ret);

  /* 变量 qP(量化) 的推导如下： 
   * – 如果输入数组 c 与亮度残差块相关且 sMbFlag 等于 0，
   * – 否则，如果输入数组 c 与亮度残差块相关且 sMbFlag 等于 1，
   * – 否则，如果输入数组 c 与色度残差块相关且 sMbFlag 等于 0，
   * – 否则（输入数组 c 与色度残差块相关且 sMbFlag 等于 1），*/
  int32_t qP = 0;
  if (isChroma == 0 && sMbFlag == 0)
    qP = mb.QP1Y;
  else if (isChroma == 0 && sMbFlag)
    qP = mb.QSY;
  else if (isChroma && sMbFlag == 0)
    qP = isChromaCb ? mb.QP1Cb : mb.QP1Cr;
  else if (isChroma && sMbFlag)
    qP = isChromaCb ? mb.QSCb : mb.QSCr;

  /* 根据 TransformBypassModeFlag 的值，以下内容适用：*/
  if (mb.TransformBypassModeFlag) {
    for (int32_t i = 0; i <= 3; i++)
      for (int32_t j = 0; j <= 3; j++)
        r[i][j] = c[i][j];
  } else {
    /* 1. 使用 bitDepth、qP 和 c 作为输入来调用第 8.5.12.1 节中指定的残差 4x4 块的缩放过程，并将输出分配给具有元素 dij 的缩放变换系数的 4x4 数组 d。
     * 2. 使用 bitDepth 和 d 作为输入来调用第 8.5.12.2 节中指定的残差 4x4 块的变换过程，并将输出分配给具有元素 rij 的残差样本值的 4x4 数组 r。*/

    // 8.5.12.1 Scaling process for residual 4x4 blocks (反量化)
    int32_t d[4][4] = {{0}};
    scaling_for_residual_4x4_blocks(d, c, isChroma, mb.m_mb_pred_mode, qP);

    // 8.5.12.2 Transformation process for residual 4x4 blocks （反整数变换）
    transformation_for_residual_4x4_blocks(d, r);
  }

  return 0;
}

//8.5.12.1 Scaling process for residual 4x4 blocks
/* 输入： – 变量 bitDepth 和 qP， – 具有元素 cij 的 4x4 数组 c，它是与亮度分量的残差块相关的数组或与色度分量的残差块相关的数组。  
 * 输出: 缩放变换系数 d 的 4x4 数组，其中元素为 dij。 */
/* 反量化操作 */
int PictureBase::scaling_for_residual_4x4_blocks(
    int32_t d[4][4], int32_t c[4][4], int32_t isChroma,
    const H264_MB_PART_PRED_MODE &m_mb_pred_mode, int32_t qP) {

  /* 比特流不应包含导致 c 的任何元素 cij 的数据，其中 i, j = 0..3 超出从 −2(7 + bitDepth) 到 2(7 + bitDepth) − 1（含）的整数值范围。 */
  for (int32_t i = 0; i <= 3; i++) {
    for (int32_t j = 0; j <= 3; j++) {
      /* 如果以下所有条件均为真： – i 等于 0， – j 等于 0， – c 与使用 Intra_16x16 宏块预测模式编码的亮度残差块相关，或者 c 与色度残差块相关。  变量 d00 的导出方式为 */
      if (i == 0 && j == 0 &&
          ((isChroma == 0 && m_mb_pred_mode == Intra_16x16) || isChroma))
        d[0][0] = c[0][0];
      /* 否则，适用以下规则： – 如果 qP 大于或等于 24，则缩放结果的推导如下： 其中i, j = 0..3，除非如上所述
           *  – 否则（qP 小于 24），缩放结果导出为 */
      else {
        /* TODO YangJing 量化参数24这个数有什么特殊意义？ <24-09-08 18:46:35> */
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

// 8.5.12.2 Transformation process for residual 4x4 blocks
// 类似4x4 IDCT离散余弦反变换蝶形运算
/* 输入： – 变量 bitDepth， – 具有元素 dij 的缩放变换系数 d 的 4x4 数组。
 * 输出: 剩余样本值，为 4x4 数组 r，元素为 rij。*/
int PictureBase::transformation_for_residual_4x4_blocks(int32_t d[4][4],
                                                        int32_t (&r)[4][4]) {

  /* 比特流不得包含导致 d 的任何元素 dij 的数据，其中 i, j = 0..3 超出从 −2(7 + bitDepth) 到 2(7 + bitDepth) − 1（含）的整数值范围。  变换过程应以数学上等效的方式将缩放变换系数块转换为输出样本块。*/
  int32_t f[4][4] = {{0}};
  int32_t h[4][4] = {{0}};
  for (int32_t i = 0; i <= 3; i++) {
    // 首先，按如下方式使用一维逆变换对每一（水平）行缩放变换系数进行变换。  一组中间值的计算如下：
    int32_t ei0 = d[i][0] + d[i][2];
    int32_t ei1 = d[i][0] - d[i][2];
    int32_t ei2 = (d[i][1] >> 1) - d[i][3];
    int32_t ei3 = d[i][1] + (d[i][3] >> 1);

    // 然后，根据这些中间值计算转换结果，如下所示：
    f[i][0] = ei0 + ei3;
    f[i][1] = ei1 + ei2;
    f[i][2] = ei1 - ei2;
    f[i][3] = ei0 - ei3;
  }

  for (int32_t j = 0; j <= 3; j++) {
    //然后，使用相同的一维逆变换对所得矩阵的每个（垂直）列进行变换，如下所示。  一组中间值的计算如下：
    int32_t g0j = f[0][j] + f[2][j];
    int32_t g1j = f[0][j] - f[2][j];
    int32_t g2j = (f[1][j] >> 1) - f[3][j];
    int32_t g3j = f[1][j] + (f[3][j] >> 1);

    //然后，根据这些中间值计算转换结果，如下所示：
    h[0][j] = g0j + g3j;
    h[1][j] = g1j + g2j;
    h[2][j] = g1j - g2j;
    h[3][j] = g0j - g3j;
  }

  /* 在执行一维水平逆变换和一维垂直逆变换以产生变换样本数组之后，最终构建的残差样本值导出为： */
  for (int32_t i = 0; i <= 3; i++)
    for (int32_t j = 0; j <= 3; j++)
      r[i][j] = (h[i][j] + 32) >> 6;

  return 0;
}

// 8.5.13 Scaling and transformation process for residual 8x8 blocks
int PictureBase::Scaling_and_transformation_process_for_residual_8x8_blocks(
    int32_t c[8][8], int32_t (&r)[8][8], int32_t isChroma, int32_t isChromaCb) {
  // SliceHeader &slice_header = m_h264_slice_header;

  // int32_t bitDepth = 0;
  int32_t qP = 0;

  if (isChroma == 0) // If the input array c relates to a luma residual block
  {
    // bitDepth = m_slice.m_sps.BitDepthY;
    qP = m_mbs[CurrMbAddr].QP1Y;
  } else // if (isChroma == 1) //the input array c relates to a chroma residual
         // block
  {
    // bitDepth = m_slice.m_sps.BitDepthC;
    if (isChromaCb == 1) {
      qP = m_mbs[CurrMbAddr].QP1Cb;
    } else if (isChromaCb == 0) {
      qP = m_mbs[CurrMbAddr].QP1Cr;
    }
  }

  //---------------------
  if (m_mbs[CurrMbAddr].TransformBypassModeFlag == 1) {
    for (int32_t i = 0; i <= 7; i++) {
      for (int32_t j = 0; j <= 7; j++) {
        r[i][j] = c[i][j];
      }
    }
  } else {
    // 8.5.13.1 Scaling process for residual 8x8 blocks
    int32_t d[8][8] = {{0}};

    for (int32_t i = 0; i <= 7; i++) {
      for (int32_t j = 0; j <= 7; j++) {
        if (qP >= 36) {
          d[i][j] = (c[i][j] * LevelScale8x8[qP % 6][i][j]) << (qP / 6 - 6);
        } else // if (qP < 36)
        {
          d[i][j] = (c[i][j] * LevelScale8x8[qP % 6][i][j] +
                     h264_power2(5 - qP / 6)) >>
                    (6 - qP / 6);
        }
      }
    }

    // 8.5.13.2 Transformation process for residual 8x8 blocks
    // 类似4x4 IDC离散余弦反变换蝶形运算

    int32_t g[8][8];
    int32_t m[8][8];

    for (int32_t i = 0; i <= 7; i++) // 先行变换
    {
      int32_t ei0 = d[i][0] + d[i][4];
      int32_t ei1 = -d[i][3] + d[i][5] - d[i][7] - (d[i][7] >> 1);
      int32_t ei2 = d[i][0] - d[i][4];
      int32_t ei3 = d[i][1] + d[i][7] - d[i][3] - (d[i][3] >> 1);
      int32_t ei4 = (d[i][2] >> 1) - d[i][6];
      int32_t ei5 = -d[i][1] + d[i][7] + d[i][5] + (d[i][5] >> 1);
      int32_t ei6 = d[i][2] + (d[i][6] >> 1);
      int32_t ei7 = d[i][3] + d[i][5] + d[i][1] + (d[i][1] >> 1);

      int32_t fi0 = ei0 + ei6;
      int32_t fi1 = ei1 + (ei7 >> 2);
      int32_t fi2 = ei2 + ei4;
      int32_t fi3 = ei3 + (ei5 >> 2);
      int32_t fi4 = ei2 - ei4;
      int32_t fi5 = (ei3 >> 2) - ei5;
      int32_t fi6 = ei0 - ei6;
      int32_t fi7 = ei7 - (ei1 >> 2);

      g[i][0] = fi0 + fi7;
      g[i][1] = fi2 + fi5;
      g[i][2] = fi4 + fi3;
      g[i][3] = fi6 + fi1;
      g[i][4] = fi6 - fi1;
      g[i][5] = fi4 - fi3;
      g[i][6] = fi2 - fi5;
      g[i][7] = fi0 - fi7;
    }

    for (int32_t j = 0; j <= 7; j++) // 再列变换
    {
      int32_t h0j = g[0][j] + g[4][j];
      int32_t h1j = -g[3][j] + g[5][j] - g[7][j] - (g[7][j] >> 1);
      int32_t h2j = g[0][j] - g[4][j];
      int32_t h3j = g[1][j] + g[7][j] - g[3][j] - (g[3][j] >> 1);
      int32_t h4j = (g[2][j] >> 1) - g[6][j];
      int32_t h5j = -g[1][j] + g[7][j] + g[5][j] + (g[5][j] >> 1);
      int32_t h6j = g[2][j] + (g[6][j] >> 1);
      int32_t h7j = g[3][j] + g[5][j] + g[1][j] + (g[1][j] >> 1);

      int32_t k0j = h0j + h6j;
      int32_t k1j = h1j + (h7j >> 2);
      int32_t k2j = h2j + h4j;
      int32_t k3j = h3j + (h5j >> 2);
      int32_t k4j = h2j - h4j;
      int32_t k5j = (h3j >> 2) - h5j;
      int32_t k6j = h0j - h6j;
      int32_t k7j = h7j - (h1j >> 2);

      m[0][j] = k0j + k7j;
      m[1][j] = k2j + k5j;
      m[2][j] = k4j + k3j;
      m[3][j] = k6j + k1j;
      m[4][j] = k6j - k1j;
      m[5][j] = k4j - k3j;
      m[6][j] = k2j - k5j;
      m[7][j] = k0j - k7j;
    }

    //------------------------------------
    for (int32_t i = 0; i <= 7; i++) {
      for (int32_t j = 0; j <= 7; j++) {
        r[i][j] = (m[i][j] + 32) >> 6;
      }
    }
  }

  return 0;
}

// 8.5.14 Picture construction process prior to deblocking filter process (去块过滤过程之前的图片构造过程)
/* 输入： 
 * – 包含元素 uij 的样本数组 u，它是 16x16 亮度块或 (MbWidthC)x(MbHeightC) 色度块或 4x4 亮度块或 4x4 色度块或 8x8 亮度块，或者，当 ChromaArrayType等于 3，8x8 色度块， 
 * – 当 u 不是 16x16 亮度块或 (MbWidthC)x(MbHeightC) 色度块时，块索引 luma4x4BlkIdx 或 chroma4x4BlkIdx 或 luma8x8BlkIdx 或 cb4x4BlkIdx 或 cr4x4BlkIdx 或 cb8x8BlkIdx 或idx。*/
int PictureBase::picture_construction_process_prior_to_deblocking_filter(
    int32_t *u, int32_t nW, int32_t nH, int32_t BlkIdx, int32_t isChroma,
    int32_t PicWidthInSamples, uint8_t *pic_buff) {

  /* ------------------ 设置别名 ------------------ */
  const bool MbaffFrameFlag = m_slice.slice_header.MbaffFrameFlag;
  const int32_t mb_field_decoding_flag =
      m_mbs[CurrMbAddr].mb_field_decoding_flag;
  const uint32_t ChromaArrayType = m_slice.m_sps.ChromaArrayType;
  const int32_t SubWidthC = m_slice.m_sps.SubWidthC;
  const int32_t SubHeightC = m_slice.m_sps.SubHeightC;
  /* ------------------  End ------------------ */

  /* 当前宏块的左上角亮度样本的位置是通过调用第 6.4.1 节中的逆宏块扫描过程来导出的，其中 CurrMbAddr 作为输入，输出被分配给 ( xP, yP )。 */
  int32_t xP = 0, yP = 0;
  // 6.4.1 Inverse macroblock scanning process
  int ret = inverse_macroblock_scanning_process(MbaffFrameFlag, CurrMbAddr,
                                                mb_field_decoding_flag, xP, yP);
  RET(ret);

  /* 当 u 是亮度块时，对于亮度块的每个样本 uij，指定以下有序步骤：
   * 1. 根据块 u 的大小，适用以下规则： 
   * – 如果 u 是 16x16 亮度块，则宏块内 16x16 亮度块的左上角样本的位置 ( xO, yO ) 设置为等于 ( 0, 0 ) 且变量 nE 设置为等于 16。 
   * – 否则，如果 u 是 4x4 亮度块，则通过调用逆函数导出宏块内索引为 luma4x4BlkIdx 的 4x4 亮度块左上角样本的位置第 6.4.3 节中的 4x4 亮度块扫描过程，以 luma4x4BlkIdx 作为输入，输出分配给 ( xO, yO )，变量 nE 设置为等于 4。
   * – 否则（u 是 8x8 亮度块），位置宏块内部具有索引 luma8x8BlkIdx 的 8x8 亮度块的左上角样本是通过调用第 6.4.5 节中的逆 8x8 亮度块扫描过程而导出的，其中 luma8x8BlkIdx 作为输入，输出分配给 ( xO, yO )，并且变量 nE 设置为等于 8。*/
  if (isChroma == 0) {
    int32_t xO, yO, nE;
    if (nW == 16 && nH == 16)
      xO = 0, yO = 0, nE = 16;
    else if (nW == 4 && nH == 4) {
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

    /* 2. 根据变量 MbaffFrameFlag 和当前宏块，以下情况适用： 
     * – 如果 MbaffFrameFlag 等于 1 并且当前宏块是场宏块，则 S′L[ xP + xO + j, yP + 2 * ( yO + i ) ] = uij 其中 i, j = 0..nE − 1 
     * – 否则（MbaffFrameFlag 等于 0 或当前宏块是帧宏块），S′L[ xP + xO + j, yP + yO + i ] = uij 其中 i, j = 0..nE − 1 */
    if (MbaffFrameFlag && mb_field_decoding_flag) {
      for (int32_t i = 0; i <= nE - 1; i++)
        for (int32_t j = 0; j <= nE - 1; j++) {
          int32_t y = yP + 2 * (yO + i);
          int32_t x = xP + xO + j;
          pic_buff[y * PicWidthInSamples + x] = u[i * nE + j];
        }
    } else {
      for (int32_t i = 0; i <= nE - 1; i++)
        for (int32_t j = 0; j <= nE - 1; j++) {
          int32_t y = yP + yO + i;
          int32_t x = xP + xO + j;
          pic_buff[y * PicWidthInSamples + x] = u[i * nE + j];
        }
    }
  }

  /* 当 u 是色度块时，对于色度块的每个样本 uij，指定以下有序步骤： 
   * 1. 对于 Cb 色度分量，将变量 S'C 中的下标 C 替换为 Cb;
   *    对于 Cr 色度分量，将变量 S'C 中的下标 C 替换为 Cr成分。
   * 2. 根据块 u 的大小，适用以下规则： 
   *    – 如果 u 是 (MbWidthC)x(MbHeightC) Cb 或 Cr 块，则变量 nW 设置为等于 MbWidthC，变量 nH 设置为等于 MbHeightC，并且宏块内的(nW)x(nH) Cb或Cr块的左上角样本的位置(xO,yO)被设置为等于(0,0)。  
   *    – 否则，如果 u 是 4x4 Cb 或 Cr 块，则变量 nW 和 nH 设置为等于 4，并且根据变量 ChromaArrayType，内部具有索引 chroma4x4BlkIdx 的 4x4 Cb 或 Cr 块的左上角样本的位置宏块的推导如下：
   *         – 如果 ChromaArrayType 等于 1 或 2，则宏块内索引为 chroma4x4BlkIdx 的 4x4 色度块左上角样本的位置是通过调用第 6.4.7 节中的逆 4x4 色度块扫描过程（以 chroma4x4BlkIdx 作为输入）得出的并将输出分配给 ( xO, yO )。  
   *         – 否则（ChromaArrayType 等于 3），宏块内索引为 cb4x4BlkIdx 的 4x4 Cb 块或索引为 cr4x4BlkIdx 的 4x4 Cr 块的左上角样本的位置是通过调用逆 4x4 Cb 或 Cr 块扫描过程得出的在第 6.4.4 节中，以 cb4x4BlkIdx 或 cr4x4BlkIdx 作为输入，并将输出分配给 ( xO, yO )。
   *    – 否则（当 ChromaArrayType 等于 3 时，u 是 8x8 Cb 或 Cr 块），变量 nW 和 nH 设置为等于 8，并且索引为 cb8x8BlkIdx 的 8x8 Cb 块或带有索引的 Cr 块的左上角样本的位置宏块内的索引 cr8x8BlkIdx 是通过调用第 6.4.6 节中的逆 8x8 Cb 或 Cr 块扫描过程来导出的，其中 cb8x8BlkIdx 或 cr8x8BlkIdx 作为输入，输出分配给 ( xO, yO )。*/
  else if (isChroma) {
    int32_t xO, yO;
    if (nW == MbWidthC && nH == MbHeightC) {
      xO = 0, yO = 0;
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

    /* 3. 根据变量 MbaffFrameFlag 和当前宏块，以下情况适用： 
     * – 如果 MbaffFrameFlag 等于 1 并且当前宏块是场宏块
     * – 否则（MbaffFrameFlag等于0或当前宏块是帧宏块）*/
    if (MbaffFrameFlag && mb_field_decoding_flag) {
      for (int32_t i = 0; i <= nH - 1; i++)
        for (int32_t j = 0; j <= nW - 1; j++) {
          int32_t x = (xP / SubWidthC) + xO + j;
          int32_t y = ((yP + SubHeightC - 1) / SubHeightC) + 2 * (yO + i);
          pic_buff[y * PicWidthInSamples + x] = u[i * nW + j];
        }
    } else {
      for (int32_t i = 0; i <= nH - 1; i++)
        for (int32_t j = 0; j <= nW - 1; j++) {
          int32_t x = ((xP / SubWidthC) + xO + j);
          int32_t y = ((yP / SubHeightC) + yO + i);
          pic_buff[y * PicWidthInSamples + x] = u[i * nW + j];
        }
    }
  }

  return 0;
}

// 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
int PictureBase::
    inverse_scanning_for_4x4_transform_coefficients_and_scaling_lists(
        int32_t values[16], int32_t (&c)[4][4], int32_t field_scan_flag) {
  // Table 8-13 – Specification of mapping of idx to cij for zig-zag and field
  // scan
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
  } else // if (field_scan_flag == 1)
  {
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

// 8.5.7 Inverse scanning process for 8x8 transform coefficients and scaling
// lists
int PictureBase::
    Inverse_scanning_process_for_8x8_transform_coefficients_and_scaling_lists(
        int32_t values[64], int32_t (&c)[8][8], int32_t field_scan_flag) {
  // Table 8-14 – Specification of mapping of idx to cij for 8x8 zig-zag and 8x8
  // field scan
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
  } else // if (field_scan_flag == 1)
  {
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

// 8.5.8 Derivation process for chroma quantisation parameters (色度量化参数的推导过程)
/* 输出： – QPC：每个色度分量 Cb 和 Cr 的色度量化参数， 
 * – QSC：解码 SP 和 SI 切片所需的每个色度分量 Cb 和 Cr 的附加色度量化参数（如果适用）*/
int PictureBase::derivation_chroma_quantisation_parameters(int32_t isChromaCb) {
  int32_t qPOffset = 0;
  if (isChromaCb == 1)
    qPOffset = m_slice.m_pps.chroma_qp_index_offset;
  else
    qPOffset = m_slice.m_pps.second_chroma_qp_index_offset;

  int32_t qPI = CLIP3(-(int32_t)m_slice.m_sps.QpBdOffsetC, 51,
                      m_mbs[CurrMbAddr].QPY + qPOffset);

  // Table 8-15 – Specification of QPC as a function of qPI
  int32_t QPC = 0;
  if (qPI < 30)
    QPC = qPI;
  else {
    // int32_t qPIs[] = {30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    // 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
    int32_t QPCs[] = {29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36,
                      36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 39};

    int32_t index = qPI - 30;
    QPC = QPCs[index];
  }

  int32_t QP1C = QPC + m_slice.m_sps.QpBdOffsetC;

  if (isChromaCb == 1) {
    m_mbs[CurrMbAddr].QPCb = QPC;
    m_mbs[CurrMbAddr].QP1Cb = QP1C;
  } else // the chroma component is the Cr component
  {
    m_mbs[CurrMbAddr].QPCr = QPC;
    m_mbs[CurrMbAddr].QP1Cr = QP1C;
  }

  // When the current slice is an SP or SI slice, QSC is derived using the above
  // process, substituting QPY with QSY and QPC with QSC.
  if (m_slice.slice_header.slice_type == SLICE_SP ||
      m_slice.slice_header.slice_type == SLICE_SI ||
      m_slice.slice_header.slice_type == SLICE_SP2 ||
      m_slice.slice_header.slice_type == SLICE_SI2) {
    m_mbs[CurrMbAddr].QSY = m_mbs[CurrMbAddr].QPY;

    if (isChromaCb == 1) {
      m_mbs[CurrMbAddr].QSCb = m_mbs[CurrMbAddr].QPCb;
      m_mbs[CurrMbAddr].QS1Cb = m_mbs[CurrMbAddr].QP1Cb;
    } else // the chroma component is the Cr component
    {
      m_mbs[CurrMbAddr].QSCr = m_mbs[CurrMbAddr].QPCr;
      m_mbs[CurrMbAddr].QS1Cr = m_mbs[CurrMbAddr].QP1Cr;
    }
  }

  return 0;
}

// 8.5.8 Derivation process for chroma quantisation parameters
int PictureBase::get_chroma_quantisation_parameters2(int32_t QPY,
                                                     int32_t isChromaCb,
                                                     int32_t &QPC) {
  int32_t qPOffset = 0;

  //SliceHeader &slice_header = m_h264_slice_header;

  if (isChromaCb == 1) // If the chroma component is the Cb component
  {
    qPOffset = m_slice.m_pps.chroma_qp_index_offset;
  } else // the chroma component is the Cr component
  {
    qPOffset = m_slice.m_pps.second_chroma_qp_index_offset;
  }

  int32_t qPI = CLIP3(-(int32_t)m_slice.m_sps.QpBdOffsetC, 51, QPY + qPOffset);

  // Table 8-15 – Specification of QPC as a function of qPI
  QPC = 0;
  if (qPI < 30) {
    QPC = qPI;
  } else {
    // int32_t qPIs[] = {30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    // 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
    int32_t QPCs[] = {29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36,
                      36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 39};

    int32_t index = qPI - 30;
    QPC = QPCs[index];
  }

  return 0;
}

// 8.5.9 Derivation process for scaling functions
int PictureBase::scaling_functions(int32_t isChroma, int32_t isChromaCb) {
  int ret = 0;
  int32_t mbIsInterFlag = 0;

  //SliceHeader &slice_header = m_h264_slice_header;

  if (IS_INTRA_Prediction_Mode(m_mbs[CurrMbAddr].m_mb_pred_mode)) {
    mbIsInterFlag = 0;
  } else {
    mbIsInterFlag = 1;
  }

  int32_t iYCbCr = 0;

  if (m_slice.m_sps.separate_colour_plane_flag == 1) {
    iYCbCr = m_slice.slice_header.colour_plane_id;
  } else {
    if (isChroma == 0) // If the scaling function LevelScale4x4 or LevelScale8x8
                       // is derived for a luma residual block,
    {
      iYCbCr = 0;
    } else if (isChroma == 1 &&
               isChromaCb ==
                   1) // if the scaling function LevelScale4x4 or LevelScale8x8
                      // is derived for a chroma residual block and the chroma
                      // component is equal to Cb,
    {
      iYCbCr = 1;
    } else {
      iYCbCr = 2;
    }
  }

  //-------------------------
  // The inverse scanning process for 4x4 transform coefficients and scaling
  // lists as specified in clause 8.5.6 is invoked with ScalingList4x4[ iYCbCr +
  // ( (mbIsInterFlag = = 1 ) ? 3 : 0 )] as the input and the output is assigned
  // to the 4x4 matrix weightScale4x4.

  int32_t weightScale4x4[4][4] = {{0}};

  ret = inverse_scanning_for_4x4_transform_coefficients_and_scaling_lists(
      (int32_t *)m_slice.slice_header
          .ScalingList4x4[iYCbCr + ((mbIsInterFlag == 1) ? 3 : 0)],
      weightScale4x4,
      m_mbs[CurrMbAddr].field_pic_flag |
          m_mbs[CurrMbAddr].mb_field_decoding_flag);
  RETURN_IF_FAILED(ret != 0, ret);

  int32_t v4x4[6][3] = {
      {10, 16, 13}, {11, 18, 14}, {13, 20, 16},
      {14, 23, 18}, {16, 25, 20}, {18, 29, 23},
  };

  // LevelScale4x4( m, i, j ) = weightScale4x4( i, j ) * normAdjust4x4( m, i, j
  // );
  for (int32_t m = 0; m <= 5; m++) {
    for (int32_t i = 0; i <= 3; i++) {
      for (int32_t j = 0; j <= 3; j++) {
        if (i % 2 == 0 && j % 2 == 0) {
          LevelScale4x4[m][i][j] = weightScale4x4[i][j] * v4x4[m][0];
        } else if (i % 2 == 1 && j % 2 == 1) {
          LevelScale4x4[m][i][j] = weightScale4x4[i][j] * v4x4[m][1];
        } else {
          LevelScale4x4[m][i][j] = weightScale4x4[i][j] * v4x4[m][2];
        }
      }
    }
  }

  //--------------------------------------
  // The inverse scanning process for 8x8 transform coefficients and scaling
  // lists as specified in clause 8.5.7 is invoked with ScalingList8x8[ 2 *
  // iYCbCr + mbIsInterFlag ] as the input and the output is assigned to the 8x8
  // matrix weightScale8x8.

  int32_t weightScale8x8[8][8] = {{0}};

  ret =
      Inverse_scanning_process_for_8x8_transform_coefficients_and_scaling_lists(
          (int32_t *)
              m_slice.slice_header.ScalingList8x8[2 * iYCbCr + mbIsInterFlag],
          weightScale8x8,
          m_mbs[CurrMbAddr].field_pic_flag |
              m_mbs[CurrMbAddr].mb_field_decoding_flag);
  RETURN_IF_FAILED(ret != 0, ret);

  int32_t v8x8[6][6] = {
      {20, 18, 32, 19, 25, 24}, {22, 19, 35, 21, 28, 26},
      {26, 23, 42, 24, 33, 31}, {28, 25, 45, 26, 35, 33},
      {32, 28, 51, 30, 40, 38}, {36, 32, 58, 34, 46, 43},
  };

  // LevelScale8x8( m, i, j ) = weightScale8x8( i, j ) * normAdjust8x8( m, i, j
  // )
  for (int32_t m = 0; m <= 5; m++) {
    for (int32_t i = 0; i <= 7; i++) {
      for (int32_t j = 0; j <= 7; j++) {
        if (i % 4 == 0 && j % 4 == 0) {
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][0];
        } else if (i % 2 == 1 && j % 2 == 1) {
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][1];
        } else if (i % 4 == 2 && j % 4 == 2) {
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][2];
        } else if ((i % 4 == 0 && j % 2 == 1) || (i % 2 == 1 && j % 4 == 0)) {
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][3];
        } else if ((i % 4 == 0 && j % 4 == 2) || (i % 4 == 2 && j % 4 == 0)) {
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][4];
        } else {
          LevelScale8x8[m][i][j] = weightScale8x8[i][j] * v8x8[m][5];
        }
      }
    }
  }

  return 0;
}

// 8.5.10 Scaling and transformation process for DC transform coefficients for
// Intra_16x16 macroblock type
int PictureBase::
    Scaling_and_transformation_process_for_DC_transform_coefficients_for_Intra_16x16_macroblock_type(
        int32_t bitDepth, int32_t qP, int32_t c[4][4], int32_t (&dcY)[4][4]) {
  // int ret = 0;
  // SliceHeader &slice_header = m_h264_slice_header;

  if (m_mbs[CurrMbAddr].TransformBypassModeFlag == 1) {
    for (int32_t i = 0; i <= 3; i++) {
      for (int32_t j = 0; j <= 3; j++) {
        dcY[i][j] = c[i][j];
      }
    }
  } else // if (m_mbs[CurrMbAddr].TransformBypassModeFlag == 0)
  {
    int32_t f[4][4];
    int32_t g[4][4];

    // The inverse transform for the 4x4 luma DC transform coefficients
    // 4x4 luma亮度直流系数反变换
    //            | 1   1   1   1 |   | c00 c01 c02 c03 |   | 1   1   1   1 |
    //  f[4][4] = | 1   1  -1  -1 | * | c10 c11 c12 c13 | * | 1   1  -1  -1 |
    //            | 1  -1  -1   1 |   | c20 c21 c22 c23 |   | 1  -1  -1   1 |
    //            | 1  -1   1  -1 |   | c30 c31 c32 c33 |   | 1  -1   1  -1 |

    // 先行变换
    g[0][0] = c[0][0] + c[1][0] + c[2][0] + c[3][0];
    g[0][1] = c[0][1] + c[1][1] + c[2][1] + c[3][1];
    g[0][2] = c[0][2] + c[1][2] + c[2][2] + c[3][2];
    g[0][3] = c[0][3] + c[1][3] + c[2][3] + c[3][3];

    g[1][0] = c[0][0] + c[1][0] - c[2][0] - c[3][0];
    g[1][1] = c[0][1] + c[1][1] - c[2][1] - c[3][1];
    g[1][2] = c[0][2] + c[1][2] - c[2][2] - c[3][2];
    g[1][3] = c[0][3] + c[1][3] - c[2][3] - c[3][3];

    g[2][0] = c[0][0] - c[1][0] - c[2][0] + c[3][0];
    g[2][1] = c[0][1] - c[1][1] - c[2][1] + c[3][1];
    g[2][2] = c[0][2] - c[1][2] - c[2][2] + c[3][2];
    g[2][3] = c[0][3] - c[1][3] - c[2][3] + c[3][3];

    g[3][0] = c[0][0] - c[1][0] + c[2][0] - c[3][0];
    g[3][1] = c[0][1] - c[1][1] + c[2][1] - c[3][1];
    g[3][2] = c[0][2] - c[1][2] + c[2][2] - c[3][2];
    g[3][3] = c[0][3] - c[1][3] + c[2][3] - c[3][3];

    // 再列变换
    f[0][0] = g[0][0] + g[0][1] + g[0][2] + g[0][3];
    f[0][1] = g[0][0] + g[0][1] - g[0][2] - g[0][3];
    f[0][2] = g[0][0] - g[0][1] - g[0][2] + g[0][3];
    f[0][3] = g[0][0] - g[0][1] + g[0][2] - g[0][3];

    f[1][0] = g[1][0] + g[1][1] + g[1][2] + g[1][3];
    f[1][1] = g[1][0] + g[1][1] - g[1][2] - g[1][3];
    f[1][2] = g[1][0] - g[1][1] - g[1][2] + g[1][3];
    f[1][3] = g[1][0] - g[1][1] + g[1][2] - g[1][3];

    f[2][0] = g[2][0] + g[2][1] + g[2][2] + g[2][3];
    f[2][1] = g[2][0] + g[2][1] - g[2][2] - g[2][3];
    f[2][2] = g[2][0] - g[2][1] - g[2][2] + g[2][3];
    f[2][3] = g[2][0] - g[2][1] + g[2][2] - g[2][3];

    f[3][0] = g[3][0] + g[3][1] + g[3][2] + g[3][3];
    f[3][1] = g[3][0] + g[3][1] - g[3][2] - g[3][3];
    f[3][2] = g[3][0] - g[3][1] - g[3][2] + g[3][3];
    f[3][3] = g[3][0] - g[3][1] + g[3][2] - g[3][3];

    //-------------------------------
    if (qP >= 36) {
      for (int32_t i = 0; i <= 3; i++) {
        for (int32_t j = 0; j <= 3; j++) {
          // dcYij = ( fij * LevelScale4x4( qP % 6, 0, 0 ) ) << ( qP / 6 − 6 );
          dcY[i][j] = (f[i][j] * LevelScale4x4[qP % 6][0][0]) << (qP / 6 - 6);
        }
      }
    } else // if (qP < 36)
    {
      for (int32_t i = 0; i <= 3; i++) {
        for (int32_t j = 0; j <= 3; j++) {
          // dcYij = ( fij * LevelScale4x4( qP % 6, 0, 0 ) + ( 1 << ( 5 − qP /
          // 6) ) ) >> ( 6 − qP / 6 );
          dcY[i][j] =
              (f[i][j] * LevelScale4x4[qP % 6][0][0] + (1 << (5 - qP / 6))) >>
              (6 - qP / 6);
        }
      }
    }
  }

  return 0;
}

// 8.6 Decoding process for P macroblocks in SP slices or SI macroblocks
// This process is invoked when decoding P macroblock types in an SP slice type
// or the SI macroblock type in SI slices.
int PictureBase::
    Decoding_process_for_P_macroblocks_in_SP_slices_or_SI_macroblocks() {
  return -1;
}
