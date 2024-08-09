#include "pch.h"
#include "CustomQuickchat.h"
#include "GuiTools.hpp"


void CustomQuickchat::RenderSettings()
{
	auto enabled_cvar =							cvarManager->getCvar(CvarNames::enabled);
	auto enableSTTNotifications_cvar =			cvarManager->getCvar(CvarNames::enableSTTNotifications);
	auto sequenceTimeWindow_cvar =				cvarManager->getCvar(CvarNames::sequenceTimeWindow);
	auto speechProcessingTimeout_cvar =			cvarManager->getCvar(CvarNames::speechProcessingTimeout);
	auto beginSpeechTimeout_cvar =				cvarManager->getCvar(CvarNames::beginSpeechTimeout);
	auto notificationDuration_cvar =			cvarManager->getCvar(CvarNames::notificationDuration);
	auto autoDetectInterpreterPath_cvar =		cvarManager->getCvar(CvarNames::autoDetectInterpreterPath);
	auto pythonInterpreterPath_cvar =			cvarManager->getCvar(CvarNames::pythonInterpreterPath);
	auto overrideDefaultQuickchats_cvar =		cvarManager->getCvar(CvarNames::overrideDefaultQuickchats);
	auto blockDefaultQuickchats_cvar =			cvarManager->getCvar(CvarNames::blockDefaultQuickchats);


	// ---------------- calculate ImGui::BeginChild sizes ------------------

	ImVec2 availableSpace = ImGui::GetContentRegionAvail();
	availableSpace.y -= 4;		// act as if availableSpace height is 4px smaller, bc for some reason availableSpace height is cap (prevents scroll bars)
	float headerHeight = 80.0f;
	float footerHeight = 35.0f;
	float contentHeight = availableSpace.y - footerHeight;

	ImVec2 contentSize = ImVec2(0, contentHeight);
	ImVec2 footerSize = ImVec2(0, footerHeight);
	ImVec2 headerSize = ImVec2(0, headerHeight);

	// ----------------------------------------------------------------------


	if (ImGui::BeginChild("Content#cqc", contentSize))
	{
		GUI::SettingsHeader("Header##cqc", headerSize, false);

		bool chatsOn = enabled_cvar.getBoolValue();
		if (ImGui::Checkbox("Enabled", &chatsOn))
		{
			cvarManager->executeCommand(CvarNames::toggleEnabled);
		}

		if (chatsOn)
		{
			GUI::Spacing(4);
		
			bool overrideDefaultQuickchats = overrideDefaultQuickchats_cvar.getBoolValue();
			if (ImGui::Checkbox("Override default quickchats", &overrideDefaultQuickchats))
			{
				overrideDefaultQuickchats_cvar.setValue(overrideDefaultQuickchats);
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Block default quickchat if it conflicts with a custom chat (prevents both chats from being sent)");
			}

			GUI::Spacing(2);

			bool blockDefaultQuickchats = blockDefaultQuickchats_cvar.getBoolValue();
			if (ImGui::Checkbox("Block all default quickchats", &blockDefaultQuickchats))
			{
				blockDefaultQuickchats_cvar.setValue(blockDefaultQuickchats);
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Block all default quickchats... without having to unbind them :)");
			}
			
			GUI::Spacing(4);

			// --------------------------- speech-to-text ------------------------------

			if (ImGui::CollapsingHeader("speech-to-text settings", ImGuiTreeNodeFlags_None))
			{
				GUI::Spacing(2);
				
				bool autoDetectInterpreterPath = autoDetectInterpreterPath_cvar.getBoolValue();
				if (ImGui::Checkbox("Auto detect python interpreter", &autoDetectInterpreterPath))
				{
					autoDetectInterpreterPath_cvar.setValue(autoDetectInterpreterPath);
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Uncheck and manually enter the filepath to pythonw.exe if you get an error about pythonw.exe not found");
				}

				if (!autoDetectInterpreterPath)
				{
					GUI::Spacing(2);

					// filepath to pythonw.exe
					std::string pythonInterpreterPath = pythonInterpreterPath_cvar.getStringValue();
					if (ImGui::InputText("pythonw.exe filepath", &pythonInterpreterPath))
					{
						pythonInterpreterPath_cvar.setValue(pythonInterpreterPath);
					}

					ImGui::SameLine();

					if (ImGui::Button("Apply"))
					{
						gameWrapper->Execute([this](GameWrapper* gw)
							{
								pyInterpreter = findPythonInterpreter();
							});
					}

				}

				GUI::Spacing(4);

				// open bindings window button
				if (ImGui::Button("Calibrate Microphone"))
				{
					gameWrapper->Execute([this](GameWrapper* gw) {
						StartSpeechToText("lobby", "", true, true);  // calibrate mic energy threshold
						UpdateMicCalibration(4);
						});
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("calibrates microphone sensitivity level (for the plugin) based on a 2 second sample of background noise");
				}

				GUI::Spacing(2);

				// to keep the precision of the double without truncating digits in the string
				std::ostringstream oss;
				oss << std::setprecision(16) << micEnergyThreshold;
				std::string micThresholdStr = oss.str();

				std::string thresholdStr = "Energy threshold: " + micThresholdStr;
				ImGui::Text(thresholdStr.c_str());

				GUI::Spacing(4);

				// chat notifications
				bool speechToTextNotificationsOn = enableSTTNotifications_cvar.getBoolValue();
				if (ImGui::Checkbox("Enable speech-to-text notifications", &speechToTextNotificationsOn))
				{
					enableSTTNotifications_cvar.setValue(speechToTextNotificationsOn);
				}

				if (speechToTextNotificationsOn)
				{
					GUI::Spacing(2);

					// popup notification duration
					float notificationDuration = notificationDuration_cvar.getFloatValue();
					ImGui::SliderFloat("duration of notifications", &notificationDuration, 1.5f, 10.0f, "%.1f seconds");
					notificationDuration_cvar.setValue(notificationDuration);

					GUI::Spacing(2);

					// test popup notifications
					if (ImGui::Button("Test Notification"))
					{
						gameWrapper->Execute([this, notificationDuration](GameWrapper* gw) {

							Instances.SpawnNotification("Terry A Davis", "You can see 'em if you're driving. You just run them over. That's what you do.", notificationDuration);
							
							});
					}
				}

				GUI::Spacing(4);

				// start speech timeout
				float waitForSpeechTimeout = beginSpeechTimeout_cvar.getFloatValue();
				ImGui::SliderFloat("timeout to start speaking", &waitForSpeechTimeout, 1.5f, 10.0f, "%.1f seconds");
				beginSpeechTimeout_cvar.setValue(waitForSpeechTimeout);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("max time to wait for start of speech");
				}

				GUI::Spacing(2);

				// processing timeout
				int processSpeechTimeout = speechProcessingTimeout_cvar.getFloatValue();
				ImGui::SliderInt("timeout for processing speech", &processSpeechTimeout, 3.0f, 20.0f, "%.0f seconds");
				speechProcessingTimeout_cvar.setValue(processSpeechTimeout);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("max time to spend processing speech\t(will abort speech-to-text attempt if exceeded)");
				}

			}

			// -------------------------------------------------------------------------

			GUI::Spacing(8);

			// open bindings window button
			if (ImGui::Button("Open Bindings Menu"))
			{
				gameWrapper->Execute([this](GameWrapper* gw) {
					cvarManager->executeCommand("togglemenu " + GetMenuName());
					});
			}

		}
	}
	ImGui::EndChild();

	GUI::SettingsFooter("Footer##cqc", footerSize, availableSpace.x, false);
}


void CustomQuickchat::RenderWindow()
{
	ImGui::BeginTabBar("##Tabs");

	if (ImGui::BeginTabItem("Bindings"))
	{
		RenderAllBindings();
		ImGui::SameLine();
		RenderBindingDetails();
		
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Word Variations"))
	{
		RenderAllVariationListNames();
		ImGui::SameLine();
		RenderVariationListDetails();
		
		ImGui::EndTabItem();
	}
}


void CustomQuickchat::RenderAllBindings()
{
	if (ImGui::BeginChild("##BindingsList", ImVec2(300, 0), true))
	{
		ImGui::TextUnformatted("Bindings:");
		ImGui::Separator();

		for (int i = 0; i < Bindings.size(); i++)
		{
			Binding binding = Bindings[i];
			if (ImGui::Selectable((binding.chat + "##" + std::to_string(i)).c_str(), i == selectedBindingIndex))
			{
				selectedBindingIndex = i;
			}
		}

		GUI::Spacing(2);

		if (ImGui::Button("Add New Binding", ImVec2(-1, 0)))
		{
			AddEmptyBinding();

			selectedBindingIndex = Bindings.empty() ? 0 : Bindings.size() - 1;
		}
	}
	ImGui::EndChild();
}


void CustomQuickchat::RenderBindingDetails()
{
	if (ImGui::BeginChild("##BindingsView", ImVec2(0, 0), true))
	{
		if (Bindings.empty())
		{
			GUI::Spacing(4);

			ImGui::TextUnformatted("add a binding...");

			ImGui::EndChild();
			return;
		}

		Binding selectedBinding = Bindings[selectedBindingIndex];

		// binding display section title
		ImGui::TextUnformatted(selectedBinding.chat.c_str());
		ImGui::Separator();

		GUI::Spacing(4);

		ImGui::TextColored(ImVec4(1, 1, 0, 1), "Chat:");

		GUI::Spacing(2);

		// binding chat
		std::string bindingChat = selectedBinding.chat;
		ImGui::InputTextWithHint("chat", "let me cook", &bindingChat);
		Bindings[selectedBindingIndex].chat = bindingChat;

		GUI::Spacing(2);

		// binding chat mode
		ImGui::SearchableCombo("chat mode", &Bindings[selectedBindingIndex].chatMode, possibleChatModes, "", "");
		
		GUI::Spacing(8);
		
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "How it's triggered:");

		GUI::Spacing(2);

		// binding type
		ImGui::SearchableCombo("binding type", &Bindings[selectedBindingIndex].typeNameIndex, possibleBindingTypes, "", "");
		
		GUI::Spacing(4);
		
		if (possibleBindingTypes[Bindings[selectedBindingIndex].typeNameIndex] == "button sequence" && (Bindings[selectedBindingIndex].buttonNameIndexes.size() < 2))
		{
			ImGui::Text("*** Button sequence bindings must use 2 buttons! ***");
			GUI::Spacing(2);
		}

		// binding buttons
		for (int i = 0; i < selectedBinding.buttonNameIndexes.size(); i++)
		{
			// button dropdown
			std::string label = "Button " + std::to_string(i + 1);
			ImGui::SearchableCombo(label.c_str(), &Bindings[selectedBindingIndex].buttonNameIndexes[i], possibleKeyNames, "", "");

			ImGui::SameLine();

			// remove button
			std::string removeLine = "Remove##" + std::to_string(i + 1);
			if (ImGui::Button(removeLine.c_str()))
			{
				Bindings[selectedBindingIndex].buttonNameIndexes.erase(Bindings[selectedBindingIndex].buttonNameIndexes.begin() + i);
			}

			GUI::Spacing(2);
		}

		if (!((possibleBindingTypes[Bindings[selectedBindingIndex].typeNameIndex] == "button sequence") && (Bindings[selectedBindingIndex].buttonNameIndexes.size() >= 2)))
		{
			// add new button
			if (ImGui::Button("Add New Button"))
			{
				Bindings[selectedBindingIndex].buttonNameIndexes.push_back(0);
			}
		}

		GUI::Spacing(4);

		if (ImGui::Button("Save"))
		{
			WriteBindingsToJson();
		}

		GUI::Spacing(6);

		// delete binding button
		ImGui::PushID(0);
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
		
		if (ImGui::Button("Delete Binding", ImVec2(0, 0)))
		{
			ImGui::PopStyleColor(3);
			ImGui::PopID();
			DeleteBinding(selectedBindingIndex);
		}
		else {
			ImGui::PopStyleColor(3);
			ImGui::PopID();
		}

	}

	ImGui::EndChild();
}


void CustomQuickchat::RenderAllVariationListNames()
{
	if (ImGui::BeginChild("##VariationsList", ImVec2(300, 0), true))
	{
		ImGui::TextUnformatted("Variation lists:");
		ImGui::Separator();

		for (int i = 0; i < Variations.size(); i++)
		{
			VariationList list = Variations[i];
			if (ImGui::Selectable((list.listName + "##" + std::to_string(i)).c_str(), i == selectedVariationIndex))
			{
				selectedVariationIndex = i;
			}
		}

		GUI::Spacing(2);

		if (ImGui::Button("Add New List", ImVec2(-1, 0)))
		{
			AddEmptyVariationList();

			selectedVariationIndex = Variations.empty() ? 0 : Variations.size() - 1;
		}
	}
	ImGui::EndChild();
}


void CustomQuickchat::RenderVariationListDetails()
{
	if (ImGui::BeginChild("##VariationView", ImVec2(0, 0), true))
	{
		if (Variations.empty())
		{
			GUI::Spacing(4);

			ImGui::TextUnformatted("add a word variation list...");

			ImGui::EndChild();
			return;
		}

		VariationList selectedVariation = Variations[selectedVariationIndex];

		// binding display section title
		ImGui::TextUnformatted(selectedVariation.listName.c_str());
		ImGui::Separator();

		GUI::Spacing(6);
		
		// variation list name
		std::string variationListName = selectedVariation.listName;
		ImGui::InputTextWithHint("list name", "compliment", &variationListName);
		Variations[selectedVariationIndex].listName = variationListName;

		GUI::Spacing(2);

		// variations (raw text)
		std::string variationRawListStr = selectedVariation.unparsedString;
		ImGui::InputTextMultiline("variations", &variationRawListStr, ImVec2(0,350));
		Variations[selectedVariationIndex].unparsedString = variationRawListStr;

		GUI::Spacing(4);

		if (ImGui::Button("Save")) {
			UpdateDataFromVariationStr();
			WriteVariationsToJson();
		}

		GUI::Spacing(6);

		// delete variation list button
		ImGui::PushID(0);
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
		if (ImGui::Button("Delete List", ImVec2(0, 0)))
		{
			ImGui::PopStyleColor(3);
			ImGui::PopID();
			DeleteVariationList(selectedVariationIndex);
		}
		else {
			ImGui::PopStyleColor(3);
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
}