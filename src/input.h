#ifndef INPUT_H
#define INPUT_H

#include "controls.h"
#include <cstdint>
#include <imgui.h>

struct input_state_t {
  struct ip_address_t {
    uint8_t ip[4];
    uint16_t port;
  } duck_address;
  enum class control_mode_t { automatic = 0, manual = 1 } control_mode;
  float left_rotor_speed, right_rotor_speed;
};

bool update_input_state(input_state_t &state) {
  bool changed = false;
  changed |= Controls::WrapWidget(
      [](const char *label, input_state_t::ip_address_t address) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, {0, 0});
        ImGui::PushItemWidth((ImGui::GetContentRegionAvail().x -
                              (ImGui::CalcTextSize(".").x +
                               ImGui::GetStyle().ItemSpacing.x * 2) *
                                  3 -
                              (ImGui::CalcTextSize(":").x +
                               ImGui::GetStyle().ItemSpacing.x * 2)) /
                             5);
        bool result = false;
        result |=
            ImGui::InputScalar("##0", ImGuiDataType_U8, &address.ip[0], 0, 0);
        ImGui::SameLine();
        ImGui::Text(".");
        ImGui::SameLine();
        result |=
            ImGui::InputScalar("##1", ImGuiDataType_U8, &address.ip[1], 0, 0);
        ImGui::SameLine();
        ImGui::Text(".");
        ImGui::SameLine();
        result |=
            ImGui::InputScalar("##2", ImGuiDataType_U8, &address.ip[2], 0, 0);
        ImGui::SameLine();
        ImGui::Text(".");
        ImGui::SameLine();
        result |=
            ImGui::InputScalar("##3", ImGuiDataType_U8, &address.ip[3], 0, 0);
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        result |=
            ImGui::InputScalar("##4", ImGuiDataType_U16, &address.port, 0, 0);
        ImGui::SameLine();
        ImGui::PopItemWidth();
        ImGui::PopStyleVar(1);

        return result;
      },
      "IP Address", state.duck_address);

  Controls::Spinner(
      "Connecting...", ImGui::GetFrameHeight() / 2, ImGui::GetFrameHeight() / 4,
      ImGui::GetColorU32(ImGuiCol_CheckMark)); // todo: actually connect

  changed |= Controls::ToggleButton("Enable manual controls",
                                    (bool *)&state.control_mode);
  ImGui::BeginDisabled(state.control_mode ==
                       input_state_t::control_mode_t::automatic);
  ImGui::Indent(ImGui::GetFontSize());
  {
    changed |= Controls::DragFloat("Left rotor speed", &state.left_rotor_speed,
                                   0.005f, 0.0f, 1.0f);
    changed |= Controls::DragFloat(
        "Right rotor speed", &state.right_rotor_speed, 0.005f, 0.0f, 1.0f);
  }
  ImGui::EndDisabled();
  ImGui::Unindent(ImGui::GetFontSize());

  return changed;
}

#endif