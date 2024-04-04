#include "NaluSPS.hpp"
#include "BitStream.hpp"
#include <map>

std::map<std::string, int> map_profile_idc{{"PROFILE_BASELINE", 66},
                                           {"PROFILE_MAIN", 77},
                                           {"PROFILE_EXTENDED", 88},
                                           {"PROFILE_HIGH", 100},
                                           {"PROFILE_HIGH_10", 110},
                                           {"PROFILE_HIGH_422", 122},
                                           {"PROFILE_HIGH_444_PREDICTIVE", 244},
                                           {"PROFILE_HIGH_444_INTRA", 44}};

// namespace PROFILE_IDC{
//    enum Type{
//       PROFILE_BASELINE = 66,
//       PROFILE_MAIN = 77,
//       PROFILE_EXTENDED = 88,
//       PROFILE_HIGH = 100,
//       PROFILE_HIGH_10 = 110,
//       PROFILE_HIGH_422 = 122,
//       PROFILE_HIGH_444_PREDICTIVE = 244,
//       PROFILE_HIGH_10_INTRA = 100,
//       PROFILE_HIGH_444_INTRA = 44
//    };
//    static const Type All[] =
//    {PROFILE_BASELINE,PROFILE_MAIN,PROFILE_EXTENDED,PROFILE_HIGH,PROFILE_HIGH_10,PROFILE_HIGH_422,PROFILE_HIGH_444_PREDICTIVE,PROFILE_HIGH_10_INTRA,PROFILE_HIGH_444_INTRA};
// }

// NaluSPS::NaluSPS(const Nalu &nalu) : Nalu(nalu) {}

////Read SPS NALU Segment
// int NaluSPS::DoParse(){
//    uint8_t *buf = rbsp.buf;
//    int bufLen = rbsp.len;
//    BitStream bitStream(buf + 1 , bufLen - 1);
//
//    uint32_t bit_count = 8;
//    uint32_t profile_idc = bitStream.ReadU(bit_count);
//    for (auto it : map_profile_idc) {
//       if(profile_idc == it.second){
//          //TODO 还需要做熵解码
////         printf("\033[31mProfile type:%s\033[0m,value:%u\n",
/// it.first.c_str(),it.second);
//      }
//   }
//
//   return 0;
//}
