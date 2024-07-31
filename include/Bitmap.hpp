#ifndef BITMAP_HPP_CKMEXONJ
#define BITMAP_HPP_CKMEXONJ

//----------Bitmap-------------------
typedef struct {
  //    unsigned short    bfType;
  unsigned int bfSize;
  unsigned short bfReserved1;
  unsigned short bfReserved2;
  unsigned int bfOffBits;
} MY_BitmapFileHeader;

typedef struct {
  unsigned int biSize;
  int biWidth;
  int biHeight;
  unsigned short biPlanes;
  unsigned short biBitCount;
  unsigned int biCompression;
  unsigned int biSizeImage;
  int biXPelsPerMeter;
  int biYPelsPerMeter;
  unsigned int biClrUsed;
  unsigned int biClrImportant;
} MY_BitmapInfoHeader;

typedef struct {
  unsigned char rgbBlue;     // 该颜色的蓝色分量
  unsigned char rgbGreen;    // 该颜色的绿色分量
  unsigned char rgbRed;      // 该颜色的红色分量
  unsigned char rgbReserved; // 保留值
} MY_RgbQuad;

#endif /* end of include guard: BITMAP_HPP_CKMEXONJ */
