#include "AnnexBReader.hpp"
#include "GOP.hpp"
#include "Nalu.hpp"

int32_t g_PicNumCnt = 0;

int main() {
  string filePath = "./source_cut_10_frames.h264";
  // std::string filePath = "./demo_10_frames.h264";
  /* 1. 用类封装h264文件的打开文件、读取NUL、存储NUL的操作 */
  AnnexBReader reader(filePath);
  int result = reader.open();
  if (result)
    return -1;

  /* 2. 创建一个NUL类，用于存储NUL数据，它与NUL具有同样的数据结构 */
  GOP *gop = new GOP();
  /* 初始化第一个Nal */
  Nalu nalu;
  Frame *frame = gop->m_DecodedPictureBuffer[0];
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
      int is_need_flush = 0;
      switch (nalu.nal_unit_type) {
      case 1: /* Slice(non-VCL) */
        /* 11-2. 解码普通帧 */
        std::cout << "Original Slice -> {" << std::endl;
        is_need_flush = 1; // 针对IDR帧后，需要flush一次
        // do_callback(nalu, gop, is_need_flush); // 回调操作
        Frame *newEmptyPicture;
        frame->m_current_picture_ptr
            ->end_decode_the_picture_and_get_a_new_empty_picture(
                newEmptyPicture);
        frame = newEmptyPicture;
        nalu.extractSliceparameters(rbsp, *gop, *frame);
        std::cout << " }" << std::endl;
        /* TODO YangJing number = 5 为B帧，则需要解码后一个P帧<24-08-14
         * 22:39:46> */
        if (number == 5)
          exit(0);
        break;
      case 2: /* DPA(non-VCL) */
        break;
      case 5: /* IDR(VCL) */
        /* 11-1. 解码立即刷新帧 GOP[0] */
        std::cout << "IDR -> {" << std::endl;
        nalu.extractIDRparameters(rbsp, *gop, *frame);
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
        gop->m_spss[0] = nalu.getSPS();
        std::cout << " }" << std::endl;
        break;
      case 8: /* PPS(VCL) */
        /* 9. 解码PPS中信息 */
        std::cout << "PPS -> {" << std::endl;
        nalu.extractPPSparameters(rbsp);
        gop->m_ppss[0] = nalu.getPPS();
        std::cout << " }" << std::endl;
        break;
      }

      /* 已读取完成所有NAL */
      if (result == 0)
        break;
    } else {
      std::cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
                << std::endl;
      break;
    }
  }
  reader.close();
  return 0;
}
