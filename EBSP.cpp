#include "EBSP.hpp"

EBSP::EBSP() {}

EBSP::~EBSP() {
  if (_buf)
    delete[] _buf;
}
