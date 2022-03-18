#ifndef UI_H
#define UI_H

#include <imgui.h>
#include <implot.h>
#include <optional>
#include <sstream>
#include <string_view>

#include "address.h"
#include "frame_stats.h"
#include "gui_context.h"
#include "motor_data.h"
#include "sensor_data.h"

class UI {
public:
  UI(std::optional<Address> &address, const Texture &img,
     const SensorData &sensor_data)
      : current_address{address}, camera_view{img}, sensor_data{sensor_data} {}

  template <typename F>
  void update(MotorData &motor_data, F &&reconnect_handler) {
    if (ImGui::Begin("Control Center")) {
      // Update address
      {
        const bool changed = ImGui::InputText("Host", host, bufsz);

        ImGui::BeginDisabled();
        ImGui::InputText("Service", service, bufsz);
        strcpy(service, MOTOR_TCP_PORT);
        ImGui::EndDisabled();

        if (changed) {
          reconnect_handler(std::string_view{host}, std::string_view{service});
        }

        if (current_address.has_value()) {
          std::stringstream temp{};
          temp << *current_address;
          ImGui::Text("%s", ("Connected to " + temp.str()).c_str());
        } else {
          ImGui::Text("Connecting...");
        }
      }

      // Update motor data
      {
        constexpr double min_speed = 0;
        constexpr double max_speed = 1;
        ImGui::DragScalar("Left speed", ImGuiDataType_Double, &motor_data.left_speed, 0.05, &min_speed, &max_speed);
        ImGui::DragScalar("Right speed", ImGuiDataType_Double, &motor_data.right_speed, 0.05, &min_speed, &max_speed);
      }

      // Display FPS
      ImGui::Text("GUI Rendering Performance: %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      const float fps = 1e9f / frame_stats.frametime.count();
      constexpr float max_packet_sz = 65507;
      constexpr float bytes_to_megabits = 1.0f / 1024.0f / 1024.0f * 8.0f;
      ImGui::Text("Video Streaming Performance: %.3f ms/frame (%.1f FPS)",
                  frame_stats.frametime.count() / 1e6f, fps);
      ImGui::Text("Network Performance: %.3f%% packet size used (%.3f Mbps)",
                  frame_stats.framesize / max_packet_sz * 100.0f,
                  fps * frame_stats.framesize * bytes_to_megabits);
    }
    ImGui::End();

    if (ImGui::Begin("Camera View"))
      ImGui::Image(camera_view.handle(), ImGui::GetContentRegionAvail());
    ImGui::End();

    if (ImGui::Begin("Sensor Data")) {
      static ImGuiTableFlags flags =
          ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
          ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
          ImGuiTableFlags_Reorderable;
      if (ImGui::BeginTable("##sensor_readings", 3, flags, ImVec2(-1, 0))) {
        ImGui::TableSetupColumn("Sensor", ImGuiTableColumnFlags_WidthFixed,
                                75.0f);
        ImGui::TableSetupColumn("Reading", ImGuiTableColumnFlags_WidthFixed,
                                75.0f);
        ImGui::TableSetupColumn("Graph");
        ImGui::TableHeadersRow();
        ImPlot::PushColormap(ImPlotColormap_Cool);
        for (int row = 0; row < sensor_data.sensor_count; row++) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::Text("%s", sensor_data.name(row));
          ImGui::TableSetColumnIndex(1);
          if (!sensor_data[row].second_range().empty())
            ImGui::Text("%f", sensor_data[row].second_range().back());
          else
            ImGui::Text("%f", sensor_data[row].first_range().back());
          ImGui::TableSetColumnIndex(2);
          ImGui::PushID(row);

          ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
          if (ImPlot::BeginPlot("##graph", ImVec2(-1, 35),
                                ImPlotFlags_CanvasOnly | ImPlotFlags_NoChild)) {
            ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_NoDecorations,
                              ImPlotAxisFlags_NoDecorations);
            ImPlot::SetupAxesLimits(0, sensor_data[row].size() - 1,
                                    sensor_data.min_val(row),
                                    sensor_data.max_val(row), ImGuiCond_Always);
            ImPlot::PushStyleColor(ImPlotCol_Line,
                                   ImPlot::GetColormapColor(row));
            ImPlot::PlotLine("##graph", sensor_data[row].first_range().data(),
                             sensor_data[row].first_range().size(), 1, 0, 0);
            ImPlot::PlotLine("##graph", sensor_data[row].second_range().data(),
                             sensor_data[row].second_range().size(), 1,
                             sensor_data[row].first_range().size(), 0);
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::PopStyleVar();
            ImPlot::PopStyleColor();
            ImPlot::EndPlot();
          }
          ImPlot::PopStyleVar();
          ImGui::PopID();
        }
        ImPlot::PopColormap();
        ImGui::EndTable();
      }
    }
    ImGui::End();
  }

  void set_frame_stats(const FrameStats &stats) {
    static int cnt = 0;
    constexpr int every = 64;
    if (cnt++ % every == 0) {
      frame_stats = stats;
    }
  }

private:
  static constexpr int bufsz = 512;
  char host[bufsz]{};
  char service[bufsz]{};
  std::optional<Address> &current_address;
  const Texture &camera_view;
  const SensorData &sensor_data;
  FrameStats frame_stats;
};

#endif