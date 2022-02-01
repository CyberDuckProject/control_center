#ifndef UI_H
#define UI_H

#include <imgui.h>
#include <implot.h>
#include <optional>
#include <sstream>
#include <string_view>

#include "address.h"
#include "frame_stats.h"
#include "motor_data.h"
#include "sensor_data.h"

class UI
{
public:
  UI(std::optional<Address> &address, const Texture &img, const SensorData &sensor_data)
      : current_address{address}, camera_view{img}, sensor_data{sensor_data} {}

  template <typename F>
  void update(MotorData &motor_data, F &&reconnect_handler)
  {
    if (ImGui::Begin("Control Center"))
    {
      // Update address
      {
        const bool changed = ImGui::InputText("Host", host, bufsz);

        ImGui::BeginDisabled();
        ImGui::InputText("Service", service, bufsz);
        strcpy(service, MOTOR_TCP_PORT);
        ImGui::EndDisabled();

        if (changed)
        {
          reconnect_handler(std::string_view{host}, std::string_view{service});
        }

        if (current_address.has_value())
        {
          std::stringstream temp{};
          temp << *current_address;
          ImGui::Text("%s", ("Connected to " + temp.str()).c_str());
        }
        else
        {
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

    if (ImGui::Begin("Sensor Data"))
    {
      static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                                     ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;
      static bool anim = true;
      static int offset = 0;
      ImGui::BulletText("Plots can be used inside of ImGui tables as another means of creating subplots.");
      ImGui::Checkbox("Animate", &anim);
      if (anim)
        offset = (offset + 1) % 100;
      if (ImGui::BeginTable("##table", 3, flags, ImVec2(-1, 0)))
      {
        ImGui::TableSetupColumn("Electrode", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Voltage", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("EMG Signal");
        ImGui::TableHeadersRow();
        ImPlot::PushColormap(ImPlotColormap_Cool);
        for (int row = 0; row < 10; row++)
        {
          ImGui::TableNextRow();
          static float data[100];
          srand(row);
          for (int i = 0; i < 100; ++i)
            data[i] = rand() / RAND_MAX * 10.0f;
          ImGui::TableSetColumnIndex(0);
          ImGui::Text("EMG %d", row);
          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%.3f V", data[offset]);
          ImGui::TableSetColumnIndex(2);
          ImGui::PushID(row);
          constexpr auto sparkline = [](const char *id, const float *values, int count, float min_v, float max_v, int offset, const ImVec4 &col, const ImVec2 &size)
          {
            ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
            if (ImPlot::BeginPlot(id, size, ImPlotFlags_CanvasOnly | ImPlotFlags_NoChild))
            {
              ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
              ImPlot::SetupAxesLimits(0, count - 1, min_v, max_v, ImGuiCond_Always);
              ImPlot::PushStyleColor(ImPlotCol_Line, col);
              ImPlot::PlotLine(id, values, count, 1, 0, offset);
              ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
              ImPlot::PlotShaded(id, values, count, 0, 1, 0, offset);
              ImPlot::PopStyleVar();
              ImPlot::PopStyleColor();
              ImPlot::EndPlot();
            }
            ImPlot::PopStyleVar();
          };
          sparkline("##spark", data, 100, 0, 11.0f, offset, ImPlot::GetColormapColor(row), ImVec2(-1, 35));
          ImGui::PopID();
        }
        ImPlot::PopColormap();
        ImGui::EndTable();
      }
    }
    ImGui::End();
  }

  void set_frame_stats(const FrameStats &stats)
  {
    static int cnt = 0;
    constexpr int every = 64;
    if (cnt++ % every == 0)
    {
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