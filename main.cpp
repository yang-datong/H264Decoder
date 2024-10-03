#include "AnnexBReader.hpp"
#include "Frame.hpp"
#include "GOP.hpp"
#include "Nalu.hpp"
#include <ostream>

typedef enum _OUTPUT_FILE_TYPE { NON, BMP, YUV } OUTPUT_FILE_TYPE;

int32_t g_PicNumCnt = 0;

int flushFrame(GOP *&gop, Frame *&frame, bool isFromIDR,
               OUTPUT_FILE_TYPE output_file_type = NON);

int main(int argc, char *argv[]) {
  /* 关闭io输出同步 */
  // std::ios::sync_with_stdio(false);

  string filePath;
  if (argc > 1 && argv[1] != NULL)
    filePath = argv[1];
  else {
    /* 1920x1080 */
    //filePath = "./test/source_cut_10_frames.h264";
    /* 1920x1080 无B帧*/
    //filePath = "./test/source_cut_10_frames_no_B.h264";
    /* 714x624 帧编码 */
    filePath = "./test/demo_10_frames.h264";
    /* 714x624 场编码(IDR帧解码出来的图片部分绿屏，找到原因了，宏块数量不对，会是slice_skip_flag的问题吗？） */
    //filePath = "./test/demo_10_frames_interlace.h264";
    /* 714x624 场编码(隔行扫描，顶场优先)*/
    //filePath = "./test/demo_10_frames_TFF.h264";
    /* 714x624 帧编码(CAVLC 熵编码模式,即profile=baseline) */
    //filePath = "./test/demo_10_frames_cavlc.h264";
    /* 714x624 场编码(CAVLC 熵编码模式，有一点点绿色宏块)*/
    //filePath = "./test/demo_10_frames_cavlc_and_interlace.h264";
    /* 714x624 帧编码(CABAC 熵编码模式 + 无损编码(lossless=1) + TransformBypassMode + YUV444 ,段错误。。) */
    //filePath = "./test/demo_10_frames_TransformBypassModeFlag.h264";
    //ok
    //filePath = "./test/1280x720_60_fps.h264";
    //用于测试GOP, IDR的处理（ok，其中NAL[3527], GOP[3204], 161个IDR帧
    //filePath = "./test/854x480_60_fps_20_gop.h264";
    //filePath = "./test/854x480_60_fps_20_gop_and_I_Slice.h264";
    /* 全I帧 */
    //filePath = "./test/demo_10_frames_All_I_Slice.h264";
    //filePath = "./tmp.h264";
    //TODO 造一个单帧多Slice的文件，用于测试 "宏块映射到Slice Group" <24-09-16 00:48:27, YangJing>
  }

  /* 1. 打开文件、读取NUL、存储NUL的操作 */
  AnnexBReader reader(filePath);
  int result = reader.open();
  if (result) return -1;

  /* 2. 创建一个GOP用于存放解码后的I、P、B帧序列 */
  GOP *gop = new GOP();
  Frame *frame = gop->m_DecodedPictureBuffer[0];

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
      if (nalu.nal_unit_type > 21)
        std::cout << "Unknown Nalu Type !!!" << std::endl;

      switch (nalu.nal_unit_type) {
      case 1: /* Slice(non-VCL) */
        /* 11-2. 解码普通帧 */
        cout << "Original Slice -> {" << endl;
        flushFrame(gop, frame, false, BMP);
        /* 初始化bit处理器，填充slice的数据 */
        bitStream = new BitStream(rbsp.buf, rbsp.len);
        /* 此处根据SliceHeader可判断A Frame =? A Slice */
        nalu.extractSliceparameters(*bitStream, *gop, *frame);
        frame->decode(*bitStream, gop->m_DecodedPictureBuffer, *gop);
        cout << " }" << endl;
        break;
      case 2: /* DPA(non-VCL) */
        std::cout << "Not Support DPA!" << std::endl;
        break;
      case 3: /* DPB(non-VCL) */
        std::cout << "Not Support DPB!" << std::endl;
        break;
      case 4: /* DPC(non-VCL) */
        std::cout << "Not Support DPC!" << std::endl;
        break;
      case 5: /* IDR Slice(VCL) */
        //gop->flush();
        /* 11-1. 解码立即刷新帧 GOP[0] */
        cout << "IDR Slice -> {" << endl;
        /* 提供给外层程序一定是Frame，即一帧数据，而不是一个Slice，因为如果存在多个Slice为一帧的情况外层处理就很麻烦 */
        flushFrame(gop, frame, true, BMP);
        /* 初始化bit处理器，填充idr的数据 */
        bitStream = new BitStream(rbsp.buf, rbsp.len);
        /* 这里通过解析SliceHeader后可以知道一个Frame到底是几个Slice，通过直接调用frame->decode，在内部对每个Slice->decode() （如果存在多个Slice的情况，可以通过first_mb_in_slice判断，如果每个Slice都为0,则表示每个Slice都是一帧数据，当first_mb_in_slice>0，则表示与前面的一个或多个Slice共同组成一个Frame） */
        nalu.extractIDRparameters(*bitStream, *gop, *frame);
        frame->decode(*bitStream, gop->m_DecodedPictureBuffer, *gop);
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
        gop->max_num_reorder_frames =
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
        break;
      case 10:
        //end_of_seq_rbsp();
        break;
      case 11:
        //end_of_stream_rbsp();
        break;
      case 12:
        //filler_data_rbsp();
        break;
      case 13:
        //seq_parameter_set_extension_rbsp();
        break;
      case 14:
        //prefix_nal_unit_rbsp();
        break;
      case 15:
        //subset_seq_parameter_set_rbsp();
        break;
      case 16:
        //depth_parameter_set_rbsp();
        break;
      case 19:
        //slice_layer_without_partitioning_rbsp();
        break;
      case 20:
        //slice_layer_extension_rbsp();
        break;
      case 21: /* 3D-AVC texture view */
        //slice_layer_extension_rbsp();
        break;
      default:
        cout << "Error nal_unit_type:" << nalu.nal_unit_type << endl;
      }

      /* 已读取完成所有NAL */
      if (result == 0) break;
    } else {
      cerr << "An error occurred on " << __FUNCTION__ << "():" << __LINE__
           << endl;
      break;
    }
    std::cout << std::endl;
  }

  /* 读取完所有Nalu，并送入解码后，则将缓存中所有的Frame读取出来，准备退出 */
  reader.close();
  return 0;
}

/* TODO：GOP flush还有问题，最后两帧如何处理？ */
/* 清空单帧，若当IDR解码完成时，则对整个GOP进行flush */
int flushFrame(GOP *&gop, Frame *&frame, bool isFromIDR,
               OUTPUT_FILE_TYPE output_file_type) {
  if (frame != NULL && frame->m_current_picture_ptr != NULL) {
    Frame *newEmptyPicture = nullptr;
    frame->m_current_picture_ptr
        ->end_decode_the_picture_and_get_a_new_empty_picture(newEmptyPicture);

    //当上一帧完成解码后，且解码帧为IDR帧，则进行GOP -> flush
    if (isFromIDR == false)
      if (frame->m_picture_frame.m_slice->slice_header->IdrPicFlag)
        gop->flush();

    Frame *outPicture = nullptr;
    gop->getOneOutPicture(frame, outPicture);
    if (outPicture != nullptr) {
      //标记为闲置状态，以便后续回收重复利用
      outPicture->m_is_in_use = 0;
      if (output_file_type == BMP) {

      } else if (output_file_type == YUV) {
        outPicture->m_picture_frame.writeYUV("output.yuv");
        if (frame->m_picture_frame.m_slice->slice_header->IdrPicFlag)
          std::cout << "\tffplay -video_size "
                    << outPicture->m_picture_frame.PicWidthInSamplesL << "x"
                    << outPicture->m_picture_frame.PicHeightInSamplesL
                    << " output.yuv" << std::endl;
      }
    }

    frame = newEmptyPicture;
  }
  return 0;
}
