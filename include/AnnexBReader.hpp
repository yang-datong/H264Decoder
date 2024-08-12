#include "Nalu.hpp"
#include <cstdint>
#include <fstream>
#include <stdlib.h>
#include <string>

class AnnexBReader {
  std::string _filePath;
  std::ifstream _file;

  uint8_t *_buffer = nullptr;
  int _bufferLen = 0;

  /* 是否读取文件数据完成 */
  bool _isRead = false;

  /* 用于从文件中读取二进制数据即buffer */
  int readData();

  bool checkStartLen(int &startCodeLen, uint8_t *buffer, int bufferLen);

  bool findStartcode(int &startcodeLen, uint8_t *buffer, int bufferLen);

 public:
  AnnexBReader(std::string &filePath) : _filePath(filePath) {}
  ~AnnexBReader();

  /* 只是打开文件 */
  int open();

  /* 用于读取264数据并分析数据最终截取一个Nalu的数据到对象中 */
  int readNalu(Nalu &nalu);

  /* 关闭文件 */
  void close();
};
