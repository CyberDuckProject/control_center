#include <asio.hpp>
#include <functional>
#include <iostream>

#include "gui_context.h"
#include "motor_data.h"
#include "texture_update_data.h"
#include "receiving_loop.h"
#include "timer_loop.h"
#include "transmitter.h"
#include "sensor_data.h"
#include "ui.h"

using tcp = asio::ip::tcp;
using udp = asio::ip::udp;

int main(int, char **)
{
  asio::io_context ctx;

  Transmitter transmitter{ctx};
  MotorData motor_data{};
  TimerLoop transmission_loop{asio::steady_timer{ctx}, std::chrono::milliseconds{100}, [&transmitter, &motor_data]()
                              {
                                transmitter.async_send(motor_data, [](...) {});
                              }};

  // This has to be static and the reason why is rather interesting.
  // This address is changed inside the transmitter.async_connect callback.
  // That callback runs on a different thread. For some reason
  // the address space on that thread is different from that of the main
  // thread, despite the fact that it should be the same.
  // So, if the address was not static, it couldn't get modified in any
  // way from the worker thread. I don't know what causes that nor how to
  // fix it. TODO: figure this out.
  static std::optional<Address> address{};

  GUIContext gui_ctx{23.0f};

  SensorData sensor_data;
  auto camera_view = gui_ctx.create_texture(1385, 1080);
  UI ui{address, camera_view, sensor_data};

  TextureUpdateData update_data{camera_view};
  int current_frame_number{};
  ReceivingLoop video_receiving_loop{
      udp::socket{ctx, udp::endpoint{asio::ip::address_v4::any(), VIDEO_UDP_PORT}},
      [&current_frame_number, &update_data]()
      {
        return std::array{asio::buffer(&current_frame_number, sizeof(current_frame_number)), update_data.begin_receiving_data()};
      },
      [&update_data](asio::error_code ec, std::size_t bytes_received, const udp::endpoint & /*sender*/) {
        update_data.end_receiving_data(bytes_received);
      }};

  Controller controller;

  std::vector<std::thread> workers;
  {
    asio::io_context::work work{ctx};
    const int worker_count = 1;
    for (int i = 0; i < worker_count; ++i)
    {
      workers.emplace_back([&ctx]
                           {
                             std::cout << "began!\n";
                             ctx.run();
                             std::cout << "done!\n";
                           });
    }

    while (!gui_ctx.should_close())
    {
      gui_ctx.pollEvents([&motor_data, &controller](const SDL_Event &event)
                         {
                           if (event.type == SDL_CONTROLLERAXISMOTION)
                           {
                             motor_data.left_speed = std::clamp(-controller.left_y(), 0.0f, 1.0f);
                             motor_data.right_speed =
                                 std::clamp(-controller.right_y(), 0.0f, 1.0f);
                           }
                         });

      gui_ctx.update_texture(camera_view, update_data.data());

      //ui.set_frame_stats(receiving_loop.last_frame_stats()); TODO: reimplement frame stats

      gui_ctx.render([&ui, &motor_data, &transmitter]
                     {
                       ui.update(motor_data, [&transmitter, &ui](std::string_view host,
                                                                 std::string_view service)
                                 {
                                   address = std::nullopt;
                                   transmitter.async_connect(
                                       host, service,
                                       [](asio::error_code ec, const tcp::endpoint &endpoint)
                                       {
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
  for (auto &worker : workers)
  {
    worker.join();
  }

  return 0;
}