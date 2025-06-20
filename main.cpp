#include "Nalu.hpp"
#include "decode.hpp"

int main(int argc, char *argv[]) {
  std::string filePath;
  if (argc > 1 && argv[1] != nullptr)
    filePath = argv[1];
  else {
    /* 1920x1080 */
    //filePath = "./test/source_cut_10_frames.h264";
    /* 1920x1080 无B帧*/
    //filePath = "./test/source_cut_10_frames_no_B.h264";
    /* 714x624 帧编码 */
    filePath = "./test/demo_10_frames.h264";
    /* 714x624 帧编码-baseline */
    //filePath = "./test/demo_10_frames_baseline.h264";
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

    // ffmpeg -i ./test/demo_10_frames.h264 -c:v libx264 -x264-params direct=temporal -c:a copy demo_10_frames_temporal_direct.h264
    //filePath = "./test/demo_10_frames_temporal_direct.h264";
  }

  int result;
  OUTPUT_FILE_TYPE g_OutputFileType = YUV;
  //OUTPUT_FILE_TYPE g_OutputFileType = BMP;
  AnnexBReader *reader = nullptr;
  RET(decode_init(reader, filePath, g_OutputFileType));
  int number = 0;
  /* 这里只对文件进行解码，所以只有AnnesB格式 */
  while (true) {
    /* 3. 一个NUL类，用于存储NUL数据，它与NUL具有同样的数据结构 */
    Nalu nalu;
    /* 3. 循环读取一个个的Nalu */
    result = reader->readNalu(nalu);
    if (result == 1 || result == 0) {
      decode(nalu, number);

      /* 已读取完成所有NAL */
      if (result == 0) break;
    } else {
      RET(-1);
      break;
    }
    //cout << endl;
  }
  decode_flush();
  decode_relase();
  return 0;
}
