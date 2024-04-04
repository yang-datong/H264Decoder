#include "./RBSP.hpp"

RBSP::RBSP() {}

RBSP::~RBSP() {
  if (_buf)
    delete[] _buf;
}
