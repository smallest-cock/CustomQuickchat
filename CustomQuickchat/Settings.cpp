#include "pch.h"
#include "CustomQuickchat.h"


void CustomQuickchat::RenderSettings() {
	CVarWrapper chatsOnCvar = cvarManager->getCvar("customQuickchat_chatsOn");
	CVarWrapper macroTimeWindowCvar = cvarManager->getCvar("customQuickchat_macroTimeWindow");


	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::TextColored(ImVec4(1, 0, 1, 1), "Plugin made by SSLow");

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::Separator();

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();

	if (!chatsOnCvar) { return; }
	bool chatsOn = chatsOnCvar.getBoolValue();
	if (ImGui::Checkbox("Use custom quickchats", &chatsOn)) {
		cvarManager->executeCommand("customQuickchat_toggle");
	}

	if (chatsOn) {
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		float macroTimeWindow = macroTimeWindowCvar.getFloatValue();
		ImGui::SliderFloat("button sequence time window", &macroTimeWindow, 0.0f, 10.0f, "%.1f seconds");
		macroTimeWindowCvar.setValue(macroTimeWindow);
	}

}