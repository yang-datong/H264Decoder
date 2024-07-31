#include "AnnexBReader.hpp"
#include "H264PicturesGOP.hpp"
#include "NaluPPS.hpp"

int32_t g_PicNumCnt = 0;

CH264PicturesGOP *pictures_gop = new CH264PicturesGOP;

int my_output_frame_callback(Nalu *outPicture, void *userData, int errorCode) {
  int ret = 0;

  if (outPicture) {
    static int s_PicNumCnt = 0;

    char *outDir = (char *)userData;

    char filename[600] = {0};
    sprintf(filename, "%s/out_%dx%d.%d.bmp", outDir,
            outPicture->m_picture_frame.PicWidthInSamplesL,
            outPicture->m_picture_frame.PicHeightInSamplesL, s_PicNumCnt);

    printf("my_output_frame_callback(): m_PicNumCnt=%d(%s); PicOrderCnt=%d; "
           "filename=%s;\n",
           outPicture->m_picture_frame.m_PicNumCnt,
           H264_SLIECE_TYPE_TO_STR(
               outPicture->m_picture_frame.m_h264_slice_header.slice_type),
           outPicture->m_picture_frame.PicOrderCnt, filename);

    ret = outPicture->m_picture_frame.saveToBmpFile(filename);
    if (ret != 0) {
      printf("outPicture->m_picture_frame.saveToBmpFile() failed! %s\n",
             filename);
      return -1;
    }

    s_PicNumCnt++;

  } else
    // 表示解码结束了
    return -1;

  return 0;
}

int do_callback(Nalu *picture_current, CH264PicturesGOP *pictures_gop,
                int32_t is_need_flush) {
  int ret = 0;
  Nalu *outPicture = NULL;
  int errorCode = 0;

  if (is_need_flush) { // 说明当前已解码完毕的帧是IDR帧
    while (1) {
      ret = pictures_gop->getOneOutPicture(NULL, outPicture); // flush操作
      RETURN_IF_FAILED(ret != 0, ret);

      if (my_output_frame_callback != NULL) {
        ret = my_output_frame_callback(
            outPicture, NULL,
            errorCode); // 当找到可输出的帧时，主动通知外部用户
        if (ret != 0) {
          return -1; // 直接退出
        }
        outPicture->m_is_in_use = 0; // 标记为闲置状态，以便后续回收重复利用
      } else                         // if (outPicture == NULL)
             // //说明已经flush完毕，DPB缓存中已经没有可输出的帧了
        break;
    }
  }

  //-----------------------------------------------------------------------
  ret = pictures_gop->getOneOutPicture(picture_current, outPicture);
  RETURN_IF_FAILED(ret != 0, ret);

  if (outPicture != NULL) {
    if (my_output_frame_callback != NULL) {
      ret = my_output_frame_callback(outPicture, NULL, errorCode);
      // 当找到可输出的帧时，主动通知外部用户
    }

    outPicture->m_is_in_use = 0; // 标记为闲置状态，以便后续回收重复利用
  }

  return 0;
}

int main() {
  std::string filePath = "./source_cut_10_frames.h264";
  // std::string filePath = "./demo_10_frames.h264";
  /* 1. 用类封装h264文件的打开文件、读取NUL、存储NUL的操作 */
  AnnexBReader reader(filePath);
  int result = reader.open();
  if (result)
    return -1;

  /* 2. 创建一个NUL类，用于存储NUL数据，它与NUL具有同样的数据结构 */
  Nalu nalu;

  EBSP ebsp;
  RBSP rbsp;
  int number = 0;
  while (true) {
    /* 3. 循环读取一个个的Nalu */
    result = reader.readNalu(nalu);

    if (result == 1 || result == 0) {
      printf("Reading a NAL[%d]{%d %d %d %d},Buffer len[%d] \n", ++number,
             nalu._buffer[0], nalu._buffer[1], nalu._buffer[2], nalu._buffer[3],
             nalu._len);

      /* 4. 从NAL中解析出EBSP */
      nalu.parseEBSP(ebsp);
      // printf("\tEBSP[%d]{%d %d %d %d},Buffer len[%d] \n", number,
      // ebsp._buf[0],
      //        ebsp._buf[1], ebsp._buf[2], ebsp._buf[3], ebsp._len);

      /* 5. 从EBSP中解析出RBSP */
      nalu.parseRBSP(ebsp, rbsp);
      // printf("\tRBSP[%d]{%d %d %d %d},Buffer len[%d]\n", number,
      // rbsp._buf[0],
      //        rbsp._buf[1], rbsp._buf[2], rbsp._buf[3], rbsp._len);

      /* 6. 从RBSP中解析出SODB */
      // nalu.parseSODB(rbsp, SODB);

      /* 7. 从RBSP中解析出NAL头部 (已移动到内部函数）*/
      // nalu.parseHeader(rbsp);
      // printf("\tforbidden_zero_bit:%d,nal_ref_idc:%d,nal_unit_type:%d\n",
      //        nalu.forbidden_zero_bit, nalu.nal_ref_idc, nalu.nal_unit_type);

      /* 见T-REC-H.264-202108-I!!PDF-E.pdf 87页 */
      switch (nalu.nal_unit_type) {
      case 1: /* Slice(non-VCL) */
        /* 11-2. 解码普通帧 */
        std::cout << "Original Slice -> {" << std::endl;
        nalu.extractSliceparameters(rbsp);
        std::cout << " }" << std::endl;
        break;
      case 2: /* DPA(non-VCL) */
        break;
      case 5: /* IDR(VCL) */
        /* 11-1. 解码立即刷新帧 GOP[0] */
        std::cout << "IDR -> {" << std::endl;
        nalu.extractIDRparameters(rbsp);
        std::cout << " }" << std::endl;
        break;
      case 6: /* SEI(VCL) */
        /* 10. 解码SEI补充增强信息 */
        std::cout << "SEI -> {" << std::endl;
        nalu.extractSEIparameters(rbsp);
        std::cout << " }" << std::endl;
        break;
      case 7: /* SPS(VCL) */
        /* 8. 解码SPS中信息 */
        std::cout << "SPS -> {" << std::endl;
        nalu.extractSPSparameters(rbsp);
        std::cout << " }" << std::endl;
        break;
      case 8: /* PPS(VCL) */
        /* 9. 解码PPS中信息 */
        std::cout << "PPS -> {" << std::endl;
        nalu.extractPPSparameters(rbsp);
        std::cout << " }" << std::endl;
        break;
      }

      /* 已读取完成所有NAL */
      if (result == 0)
        break;
    } else {
      std::cerr << "\033[31mReading error\033[0m" << std::endl;
      break;
    }
  }

  //--------flush操作--------------
  if (result == 0) {
    // int is_need_flush = nalu.m_picture_frame.m_h264_slice_header.IdrPicFlag;
    //  当前已解码完毕的帧是否是IDR帧，如果是IDR帧则lush
    do_callback(&nalu, pictures_gop, 1); // 回调操作
    // RETURN_IF_FAILED(ret != 0, ret);
    do_callback(NULL, pictures_gop, 1); // 最后再回调一次
  }
  reader.close();
  return 0;
}
