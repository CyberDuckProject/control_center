#ifndef RECEIVING_LOOP_H
#define RECEIVING_LOOP_H

#include <algorithm>
#include <asio.hpp>

#include "address.h"
#include "frame_stats.h"
#include "gui_context.h"
#include "texture_update_data.h"

class ReceivingLoop {
private:
  using udp = asio::ip::udp;

  udp::socket socket;
  udp::endpoint remote;
  TextureUpdateData &update_data;

  std::chrono::steady_clock::time_point frame_begin_recv;
  FrameStats frame_stats;

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
    frame_stats.frametime = now - frame_begin_recv;
    frame_begin_recv = now;
    frame_stats.framesize = bytes_received;

    socket.async_receive_from(update_data.get_compressed_buffer(), remote,
                              std::ref(*this));
  }

  // Stats
  FrameStats last_frame_stats() const { return frame_stats; }
};

#endif
