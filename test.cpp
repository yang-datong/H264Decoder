#include "Nalu.hpp"
#include "decode.hpp"

int main(int argc, char *argv[]) {
  std::string filePath;
  if (argc > 1 && argv[1] != nullptr)
    filePath = argv[1];
  else
    filePath = "./test/demo_10_frames.h264";

  decode_init();

  //NOTE: test nalu
  AnnexBReader reader(filePath);
  if (reader.open()) return -1;

  int ret = 0;
  while (true) {
    /* 3. 一个NUL类，用于存储NUL数据，它与NUL具有同样的数据结构 */
    Nalu nalu;
    /* 3. 循环读取一个个的Nalu */
    int result = reader.readNalu(nalu);
    if (result == 1 || result == 0) {
      ret = decode(nalu.buffer, nalu.len);
      if (ret == 0) {
        //For H.264 -> IDR Slice, Original Slice
        nalu.nal_unit_type = decode_get_nal_unit_type();
        if (nalu.nal_unit_type == 5 || nalu.nal_unit_type == 1) {
          int poc = decode_get_poc_value();
          int poc_residual = decode_get_poc_residual_value();
          std::cout << "poc:" << poc << std::endl;
          std::cout << "poc_residual:" << poc_residual << std::endl;
        }
      }

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
