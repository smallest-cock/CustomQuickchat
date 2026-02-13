#include "pch.h"
#include "GuiBase.hpp"
#include "PluginConfig.hpp"

std::string SettingsWindowBase::GetPluginName() { return PLUGIN_NAME; }
void        SettingsWindowBase::SetImGuiContext(uintptr_t ctx) { ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext *>(ctx)); }

std::string PluginWindowBase::GetMenuName() { return PLUGIN_NAME_NO_SPACES; }
std::string PluginWindowBase::GetMenuTitle() { return menuTitle_; }
void        PluginWindowBase::SetImGuiContext(uintptr_t ctx) { ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext *>(ctx)); }
bool        PluginWindowBase::ShouldBlockInput() { return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard; }
bool        PluginWindowBase::IsActiveOverlay() { return true; }
void        PluginWindowBase::OnOpen() { isWindowOpen_ = true; }
void        PluginWindowBase::OnClose() { isWindowOpen_ = false; }

void PluginWindowBase::Render() {
	ImGui::SetNextWindowSizeConstraints(ImVec2(800, 600), ImVec2(FLT_MAX, FLT_MAX));
	if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None)) {
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	RenderWindow();

	ImGui::End();

	if (!isWindowOpen_) {
		_globalCvarManager->executeCommand("togglemenu " + GetMenuName());
		_globalCvarManager->executeCommand("writeconfig");
	}
}
