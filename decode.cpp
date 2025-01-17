#include "decode.hpp"
#include "BitStream.hpp"
#include "Frame.hpp"
#include "GOP.hpp"
#include "Image.hpp"
#include "Nalu.hpp"
#include <ostream>

#ifdef DISABLE_COUT
#define cout                                                                   \
  if (false) std::cout
#endif

int decode(Nalu &nalu, int &number);

void printfNALBytes(int &number, const Nalu &nalu);
void printfEBSPBytes(int number, const Nalu::EBSP &ebsp);
void printfRBSPBytes(int number, const Nalu::RBSP &rbsp);
int outputFrame(GOP *gop, Frame *frame);
int flushFrame(GOP *gop, Frame *&frame, bool isFromIDR);

int32_t g_Width = 0, g_Height = 0;
int32_t g_PicNumCnt = 0;

//NOTE: 由外部控制
OUTPUT_FILE_TYPE g_OutputFileType = NON;

AnnexBReader *g_reader = nullptr;
GOP *gop = nullptr;
Frame *frame = nullptr;
BitStream *bitStream = nullptr;

int decode_flush() {
  /* 最后一个解码帧 */
  flushFrame(gop, frame, true);

  /* 将剩余的缓存帧全部输出 */
  for (int i = 0; i < gop->m_max_num_reorder_frames; ++i)
    outputFrame(gop, nullptr);

  if (g_OutputFileType == YUV)
    cout << "\tffplay -video_size " << g_Width << "x" << g_Height
         << " output.yuv" << endl;
  /* 读取完所有Nalu，并送入解码后，则将缓存中所有的Frame读取出来，准备退出 */

  return 0;
}

int decode(uint8_t *buffer, int buffer_len) {
  AnnexBReader reader;

  Nalu nalu;
  nalu.setBuffer(buffer, buffer_len);

  int startcodeLen;
  if (!reader.findStartcode(startcodeLen, buffer, buffer_len)) {
    // 第一次读肯定是能够读取到的，如果没读取到说明不是h264文件
    std::cerr << "\033[31m findStartcode() \033[0m" << std::endl;
    return -1;
  }
  nalu.startCodeLenth = startcodeLen;

  int number = 0;
  decode(nalu, number);
  return 0;
}

int gPicOrderCnt = -1;
int gResidualPicOrderCnt = -1;
int gNaluType = -1;

int decode_get_poc_value() { return gPicOrderCnt; }
int decode_get_poc_residual_value() { return gResidualPicOrderCnt; }
int decode_get_nal_unit_type() { return gNaluType; }

int decode(Nalu &nalu, int &number) {
  Nalu::EBSP ebsp;
  Nalu::RBSP rbsp;
  SEI sei;
  printfNALBytes(number, nalu);

  /* 4. 从NAL中解析出EBSP */
  nalu.parseEBSP(ebsp);
  printfEBSPBytes(number, ebsp);

  /* 5. 从EBSP中解析出RBSP */
  nalu.parseRBSP(ebsp, rbsp);
  printfRBSPBytes(number, rbsp);

  /* 6. 从RBSP中解析出SODB(TODO） */
  // nalu.parseSODB(rbsp, SODB);

  /* T-REC-H.264-202108-I!!PDF-E.pdf -> page 87 */
  if (nalu.nal_unit_type > 21) cout << "Unknown Nalu Type !!!" << endl;
  gNaluType = nalu.nal_unit_type;
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
    gPicOrderCnt = frame->m_current_picture_ptr->PicOrderCnt;
    if (frame->m_current_picture_ptr && frame->m_picture_previous)
      gResidualPicOrderCnt = frame->m_current_picture_ptr->PicOrderCnt -
                             frame->m_picture_previous->PicOrderCnt;
    else
      gResidualPicOrderCnt = 0;
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
    /* FIXME:提供给外层程序一定是Frame，即一帧数据，而不是一个Slice，因为如果存在多个Slice为一帧的情况外层处理就很麻烦 */
    flushFrame(gop, frame, true);
    /* 初始化bit处理器，填充idr的数据 */
    bitStream = new BitStream(rbsp.buf, rbsp.len);
    nalu.extractIDRparameters(*bitStream, *gop, *frame);
    frame->decode(*bitStream, gop->m_dpb, *gop);
    gPicOrderCnt = frame->m_current_picture_ptr->PicOrderCnt;
    if (frame->m_current_picture_ptr && frame->m_picture_previous)
      gResidualPicOrderCnt = frame->m_current_picture_ptr->PicOrderCnt -
                             frame->m_picture_previous->PicOrderCnt;
    else
      gResidualPicOrderCnt = 0;
    cout << " }" << endl;
    break;
  case 6: /* SEI（补充信息）(VCL) */
    /* 10. 解码SEI补充增强信息：场编码的图像在每个Slice前出现SEI以提供必要的解码辅助信息 */
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
    nalu.extractPPSparameters(rbsp, gop->m_ppss, gop->last_pps_id,
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
  return 0;
}

int decode_init() {
  g_OutputFileType = NON;
  /* 2. 创建一个GOP用于存放解码后的I、P、B帧序列 */
  gop = new GOP();
  frame = gop->m_dpb[0];
  return 0;
}

int decode_init(AnnexBReader *&reader, string &filePath,
                OUTPUT_FILE_TYPE outputFileType) {
  g_OutputFileType = outputFileType;
  /* 1. 打开文件、读取NUL、存储NUL的操作 */
  g_reader = new AnnexBReader(filePath);
  if (g_reader == nullptr) return -1;
  if (g_reader->open()) return -1;

  reader = g_reader;

  /* 2. 创建一个GOP用于存放解码后的I、P、B帧序列 */
  gop = new GOP();
  frame = gop->m_dpb[0];
  return 0;
}

void decode_relase() {
  if (gop) delete gop;
  if (bitStream) delete bitStream;
  if (g_reader) g_reader->close();
  if (g_reader) delete g_reader;
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

void printfNALBytes(int &number, const Nalu &nalu) {
  cout << "Reading a NAL[" << ++number << "]{" << (int)nalu.buffer[0] << " "
       << (int)nalu.buffer[1] << " " << (int)nalu.buffer[2] << " "
       << (int)nalu.buffer[3] << "}, Buffer len[" << nalu.len << "]";
}

void printfEBSPBytes(int number, const Nalu::EBSP &ebsp) {
  cout << "   --->   EBSP[" << number << "]{" << (int)ebsp.buf[0] << " "
       << (int)ebsp.buf[1] << " " << (int)ebsp.buf[2] << " " << (int)ebsp.buf[3]
       << "}, Buffer len[" << ebsp.len << "]";
}

void printfRBSPBytes(int number, const Nalu::RBSP &rbsp) {
  cout << "  --->   RBSP[" << number << "]{" << (int)rbsp.buf[0] << " "
       << (int)rbsp.buf[1] << " " << (int)rbsp.buf[2] << " " << (int)rbsp.buf[3]
       << "}, Buffer len[" << rbsp.len << "]" << endl;
}
