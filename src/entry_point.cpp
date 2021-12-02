#include "SDL.h"
#include "console.h"
#include "controls.h"
#include "gui_context.h"
#include "input.h"
#include <algorithm>
#include <asio.hpp>
#include <asio/ip/tcp.hpp>
#include <imgui.h>

constexpr float font_size = 23.0f;

asio::awaitable<void> main_loop(asio::io_context &ctx)
{
  static input_state_t input;
  static console_t console{1000};
  try
  {
    constexpr auto wnd_flags =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
    if (ImGui::Begin("CyberDuck Control Center", nullptr, wnd_flags))
    {
      ImGui::SetWindowPos({0, 0});
      ImGui::SetWindowSize(ImGui::GetWindowViewport()->Size);

      const bool changed = update_input_state(input);

      if (ImGui::Button("Add dummy entry"))
      {
        auto numstr = std::to_string(rand());
        console_t::log_entry entry;
        entry.type = static_cast<console_t::log_entry_type>(rand() % 3);
        strcpy(entry.message, numstr.c_str());
        console.add_entry(entry);
      }

      console.display();

      if (changed)
      {
        static asio::ip::tcp::socket socket{ctx};
        socket.async_connect(asio::ip::tcp::endpoint{asio::ip::address_v4{
                                                         *reinterpret_cast<uint32_t *>(&input.duck_address.ip[0])},
                                                     input.duck_address.port},
                             [&](auto &&...)
                             {
                               socket.send(asio::buffer<float>(std::array{input.left_rotor_speed, input.left_rotor_speed}));
                             });
      }
      else
        co_await asio::this_coro::executor;
    }
  }
  catch (const std::exception &e)
  {
    console_t::log_entry entry;
    entry.type = console_t::log_entry_type::warning;
    strcpy(entry.message, e.what());
    console.add_entry(entry);
  }
  ImGui::End();

  co_return;
}

int main(int, char **)
{
  gui_context gui_ctx{font_size};
  asio::io_context io_ctx;
  asio::co_spawn(io_ctx, gui_ctx.run(main_loop, io_ctx), asio::detached);
  io_ctx.run();
  return 0;
}
