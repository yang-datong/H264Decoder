#ifndef FIELD_HPP_YGAKKXFT
#define FIELD_HPP_YGAKKXFT

#include "Frame.hpp"
#include "Slice.hpp"
#include <vector>

class Field : public Frame {
 public:
  enum Type { TOP, BOTTOM };

 private:
  //Type type;
  std::vector<Slice> slices;

 public:
  //Field(const Type type) : type(type){};
  void encode() override;
  void decode() override;
};

#endif /* end of include guard: FIELD_HPP_YGAKKXFT */
