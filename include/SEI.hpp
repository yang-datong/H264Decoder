#ifndef SEI_HPP_TGZDOMB3
#define SEI_HPP_TGZDOMB3

#include "Common.hpp"
#include "PPS.hpp"
#include "RBSP.hpp"

class SEI : public RBSP {
 public:
  uint8_t *_buf = nullptr;
  int _len = 0;

  PPS pps;

  int extractParameters();
  void sei_message(BitStream &bitStream);
  void sei_payload(BitStream &bitStream, long payloadType, long payloadSize);

 public:
};

#endif /* end of include guard: SEI_HPP_TGZDOMB3 */
