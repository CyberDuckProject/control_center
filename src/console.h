#ifndef CONSOLE_H
#define CONSOLE_H

#include "controls.h"
#include <cstddef>
#include <imgui.h>
#include <memory>
#include <regex>

class console_t {
public:
  static constexpr size_t max_log_entry_length = 1024;
  enum class log_entry_type { info, warning, error };
  struct log_entry {
    log_entry_type type;
    char message[max_log_entry_length];
  };

private:
  size_t history_size;
  size_t end_entry;
  std::unique_ptr<log_entry[]> log;
  bool show_info = true, show_warning = true, show_error = true;
  bool use_filter = false;
  std::regex search_filter;

public:
  console_t(size_t history_size)
      : history_size{history_size}, end_entry{0},
        log{std::make_unique<log_entry[]>(history_size)} {}

  void add_entry(const log_entry &entry) {
    log[end_entry] = entry;
    ++end_entry;
    end_entry %= history_size;
  }

  void display() {
    show_controls();

    if (ImGui::BeginTable("Console", 1,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                              ImGuiTableFlags_BordersOuter)) {

      for (int i = 0; i < history_size; ++i) {
        const int idx = (end_entry + i) % history_size;
        if (log[idx].message[0] != '\0') {
          switch (log[idx].type) {
          case console_t::log_entry_type::info:
            if (!show_info)
              continue;
            break;
          case console_t::log_entry_type::warning:
            if (!show_warning)
              continue;
            break;
          case console_t::log_entry_type::error:
            if (!show_error)
              continue;
            break;
          }

          if (use_filter) {
            std::cmatch m;
            std::regex_search(log[idx].message, m, search_filter);
            if (m.empty())
              continue;
          }

          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);

          ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_Text);
          if (log[idx].type == console_t::log_entry_type::error) {
            col = {0.6f, 0.09f, 0.09f, 1.0f};
          } else if (log[idx].type == console_t::log_entry_type::warning) {
            col = {0.6f, 0.51f, 0.09f, 1.0f};
          }
          ImGui::PushStyleColor(ImGuiCol_Text, col);
          ImGui::TextUnformatted(log[idx].message);
          ImGui::PopStyleColor();
        }
      }
      ImGui::EndTable();
    }
  }

private:
  void show_controls() {
    if (ImGui::BeginTable("Console Controls", 5, ImGuiTableFlags_Borders)) {
      ImGui::TableNextColumn();
      if (ImGui::Button("Clear")) {
        for (int i = 0; i < history_size; ++i) {
          log[i].message[0] = '\0';
        }
      }

      ImGui::TableNextColumn();
      constexpr size_t buf_sz = 512;
      static char buf[buf_sz];
      ImGui::InputTextWithHint("##Search", "Regex search...", buf, buf_sz);
      try {
        search_filter = buf;
        use_filter = (buf[0] != '\0');
      } catch (const std::regex_error &e) {
        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          ImGui::SetTooltip(e.what());
          ImGui::EndTooltip();
        }
        use_filter = false;
      }

      ImGui::TableNextColumn();
      Controls::ToggleButton("Info", &show_info);
      ImGui::TableNextColumn();
      Controls::ToggleButton("Warning", &show_warning);
      ImGui::TableNextColumn();
      Controls::ToggleButton("Error", &show_error);

      ImGui::EndTable();
    }
  }
};

#endif