#ifndef RECEIVING_LOOP_H
#define RECEIVING_LOOP_H

#include <algorithm>
#include <asio.hpp>

#include "address.h"
#include "gui_context.h"
#include "texture_update_data.h"

class ReceivingLoop {
private:
  using udp = asio::ip::udp;

  udp::socket socket;
  udp::endpoint remote;
  TextureUpdateData &update_data;
  std::chrono::steady_clock::time_point frame_begin_recv;
  std::chrono::steady_clock::duration frametime;

public:
  ReceivingLoop(asio::io_context &ctx, TextureUpdateData &update_data)
      : socket{ctx, udp::endpoint{udp::v6(), VIDEO_PORT}}, update_data{
                                                               update_data} {
    (*this)({}, 0);
  }
  void operator()(asio::error_code ec, std::size_t bytes_received) {
    update_data.set_recieved_data(bytes_received);
    update_data.data();

    const auto now = std::chrono::steady_clock::now();
    frametime = now - frame_begin_recv;
    frame_begin_recv = now;

    socket.async_receive_from(update_data.get_compressed_buffer(), remote,
                              std::ref(*this));
  }

  std::chrono::steady_clock::duration last_frametime() const {
    return frametime;
  }
};

#endif
