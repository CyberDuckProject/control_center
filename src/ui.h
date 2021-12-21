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
        bool changed = false;

        changed |= ImGui::InputText("Host", host, bufsz);
        changed |= ImGui::InputText("Service", service, bufsz);

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
      ImGui::Text("Performance: %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

      ImGui::Image(camera_view.handle(), ImGui::GetContentRegionAvail());
    }
    ImGui::End();
  }

private:
  static constexpr int bufsz = 512;
  char host[bufsz]{};
  char service[bufsz]{};
  std::optional<Address> &current_address;
  Texture &camera_view;
};

#endif