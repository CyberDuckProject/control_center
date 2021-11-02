#ifndef CONTROLS_H
#define CONTROLS_H

#include <imgui.h>
#include <imgui_internal.h>
#include <type_traits>

namespace Controls {
template <typename WidgetFunc, typename... Args>
bool WrapWidget(WidgetFunc &&f, const char *label, Args &&...args) {
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {0.0f, 0.0f});
  bool result = false;
  if (ImGui::BeginTable(label, 2, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 1);
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 2);

    ImGui::TableNextColumn();
    ImGui::Text(label);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1);
    ImGui::PushID(label);
    result = f("", std::forward<Args>(args)...);
    ImGui::PopID();
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
  return result;
}

bool DragFloat(const char *label, float *v, float v_speed = 1.0f,
               float v_min = 0.0f, float v_max = 0.0f,
               const char *format = "%.3f", ImGuiSliderFlags flags = 0) {
  return WrapWidget(
      static_cast<bool (*)(const char *, float *, float, float, float,
                           const char *, ImGuiSliderFlags)>(&ImGui::DragFloat),
      label, v, v_speed, v_min, v_max, format, flags);
}

// from https://github.com/ocornut/imgui/issues/1901 by @zfedoran.
// Licensed under MIT:
// https://github.com/ocornut/imgui/issues/1901#issuecomment-755798471.
bool BufferingBar(const char *label, float value, const ImVec2 &size_arg,
                  const ImU32 &bg_col, const ImU32 &fg_col) {
  using namespace ImGui;
  ImGuiWindow *window = GetCurrentWindow();
  if (window->SkipItems)
    return false;

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);

  ImVec2 pos = window->DC.CursorPos;
  ImVec2 size = size_arg;
  size.x -= style.FramePadding.x * 2;

  const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
  ItemSize(bb, style.FramePadding.y);
  if (!ItemAdd(bb, id))
    return false;

  // Render
  const float circleStart = size.x * 0.7f;
  const float circleEnd = size.x;
  const float circleWidth = circleEnd - circleStart;

  window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y),
                                  bg_col);
  window->DrawList->AddRectFilled(
      bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);

  const float t = g.Time;
  const float r = size.y / 2;
  const float speed = 1.5f;

  const float a = speed * 0;
  const float b = speed * 0.333f;
  const float c = speed * 0.666f;

  const float o1 =
      (circleWidth + r) * (t + a - speed * (int)((t + a) / speed)) / speed;
  const float o2 =
      (circleWidth + r) * (t + b - speed * (int)((t + b) / speed)) / speed;
  const float o3 =
      (circleWidth + r) * (t + c - speed * (int)((t + c) / speed)) / speed;

  window->DrawList->AddCircleFilled(
      ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
  window->DrawList->AddCircleFilled(
      ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
  window->DrawList->AddCircleFilled(
      ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);
}

// from https://github.com/ocornut/imgui/issues/1901 by @zfedoran.
// Licensed under MIT:
// https://github.com/ocornut/imgui/issues/1901#issuecomment-755798471.
bool Spinner(const char *label, float radius, int thickness,
             const ImU32 &color) {
  return WrapWidget(
      [](const char *label, float radius, int thickness, const ImU32 &color) {
        using namespace ImGui;
        ImGuiWindow *window = GetCurrentWindow();
        if (window->SkipItems)
          return false;

        ImGuiContext &g = *GImGui;
        const ImGuiStyle &style = g.Style;
        const ImGuiID id = window->GetID(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size((radius)*2, (radius + style.FramePadding.y) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id))
          return false;

        // Render
        window->DrawList->PathClear();

        int num_segments = 100;
        int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

        const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
        const float a_max =
            IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

        const ImVec2 centre =
            ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

        for (int i = 0; i < num_segments; i++) {
          const float a =
              a_min + ((float)i / (float)num_segments) * (a_max - a_min);
          window->DrawList->PathLineTo(
              ImVec2(centre.x + ImCos(a + g.Time * 8) * radius,
                     centre.y + ImSin(a + g.Time * 8) * radius));
        }

        window->DrawList->PathStroke(color, false, thickness);
      },
      label, radius, thickness, color);
}

// https://github.com/ocornut/imgui/issues/1537
bool ToggleButton(const char *label, bool *v) {
  return WrapWidget(
      [](const char *str_id, bool *v) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        float height = ImGui::GetFrameHeight();
        float width = height * 1.55f;
        float radius = height * 0.50f;

        ImGui::InvisibleButton(str_id, ImVec2(width, height));
        bool changed = false;
        if (ImGui::IsItemClicked()) {
          *v = !*v;
          changed = true;
        }

        float t = *v ? 1.0f : 0.0f;

        ImGuiContext &g = *GImGui;
        float ANIM_SPEED = 0.08f;
        if (g.LastActiveId ==
            g.CurrentWindow->GetID(
                str_id)) // && g.LastActiveIdTimer < ANIM_SPEED)
        {
          float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
          t = *v ? (t_anim) : (1.0f - t_anim);
        }

        ImU32 col_bg;
        if (ImGui::IsItemHovered())
          col_bg = ImGui::GetColorU32(
              ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered),
                     ImGui::GetStyleColorVec4(ImGuiCol_CheckMark), t));
        else
          col_bg = ImGui::GetColorU32(
              ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_FrameBg),
                     ImGui::GetStyleColorVec4(ImGuiCol_CheckMark), t));

        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg,
                                 height * 0.5f);
        draw_list->AddCircleFilled(
            ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius),
            radius - 1.5f, IM_COL32(255, 255, 255, 255));

        return changed;
      },
      label, v);
}
// https://github.com/ocornut/imgui/issues/942
static bool KnobFloat(const char *label, float *p_value, float v_min,
                      float v_max) {
  return WrapWidget(
      [](const char *label, float *p_value, float v_min, float v_max) {
        ImGuiIO &io = ImGui::GetIO();
        ImGuiStyle &style = ImGui::GetStyle();

        float radius_outer = ImGui::GetFrameHeight() / 2;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 center = ImVec2(pos.x + radius_outer, pos.y + radius_outer);
        float line_height = ImGui::GetTextLineHeight();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        float ANGLE_MIN = 3.141592f * 0.75f;
        float ANGLE_MAX = 3.141592f * 2.25f;

        ImGui::InvisibleButton(
            label, ImVec2(radius_outer * 2, radius_outer * 2 + line_height +
                                                style.ItemInnerSpacing.y));
        bool value_changed = false;
        bool is_active = ImGui::IsItemActive();
        bool is_hovered = ImGui::IsItemActive();
        if (is_active && io.MouseDelta.x != 0.0f) {
          float step = (v_max - v_min) / 200.0f;
          *p_value += io.MouseDelta.x * step;
          if (*p_value < v_min)
            *p_value = v_min;
          if (*p_value > v_max)
            *p_value = v_max;
          value_changed = true;
        }

        float t = (*p_value - v_min) / (v_max - v_min);
        float angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * t;
        float angle_cos = cosf(angle), angle_sin = sinf(angle);
        float radius_inner = radius_outer * 0.40f;
        draw_list->AddCircleFilled(center, radius_outer,
                                   ImGui::GetColorU32(ImGuiCol_FrameBg), 16);
        draw_list->AddLine(ImVec2(center.x + angle_cos * radius_inner,
                                  center.y + angle_sin * radius_inner),
                           ImVec2(center.x + angle_cos * (radius_outer - 2),
                                  center.y + angle_sin * (radius_outer - 2)),
                           ImGui::GetColorU32(ImGuiCol_SliderGrabActive), 2.0f);
        draw_list->AddCircleFilled(
            center, radius_inner,
            ImGui::GetColorU32(is_active    ? ImGuiCol_ButtonActive
                               : is_hovered ? ImGuiCol_ButtonHovered
                                            : ImGuiCol_Button),
            16);
        draw_list->AddText(
            ImVec2(pos.x, pos.y + radius_outer * 2 + style.ItemInnerSpacing.y),
            ImGui::GetColorU32(ImGuiCol_Text), label);

        if (is_active || is_hovered) {
          ImGui::SetNextWindowPos(ImVec2(pos.x - style.WindowPadding.x,
                                         pos.y - line_height -
                                             style.ItemInnerSpacing.y -
                                             style.WindowPadding.y));
          ImGui::BeginTooltip();
          ImGui::Text("%.3f", *p_value);
          ImGui::EndTooltip();
        }

        return value_changed;
      },
      label, p_value, v_min, v_max);
}
} // namespace Controls

#endif