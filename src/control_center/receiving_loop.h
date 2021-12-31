#ifndef RECEIVING_LOOP_H
#define RECEIVING_LOOP_H

#include <algorithm>
#include <asio.hpp>
#include <iostream>

#include "address.h"
#include "frame_stats.h"
#include "gui_context.h"
#include "texture_update_data.h"
#include "sensor_data.h"

class ReceivingLoop
{
private:
  using udp = asio::ip::udp;

  udp::socket socket;
  udp::endpoint remote;
  TextureUpdateData &update_data;
  SensorData &sensor_data;

  std::chrono::steady_clock::time_point frame_begin_recv;
  FrameStats frame_stats;
  int frame_number = 0;

  struct Header
  {
    uint64_t type;
    int64_t time;
  } header;
  float sensor_reading;

public:
  ReceivingLoop(asio::io_context &ctx, TextureUpdateData &update_data, SensorData &sensor_data)
      : socket{ctx, udp::endpoint{asio::ip::address_v4::any(), VIDEO_PORT}}, update_data{update_data}, sensor_data{sensor_data}
  {
    receive_next();
  }
  void on_received_frame(asio::error_code ec, std::size_t bytes_received)
  {
    update_data.set_recieved_data(bytes_received);
    update_data.data();

    const auto now = std::chrono::steady_clock::now();
    frame_stats.frametime = now - frame_begin_recv;
    if (frame_stats.frametime.count() > 1000000000ll)
      std::cout << "slow frame idx " << frame_number << " (" << frame_stats.frametime.count() << "ns)\n";
    frame_begin_recv = now;
    frame_stats.framesize = bytes_received;

    receive_next();
  }
  void on_received_reading(asio::error_code ec, std::size_t bytes_received)
  {
    sensor_data.add_reading(static_cast<SensorType>(header.type), header.time, sensor_reading);
    receive_next();
  }
  void receive_next()
  {
    socket.async_receive_from(asio::buffer(&header, sizeof(header)), remote, [this](asio::error_code ec, std::size_t bytes_received)
                              {
                                using namespace std::placeholders;
                                if (header.type == 0)
                                {
                                  socket.async_receive_from(std::array{asio::buffer(&frame_number, sizeof(frame_number)), update_data.get_compressed_buffer()}, remote,
                                                            std::bind(&ReceivingLoop::on_received_frame, this, _1, _2));
                                }
                                else
                                {
                                  socket.async_receive_from(asio::buffer(&sensor_reading, sizeof(sensor_reading)), remote,
                                                            std::bind(&ReceivingLoop::on_received_reading, this, _1, _2));
                                }
                              });
  }

  // Stats
  FrameStats last_frame_stats() const { return frame_stats; }
};

#endif
