#include "Image.hpp"
#include "Bitmap.hpp"

int Image::saveToBmpFile(PictureBase &pic, const char *filename) {
  int32_t W = pic.PicWidthInSamplesL;
  int32_t H = pic.PicHeightInSamplesL;
  MY_BITMAP bitmap;
  createEmptyImage(bitmap, W, H, 24);
  convertYuv420pToBgr24(W, H, pic.m_pic_buff_luma, (uint8_t *)bitmap.bmBits,
                        bitmap.bmWidthBytes);
  RET(saveBmp(filename, &bitmap));
  if (bitmap.bmBits) {
    free(bitmap.bmBits);
    bitmap.bmBits = nullptr;
  }
  return 0;
}

int Image::saveBmp(const char *filename, MY_BITMAP *pBitmap) {
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

int Image::convertYuv420pToBgr24(uint32_t width, uint32_t height,
                                 const uint8_t *yuv420p, uint8_t *bgr24,
                                 uint32_t widthBytesBgr24) {
  int32_t W = width, H = height, channels = 3;
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      uint8_t Y = yuv420p[y * W + x];
      uint8_t U = yuv420p[H * W + (y / 2) * (W / 2) + x / 2];
      uint8_t V = yuv420p[H * W + H * W / 4 + (y / 2) * (W / 2) + x / 2];

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

int Image::convertYuv420pToBgr24FlipLines(uint32_t width, uint32_t height,
                                          const uint8_t *yuv420p,
                                          uint8_t *bgr24,
                                          uint32_t widthBytesBgr24) {
  int32_t W = width, H = height, channels = 3;
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      uint8_t Y = yuv420p[y * W + x];
      uint8_t U = yuv420p[H * W + (y / 2) * (W / 2) + x / 2];
      uint8_t V = yuv420p[H * W + H * W / 4 + (y / 2) * (W / 2) + x / 2];

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

int Image::createEmptyImage(MY_BITMAP &bitmap, int32_t width, int32_t height,
                            int32_t bmBitsPixel) {
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

#include <fstream>

/* 所有解码的帧写入到一个文件 */
int Image::writeYUV(PictureBase &pic, const char *filename) {
  static bool isFrist = false;
  if (isFrist == false) {
    std::ifstream f(filename);
    if (f.good()) remove(filename);
    isFrist = true;
  }

  FILE *fp = fopen(filename, "ab+");
  if (fp == NULL) return -1;

  int32_t lumaSize = pic.PicWidthInSamplesL * pic.PicHeightInSamplesL;
  int32_t chromaSize = pic.PicWidthInSamplesC * pic.PicHeightInSamplesC;
  fwrite(pic.m_pic_buff_luma, lumaSize, 1, fp);
  fwrite(pic.m_pic_buff_cb, chromaSize, 1, fp);
  fwrite(pic.m_pic_buff_cr, chromaSize, 1, fp);

  fclose(fp);
  return 0;
}
