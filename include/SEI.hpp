#ifndef SEI_HPP_TGZDOMB3
#define SEI_HPP_TGZDOMB3

#include "Common.hpp"
#include "SPS.hpp"
#include <string>

class SEI {
 public:
  uint8_t *_buf = nullptr;
  int _len = 0;

 private:
  long payloadType = 0;
  long payloadSize = 0;
  SPS *sps;

 public:
  int extractParameters(SPS &sps);

 private:
  BitStream *_bs = nullptr;
  std::string _text;

  uint32_t *initial_cpb_removal_delay = nullptr;
  uint32_t *initial_cpb_removal_delay_offset = nullptr;

  void sei_message();
  void sei_payload();
  void buffering_period();
  void reserved_sei_message();
};

#endif /* end of include guard: SEI_HPP_TGZDOMB3 */
