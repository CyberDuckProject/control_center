#include <asio.hpp>
#include <imgui.h>
#include <algorithm>
#include "SDL.h"
#include "gui_context.h"
#include "controls.h"

constexpr float font_size = 23.0f;

asio::awaitable<void> main_loop(asio::io_context& ctx)
{
	constexpr auto wnd_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	if (ImGui::Begin("CyberDuck Control Center", nullptr, wnd_flags))
	{
		ImGui::SetWindowPos({ 0,0 });
		ImGui::SetWindowSize(ImGui::GetWindowViewport()->Size);

		static int duck_address[5];
		Controls::WrapWidget(
		[](const char* label, int address[5]){
			ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, {0, 0});
			ImGui::PushItemWidth((ImGui::GetContentRegionAvail().x - (ImGui::CalcTextSize(".").x + ImGui::GetStyle().ItemSpacing.x * 2) * 3 - (ImGui::CalcTextSize(":").x + ImGui::GetStyle().ItemSpacing.x * 2)) / 5);
			bool result = false;
			result |= ImGui::InputInt("##0", &address[0], 0, 0); ImGui::SameLine(); ImGui::Text("."); ImGui::SameLine();
			result |= ImGui::InputInt("##1", &address[1], 0, 0); ImGui::SameLine(); ImGui::Text("."); ImGui::SameLine();
			result |= ImGui::InputInt("##2", &address[2], 0, 0); ImGui::SameLine(); ImGui::Text("."); ImGui::SameLine();
			result |= ImGui::InputInt("##3", &address[3], 0, 0); ImGui::SameLine(); ImGui::Text(":"); ImGui::SameLine();
			result |= ImGui::InputInt("##4", &address[4], 0, 0); ImGui::SameLine();
			ImGui::PopItemWidth();
			ImGui::PopStyleVar(1);

			for (int i = 0; i < 4; ++i)
				address[i] = std::clamp(address[i], 0, 255);
			address[4] = std::clamp(address[4], 0, 65535);

			return result;
		},
		"IP Address", duck_address);

		Controls::Spinner("Connecting...", font_size / 2, font_size / 4, ImGui::GetColorU32(ImGuiCol_CheckMark)); // todo: actually connect
		
		static bool manual_mode = false;
		Controls::ToggleButton("Enable manual controls", &manual_mode);
		bool changed = false;
		ImGui::BeginDisabled(!manual_mode); ImGui::Indent(ImGui::GetFontSize());
		{
			static float vl, vr;
			changed |= Controls::DragFloat("Left rotor speed", &vl, 0.005f, 0.0f, 1.0f);
			changed |= Controls::DragFloat("Right rotor speed", &vr, 0.005f, 0.0f, 1.0f);
		}
		ImGui::EndDisabled(); ImGui::Unindent(ImGui::GetFontSize());

		if (changed)
			co_await asio::this_coro::executor; // TODO: communicate with duck
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
