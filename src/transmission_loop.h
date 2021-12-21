#ifndef TRANSMISSION_LOOP_H
#define TRANSMISSION_LOOP_H

#include <asio.hpp>

#include "motor_data.h"
#include "transmitter.h"

class TransmissionLoop {
private:
  Transmitter &transmitter;
  MotorData &motor_data;
  asio::steady_timer &timer;
  using duration = std::chrono::steady_clock::duration;
  duration interval;

  void send() {}

public:
  TransmissionLoop(Transmitter &transmitter, asio::steady_timer &timer,
                   MotorData &motor_data, const duration &interval)
      : transmitter{transmitter},
        motor_data{motor_data}, timer{timer}, interval{interval} {
    (*this)({});
  }
  TransmissionLoop(const TransmissionLoop &other) = default;

  void operator()(asio::error_code ec) {
    transmitter.async_send(motor_data, [](...) {});

    timer.expires_from_now(interval);
    timer.async_wait(*this);
  }
};

#endif