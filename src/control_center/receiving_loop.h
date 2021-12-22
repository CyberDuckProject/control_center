#ifndef RECEIVING_LOOP_H
#define RECEIVING_LOOP_H

#include <algorithm>
#include <asio.hpp>

#include "address.h"
#include "gui_context.h"

struct TextureUpdateData {
  TextureUpdateData(const Texture &tex)
      : has_update{true}, width{tex.width()}, height{tex.height()},
        pixels{std::make_unique<Pixel[]>(width * height)} {
    for (int i = 0; i < width * height; ++i) {
      pixels[i] = {255, 0, 255, 255};
    }
  }
  bool has_update;
  const size_t width, height;
  std::unique_ptr<Pixel[]> pixels;
};

class ReceivingLoop {
private:
  using udp = asio::ip::udp;
  struct ReceivedPixel {
    char r, g, b;
  };

  udp::socket socket;
  std::unique_ptr<ReceivedPixel[]> buffer;
  size_t buffer_size_bytes;
  udp::endpoint remote;
  TextureUpdateData &update_data;
  std::chrono::steady_clock::time_point frame_begin_recv;
  std::chrono::steady_clock::duration frametime;
  int row = 0;
  int rows_rcvd = 0;

public:
  ReceivingLoop(asio::io_context &ctx, TextureUpdateData &update_data)
      : socket{ctx, udp::endpoint{udp::v6(), VIDEO_PORT}},
        buffer{std::make_unique<ReceivedPixel[]>(update_data.width *
                                                 update_data.height)},
        buffer_size_bytes{update_data.width * update_data.height *
                          sizeof(ReceivedPixel)},
        update_data{update_data} {
    (*this)({}, sizeof(row));
  }
  void operator()(asio::error_code ec, std::size_t bytes_received) {
    const size_t iters = (bytes_received - sizeof(int)) / sizeof(ReceivedPixel);
    for (int i = 0; i < iters; ++i) {
      update_data.pixels[i + row * update_data.width].r = buffer[i].r;
      update_data.pixels[i + row * update_data.width].g = buffer[i].g;
      update_data.pixels[i + row * update_data.width].b = buffer[i].b;
    }
    update_data.has_update = true;

    ++rows_rcvd;
    if (rows_rcvd == update_data.height) {
      const auto now = std::chrono::steady_clock::now();
      frametime = now - frame_begin_recv;

      frame_begin_recv = now;
      rows_rcvd = 0;
    }

    socket.async_receive_from(
        std::array{asio::buffer(&row, sizeof(row)),
                   asio::buffer(buffer.get(), buffer_size_bytes)},
        remote, std::ref(*this));
  }

  std::chrono::steady_clock::duration last_frametime() const {
    return frametime;
  }
};

#endif
