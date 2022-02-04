#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <span>

template <typename T, size_t Size> class Ringbuffer {
private:
  T data[Size];
  size_t separator{0};

public:
  void push_back(const T &x) noexcept {
    data[separator] = x;
    separator = (separator + 1) % Size;
  }
  const std::span<const T> first_range() const noexcept {
    return {data + separator, data + Size};
  }
  const std::span<const T> second_range() const noexcept {
    return {data, data + separator};
  }
  constexpr size_t size() const noexcept { return Size; }
};

#endif