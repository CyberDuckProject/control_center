#include <asio.hpp>
#include <functional>
#include <iostream>

#include "gui_context.h"
#include "motor_data.h"
#include "transmission_loop.h"
#include "transmitter.h"
#include "ui.h"

using tcp = asio::ip::tcp;

int x;

int main(int, char **) {
  asio::io_context ctx;
  auto worker = std::thread([&ctx] { ctx.run(); });

  {
    asio::io_context::work work{ctx};

    Transmitter transmitter{ctx}; // TODO: refactor into server that also recvs
    MotorData motor_data{};
    asio::steady_timer timer{ctx};
    TransmissionLoop transmission_loop{transmitter, timer, motor_data,
                                       std::chrono::milliseconds{100}};

    // This has to be static and the reason why is rather interesting.
    // This address is changed inside the transmitter.async_connect callback.
    // That callback runs on a different thread. For some reason
    // the address space on that thread is different from that of the main
    // thread, despite the fact that it should be the same.
    // So, if the address was not static, it couldn't get modified in any
    // way from the worker thread. I don't know what causes that nor how to
    // fix it. TODO: figure this out.
    static std::optional<Address> address{};

    // Reciever reciever{ctx};
    GUIContext gui_ctx{23.0f};

    int w = 200, h = 100; //------------------------------------------------TODO
    auto img = gui_ctx.create_texture(w, h); //-----------------------------TODO

    UI ui{address, img};
    // RecievingLoop recieving_loop{reciever, ui};

    Controller controller;

    while (!gui_ctx.should_close()) {
      gui_ctx.pollEvents([&motor_data, &controller](const SDL_Event &event) {
        if (event.type == SDL_CONTROLLERAXISMOTION) {
          motor_data.left_speed = std::clamp(-controller.left_y(), 0.0f, 1.0f);
          motor_data.right_speed =
              std::clamp(-controller.right_y(), 0.0f, 1.0f);
        }
      });

      [&]() { //------------------------------------------------------------TODO
        auto pixels = std::make_unique<Pixel[]>(w * h);
        static int o = 1;
        for (int i = 0; i < w * h; ++i) {
          if (i % o == 0) {
            pixels[i].r = 255;
            pixels[i].g = 255;
            pixels[i].b = 255;
            pixels[i].a = 128;
          } else {
            pixels[i].r = 0;
            pixels[i].g = 0;
            pixels[i].b = 0;
            pixels[i].a = 128;
          }
        }
        ++o;
        gui_ctx.update_texture(img, pixels.get());
      }();

      gui_ctx.render([&ui, &motor_data, &transmitter] {
        ui.update(motor_data, [&transmitter, &ui](std::string_view host,
                                                  std::string_view service) {
          transmitter.async_connect(
              host, service,
              [](asio::error_code ec, const tcp::endpoint &endpoint) {
                if (!ec)
                  address = Address{endpoint};
                else
                  address = std::nullopt;
              });
        });
      });
    }
  }

  ctx.stop(); // TODO: this shouldn't be neccesary (~work should suffice)
  worker.join();

  return 0;
}