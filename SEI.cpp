#include "SEI.hpp"
#include "BitStream.hpp"
#include <iostream>
#include <ostream>

void SEI::sei_message(BitStream &bitStream) {
  long payloadType = 0;
  while (bitStream.readUn(8) == 0xFF) {
    int8_t ff_byte = bitStream.readUn(8);
    payloadType += 255;
  }
  uint8_t last_payload_type_byte = bitStream.readUn(8);
  payloadType += last_payload_type_byte;

  long payloadSize = 0;
  while (bitStream.readUn(8) == 0xFF) {
    int8_t ff_byte = bitStream.readUn(8);
    payloadSize += 255;
  }
  uint8_t last_payload_size_byte = bitStream.readUn(8);
  payloadSize += last_payload_size_byte;
  sei_payload(bitStream, payloadType, payloadSize);
  std::cout << "\tpayloadType:" << payloadType << std::endl;
  std::cout << "\tpayloadSize:" << payloadSize << std::endl;
}

void SEI::sei_payload(BitStream &bitStream, long payloadType,
                      long payloadSize) {
  /* TODO YangJing 忽略了一些if elseif, 见T-REC-H.264-202108-I!!PDF-E.pdf
   * 331-332页 <24-04-05 02:19:55> */
  if (!bitStream.byte_aligned()) {
    int8_t bit_equal_to_one = bitStream.readU1();
    while (!bitStream.byte_aligned())
      int8_t bit_equal_to_zero = bitStream.readU1();
  }
}

int SEI::extractParameters() {
  /* 初始化bit处理器，填充sei的数据 */
  BitStream bitStream(_buf, _len);
  do {
    sei_message(bitStream);
  } while (pps.more_rbsp_data());
  return 0;
}
