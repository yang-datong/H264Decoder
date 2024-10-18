#ifndef IMAGE_HPP_9DPBJPRW
#define IMAGE_HPP_9DPBJPRW
#include "Common.hpp"
#include "PictureBase.hpp"

class Image {
 public:
  int saveToBmpFile(PictureBase &pic, const char *filename);
  int writeYUV(PictureBase &pic, const char *filename);

 private:
  int saveBmp(const char *filename, MY_BITMAP *pBitmap);

  // 图像不是上下翻转的，主要用于保存成BMP图片
  int convertYuv420pToBgr24(uint32_t width, uint32_t height,
                            const uint8_t *yuv420p, uint8_t *bgr24,
                            uint32_t widthBytesBgr24);
  // 图像是上下翻转的，主要用于播放器画图
  int convertYuv420pToBgr24FlipLines(uint32_t width, uint32_t height,
                                     const uint8_t *yuv420p, uint8_t *bgr24,
                                     uint32_t widthBytesBgr24);

  // 在内存中创建一幅空白位图
  int createEmptyImage(MY_BITMAP &bitmap, int32_t width, int32_t height,
                       int32_t bmBitsPixel);
};

#endif /* end of include guard: IMAGE_HPP_9DPBJPRW */
