#include "AnnexBReader.hpp"
#include "Frame.hpp"
#include "GOP.hpp"
#include "Image.hpp"
#include "Nalu.hpp"
#include <ostream>

typedef enum _OUTPUT_FILE_TYPE { NON, BMP, YUV } OUTPUT_FILE_TYPE;

int32_t g_Width = 0;
int32_t g_Height = 0;
int32_t g_PicNumCnt = 0;
//int32_t g_OutputFileType = YUV;
int32_t g_OutputFileType = BMP;

int outputFrame(GOP *gop, Frame *frame);
int flushFrame(GOP *gop, Frame *&frame, bool isFromIDR);

int main(int argc, char *argv[]) {
  /* 关闭io输出同步 */
  // ios::sync_with_stdio(false);

  string filePath;
  if (argc > 1 && argv[1] != nullptr)
    filePath = argv[1];
  else {
    filePath = "./demo_10_frames.avif";
  }

  /* 1. 打开文件、读取NUL、存储NUL的操作 */
  AnnexBReader reader(filePath);
  int result = reader.open();
  RET(result);

  /* 2. 创建一个GOP用于存放解码后的I、P、B帧序列 */
  GOP *gop = new GOP();
  Frame *frame = gop->m_dpb[0];

  BitStream *bitStream = nullptr;

  int number = 0;
  /* 这里只对文件进行解码，所以只有AnnesB格式 */
  while (true) {
    /* 3. 一个NUL类，用于存储NUL数据，它与NUL具有同样的数据结构 */
    Nalu nalu;
    Nalu::EBSP ebsp;
    Nalu::RBSP rbsp;
    SEI sei;
    /* 3. 循环读取一个个的Nalu */
    result = reader.readNalu(nalu);

    if (result == 1 || result == 0) {
      cout << "Reading a NAL[" << ++number << "]{" << (int)nalu.buffer[0] << " "
           << (int)nalu.buffer[1] << " " << (int)nalu.buffer[2] << " "
           << (int)nalu.buffer[3] << "}, Buffer len[" << nalu.len << "]";

      /* 4. 从NAL中解析出EBSP */
      nalu.parseEBSP(ebsp);
      cout << "   --->   EBSP[" << number << "]{" << (int)ebsp.buf[0] << " "
           << (int)ebsp.buf[1] << " " << (int)ebsp.buf[2] << " "
           << (int)ebsp.buf[3] << "}, Buffer len[" << ebsp.len << "]";

      /* 5. 从EBSP中解析出RBSP */
      nalu.parseRBSP(ebsp, rbsp);
      cout << "  --->   RBSP[" << number << "]{" << (int)rbsp.buf[0] << " "
           << (int)rbsp.buf[1] << " " << (int)rbsp.buf[2] << " "
           << (int)rbsp.buf[3] << "}, Buffer len[" << rbsp.len << "]" << endl;

      /* 6. 从RBSP中解析出SODB(未实现） */
      // nalu.parseSODB(rbsp, SODB);

      /* 见T-REC-H.264-202108-I!!PDF-E.pdf 87页 */
      if (nalu.nal_unit_type > 21) cout << "Unknown Nalu Type !!!" << endl;

      switch (nalu.nal_unit_type) {
      case 1: /* Slice(non-VCL) */
        /* 11-2. 解码普通帧 */
        cout << "Original Slice -> {" << endl;
        flushFrame(gop, frame, false);
        /* 初始化bit处理器，填充slice的数据 */
        bitStream = new BitStream(rbsp.buf, rbsp.len);
        /* 此处根据SliceHeader可判断A Frame =? A Slice */
        nalu.extractSliceparameters(*bitStream, *gop, *frame);
        frame->decode(*bitStream, gop->m_dpb, *gop);
        cout << " }" << endl;
        break;
      case 2: /* DPA(non-VCL) */
        cout << "Not Support DPA!" << endl;
        break;
      case 3: /* DPB(non-VCL) */
        cout << "Not Support DPB!" << endl;
        break;
      case 4: /* DPC(non-VCL) */
        cout << "Not Support DPC!" << endl;
        break;
      case 5: /* IDR Slice(VCL) */
        /* 11-1. 解码立即刷新帧 GOP[0] */
        cout << "IDR Slice -> {" << endl;
        /* 提供给外层程序一定是Frame，即一帧数据，而不是一个Slice，因为如果存在多个Slice为一帧的情况外层处理就很麻烦 */
        flushFrame(gop, frame, true);
        /* 初始化bit处理器，填充idr的数据 */
        bitStream = new BitStream(rbsp.buf, rbsp.len);
        /* 这里通过解析SliceHeader后可以知道一个Frame到底是几个Slice，通过直接调用frame->decode，在内部对每个Slice->decode() （如果存在多个Slice的情况，可以通过first_mb_in_slice判断，如果每个Slice都为0,则表示每个Slice都是一帧数据，当first_mb_in_slice>0，则表示与前面的一个或多个Slice共同组成一个Frame） */
        nalu.extractIDRparameters(*bitStream, *gop, *frame);
        frame->decode(*bitStream, gop->m_dpb, *gop);
        cout << " }" << endl;
        break;
      case 6: /* SEI（补充信息）(VCL) */
        /* 10. 解码SEI补充增强信息：
         * 场编码的图像在每个Slice前出现SEI以提供必要的解码辅助信息 */
        cout << "SEI -> {" << endl;
        nalu.extractSEIparameters(rbsp, sei, gop->m_spss[gop->last_pps_id]);
        cout << " }" << endl;
        break;
      case 7: /* SPS(VCL) */
        /* 8. 解码SPS中信息 */
        cout << "SPS -> {" << endl;
        nalu.extractSPSparameters(rbsp, gop->m_spss, gop->last_sps_id);
        gop->m_max_num_reorder_frames =
            gop->m_spss[gop->last_sps_id].max_num_reorder_frames;
        cout << " }" << endl;
        break;
      case 8: /* PPS(VCL) */
        /* 9. 解码PPS中信息 */
        cout << "PPS -> {" << endl;
        nalu.extractPPSparameters(
            rbsp, gop->m_ppss, gop->last_pps_id,
            gop->m_spss[gop->last_sps_id].chroma_format_idc);
        cout << " }" << endl;
        break;
      case 9: /* 7.3.2.4 Access unit delimiter RBSP syntax */
        /* 该Nalu的优先级很高，如果存在则它会在SPS前出现 */
        //access_unit_delimiter_rbsp();
        cerr << "access_unit_delimiter_rbsp()" << endl;
        break;
      case 10:
        //end_of_seq_rbsp();
        cerr << "end_of_seq_rbsp()" << endl;
        break;
      case 11:
        //end_of_stream_rbsp();
        cerr << "end_of_stream_rbsp()" << endl;
        break;
      case 12:
        //filler_data_rbsp();
        cerr << "filler_data_rbsp()" << endl;
        break;
      case 13:
        //seq_parameter_set_extension_rbsp();
        cerr << "seq_parameter_set_extension_rbsp()" << endl;
        break;
      case 14:
        //prefix_nal_unit_rbsp();
        cerr << "prefix_nal_unit_rbsp()" << endl;
        break;
      case 15:
        //subset_seq_parameter_set_rbsp();
        cerr << "subset_seq_parameter_set_rbsp()" << endl;
        break;
      case 16:
        //depth_parameter_set_rbsp();
        cerr << "depth_parameter_set_rbsp()" << endl;
        break;
      case 19:
        //slice_layer_without_partitioning_rbsp();
        cerr << "slice_layer_without_partitioning_rbsp()" << endl;
        break;
      case 20:
        //slice_layer_extension_rbsp();
        cerr << "slice_layer_extension_rbsp()" << endl;
        break;
      case 21: /* 3D-AVC texture view */
        //slice_layer_extension_rbsp();
        cerr << "slice_layer_extension_rbsp()" << endl;
        break;
      default:
        cerr << "Error nal_unit_type:" << nalu.nal_unit_type << endl;
      }

      /* 已读取完成所有NAL */
      if (result == 0) break;
    } else {
      RET(-1);
      break;
    }
    cout << endl;
  }

  /* 最后一个解码帧 */
  flushFrame(gop, frame, true);

  /* 将剩余的缓存帧全部输出 */
  for (int i = 0; i < gop->m_max_num_reorder_frames; ++i)
    outputFrame(gop, nullptr);

  if (g_OutputFileType == YUV)
    cout << "\tffplay -video_size " << g_Width << "x" << g_Height
         << " output.yuv" << endl;
  /* 读取完所有Nalu，并送入解码后，则将缓存中所有的Frame读取出来，准备退出 */
  reader.close();
  delete gop;
  delete bitStream;
  return 0;
}

/* 清空单帧，若当IDR解码完成时，则对整个GOP进行flush */
// 输入的frame：表示上一解码完成的帧
// 输出的frame：表示一个新帧（从缓冲区中重复利用的帧）
int flushFrame(GOP *gop, Frame *&frame, bool isFromIDR) {
  if (frame != nullptr && frame->m_current_picture_ptr != nullptr) {
    Frame *newEmptyPicture = nullptr;
    frame->m_current_picture_ptr->getEmptyFrameFromDPB(newEmptyPicture);

    //当上一帧完成解码且为IDR帧，则进行GOP -> flush
    if (isFromIDR == false)
      if (frame->m_picture_frame.m_slice->slice_header->IdrPicFlag) {
        g_Width = frame->m_picture_frame.PicWidthInSamplesL;
        g_Height = frame->m_picture_frame.PicHeightInSamplesL;
        gop->flush();
      }

    outputFrame(gop, frame);
    frame = newEmptyPicture;
  }
  return 0;
}

int outputFrame(GOP *gop, Frame *frame) {
  Frame *outPicture = nullptr;
  gop->outputOneFrame(frame, outPicture);
  // 在含B帧的情况下，解码后的帧还需要排序POC，按照POC顺序进行输出，这里不一定有帧输出
  if (outPicture != nullptr) {
    static int index = 0;
    //标记为闲置状态，以便后续回收重复利用
    outPicture->m_is_in_use = false;
    Image image;
    if (g_OutputFileType == BMP) {
      string output_file;
      const uint32_t slice_type =
          outPicture->slice->slice_header->slice_type % 5;
      if (slice_type == SLICE_I)
        output_file = "output_I_" + to_string(index++) + ".bmp";
      else if (slice_type == SLICE_P)
        output_file = "output_P_" + to_string(index++) + ".bmp";
      else if (slice_type == SLICE_B)
        output_file = "output_B_" + to_string(index++) + ".bmp";
      else {
        std::cerr << "Unrecognized slice type:"
                  << outPicture->slice->slice_header->slice_type << std::endl;
        return -1;
      }
      image.saveToBmpFile(outPicture->m_picture_frame, output_file.c_str());
    } else if (g_OutputFileType == YUV)
      image.writeYUV(outPicture->m_picture_frame, "output.yuv");
  }
  return 0;
}
