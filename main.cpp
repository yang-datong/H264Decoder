#include "AnnexBReader.hpp"
#include "BitStream.hpp"
#include "NaluPPS.hpp"
#include "NaluSPS.hpp"

int main() {
  std::string filePath = "./demo_10_frames.h264";
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
      printf("\tEBSP[%d]{%d %d %d %d},Buffer len[%d] \n", number, ebsp._buf[0],
             ebsp._buf[1], ebsp._buf[2], ebsp._buf[3], ebsp._len);

      /* 5. 从EBSP中解析出RBSP */
      nalu.parseRBSP(ebsp, rbsp);
      printf("\tRBSP[%d]{%d %d %d %d},Buffer len[%d]\n", number, rbsp._buf[0],
             rbsp._buf[1], rbsp._buf[2], rbsp._buf[3], rbsp._len);

      /* 6. 从RBSP中解析出SODB */
      // nalu.parseSODB(rbsp, SODB);

      /* 7. 从RBSP中解析出NAL头部 (已移动到内部函数）*/
      // nalu.parseHeader(rbsp);
      // printf("\tforbidden_zero_bit:%d,nal_ref_idc:%d,nal_unit_type:%d\n",
      //        nalu.forbidden_zero_bit, nalu.nal_ref_idc, nalu.nal_unit_type);

      switch (nalu.nal_unit_type) {
      case 1: /* Slice */
        /* TODO YangJing decode slice and slice head <24-04-02 22:08:03> */
        break;
      case 2: /* DPA */
        break;
      case 5: /* IDR 立即刷新帧 GOP[0] */
        nalu.extractIDRparameters(rbsp);
        break;
      case 6: /* SEI */
        break;
      case 7: /* SPS */
        /* 8. 解码SPS中信息 */
        nalu.extractSSPparameters(rbsp);
        break;
      case 8: /* PPS */
        std::cout << "PPS" << std::endl;
        break;
      }

      if (result == 0)
        /* 已读取完成所有NAL */
        break;
    } else {
      std::cerr << "\033[31mReading error\033[0m" << std::endl;
      break;
    }
  }

  /* TODO YangJing flush操作 <24-04-02 22:13:05> */

  reader.close();
  return 0;
}
