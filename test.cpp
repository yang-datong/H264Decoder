#include "decode.hpp"
#include <cstdint>

//解码函数，只进行基本的Exp-Golomb解码，不涉及Slice Data解码
static int decode_poc(uint8_t *const buffer, int buffer_len) {
  static int delta = 0xff;
  int ret = decode(buffer, buffer_len);
  if (ret == 0) {
    //For H.264 -> Original Slice
    int nal_unit_type = decode_get_nal_unit_type();
    if (nal_unit_type == 1) {
      //返回的POC值
      int poc = decode_get_poc_value();
      //返回的POC残差值，相对于前一帧（相邻)POC的差值
      int poc_residual = decode_get_poc_residual_value();
      //std::cout << "poc:" << poc << std::endl;
      //std::cout << "poc_residual:" << poc_residual << std::endl;

      if (delta == 0xff)
        delta = poc_residual;
      else if (delta != poc_residual)
        return -1; //POC残差值不一致，说明发生跳帧

      delta = poc_residual;
    }
  }
  return ret;
}

int main(int argc, char *argv[]) {
  // 内部初始化工作，比如分配一个DPB内存，以及分配一个帧内存
  decode_init();

  int ret;
  while (true) {
    //NOTE: buffer -> 你的二进制码流，必须包含001 0001, len -> 你的二进制码流长度
    //ret = decode_poc(buffer, len); 
    if (ret) return -1;

    /* 已读取完成所有NAL */
    //break;
  }

  // 最后一个解码帧，以及释放缓存的参考帧
  decode_flush();
  // 清理内存、文件资源
  decode_relase();
  return 0;
}
