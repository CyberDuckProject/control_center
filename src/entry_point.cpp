#include "transmitter.h"
#include <SDL.h>
#include <asio.hpp>
#include <iostream>
#include <thread>
#include <utility>

using asio::ip::tcp;

struct data {
  float left;
  float right;
};

SDL_GameController *controller;

void find_controller() {
  for (int i = 0; i < SDL_NumJoysticks(); ++i) {
    if (SDL_IsGameController(i)) {
      controller = SDL_GameControllerOpen(i);
      if (controller) {
        break;
      }
    }
  }
}

float left_x() {
  return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) /
         32767.0f;
}

float right_x() {
  return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) /
         32767.0f;
}

float left_y() {
  return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) /
         32767.0f;
}

float right_y() {
  return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) /
         32767.0f;
}

asio::io_context ctx;
Transmitter transmitter{ctx};
data cfg{};
void send_speed(...) {
  static asio::steady_timer timer{ctx};

  std::cout << transmitter.remote_address() << '\n';
  transmitter.async_send(cfg, [](...) {});

  timer.expires_from_now(std::chrono::milliseconds{100});
  timer.async_wait(send_speed);
}

int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
  find_controller();

  transmitter.async_connect("localhost", "13", send_speed);

  std::cout << (void *)controller << '\n';

  std::thread worker{[&]() { ctx.run(); }};

 

  {
    asio::io_context::work work{ctx};
    while (true) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_CONTROLLERAXISMOTION) {
          data cfg{};
          cfg.left = std::clamp(-left_y() * 1.0f, 0.0f, 1.0f);
          cfg.right = std::clamp(-right_y() * 1.0f, 0.0f, 1.0f);

          transmitter.async_send(cfg, [](...) {});
          std::cout << "SENT: left: " << cfg.left << ", right: " << cfg.right
                    << '\n';
        }
      }
      std::cin >> cfg.left >> cfg.right;
    }
  }

  worker.join();
}