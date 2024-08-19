#include "AnnexBReader.hpp"
#include "Nalu.hpp"
#include <cstdint>
#include <cstring>
#include <ios>

AnnexBReader::~AnnexBReader() { close(); }

int AnnexBReader::open() {
  _file.open(_filePath, std::ios::binary | std::ios::in);
  if (!_file.is_open()) {
    std::cerr << "\033[31m __file_type is not open!\033[0m" << std::endl;
    return -1;
  }
  return 0;
}

void AnnexBReader::close() {
  if (_file) _file.close();
}

bool AnnexBReader::checkStartLen(int &startCodeLen, uint8_t *buffer,
                                 int bufferLen) {
  startCodeLen = 0;
  if (bufferLen < 3) return false;
  if (bufferLen == 3 && buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 1) {
    startCodeLen = 3;
  } else if (bufferLen > 3 && buffer[0] == 0 && buffer[1] == 0 &&
             buffer[2] == 1) {
    startCodeLen = 3;
  } else if (bufferLen > 3 && buffer[0] == 0 && buffer[1] == 0 &&
             buffer[2] == 0 && buffer[3] == 1) {
    startCodeLen = 4;
  }
  return startCodeLen != 0;
}

bool AnnexBReader::findStartcode(int &startcodeLen, uint8_t *buffer,
                                 int bufferLen) {
  startcodeLen = 0;
  if (bufferLen < 3)
    return false;
  else if (bufferLen >= 3 && buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 1)
    startcodeLen = 3;
  else if (bufferLen > 3 && buffer[0] == 0 && buffer[1] == 0 &&
           buffer[2] == 0 && buffer[3] == 1)
    startcodeLen = 4;
  return startcodeLen == 0 ? false : true;
}

int AnnexBReader::readNalu(Nalu &nalu) {
  while (1) {
    if (readData()) return -1;
    /* 读取完一点数据后就开始检查startcode：
     * 1. startcode的长度不稳定（或3或4 bytes)
     * 2. 文件首个字节即startcode
     * 3. 读到下一个startcode为止，算一个Nul
     * */

    int startcodeLen;
    if (!findStartcode(startcodeLen, _buffer, _bufferLen)) {
      // 第一次读肯定是能够读取到的，如果没读取到说明不是h264文件
      std::cerr << "\033[31m findStartcode() \033[0m" << std::endl;
      return -1;
    }
    nalu.startCodeLenth = startcodeLen;

    for (int i = startcodeLen; i < _bufferLen; i++) {
      // 第二次就可以跳过前3-4个再开始找下一个startcode
      if (findStartcode(startcodeLen, _buffer + i, _bufferLen - i)) {
        // 如果在1024个字节内找到了第二个startcode，那么就可以封装一个NUl了
        uint8_t *nulBuffer = new uint8_t[i];
        memcpy(nulBuffer, _buffer, i);
        nalu.setBuffer(nulBuffer, i);
        uint8_t *remainBuffer = new uint8_t[_bufferLen - i];
        memcpy(remainBuffer, _buffer + i, _bufferLen - i);
        delete[] _buffer;
        _buffer = remainBuffer;
        _bufferLen = _bufferLen - i;
        // 将startcode1 ------ startcode2
        // 直接的数据放到nalu中（不包括startcode2）
        return 1;
      }
      // 如果没有在1024个字节内找到第二个startcode，说明还要继续读取buffer，那么第二次就是2048个字节，再进行找第二个stratcode，以此类推
    }

    if (_isRead) {
      // 读到了文件底部后，这里是没有以第二个startcode作为结尾的，所以直接将最后的buffer放进nalu中
      uint8_t *nulBuffer = new uint8_t[_bufferLen];
      memcpy(nulBuffer, _buffer, _bufferLen);
      nalu.setBuffer(nulBuffer, _bufferLen);
      // 完成所有nul读取，清空Reader器的buffer
      delete[] _buffer;
      _bufferLen = 0;
      break;
    }
  }
  return 0;
}

int AnnexBReader::readData() {
  if (!_file.is_open()) return -1;

  int size = 1024;
  uint8_t *fileBuffer = new uint8_t[size];
  _file.read(reinterpret_cast<char *>(fileBuffer), size);
  if (_file.gcount() != 0) {
    uint8_t *tmpBuffer = new uint8_t[_bufferLen + size];
    memcpy(tmpBuffer, _buffer, _bufferLen);
    // Nul类成员的_Buffer
    memcpy(tmpBuffer + _bufferLen, fileBuffer, size);
    // 刚读的1024 buf 追加到Nul类成员的_Buffer中
    delete[] _buffer;
    // 释放上次保存的buffer
    _buffer = tmpBuffer;
    // 存放一个新的、更大的buffer
    _bufferLen += _file.gcount();
    // 根据实际读取到的数据进行更新buffer长度（到了最后一次读取时一般是不足1024的）
  } else
    _isRead = true;
  delete[] fileBuffer;
  return 0;
}
