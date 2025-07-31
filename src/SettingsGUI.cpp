#include "pch.h"
#include "CustomQuickchat.hpp"
#include "Macros.hpp"
#include "Keys.hpp"
#include <ModUtils/gui/GuiTools.hpp>
#include "components/Instances.hpp"
#include "components/LobbyInfo.hpp"


// ##############################################################################################################
// #############################################  PLUGIN SETTINGS  ##############################################
// ##############################################################################################################

void CustomQuickchat::RenderSettings()
{
	auto enabled_cvar = getCvar(Cvars::enabled);
	if (!enabled_cvar)
		return;

	const float content_height = ImGui::GetContentRegionAvail().y - footer_height;  // available height after accounting for footer

	if (ImGui::BeginChild("PluginSettingsSection", ImVec2(0, content_height)))
	{
		GUI::alt_settings_header(h_label.c_str(), pretty_plugin_version);

		bool enabled = enabled_cvar.getBoolValue();
		if (ImGui::Checkbox("Enabled", &enabled))
		{
			runCommand(Commands::toggleEnabled);
		}

		if (enabled)
		{
			GUI::Spacing(4);
		
			// general settings
			if (ImGui::CollapsingHeader("General settings", ImGuiTreeNodeFlags_None))
				display_generalSettings();

			// chat timeouts
			if (ImGui::CollapsingHeader("Chat timeout settings", ImGuiTreeNodeFlags_None))
				display_chatTimeoutSettings();

			// speech-to-text
			if (ImGui::CollapsingHeader("speech-to-text settings", ImGuiTreeNodeFlags_None))
				display_speechToTextSettings();

			// last chat
			if (ImGui::CollapsingHeader("Last chat settings", ImGuiTreeNodeFlags_None))
				display_lastChatSettings();

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


void CustomQuickchat::display_generalSettings()
{
	auto sequenceTimeWindow_cvar =          getCvar(Cvars::sequenceTimeWindow);
	auto minBindingDelay_cvar =             getCvar(Cvars::minBindingDelay);
	auto overrideDefaultQuickchats_cvar =   getCvar(Cvars::overrideDefaultQuickchats);
	auto blockDefaultQuickchats_cvar =      getCvar(Cvars::blockDefaultQuickchats);
	auto disablePostMatchQuickchats_cvar =  getCvar(Cvars::disablePostMatchQuickchats);
	auto removeTimestamps_cvar =            getCvar(Cvars::removeTimestamps);
	auto randomize_sarcasm_cvar =           getCvar(Cvars::randomize_sarcasm);
	auto uncensorChats_cvar =				 getCvar(Cvars::uncensorChats);

	GUI::Spacing(2);

	bool overrideDefaultQuickchats = overrideDefaultQuickchats_cvar.getBoolValue();
	if (ImGui::Checkbox("Override default quickchats", &overrideDefaultQuickchats))
		overrideDefaultQuickchats_cvar.setValue(overrideDefaultQuickchats);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Block default quickchat if it conflicts with a custom chat (prevents both chats from being sent)");

	bool blockDefaultQuickchats = blockDefaultQuickchats_cvar.getBoolValue();
	if (ImGui::Checkbox("Block all default quickchats", &blockDefaultQuickchats))
		blockDefaultQuickchats_cvar.setValue(blockDefaultQuickchats);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Block all default quickchats from being sent... without having to unbind them :)");

	bool disablePostMatchQuickchats = disablePostMatchQuickchats_cvar.getBoolValue();
	if (ImGui::Checkbox("Disable custom quickchats in post-match screen", &disablePostMatchQuickchats))
		disablePostMatchQuickchats_cvar.setValue(disablePostMatchQuickchats);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Prevents your custom chats from overriding default post-match chats like 'gg'");

	bool randomize_sarcasm = randomize_sarcasm_cvar.getBoolValue();
	if (ImGui::Checkbox("Randomize sarcasm effect", &randomize_sarcasm))
		randomize_sarcasm_cvar.setValue(randomize_sarcasm);

	bool removeTimestamps = removeTimestamps_cvar.getBoolValue();
	if (ImGui::Checkbox("Remove chat timestamps", &removeTimestamps))
		removeTimestamps_cvar.setValue(removeTimestamps);

	bool uncensorChats = uncensorChats_cvar.getBoolValue();
	if (ImGui::Checkbox("Uncensor chats", &uncensorChats))
		uncensorChats_cvar.setValue(uncensorChats);


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


void CustomQuickchat::display_chatTimeoutSettings()
{
	auto disableChatTimeout_cvar =          getCvar(Cvars::disableChatTimeout);
	auto useCustomChatTimeoutMsg_cvar =     getCvar(Cvars::useCustomChatTimeoutMsg);
	auto customChatTimeoutMsg_cvar =        getCvar(Cvars::customChatTimeoutMsg);

	GUI::Spacing(2);

	bool disableChatTimeout = disableChatTimeout_cvar.getBoolValue();
	if (ImGui::Checkbox("Disable chat timeout (freeplay)", &disableChatTimeout))
	{
		disableChatTimeout_cvar.setValue(disableChatTimeout);
	}

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


void CustomQuickchat::display_speechToTextSettings()
{
#if !defined(USE_SPEECH_TO_TEXT)
	
	GUI::Spacing(4);

	ImGui::Text("This version of the plugin doesnt support speech-to-text. You can find that version on the github Releases page:");

	GUI::Spacing(2);

	GUI::ClickableLink("Releases", "https://github.com/smallest-cock/CustomQuickchat/releases/latest", ImVec4(1, 1, 0, 1));

#else

	auto enableSTTNotifications_cvar =      getCvar(Cvars::enableSTTNotifications);
	auto speechProcessingTimeout_cvar =     getCvar(Cvars::speechProcessingTimeout);
	auto beginSpeechTimeout_cvar =          getCvar(Cvars::beginSpeechTimeout);
	auto notificationDuration_cvar =        getCvar(Cvars::notificationDuration);
	auto autoCalibrateMic_cvar =            getCvar(Cvars::autoCalibrateMic);
	auto micCalibrationTimeout_cvar =       getCvar(Cvars::micCalibrationTimeout);
	auto micEnergyThreshold_cvar =          getCvar(Cvars::micEnergyThreshold);
	auto websocket_port_cvar =              getCvar(Cvars::websocket_port);

	if (!micEnergyThreshold_cvar)
		return;

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
	ImGui::Text("%s", ws_status_str.c_str());

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
		ImGui::Text("%s", thresholdStr.c_str());

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

#endif // defined(USE_SPEECH_TO_TEXT)

	GUI::Spacing(2);
}


void CustomQuickchat::display_lastChatSettings()
{
	auto user_chats_in_last_chat_cvar =         getCvar(Cvars::user_chats_in_last_chat);
	auto teammate_chats_in_last_chat_cvar =     getCvar(Cvars::teammate_chats_in_last_chat);
	auto quickchats_in_last_chat_cvar =         getCvar(Cvars::quickchats_in_last_chat);
	auto party_chats_in_last_chat_cvar =        getCvar(Cvars::party_chats_in_last_chat);
	auto team_chats_in_last_chat_cvar =         getCvar(Cvars::team_chats_in_last_chat);

	if (!user_chats_in_last_chat_cvar)
		return;

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

	ImGui::Text("Cached chats: %zu", LobbyInfo.get_match_chats_size());

	GUI::SameLineSpacing_absolute(150);

	if (ImGui::Button("Clear##chatLog"))
	{
		GAME_THREAD_EXECUTE(
			LobbyInfo.clear_stored_chats();
		);
	}

	ImGui::Text("Cached player ranks: %zu", LobbyInfo.get_match_ranks_size());

	GUI::SameLineSpacing_absolute(150);

	if (ImGui::Button("Clear##playerRanks"))
	{
		GAME_THREAD_EXECUTE(
			LobbyInfo.clear_stored_ranks();
		);
	}
}




// ##############################################################################################################
// ###############################################  PLUGIN WINDOW  ##############################################
// ##############################################################################################################

void CustomQuickchat::RenderWindow()
{
	ImGui::BeginTabBar("CQCTabs");

	if (ImGui::BeginTabItem("Bindings"))
	{
		display_bindingsList();
		ImGui::SameLine();
		display_bindingDetails();
		
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Word Variations"))
	{
		display_variationListList();
		ImGui::SameLine();
		display_variationListDetails();
		
		ImGui::EndTabItem();
	}
}


void CustomQuickchat::display_bindingsList()
{
	{
		GUI::ScopedChild c{ "SelectOrCreateBinding", ImVec2(300, 0), true };

		auto availSpace = ImGui::GetContentRegionAvail();
		{
			GUI::ScopedChild c{ "BindingsList", ImVec2(0, availSpace.y * 0.9f)};

			ImGui::TextUnformatted("Bindings:");
			ImGui::Separator();

			auto bindingsSize = m_bindings.size();
			for (int i = 0; i < bindingsSize; ++i)
			{
				const Binding& binding = m_bindings[i];

				GUI::ScopedID id{ &binding };

				if (ImGui::Selectable(binding.chat.c_str(), i == m_selectedBindingIndex))
					m_selectedBindingIndex = i;
			}
		}

		GUI::Spacing(2);

		{
			GUI::ScopedChild c{ "AddBinding" };

			if (ImGui::Button("Add New Binding", ImGui::GetContentRegionAvail()))
			{
				addEmptyBinding();
				m_selectedBindingIndex = m_bindings.empty() ? 0 : m_bindings.size() - 1;
			}
		}
	}
}


void CustomQuickchat::display_bindingDetails()
{
	//static float childScale = 0.85f;	// <--- for testing

	{
		GUI::ScopedChild c{ "EverythingAboutBinding" };

		{
			GUI::ScopedChild c{ "BindingInfo" };

			if (m_bindings.empty())
			{
				GUI::Spacing(4);
				ImGui::TextUnformatted("add a binding...");
				return;
			}

			Binding& selectedBinding = m_bindings[m_selectedBindingIndex];

			ImGui::TextUnformatted(selectedBinding.chat.c_str());
			
			{
				GUI::ScopedChild c{ "ChatDetails", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.3f), true };

				display_bindingChatDetails(selectedBinding);

				//ImGui::SliderFloat("child scale", &childScale, 0.0f, 1.0f, "%.2f");	// <--- for testing
			}

			{
				GUI::ScopedChild c{ "BindingTriggerDetails", ImVec2(0, 0), true };

				{
					//GUI::ScopedChild c{ "ActualTriggerSection", ImVec2(0, ImGui::GetContentRegionAvail().y * childScale) };
					GUI::ScopedChild c{ "ActualTriggerSection", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.85f) };
					
					display_bindingTriggerDetails(selectedBinding);
				}

				GUI::Spacing(2);

				{
					GUI::ScopedChild c{ "ButtonsSection" };

					{
						GUI::ScopedChild c{ "SaveOrDelete" };

						{
							GUI::ScopedChild c{ "SaveButton", ImVec2(ImGui::GetContentRegionAvailWidth() * 0.75f, 0) };

							if (ImGui::Button("Save", ImGui::GetContentRegionAvail()))
							{
								// update data for all bindings then write it to json
								updateBindingsData();
								writeBindingsToJson();

								GAME_THREAD_EXECUTE(
									determine_quickchat_labels();
									
									auto chat = Instances.GetInstanceOf<UGFxData_Chat_TA>();
									if (chat)
										apply_all_custom_qc_labels_to_ui(chat);

									Instances.SpawnNotification("custom quickchat", "Bindings saved!", 3);
								);
							}
						}

						ImGui::SameLine();

						{
							GUI::ScopedChild c{ "DeleteButton", ImGui::GetContentRegionAvail() };

							ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));

							if (ImGui::Button("Delete Binding", ImGui::GetContentRegionAvail()))
							{
								GAME_THREAD_EXECUTE(
									DeleteBinding(m_selectedBindingIndex);
								);
							}

							ImGui::PopStyleColor(3);
						}
					}
				}
			}
		}
	}
}


void CustomQuickchat::display_bindingChatDetails(Binding& selectedBinding)
{
	ImGui::Checkbox("Enabled", &selectedBinding.enabled);

	GUI::Spacing(2);
	
	ImGui::TextColored(GUI::Colors::Yellow, "Chat:");

	GUI::Spacing(4);

	ImGui::InputTextWithHint("Chat", "let me cook", &selectedBinding.chat);

	GUI::Spacing(2);

	if (ImGui::BeginCombo("Chat mode", possibleChatModes[static_cast<int>(selectedBinding.chatMode)].c_str()))
	{
		for (int i = 0; i < possibleChatModes.size(); ++i)
		{
			GUI::ScopedID id{ i };

			const std::string& chatModeStr = possibleChatModes[i];
			if (ImGui::Selectable(chatModeStr.c_str(), static_cast<int>(selectedBinding.chatMode) == i))
				selectedBinding.chatMode = static_cast<EChatChannel>(i);
		}
		ImGui::EndCombo();
	}
}


void CustomQuickchat::display_bindingTriggerDetails(Binding& selectedBinding)
{
	ImGui::TextColored(GUI::Colors::Yellow, "How it's triggered:");

	GUI::Spacing(4);

	if (ImGui::BeginCombo("Binding type", possibleBindingTypes[static_cast<int>(selectedBinding.bindingType)].c_str()))
	{
		for (int i = 0; i < possibleBindingTypes.size(); ++i)
		{
			GUI::ScopedID id{ i };

			const std::string& bindingTypeStr = possibleBindingTypes[i];
			if (ImGui::Selectable(bindingTypeStr.c_str(), static_cast<int>(selectedBinding.bindingType) == i))
				selectedBinding.bindingType = static_cast<EBindingType>(i);
		}
		ImGui::EndCombo();
	}

	GUI::Spacing(4);

	if (selectedBinding.bindingType == EBindingType::Sequence && selectedBinding.buttons.size() < 2)
	{
		ImGui::TextColored(GUI::Colors::LightRed, "*** Button sequence bindings must use 2 buttons! Please add a button...");
		GUI::Spacing(2);
	}

	// buttons
	for (int i = 0; i < selectedBinding.buttons.size(); ++i)
	{
		GUI::ScopedID iID{ i };

		std::string& buttonStr = selectedBinding.buttons[i];
		std::string label = "Button " + std::to_string(i + 1);

		char searchBuffer[128] = "";  // text buffer for search input

		if (ImGui::BeginSearchableCombo(label.c_str(), buttonStr.c_str(), searchBuffer, sizeof(searchBuffer), "search..."))
		{
			std::string searchQuery = Format::ToLower(searchBuffer);

			for (int j = 0; j < possibleKeyNames.size(); ++j)
			{
				GUI::ScopedID jID{ j };

				const std::string& keyNameStr = possibleKeyNames[j];
				const std::string keyNameStrLower = Format::ToLower(keyNameStr);

				if (!searchQuery.empty()) // only render option if there's text in search box & it matches the key name
				{
					if (keyNameStrLower.find(searchQuery) != std::string::npos)
					{
						if (ImGui::Selectable(keyNameStr.c_str(), keyNameStr == buttonStr))
							buttonStr = keyNameStr;
					}
				}
				else // if there's no text in search box, render all possible key options
				{
					if (ImGui::Selectable(keyNameStr.c_str(), keyNameStr == buttonStr))
						buttonStr = keyNameStr;
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SameLine();

		if (ImGui::Button("Remove"))
			selectedBinding.buttons.erase(selectedBinding.buttons.begin() + i);

		GUI::Spacing(2);
	}

	if (!(selectedBinding.bindingType == EBindingType::Sequence && selectedBinding.buttons.size() >= 2))
	{
		if (ImGui::Button("Add New Button"))
			selectedBinding.buttons.emplace_back("");
	}
}


void CustomQuickchat::display_variationListList()
{
	{
		GUI::ScopedChild c{ "VariationListSection", ImVec2(300, 0), true };

		{
			GUI::ScopedChild c{ "VariationListList", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.9f) };
			
			for (int i = 0; i < m_variations.size(); ++i)
			{
				VariationList& list = m_variations[i];
				GUI::ScopedID id{ &list };

				if (ImGui::Selectable(list.listName.c_str(), i == m_selectedVariationIndex))
					m_selectedVariationIndex = i;
			}
		}

		GUI::Spacing(2);

		{
			GUI::ScopedChild c{ "AddVariationListSection" };

			if (ImGui::Button("Add New List", ImGui::GetContentRegionAvail()))
			{
				addEmptyVariationList();
				m_selectedVariationIndex = m_variations.empty() ? 0 : m_variations.size() - 1;
			}
		}
	}
}


void CustomQuickchat::display_variationListDetails()
{
	//static float childScale = 0.9f;

	{
		GUI::ScopedChild c{ "VariationListData", ImVec2(0, 0), true };

		if (m_variations.empty())
		{
			GUI::Spacing(4);
			ImGui::TextUnformatted("add a word variation list...");
			return;
		}

		VariationList& selectedVariation = m_variations[m_selectedVariationIndex];

		{
			GUI::ScopedChild c{ "VariationView", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.9f) };
			//GUI::ScopedChild c{ "VariationView", ImVec2(0, ImGui::GetContentRegionAvail().y * childScale)};

			// binding display section title
			ImGui::TextUnformatted(selectedVariation.listName.c_str());
			ImGui::Separator();

			GUI::Spacing(6);

			// list name
			ImGui::InputTextWithHint("List name", "compliment", &selectedVariation.listName);

			GUI::Spacing(2);

			// variations (raw text)
			ImGui::InputTextMultiline("Variations", &selectedVariation.unparsedString, ImVec2(0, 350));

			GUI::Spacing(2);

			if (ImGui::Checkbox("Randomize order", &selectedVariation.shuffleWordList))
			{
				selectedVariation.nextUsableIndex = 0;
			}
				
			//ImGui::SliderFloat("child scale", &childScale, 0.0f, 1.0f, "%.2f");	// <--- for testing
		}

		GUI::Spacing(2);

		{
			GUI::ScopedChild c{ "ButtonsSection" };

			{
				GUI::ScopedChild c{ "SaveOrDelete" };

				{
					GUI::ScopedChild c{ "SaveButton", ImVec2(ImGui::GetContentRegionAvailWidth() * 0.75f, 0) };

					if (ImGui::Button("Save", ImGui::GetContentRegionAvail()))
					{
						selectedVariation.updateDataFromUnparsedString();
						writeVariationsToJson();

						GAME_THREAD_EXECUTE(
							Instances.SpawnNotification("custom quickchat", "Variations saved!", 3);
						);
					}
				}

				ImGui::SameLine();

				{
					GUI::ScopedChild c{ "DeleteButton", ImGui::GetContentRegionAvail() };

					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));

					if (ImGui::Button("Delete List", ImGui::GetContentRegionAvail()))
					{
						GAME_THREAD_EXECUTE(
							DeleteVariationList(m_selectedVariationIndex);
						);
					}

					ImGui::PopStyleColor(3);
				}
			}
		}
	}
}