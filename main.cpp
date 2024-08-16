#include "AnnexBReader.hpp"
#include "GOP.hpp"
#include "Nalu.hpp"

int32_t g_PicNumCnt = 0;

int main() {
  // string filePath = "./source_cut_10_frames.h264";
  //   string filePath = "./source_cut_10_frames_no_B.h264";
  string filePath = "./demo_10_frames.h264";

  /* 1. 打开文件、读取NUL、存储NUL的操作 */
  AnnexBReader reader(filePath);
  int result = reader.open();
  if (result)
    return -1;

  /* 2. 创建一个GOP用于存放解码后的I、P、B帧序列 */
  GOP *gop = new GOP();
  Frame *frame = gop->m_DecodedPictureBuffer[0];

  /* 3. 一个NUL类，用于存储NUL数据，它与NUL具有同样的数据结构 */
  Nalu nalu;
  EBSP ebsp;
  RBSP rbsp;

  SEI sei;

  BitStream *bitStream = nullptr;

  int number = 0;
  while (true) {
    /* 3. 循环读取一个个的Nalu */
    result = reader.readNalu(nalu);

    if (result == 1 || result == 0) {
      cout << "Reading a NAL[" << ++number << "]{" << (int)nalu._buffer[0]
           << " " << (int)nalu._buffer[1] << " " << (int)nalu._buffer[2] << " "
           << (int)nalu._buffer[3] << "}, Buffer len[" << nalu._len << "]";

      /* 4. 从NAL中解析出EBSP */
      nalu.parseEBSP(ebsp);
      cout << "   --->   EBSP[" << number << "]{" << (int)ebsp._buf[0] << " "
           << (int)ebsp._buf[1] << " " << (int)ebsp._buf[2] << " "
           << (int)ebsp._buf[3] << "}, Buffer len[" << ebsp._len << "]";

      /* 5. 从EBSP中解析出RBSP */
      nalu.parseRBSP(ebsp, rbsp);
      cout << "  --->   RBSP[" << number << "]{" << (int)rbsp._buf[0] << " "
           << (int)rbsp._buf[1] << " " << (int)rbsp._buf[2] << " "
           << (int)rbsp._buf[3] << "}, Buffer len[" << rbsp._len << "]" << endl;

      /* 6. 从RBSP中解析出SODB(未实现） */
      // nalu.parseSODB(rbsp, SODB);

      /* 见T-REC-H.264-202108-I!!PDF-E.pdf 87页 */
      int is_need_flush = 0;
      switch (nalu.nal_unit_type) {
      case 1: /* Slice(non-VCL) */
        /* 11-2. 解码普通帧 */
        cout << "Original Slice -> {" << endl;

        Frame *newEmptyPicture;
        frame->m_current_picture_ptr
            ->end_decode_the_picture_and_get_a_new_empty_picture(
                newEmptyPicture);
        frame = newEmptyPicture;

        /* 初始化bit处理器，填充slice的数据 */
        bitStream = new BitStream(rbsp._buf, rbsp._len);
        nalu.extractSliceparameters(*bitStream, *gop, *frame);

        cout << " }" << endl;
        break;
      case 2: /* DPA(non-VCL) */
        break;
      case 5: /* IDR(VCL) */
        /* 11-1. 解码立即刷新帧 GOP[0] */
        cout << "IDR -> {" << endl;

        /* 初始化bit处理器，填充idr的数据 */
        bitStream = new BitStream(rbsp._buf, rbsp._len);
        nalu.extractIDRparameters(*bitStream, *gop, *frame);

        cout << " }" << endl;
        break;
      case 6: /* SEI(VCL) */
        /* 10. 解码SEI补充增强信息 */
        cout << "SEI -> {" << endl;
        nalu.extractSEIparameters(rbsp, sei);
        cout << " }" << endl;
        break;
      case 7: /* SPS(VCL) */
        /* 8. 解码SPS中信息 */
        cout << "SPS -> {" << endl;
        nalu.extractSPSparameters(rbsp, gop->m_spss[0]);
        cout << " }" << endl;
        break;
      case 8: /* PPS(VCL) */
        /* 9. 解码PPS中信息 */
        cout << "PPS -> {" << endl;
        nalu.extractPPSparameters(rbsp, gop->m_ppss[0],
                                  gop->m_spss[0].chroma_format_idc);
        cout << " }" << endl;
        break;
      }

      /* 已读取完成所有NAL */
      if (result == 0)
        break;
    } else {
      cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
           << endl;
      break;
    }
  }
  reader.close();
  return 0;
}
