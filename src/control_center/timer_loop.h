#ifndef TIMER_LOOP_H
#define TIMER_LOOP_H

#include <asio.hpp>

#include "motor_data.h"
#include "transmitter.h"

template <typename F>
class TimerLoop
{
private:
  asio::steady_timer timer;
  using duration = std::chrono::steady_clock::duration;
  duration interval;
  F on_run;

public:
  TimerLoop(asio::steady_timer &&timer, const duration &interval, F &&on_run)
      : timer{std::move(timer)}, interval{interval}, on_run{std::move(on_run)}
  {
    (*this)({});
  }

  void operator()(asio::error_code ec)
  {
    on_run();

    timer.expires_from_now(interval);
    timer.async_wait(std::ref(*this));
  }
};

#endif