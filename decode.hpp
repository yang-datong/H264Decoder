#include "AnnexBReader.hpp"
#include "BitStream.hpp"
#include "Frame.hpp"
#include "GOP.hpp"
#include "Nalu.hpp"

typedef enum _OUTPUT_FILE_TYPE { NON, BMP, YUV } OUTPUT_FILE_TYPE;

void printfNALBytes(int &number, const Nalu &nalu);
void printfEBSPBytes(int number, const Nalu::EBSP &ebsp);
void printfRBSPBytes(int number, const Nalu::RBSP &rbsp);
int outputFrame(GOP *gop, Frame *frame);
int flushFrame(GOP *gop, Frame *&frame, bool isFromIDR);

//NOTE: 外部导出函数
extern "C" {
int decode(uint8_t *buffer, int buffer_len);
}

int decode_init(string &filePath, OUTPUT_FILE_TYPE outputFileType);
int decode_start();
int decode(Nalu &nalu, int &number);
void decode_relase();
/* 关闭io输出同步 */
// ios::sync_with_stdio(false);
