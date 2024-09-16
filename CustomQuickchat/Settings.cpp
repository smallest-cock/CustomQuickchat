#include "pch.h"
#include "CustomQuickchat.h"
#include "GuiTools.hpp"


void CustomQuickchat::RenderSettings()
{
	auto enabled_cvar = GetCvar(Cvars::enabled);
	if (!enabled_cvar) return;

	// ---------------- calculate ImGui::BeginChild sizes ------------------

	ImVec2 availableSpace = ImGui::GetContentRegionAvail();
	availableSpace.y -= 4;		// act as if availableSpace height is 4px smaller, bc for some reason availableSpace height is cap (prevents scroll bars)
	float headerHeight = 80.0f;
	float footerHeight = 35.0f;
	float contentHeight = availableSpace.y - footerHeight;

	ImVec2 contentSize =	ImVec2(0, contentHeight);
	ImVec2 footerSize =		ImVec2(0, footerHeight);
	ImVec2 headerSize =		ImVec2(0, headerHeight);

	// ----------------------------------------------------------------------

	if (ImGui::BeginChild("Content#cqc", contentSize))
	{
		GUI::SettingsHeader("Header##cqc", pretty_plugin_version, headerSize, false);

		bool enabled = enabled_cvar.getBoolValue();
		if (ImGui::Checkbox("Enabled", &enabled))
		{
			RUN_COMMAND(Cvars::toggleEnabled);
		}

		if (enabled)
		{
			GUI::Spacing(4);
		
			// general settings
			if (ImGui::CollapsingHeader("General settings", ImGuiTreeNodeFlags_None))
			{
				GeneralSettings();
			}

			// chat timeouts
			if (ImGui::CollapsingHeader("Chat timeout settings", ImGuiTreeNodeFlags_None))
			{
				ChatTimeoutSettings();
			}

			// speech-to-text
			if (ImGui::CollapsingHeader("speech-to-text settings", ImGuiTreeNodeFlags_None))
			{
				SpeechToTextSettings();
			}

			GUI::Spacing(8);

			// open bindings window button
			if (ImGui::Button("Open Bindings Menu"))
			{
				GAME_THREAD_EXECUTE(
					cvarManager->executeCommand("togglemenu " + GetMenuName());
				);
			}
		}
	}
	ImGui::EndChild();

	GUI::SettingsFooter("Footer##cqc", footerSize, availableSpace.x, false);
}


void CustomQuickchat::GeneralSettings()
{
	auto sequenceTimeWindow_cvar =			GetCvar(Cvars::sequenceTimeWindow);
	auto minBindingDelay_cvar =				GetCvar(Cvars::minBindingDelay);
	auto overrideDefaultQuickchats_cvar =	GetCvar(Cvars::overrideDefaultQuickchats);
	auto blockDefaultQuickchats_cvar =		GetCvar(Cvars::blockDefaultQuickchats);
	auto disablePostMatchQuickchats_cvar =	GetCvar(Cvars::disablePostMatchQuickchats);
	auto removeTimestamps_cvar =			GetCvar(Cvars::removeTimestamps);

	GUI::Spacing(2);

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

	GUI::Spacing(2);

	bool disablePostMatchQuickchats = disablePostMatchQuickchats_cvar.getBoolValue();
	if (ImGui::Checkbox("Disable custom quickchats in post-match screen", &disablePostMatchQuickchats))
	{
		disablePostMatchQuickchats_cvar.setValue(disablePostMatchQuickchats);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Prevents your custom chats from overriding default post-match chats like 'gg'");
	}

	GUI::Spacing(2);

	// remove chat timestamps
	bool removeTimestamps = removeTimestamps_cvar.getBoolValue();
	if (ImGui::Checkbox("Remove chat timestamps", &removeTimestamps))
	{
		removeTimestamps_cvar.setValue(removeTimestamps);
	}

	GUI::Spacing(2);

	// sequence max time window
	float sequenceTimeWindow = sequenceTimeWindow_cvar.getFloatValue();
	if (ImGui::SliderFloat("button sequence time window", &sequenceTimeWindow, 0.0f, 10.0f, "%.1f seconds"))
	{
		sequenceTimeWindow_cvar.setValue(sequenceTimeWindow);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("max time between sequence button presses");
	}

	GUI::Spacing(2);

	// min delay between bindings
	float minBindingDelay = minBindingDelay_cvar.getFloatValue();
	if (ImGui::SliderFloat("minimum delay between chats", &minBindingDelay, 0.01f, 0.5f, "%.2f seconds"))
	{
		minBindingDelay_cvar.setValue(minBindingDelay);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("can help prevent accidental chats... but also affects chat spamming speed");
	}

	GUI::Spacing(2);
}


void CustomQuickchat::ChatTimeoutSettings()
{
	auto disableChatTimeout_cvar =			GetCvar(Cvars::disableChatTimeout);
	auto useCustomChatTimeoutMsg_cvar =		GetCvar(Cvars::useCustomChatTimeoutMsg);
	auto customChatTimeoutMsg_cvar =		GetCvar(Cvars::customChatTimeoutMsg);

	GUI::Spacing(2);

	bool disableChatTimeout = disableChatTimeout_cvar.getBoolValue();
	if (ImGui::Checkbox("Disable chat timeout (freeplay)", &disableChatTimeout))
	{
		disableChatTimeout_cvar.setValue(disableChatTimeout);
	}

	GUI::Spacing(2);

	bool useCustomChatTimeoutMsg = useCustomChatTimeoutMsg_cvar.getBoolValue();
	if (ImGui::Checkbox("Custom chat timeout message", &useCustomChatTimeoutMsg))
	{
		useCustomChatTimeoutMsg_cvar.setValue(useCustomChatTimeoutMsg);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Experimental");
	}

	if (useCustomChatTimeoutMsg)
	{
		GUI::Spacing(2);

		std::string customChatTimeoutMsg = customChatTimeoutMsg_cvar.getStringValue();
		if (ImGui::InputText("Chat timeout message", &customChatTimeoutMsg))
		{
			customChatTimeoutMsg_cvar.setValue(customChatTimeoutMsg);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("TIP: use [Time] in your message to include the timeout seconds");
		}
	}

	//if (!Instances.alteredHudInstances.empty())
	//{
	//	GUI::Spacing(2);
	//
	//	ImGui::Text("Number of altered GFxHUD_TA instances: %d", Instances.alteredHudInstances.size());
	//}

	GUI::Spacing(2);
}


void CustomQuickchat::SpeechToTextSettings()
{
	auto enableSTTNotifications_cvar =		GetCvar(Cvars::enableSTTNotifications);
	auto speechProcessingTimeout_cvar =		GetCvar(Cvars::speechProcessingTimeout);
	auto beginSpeechTimeout_cvar =			GetCvar(Cvars::beginSpeechTimeout);
	auto notificationDuration_cvar =		GetCvar(Cvars::notificationDuration);
	auto searchForPyInterpreter_cvar =		GetCvar(Cvars::searchForPyInterpreter);
	auto autoDetectInterpreterPath_cvar =	GetCvar(Cvars::autoDetectInterpreterPath);
	auto pythonInterpreterPath_cvar =		GetCvar(Cvars::pythonInterpreterPath);

	GUI::Spacing(2);

	bool searchForPyInterpreter = searchForPyInterpreter_cvar.getBoolValue();
	if (ImGui::Checkbox("Search for python interpreter", &searchForPyInterpreter))
	{
		searchForPyInterpreter_cvar.setValue(searchForPyInterpreter);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Enable this to search for pythonw.exe (if your speech-to-text isn't working)");
	}

	if (searchForPyInterpreter)
	{
		bool autoDetectInterpreterPath = autoDetectInterpreterPath_cvar.getBoolValue();
		int radioState = autoDetectInterpreterPath ? 0 : 1;
		if (ImGui::RadioButton("Auto detect path to pythonw.exe", &radioState, 0))
		{
			autoDetectInterpreterPath_cvar.setValue(true);
		}
		if (ImGui::RadioButton("Manually specify path to pythonw.exe", &radioState, 1))
		{
			autoDetectInterpreterPath_cvar.setValue(false);
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
				GAME_THREAD_EXECUTE(
					pyInterpreter = findPythonInterpreter();
					);
			}
		}
	}

	GUI::Spacing(4);

	// open bindings window button
	if (ImGui::Button("Calibrate Microphone"))
	{
		GAME_THREAD_EXECUTE(
			StartSpeechToText(EChatChannel::EChatChannel_Match, "", true, true);  // calibrate mic energy threshold
		UpdateMicCalibration(4);
			);
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
		if (ImGui::SliderFloat("notification duration", &notificationDuration, 1.5f, 10.0f, "%.1f seconds"))
		{
			notificationDuration_cvar.setValue(notificationDuration);
		}

		GUI::SameLineSpacing(10);

		// test popup notifications
		if (ImGui::Button("Test"))
		{
			GAME_THREAD_EXECUTE_CAPTURE(notificationDuration,
				Instances.SpawnNotification("Terry A Davis", "You can see 'em if you're driving. You just run them over. That's what you do.", notificationDuration);
			);
		}
	}

	GUI::Spacing(2);

	// start speech timeout
	float waitForSpeechTimeout = beginSpeechTimeout_cvar.getFloatValue();
	if (ImGui::SliderFloat("timeout to start speaking", &waitForSpeechTimeout, 1.5f, 10.0f, "%.1f seconds"))
	{
		beginSpeechTimeout_cvar.setValue(waitForSpeechTimeout);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("max time to wait for start of speech");
	}

	GUI::Spacing(2);

	// processing timeout
	int processSpeechTimeout = speechProcessingTimeout_cvar.getFloatValue();
	if (ImGui::SliderInt("timeout for processing speech", &processSpeechTimeout, 3.0f, 20.0f, "%.0f seconds"))
	{
		speechProcessingTimeout_cvar.setValue(processSpeechTimeout);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("max time to spend processing speech\t(will abort speech-to-text attempt if exceeded)");
	}

	GUI::Spacing(2);
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

		Binding& selectedBinding = Bindings[selectedBindingIndex];

		// binding display section title
		ImGui::TextUnformatted(selectedBinding.chat.c_str());
		ImGui::Separator();

		GUI::Spacing(4);

		ImGui::TextColored(ImVec4(1, 1, 0, 1), "Chat:");

		GUI::Spacing(2);

		// chat
		ImGui::InputTextWithHint("chat", "let me cook", &selectedBinding.chat);

		GUI::Spacing(2);

		// -------------------------- chat mode dropdown ---------------------------
		if (ImGui::BeginCombo("chat mode", possibleChatModes[static_cast<int>(selectedBinding.chatMode)].c_str()))
		{
			for (int i = 0; i < possibleChatModes.size(); i++)
			{
				ImGui::PushID(i);

				const std::string& chatModeStr = possibleChatModes[i];
				if (ImGui::Selectable(chatModeStr.c_str(), static_cast<int>(selectedBinding.chatMode) == i))
				{
					selectedBinding.chatMode = static_cast<EChatChannel>(i);
				}
				
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		// -------------------------------------------------------------------------
		
		GUI::Spacing(8);
		
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "How it's triggered:");

		GUI::Spacing(2);

		// ------------------------- binding type dropdown -------------------------
		if (ImGui::BeginCombo("binding type", possibleBindingTypes[static_cast<int>(selectedBinding.bindingType)].c_str()))
		{
			for (int i = 0; i < possibleBindingTypes.size(); i++)
			{
				ImGui::PushID(i);

				const std::string& bindingTypeStr = possibleBindingTypes[i];
				if (ImGui::Selectable(bindingTypeStr.c_str(), static_cast<int>(selectedBinding.bindingType) == i))
					selectedBinding.bindingType = static_cast<BindingType>(i);
				
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		// -------------------------------------------------------------------------

		GUI::Spacing(4);
		
		if (selectedBinding.bindingType == BindingType::Sequence && selectedBinding.buttons.size() < 2)
		{
			ImGui::Text("*** Button sequence bindings must use 2 buttons! ***");
			GUI::Spacing(2);
		}

		// buttons
		for (int i = 0; i < selectedBinding.buttons.size(); i++)
		{
			// ---------------------------- button dropdown ----------------------------
			std::string buttonStr = selectedBinding.buttons[i];
			std::string label = "Button " + std::to_string(i + 1);

			char searchBuffer[128] = "";  // text buffer for search input

			if (ImGui::BeginSearchableCombo(label.c_str(), buttonStr.c_str(), searchBuffer, sizeof(searchBuffer), "search..."))
			{
				// convert search text to lower
				std::string searchQuery = Format::ToLower(searchBuffer);

				for (int j = 0; j < possibleKeyNames.size(); j++)
				{
					const std::string& keyNameStr = possibleKeyNames[j];
					const std::string keyNameStrLower = Format::ToLower(keyNameStr);

					ImGui::PushID(j);

					// only render option if there's text in search box & it matches the key name
					if (searchBuffer != "")
					{
						if (keyNameStrLower.find(searchQuery) != std::string::npos)
						{
							if (ImGui::Selectable(keyNameStr.c_str(), keyNameStr == buttonStr))
								selectedBinding.buttons[i] = keyNameStr;
						}
					}
					// if there's no text in search box, render all possible key options
					else
					{
						if (ImGui::Selectable(keyNameStr.c_str(), keyNameStr == buttonStr))
							selectedBinding.buttons[i] = keyNameStr;
					}

					ImGui::PopID();
				}

				ImGui::EndCombo();
			}
			// -------------------------------------------------------------------------


			ImGui::SameLine();

			// remove button
			std::string removeLine = "Remove##" + std::to_string(i + 1);
			if (ImGui::Button(removeLine.c_str()))
				selectedBinding.buttons.erase(selectedBinding.buttons.begin() + i);

			GUI::Spacing(2);
		}

		if (!(selectedBinding.bindingType == BindingType::Sequence && selectedBinding.buttons.size() >= 2))
		{
			// add new button
			if (ImGui::Button("Add New Button"))
				selectedBinding.buttons.push_back("");
		}

		GUI::Spacing(4);

		if (ImGui::Button("Save"))
		{
			WriteBindingsToJson();

			GAME_THREAD_EXECUTE(
				Instances.SpawnNotification("custom quickchat", "Bindings saved!", 3);
			);
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

		if (ImGui::Button("Save"))
		{
			UpdateDataFromVariationStr();
			WriteVariationsToJson();
			
			GAME_THREAD_EXECUTE(
				Instances.SpawnNotification("custom quickchat", "Variations saved!", 3);
			);
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