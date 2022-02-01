#ifndef TRIPPLEBUFFER_H
#define TRIPPLEBUFFER_H

#include <new>
// __cpp_lib_hardware_interference_size
// std::hardware_destructive_interference_size
// std::hardware_constructive_interference_size

#include <atomic>
// std::atomic

#include <iostream>

template <typename T>
class TrippleBuffer
{
private:
    // Determine hardware interference
#ifdef __cpp_lib_hardware_interference_size
    static constexpr size_t hardware_destructive_interference_size = std::hardware_destructive_interference_size;
    static constexpr size_t hardware_constructive_interference_size = std::hardware_constructive_interference_size;
#else // the standard library doesn't provide this information
    // conservatively assume 128 bytes
    static constexpr size_t hardware_destructive_interference_size = 128;
    static constexpr size_t hardware_constructive_interference_size = 128;
    // for finer grained control one could use Boost.Predef to detect the target architecture and
    // then choose the cacheline size according to a table like this one http://cache-line-sizes.surge.sh
#endif
public:
    // For optimal performance, the three buffers can be stored on their own
    // cache lines. This will prevent false-shering between the producer and
    // the consumer thread
    struct Storage {
         alignas(hardware_destructive_interference_size) T buffer0;
         alignas(hardware_destructive_interference_size) T buffer1;
         alignas(hardware_destructive_interference_size) T buffer2;
    };
private:
    // pointers share a cache line as always a pair of them is changed together
    // TODO: determine wheather keeping on separate cache lines is faster
    T* back;
    std::atomic<T*> middle;
    T* front;
public:
    // producer thread interface
    [[nodiscard]] T& get_back_buffer() noexcept { return *back; };
    [[nodiscard]] const T& get_back_buffer() const noexcept { return *back; };
    void swap_back() noexcept { back = middle.exchange(back, std::memory_order_release); };
public:
    // consumer thread interface
    [[nodiscard]] T& get_front_buffer() noexcept { return *front; };
    [[nodiscard]] const T& get_front_buffer() const noexcept { return *front; };
    void swap_front() noexcept { front = middle.exchange(front, std::memory_order_acquire); };
public:
    TrippleBuffer(Storage& storage) : TrippleBuffer{storage.buffer0, storage.buffer1, storage.buffer2} {};
    TrippleBuffer(T& back, T& middle, T& front) : back{&back}, middle{&middle}, front{&front} {}
};

#endif