#include <airtree/core/common/BooleanArray.hpp>
#include <iostream>

BooleanArray::BooleanArray(int size) {
  array_ = std::vector<uint64_t>(size);
}

BooleanArray::BooleanArray(std::vector<uint64_t> array) {
  array_ = array;
}

void BooleanArray::set(int index, bool value) {
  if (value) {
    (array_)[index / 64] |= 1ULL << (index % 64);
  } else {
    (array_)[index / 64] &= ~(1ULL << (index % 64));
  }
}

bool BooleanArray::get(int index) const {
  return ((array_)[index / 64] & (1ULL << (index % 64))) != 0;
}

std::vector<uint64_t> BooleanArray::get_array() const {
  return array_;
}

uint64_t BooleanArray::count() const {
  uint64_t count = 0;
  for (uint64_t i : array_) {
    count += __builtin_popcountll(i);
  }
  return count;
}

void BooleanArray::print() const {
  for (int i = array_.size() - 1; i >= 0; i--) {
    for (int j = 63; j >= 0; j--) {
      std::cout << (((array_)[i] & (1ULL << j)) != 0);
    }
  }
  std::cout << std::endl;
}