#include "Nalu.hpp"
#include <AnnexBReader.hpp>
#include <cstdint>
#include <string>

typedef enum _OUTPUT_FILE_TYPE { NON, BMP, YUV } OUTPUT_FILE_TYPE;

extern int decode_init();
extern int decode(uint8_t *buffer, int buffer_len);
extern int decode_flush();
extern void decode_relase();

extern int decode(Nalu &nalu, int &number);
extern int decode_init(AnnexBReader *&reader, std::string &filePath,
                OUTPUT_FILE_TYPE outputFileType);

/* 关闭io输出同步 */
// ios::sync_with_stdio(false);
