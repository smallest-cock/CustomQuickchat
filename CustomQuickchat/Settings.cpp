#include "pch.h"
#include "CustomQuickchat.h"



void CustomQuickchat::RenderSettings()
{
	auto enabled_cvar = GetCvar(Cvars::enabled);
	if (!enabled_cvar) return;

	const float content_height = ImGui::GetContentRegionAvail().y - footer_height;  // available height after accounting for footer

	if (ImGui::BeginChild("PluginSettingsSection", ImVec2(0, content_height)))
	{
		GUI::alt_settings_header("Plugin made by S S L o w", pretty_plugin_version);

		bool enabled = enabled_cvar.getBoolValue();
		if (ImGui::Checkbox("Enabled", &enabled))
		{
			RunCommand(Commands::toggleEnabled);
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

			// last chat
			if (ImGui::CollapsingHeader("Last chat settings", ImGuiTreeNodeFlags_None))
			{
				LastChatSettings();
			}

			GUI::Spacing(10);

			if (ImGui::Button("Send a test chat"))
			{
				GAME_THREAD_EXECUTE(
					Instances.SendChat("this is a test...", EChatChannel::EChatChannel_Match);
				);
			}

			GUI::Spacing(4);

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

	GUI::alt_settings_footer("Need help? Join the Discord", "https://discord.gg/d5ahhQmJbJ");
}


void CustomQuickchat::GeneralSettings()
{
	auto sequenceTimeWindow_cvar =          GetCvar(Cvars::sequenceTimeWindow);
	auto minBindingDelay_cvar =             GetCvar(Cvars::minBindingDelay);
	auto overrideDefaultQuickchats_cvar =   GetCvar(Cvars::overrideDefaultQuickchats);
	auto blockDefaultQuickchats_cvar =      GetCvar(Cvars::blockDefaultQuickchats);
	auto disablePostMatchQuickchats_cvar =  GetCvar(Cvars::disablePostMatchQuickchats);
	auto removeTimestamps_cvar =            GetCvar(Cvars::removeTimestamps);

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
		ImGui::SetTooltip("Block all default quickchats from being sent... without having to unbind them :)");
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
	auto disableChatTimeout_cvar =          GetCvar(Cvars::disableChatTimeout);
	auto useCustomChatTimeoutMsg_cvar =     GetCvar(Cvars::useCustomChatTimeoutMsg);
	auto customChatTimeoutMsg_cvar =        GetCvar(Cvars::customChatTimeoutMsg);

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

	GUI::Spacing(2);
}


void CustomQuickchat::SpeechToTextSettings()
{
#if !defined(USE_SPEECH_TO_TEXT)
	
	GUI::Spacing(4);

	ImGui::Text("This version of the plugin doesnt support speech-to-text. You can find that version on the github Releases page:");

	GUI::Spacing(2);

	GUI::ClickableLink("Releases", "https://github.com/smallest-cock/CustomQuickchat/releases/latest", ImVec4(1, 1, 0, 1));

#else

	auto enableSTTNotifications_cvar =      GetCvar(Cvars::enableSTTNotifications);
	auto speechProcessingTimeout_cvar =     GetCvar(Cvars::speechProcessingTimeout);
	auto beginSpeechTimeout_cvar =          GetCvar(Cvars::beginSpeechTimeout);
	auto notificationDuration_cvar =        GetCvar(Cvars::notificationDuration);
	auto autoCalibrateMic_cvar =            GetCvar(Cvars::autoCalibrateMic);
	auto micCalibrationTimeout_cvar =       GetCvar(Cvars::micCalibrationTimeout);
	auto micEnergyThreshold_cvar =          GetCvar(Cvars::micEnergyThreshold);
	auto websocket_port_cvar =              GetCvar(Cvars::websocket_port);

	if (!micEnergyThreshold_cvar) return;

	GUI::Spacing(2);

	// display websocket connection status
	bool ws_is_connected_to_server = Websocket ? Websocket->IsConnectedToServer() : false;

	std::string connection_status;
	if (!connecting_to_ws_server.load())
	{
		connection_status = ws_is_connected_to_server ? ("Connected (port " + Websocket->get_port_str() + ")") : "Not connected";
	}
	else
	{
		connection_status = "Connecting....";
	}
	std::string ws_status_str = "Websocket status:\t" + connection_status;
	ImGui::Text(ws_status_str.c_str());

	GUI::Spacing();

	ImGui::SetNextItemWidth(100);
	int websocket_port = websocket_port_cvar.getIntValue();
	if (ImGui::InputInt("Port number", &websocket_port))
	{
		websocket_port_cvar.setValue(websocket_port);
	}

	GUI::Spacing();

	if (!ws_is_connected_to_server && !connecting_to_ws_server.load())
	{
		if (ImGui::Button("Start##websocket"))
		{
			LOG("'Start' button has been clicked...");

			auto start_ws_connection = [this]()
			{
				if (Websocket && Websocket->IsConnectedToServer())
				{
					LOG("Failed to start websocket connection... it's already active!");
					return;
				}

				start_websocket_stuff();
			};

			GAME_THREAD_EXECUTE_CAPTURE(
				start_ws_connection();
			, start_ws_connection);
		}
	}
	else
	{
		if (ImGui::Button("Stop##websocket"))
		{
			LOG("'Stop' button has been clicked...");

			auto stop_ws_connection = [this]()
			{
				stop_websocket_server();
				connecting_to_ws_server.store(false);

				if (!Websocket)
				{
					LOG("[ERROR] Websocket object is null... cant stop client");
					return;
				}

				bool success = Websocket->StopClient();
				LOG(success ? "Stopping websocket client was successful" : "Stopping websocket client was unsuccessful");

				Websocket->set_connected_status(false);
			};

			GAME_THREAD_EXECUTE_CAPTURE(
				stop_ws_connection();
			, stop_ws_connection);
		}
	}


	GUI::Spacing(2);

	bool autoCalibrateMic = autoCalibrateMic_cvar.getBoolValue();
	int radioButtonVal = autoCalibrateMic ? 0 : 1;

	if (ImGui::RadioButton("Auto calibrate microphone on every listen", &radioButtonVal, 0))
	{
		autoCalibrateMic_cvar.setValue(true);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Briefly calibrates mic energy level before you start speaking (reliable)");
	}

	if (ImGui::RadioButton("Manually calibrate microphone", &radioButtonVal, 1))
	{
		autoCalibrateMic_cvar.setValue(false);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Uses a stored calibration value, eliminating the need to calibrate mic before every attempt (can be faster)");
	}

	if (!autoCalibrateMic)
	{
		GUI::Spacing(4);

		std::string thresholdStr = "Mic energy threshold: ";
		thresholdStr += calibratingMicLevel ? "calibrating....." : std::to_string(micEnergyThreshold_cvar.getIntValue());
		ImGui::Text(thresholdStr.c_str());

		GUI::Spacing(2);

		// calibrate mic button
		if (ImGui::Button("Calibrate Microphone"))
		{
			GAME_THREAD_EXECUTE(
				CalibrateMicrophone();
			);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("calibrate microphone sensitivity level (for the plugin) based on a sample of background noise");
		}

		GUI::Spacing(2);

		int micCalibrationTimeout = micCalibrationTimeout_cvar.getIntValue();
		if (ImGui::SliderInt("mic calibration timeout", &micCalibrationTimeout, 1.0f, 20.0f, "%.0f seconds"))
		{
			micCalibrationTimeout_cvar.setValue(micCalibrationTimeout);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("max amount of time to spend on a calibration attempt before aborting");
		}
	}

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

		GUI::SameLineSpacing_relative(10);

		// test popup notifications
		if (ImGui::Button("Test"))
		{
			GAME_THREAD_EXECUTE_CAPTURE(
				Instances.SpawnNotification("Terry A Davis", "You can see 'em if you're driving. You just run them over. That's what you do.", notificationDuration);
			, notificationDuration);
		}
	}

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

	// processing timeout
	int processSpeechTimeout = speechProcessingTimeout_cvar.getFloatValue();
	if (ImGui::SliderInt("timeout for processing speech", &processSpeechTimeout, 3.0f, 30.0f, "%.0f seconds"))
	{
		speechProcessingTimeout_cvar.setValue(processSpeechTimeout);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("max time to spend processing speech\t(will abort speech-to-text attempt if exceeded)");
	}

	GUI::Spacing(2);

#endif // defined(USE_SPEECH_TO_TEXT)
}


void CustomQuickchat::LastChatSettings()
{
	auto user_chats_in_last_chat_cvar =         GetCvar(Cvars::user_chats_in_last_chat);
	auto teammate_chats_in_last_chat_cvar =     GetCvar(Cvars::teammate_chats_in_last_chat);
	auto quickchats_in_last_chat_cvar =         GetCvar(Cvars::quickchats_in_last_chat);
	auto party_chats_in_last_chat_cvar =        GetCvar(Cvars::party_chats_in_last_chat);
	auto team_chats_in_last_chat_cvar =         GetCvar(Cvars::team_chats_in_last_chat);

	if (!user_chats_in_last_chat_cvar) return;

	bool user_chats_in_last_chat =      user_chats_in_last_chat_cvar.getBoolValue();
	bool quickchats_in_last_chat =      quickchats_in_last_chat_cvar.getBoolValue();
	bool teammate_chats_in_last_chat =  teammate_chats_in_last_chat_cvar.getBoolValue();
	bool party_chats_in_last_chat =     party_chats_in_last_chat_cvar.getBoolValue();
	bool team_chats_in_last_chat =      team_chats_in_last_chat_cvar.getBoolValue();

	GUI::Spacing(2);

	GUI::ClickableLink("Keywords guide", "https://github.com/smallest-cock/CustomQuickchat/blob/main/docs/Settings.md#special-effects", GUI::Colors::BlueGreen);

	GUI::Spacing(2);

	ImGui::TextColored(GUI::Colors::Yellow, "Chats to be included when searching for the last chat:");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Searching for last chat sent happens for [[lastChat]] and [[blast ...]]\n\nMore info can be found in the keywords guide above");
	}

	GUI::Spacing(2);

	if (ImGui::Checkbox("User chats", &user_chats_in_last_chat))
	{
		user_chats_in_last_chat_cvar.setValue(user_chats_in_last_chat);
	}

	if (ImGui::Checkbox("Quickchats", &quickchats_in_last_chat))
	{
		quickchats_in_last_chat_cvar.setValue(quickchats_in_last_chat);
	}

	if (ImGui::Checkbox("Teammate chats", &teammate_chats_in_last_chat))
	{
		teammate_chats_in_last_chat_cvar.setValue(teammate_chats_in_last_chat);
	}

	if (ImGui::Checkbox("Party chats", &party_chats_in_last_chat))
	{
		party_chats_in_last_chat_cvar.setValue(party_chats_in_last_chat);
	}

	if (ImGui::Checkbox("Team chats", &team_chats_in_last_chat))
	{
		team_chats_in_last_chat_cvar.setValue(team_chats_in_last_chat);
	}

	GUI::Spacing(4);

	ImGui::Text("Cached chats: %u", LobbyInfo.get_match_chats_size());

	GUI::SameLineSpacing_absolute(150);

	if (ImGui::Button("Clear##chatLog"))
	{
		GAME_THREAD_EXECUTE(
			LobbyInfo.clear_stored_chats();
		);
	}

	ImGui::Text("Cached player ranks: %u", LobbyInfo.get_match_ranks_size());

	GUI::SameLineSpacing_absolute(150);

	if (ImGui::Button("Clear##playerRanks"))
	{
		GAME_THREAD_EXECUTE(
			LobbyInfo.clear_stored_ranks();
		);
	}
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

		// chat details
		const float chat_details_height = ImGui::GetContentRegionAvail().y * 0.3f;  // 30% of parent height

		if (ImGui::BeginChild("ChatDetails", ImVec2(0, chat_details_height), true))
		{
			RenderChatDetails(selectedBinding);
		}
		ImGui::EndChild();

		// binding details
		const auto remaining_space = ImGui::GetContentRegionAvail();

		if (ImGui::BeginChild("BindingTriggerDetails", remaining_space, true))
		{
			RenderBindingTriggerDetails(selectedBinding);
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
}


void CustomQuickchat::RenderChatDetails(Binding& selectedBinding)
{
	ImGui::Checkbox("Enabled", &selectedBinding.enabled);

	GUI::Spacing(2);
	
	ImGui::TextColored(GUI::Colors::Yellow, "Chat:");

	GUI::Spacing(4);

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
}


void CustomQuickchat::RenderBindingTriggerDetails(Binding& selectedBinding)
{
	ImGui::TextColored(GUI::Colors::Yellow, "How it's triggered:");

	GUI::Spacing(4);

	// ------------------------- binding type dropdown -------------------------
	if (ImGui::BeginCombo("binding type", possibleBindingTypes[static_cast<int>(selectedBinding.bindingType)].c_str()))
	{
		for (int i = 0; i < possibleBindingTypes.size(); i++)
		{
			ImGui::PushID(i);

			const std::string& bindingTypeStr = possibleBindingTypes[i];
			if (ImGui::Selectable(bindingTypeStr.c_str(), static_cast<int>(selectedBinding.bindingType) == i))
				selectedBinding.bindingType = static_cast<EBindingType>(i);

			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	// -------------------------------------------------------------------------

	GUI::Spacing(4);

	if (selectedBinding.bindingType == EBindingType::Sequence && selectedBinding.buttons.size() < 2)
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

	if (!(selectedBinding.bindingType == EBindingType::Sequence && selectedBinding.buttons.size() >= 2))
	{
		// add new button
		if (ImGui::Button("Add New Button"))
			selectedBinding.buttons.push_back("");
	}

	GUI::Spacing(4);

	if (ImGui::Button("Save"))
	{
		// update data for all bindings then write it to json
		UpdateBindingsData();
		WriteBindingsToJson();

		GAME_THREAD_EXECUTE(
			determine_quickchat_labels();
			
			auto chat = Instances.GetInstanceOf<UGFxData_Chat_TA>();
			if (chat)
			{
				apply_all_custom_qc_labels_to_ui(chat);
			}

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