#ifndef UI_H
#define UI_H

#include <imgui.h>
#include <optional>
#include <string_view>

#include "address.h"
#include "motor_data.h"

class UI {
public:
  UI(std::optional<Address> &address, Texture &img)
      : current_address{address}, camera_view{img} {}

  template <typename F>
  void update(MotorData &motor_data, F &&reconnect_handler) {
    constexpr auto wnd_flags =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
    if (ImGui::Begin("Control Center", nullptr, wnd_flags)) {
      ImGui::SetWindowPos({0, 0});
      ImGui::SetWindowSize(ImGui::GetWindowViewport()->Size);

      // Update address
      {
        const bool changed = ImGui::InputText("Host", host, bufsz);

        ImGui::BeginDisabled();
        ImGui::InputText("Service", service, bufsz);
        strcpy(service, CYBERDUCK_SERVICE);
        ImGui::EndDisabled();

        if (changed) {
          reconnect_handler(std::string_view{host}, std::string_view{service});
        }

        if (current_address.has_value()) {
          ImGui::Text(("Connected to " +
                       (std::stringstream{} << *current_address).str())
                          .c_str());
        } else {
          ImGui::Text("Connecting...");
        }
      }

      // Update motor data
      {
        ImGui::DragFloat("Left speed", &motor_data.left_speed, 0.05, 0, 1);
        ImGui::DragFloat("Right speed", &motor_data.right_speed, 0.05, 0, 1);
      }

      // Display FPS
      ImGui::Text("GUI Rendering Performance: %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::Text("Video Streaming Performance: %.3f ms/frame (%.1f FPS)",
                  camera_frametime.count() / 1e6f,
                  1e9f / camera_frametime.count());

      ImGui::Image(camera_view.handle(), ImGui::GetContentRegionAvail());
    }
    ImGui::End();
  }

  void set_last_camera_video_frametime(
      const std::chrono::steady_clock::duration frametime) {
    static int cnt = 0;
    constexpr int every = 64;
    if (cnt++ % every == 0) {
      camera_frametime = frametime;
    }
  }

private:
  static constexpr int bufsz = 512;
  char host[bufsz]{};
  char service[bufsz]{};
  std::optional<Address> &current_address;
  Texture &camera_view;
  std::chrono::steady_clock::duration camera_frametime;
};

#endif