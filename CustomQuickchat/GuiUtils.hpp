#pragma once
#include "pch.h"
#include "Components/Components/Utils.hpp"


namespace GUI
{
	void ClickableLink(const char* label, const char* url, const ImVec4& textColor = ImVec4(1, 1, 1, 1), ImVec2 size = ImVec2(0, 0))
	{
		// default size of selectable is just size of label text
		if (size.x == 0 && size.y == 0) {
			size = ImGui::CalcTextSize(label);
		}

		ImGui::PushStyleColor(ImGuiCol_Text, textColor);

		if (ImGui::Selectable(label, false, ImGuiSelectableFlags_None, size)) {
			ShellExecute(NULL, L"open", Format::ToWcharString(url), NULL, NULL, SW_SHOWNORMAL);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			//ImGui::SetTooltip(url);
		}

		ImGui::PopStyleColor();
	}


	void Spacing(int amount = 1)
	{
		for (int i = 0; i < amount; i++) {
			ImGui::Spacing();
		}
	}


	void SettingsHeader(const char* id, const ImVec2& size, bool showBorder = false)
	{
		if (ImGui::BeginChild(id, size, showBorder))
		{
			Spacing(4);

			ImGui::TextColored(ImVec4(1, 0, 1, 1), "Plugin made by SSLow");

			Spacing(3);

			ImGui::Text(pretty_plugin_version);
			ImGui::Separator();
		}
		ImGui::EndChild();
	}


	void SettingsFooter(const char* id, const ImVec2& size, float parentWidth, bool showBorder = false)
	{
		if (ImGui::BeginChild(id, size, showBorder))
		{
			const char* linkText = "Need help? Join the Discord";

			// center the the cursor position horizontally
			ImVec2 textSize = ImGui::CalcTextSize(linkText);
			float horizontalOffset = ((parentWidth - textSize.x) / 2) - 50;	// it seems slightly shifted to the right, so subtract 50 to compensate
			ImGui::SetCursorPosX(horizontalOffset);

			// make link a lil more visible
			ImGui::SetWindowFontScale(1.3);

			ClickableLink(linkText, "https://discord.gg/tHZFsMsvDU", ImVec4(1, 1, 0, 1), textSize);
		}
		ImGui::EndChild();
	}
}