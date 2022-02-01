#ifndef FRAME_STATS_H
#define FRAME_STATS_H

#include <chrono>

struct FrameStats
{
  std::chrono::steady_clock::duration frametime;
  size_t framesize;
};

#endif