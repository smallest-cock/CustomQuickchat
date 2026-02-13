#include "pch.h"
#include "CustomQuickchat.hpp"
#include "util/Macros.hpp"
#include "util/Instances.hpp"
#include "Cvars.hpp"
#include "Keys.hpp"
#include <ModUtils/gui/GuiTools.hpp>
#include "components/LobbyInfo.hpp"
#ifdef USE_SPEECH_TO_TEXT
#include "components/SpeechToText.hpp"
#endif

// ##############################################################################################################
// #############################################  PLUGIN SETTINGS  ##############################################
// ##############################################################################################################

void CustomQuickchat::RenderSettings() {
	auto enabled_cvar = getCvar(Cvars::enabled);
	if (!enabled_cvar)
		return;

	const float contentHeight = ImGui::GetContentRegionAvail().y - FOOTER_HEIGHT; // available height after accounting for footer

	{
		GUI::ScopedChild c{"PluginSettingsSection", ImVec2(0, contentHeight)};

		GUI::alt_settings_header(h_label.c_str(), plugin_version_display, gameWrapper);

		bool enabled = enabled_cvar.getBoolValue();
		if (ImGui::Checkbox("Enabled", &enabled)) {
			runCommand(Commands::toggleEnabled);
		}

		if (enabled) {
			GUI::Spacing(4);

			// general settings
			if (ImGui::CollapsingHeader("General settings", ImGuiTreeNodeFlags_None))
				display_generalSettings();

			// chat timeouts
			if (ImGui::CollapsingHeader("Chat timeout settings", ImGuiTreeNodeFlags_None))
				display_chatTimeoutSettings();

			// speech-to-text
			if (ImGui::CollapsingHeader("Speech-to-text settings", ImGuiTreeNodeFlags_None)) {
#ifdef USE_SPEECH_TO_TEXT
				SpeechToText.display_settings();
#else
				GUI::Spacing(4);
				ImGui::Text(
				    "This version of the plugin doesnt support speech-to-text. You can find that version on the github Releases page:");
				GUI::Spacing(2);
				GUI::ClickableLink("Releases", "https://github.com/smallest-cock/CustomQuickchat/releases/latest", ImVec4(1, 1, 0, 1));
				GUI::Spacing(2);
#endif
			}

			// last chat
			if (ImGui::CollapsingHeader("Last chat settings", ImGuiTreeNodeFlags_None))
				LobbyInfo.display_settings();

			GUI::Spacing(8);

			if (ImGui::Button("Send a test chat"))
				GAME_THREAD_EXECUTE({ sendChat("this is a test...", EChatChannel::EChatChannel_Match); });

			GUI::Spacing(8);

			// open bindings window button
			std::string openMenuCmd = "togglemenu " + GetMenuName();
			if (ImGui::Button("Open Bindings Menu"))
				GAME_THREAD_EXECUTE({ cvarManager->executeCommand(openMenuCmd); }, openMenuCmd);

			GUI::Spacing(2);

			ImGui::Text("or bind this command:  ");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(200.0f);
			ImGui::InputText("##openMenuCommand", &openMenuCmd, ImGuiInputTextFlags_ReadOnly);
			GUI::CopyButton("Copy", openMenuCmd.c_str());
		}
	}

	GUI::alt_settings_footer("Need help? Join the Discord", "https://discord.gg/d5ahhQmJbJ");
}

void CustomQuickchat::display_generalSettings() {
	auto sequenceTimeWindow_cvar         = getCvar(Cvars::sequenceTimeWindow);
	auto minBindingDelay_cvar            = getCvar(Cvars::minBindingDelay);
	auto overrideDefaultQuickchats_cvar  = getCvar(Cvars::overrideDefaultQuickchats);
	auto blockDefaultQuickchats_cvar     = getCvar(Cvars::blockDefaultQuickchats);
	auto disablePostMatchQuickchats_cvar = getCvar(Cvars::disablePostMatchQuickchats);
	auto removeTimestamps_cvar           = getCvar(Cvars::removeTimestamps);
	auto randomizeSarcasm_cvar           = getCvar(Cvars::randomizeSarcasm);
	// auto uncensorChats_cvar              = getCvar(Cvars::uncensorChats);

	GUI::Spacing(2);

	bool overrideDefaultQuickchats = overrideDefaultQuickchats_cvar.getBoolValue();
	if (ImGui::Checkbox("Override default quickchats", &overrideDefaultQuickchats))
		overrideDefaultQuickchats_cvar.setValue(overrideDefaultQuickchats);
	GUI::ToolTip("Block default quickchat if it conflicts with a custom chat (prevents both chats from being sent)");

	bool blockDefaultQuickchats = blockDefaultQuickchats_cvar.getBoolValue();
	if (ImGui::Checkbox("Block all default quickchats", &blockDefaultQuickchats))
		blockDefaultQuickchats_cvar.setValue(blockDefaultQuickchats);
	GUI::ToolTip("Block all default quickchats from being sent... without having to unbind them :)");

	bool disablePostMatchQuickchats = disablePostMatchQuickchats_cvar.getBoolValue();
	if (ImGui::Checkbox("Disable custom quickchats in post-match screen", &disablePostMatchQuickchats))
		disablePostMatchQuickchats_cvar.setValue(disablePostMatchQuickchats);
	GUI::ToolTip("Prevents your custom chats from overriding default post-match chats like 'gg'");

	bool randomizeSarcasm = randomizeSarcasm_cvar.getBoolValue();
	if (ImGui::Checkbox("Randomize sarcasm effect", &randomizeSarcasm))
		randomizeSarcasm_cvar.setValue(randomizeSarcasm);

	bool removeTimestamps = removeTimestamps_cvar.getBoolValue();
	if (ImGui::Checkbox("Remove chat timestamps", &removeTimestamps))
		removeTimestamps_cvar.setValue(removeTimestamps);

	// bool uncensorChats = uncensorChats_cvar.getBoolValue();
	// if (ImGui::Checkbox("Uncensor chats", &uncensorChats))
	// 	uncensorChats_cvar.setValue(uncensorChats);

	GUI::Spacing(2);

	// sequence max time window
	float sequenceTimeWindow = sequenceTimeWindow_cvar.getFloatValue();
	if (ImGui::SliderFloat("Button sequence time window", &sequenceTimeWindow, 0.0f, 10.0f, "%.1f seconds"))
		sequenceTimeWindow_cvar.setValue(sequenceTimeWindow);

	// min delay between bindings
	float minBindingDelay = minBindingDelay_cvar.getFloatValue();
	if (ImGui::SliderFloat("Minimum delay between chats", &minBindingDelay, 0.01f, 0.5f, "%.2f seconds"))
		minBindingDelay_cvar.setValue(minBindingDelay);
	GUI::ToolTip("can help prevent accidental chats... but also affects chat spamming speed");

	GUI::Spacing(2);
}

void CustomQuickchat::display_chatTimeoutSettings() {
	// auto disableChatTimeout_cvar      = getCvar(Cvars::disableChatTimeout);
	auto useCustomChatTimeoutMsg_cvar = getCvar(Cvars::useCustomChatTimeoutMsg);
	auto customChatTimeoutMsg_cvar    = getCvar(Cvars::customChatTimeoutMsg);

	GUI::Spacing(2);

	// bool disableChatTimeout = disableChatTimeout_cvar.getBoolValue();
	// if (ImGui::Checkbox("Disable chat timeout (freeplay)", &disableChatTimeout))
	// 	disableChatTimeout_cvar.setValue(disableChatTimeout);

	bool useCustomChatTimeoutMsg = useCustomChatTimeoutMsg_cvar.getBoolValue();
	if (ImGui::Checkbox("Custom chat timeout message", &useCustomChatTimeoutMsg))
		useCustomChatTimeoutMsg_cvar.setValue(useCustomChatTimeoutMsg);

	if (useCustomChatTimeoutMsg) {
		GUI::Spacing(2);

		std::string customChatTimeoutMsg = customChatTimeoutMsg_cvar.getStringValue();
		if (ImGui::InputText("Chat timeout message", &customChatTimeoutMsg))
			customChatTimeoutMsg_cvar.setValue(customChatTimeoutMsg);
		GUI::ToolTip("TIP: use [Time] in your message to include the timeout seconds");
	}

	GUI::Spacing(2);
}

// ##############################################################################################################
// ###############################################  PLUGIN WINDOW  ##############################################
// ##############################################################################################################

void CustomQuickchat::RenderWindow() {
	ImGui::BeginTabBar("CQCTabs");

	if (ImGui::BeginTabItem("Bindings")) {
		display_bindingsList();
		ImGui::SameLine();
		display_bindingDetails();

		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Word Variations")) {
		display_variationListList();
		ImGui::SameLine();
		display_variationListDetails();

		ImGui::EndTabItem();
	}
}

void CustomQuickchat::display_bindingsList() {
	{
		GUI::ScopedChild c{"SelectOrCreateBinding", ImVec2(300, 0), true};

		auto availSpace = ImGui::GetContentRegionAvail();
		{
			GUI::ScopedChild c{"BindingsList", ImVec2(0, availSpace.y * 0.9f)};

			ImGui::TextUnformatted("Bindings:");
			ImGui::Separator();

			auto bindingsSize = m_bindings.size();
			for (int i = 0; i < bindingsSize; ++i) {
				const auto &binding = m_bindings[i];

				GUI::ScopedID id{&binding};

				if (ImGui::Selectable(binding->chat.c_str(), i == m_selectedBindingIndex))
					m_selectedBindingIndex = i;
			}
		}

		GUI::Spacing(2);

		{
			GUI::ScopedChild c{"AddBinding"};

			if (ImGui::Button("Add New Binding", ImGui::GetContentRegionAvail())) {
				addEmptyBinding();
				m_selectedBindingIndex = m_bindings.empty() ? 0 : m_bindings.size() - 1;
			}
		}
	}
}

void CustomQuickchat::display_bindingDetails() {
	// static float childScale = 0.85f;	// <--- for testing

	{
		GUI::ScopedChild c{"EverythingAboutBinding"};

		{
			GUI::ScopedChild c{"BindingInfo"};

			if (m_bindings.empty()) {
				GUI::Spacing(4);
				ImGui::TextUnformatted("Add a binding...");
				return;
			}

			auto &selectedBinding = m_bindings[m_selectedBindingIndex];
			if (!selectedBinding)
				return;

			ImGui::TextUnformatted(selectedBinding->chat.c_str());

			{
				GUI::ScopedChild c{"ChatDetails", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.3f), true};

				display_bindingChatDetails(selectedBinding);

				// ImGui::SliderFloat("child scale", &childScale, 0.0f, 1.0f, "%.2f");	// <--- for testing
			}

			{
				GUI::ScopedChild c{"BindingTriggerDetails", ImVec2(0, 0), true};

				{
					// GUI::ScopedChild c{ "ActualTriggerSection", ImVec2(0, ImGui::GetContentRegionAvail().y * childScale) };
					GUI::ScopedChild c{"ActualTriggerSection", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.85f)};

					display_bindingTriggerDetails(selectedBinding);
				}

				GUI::Spacing(2);

				{
					GUI::ScopedChild c{"ButtonsSection"};

					{
						GUI::ScopedChild c{"SaveOrDelete"};

						{
							GUI::ScopedChild c{"SaveButton", ImVec2(ImGui::GetContentRegionAvailWidth() * 0.75f, 0)};

							if (ImGui::Button("Save", ImGui::GetContentRegionAvail())) {
								updateBindingsData();

								GAME_THREAD_EXECUTE({
									std::string successMsg = writeBindingsToJson() ? "Bindings saved!" : "Unable to save bindings...";
									Instances.spawnNotification("custom quickchat", successMsg, 3, true);

									determineQuickchatLabels();

									auto *chat = Instances.getInstanceOf<UGFxData_Chat_TA>();
									if (chat)
										applyAllCustomQcLabelsToUi(chat);
								});
							}
						}

						ImGui::SameLine();

						{
							GUI::ScopedChild c{"DeleteButton", ImGui::GetContentRegionAvail()};

							ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));

							if (ImGui::Button("Delete Binding", ImGui::GetContentRegionAvail()))
								GAME_THREAD_EXECUTE({ deleteBinding(m_selectedBindingIndex); });

							ImGui::PopStyleColor(3);
						}
					}
				}
			}
		}
	}
}

void CustomQuickchat::display_bindingChatDetails(const std::shared_ptr<Binding> &selectedBinding) {
	if (!selectedBinding)
		return;

	if (ImGui::Checkbox("Enabled", &selectedBinding->bEnabled))
		GAME_THREAD_EXECUTE({ determineQuickchatLabels(); });

	GUI::Spacing(2);

	ImGui::TextColored(GUI::Colors::Yellow, "Chat:");

	GUI::Spacing(4);

	ImGui::InputTextWithHint("Chat", "let me cook", &selectedBinding->chat);

	GUI::Spacing(2);

	if (ImGui::BeginCombo("Chat mode", g_possibleChatModes[static_cast<int>(selectedBinding->chatMode)].c_str())) {
		for (int i = 0; i < g_possibleChatModes.size(); ++i) {
			GUI::ScopedID id{i};

			const std::string &chatModeStr = g_possibleChatModes[i];
			if (ImGui::Selectable(chatModeStr.c_str(), static_cast<int>(selectedBinding->chatMode) == i))
				selectedBinding->chatMode = static_cast<EChatChannel>(i);
		}
		ImGui::EndCombo();
	}
}

void CustomQuickchat::display_bindingTriggerDetails(const std::shared_ptr<Binding> &selectedBinding) {
	if (!selectedBinding)
		return;

	ImGui::TextColored(GUI::Colors::Yellow, "How it's triggered:");

	GUI::Spacing(4);

	if (ImGui::BeginCombo("Binding type", g_possibleBindingTypes[static_cast<int>(selectedBinding->bindingType)].c_str())) {
		for (int i = 0; i < g_possibleBindingTypes.size(); ++i) {
			GUI::ScopedID id{i};

			const std::string &bindingTypeStr = g_possibleBindingTypes[i];
			if (ImGui::Selectable(bindingTypeStr.c_str(), static_cast<int>(selectedBinding->bindingType) == i))
				selectedBinding->bindingType = static_cast<EBindingType>(i);
		}
		ImGui::EndCombo();
	}

	GUI::Spacing(4);

	bool notEnoughButtonsForSequence = selectedBinding->bindingType == EBindingType::Sequence && selectedBinding->buttons.size() < 2;
	if (notEnoughButtonsForSequence) {
		ImGui::TextColored(GUI::Colors::Yellow, "*** Button sequence bindings require at least 2 buttons! ***");
		GUI::Spacing(2);
	}

	// buttons
	for (int i = 0; i < selectedBinding->buttons.size(); ++i) {
		GUI::ScopedID iID{i};

		std::string &buttonStr = selectedBinding->buttons[i];
		std::string  label     = "Button " + std::to_string(i + 1);

		char searchBuffer[128] = ""; // text buffer for search input

		if (ImGui::BeginSearchableCombo(label.c_str(), buttonStr.c_str(), searchBuffer, sizeof(searchBuffer), "search...")) {
			std::string searchQuery = Format::ToLower(searchBuffer);

			for (int j = 0; j < possibleKeyNames.size(); ++j) {
				GUI::ScopedID jID{j};

				const std::string &keyNameStr      = possibleKeyNames[j];
				const std::string  keyNameStrLower = Format::ToLower(keyNameStr);

				if (!searchQuery.empty()) // only render option if there's text in search box & it matches the key name
				{
					if (keyNameStrLower.find(searchQuery) != std::string::npos) {
						if (ImGui::Selectable(keyNameStr.c_str(), keyNameStr == buttonStr))
							buttonStr = keyNameStr;
					}
				} else // if there's no text in search box, render all possible key options
				{
					if (ImGui::Selectable(keyNameStr.c_str(), keyNameStr == buttonStr))
						buttonStr = keyNameStr;
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SameLine();

		if (ImGui::Button("Remove"))
			selectedBinding->buttons.erase(selectedBinding->buttons.begin() + i);
	}

	if (notEnoughButtonsForSequence) {
		GUI::Spacing(2);
		ImGui::TextColored(GUI::Colors::Yellow, "Please add a button ...");
	}

	GUI::Spacing(2);

	if (ImGui::Button("Add New Button"))
		selectedBinding->buttons.emplace_back("");
}

void CustomQuickchat::display_variationListList() {
	{
		GUI::ScopedChild c{"VariationListSection", ImVec2(300, 0), true};

		{
			GUI::ScopedChild c{"VariationListList", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.9f)};

			for (int i = 0; i < m_variations.size(); ++i) {
				VariationList &list = m_variations[i];
				GUI::ScopedID  id{&list};

				if (ImGui::Selectable(list.listName.c_str(), i == m_selectedVariationIndex))
					m_selectedVariationIndex = i;
			}
		}

		GUI::Spacing(2);

		{
			GUI::ScopedChild c{"AddVariationListSection"};

			if (ImGui::Button("Add New List", ImGui::GetContentRegionAvail())) {
				addEmptyVariationList();
				m_selectedVariationIndex = m_variations.empty() ? 0 : m_variations.size() - 1;
			}
		}
	}
}

void CustomQuickchat::display_variationListDetails() {
	// static float childScale = 0.9f;

	{
		GUI::ScopedChild c{"VariationListData", ImVec2(0, 0), true};

		if (m_variations.empty()) {
			GUI::Spacing(4);
			ImGui::TextUnformatted("add a word variation list...");
			return;
		}

		VariationList &selectedVariation = m_variations[m_selectedVariationIndex];

		{
			GUI::ScopedChild c{"VariationView", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.9f)};
			// GUI::ScopedChild c{ "VariationView", ImVec2(0, ImGui::GetContentRegionAvail().y * childScale)};

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
				selectedVariation.nextUsableIndex = 0;

			// ImGui::SliderFloat("child scale", &childScale, 0.0f, 1.0f, "%.2f");	// <--- for testing
		}

		GUI::Spacing(2);

		{
			GUI::ScopedChild c{"ButtonsSection"};

			{
				GUI::ScopedChild c{"SaveOrDelete"};

				{
					GUI::ScopedChild c{"SaveButton", ImVec2(ImGui::GetContentRegionAvailWidth() * 0.75f, 0)};

					if (ImGui::Button("Save", ImGui::GetContentRegionAvail())) {
						selectedVariation.updateDataFromUnparsedString();
						writeVariationsToJson();

						GAME_THREAD_EXECUTE({ Instances.spawnNotification("custom quickchat", "Variations saved!", 3); });
					}
				}

				ImGui::SameLine();

				{
					GUI::ScopedChild c{"DeleteButton", ImGui::GetContentRegionAvail()};

					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));

					if (ImGui::Button("Delete List", ImGui::GetContentRegionAvail()))
						GAME_THREAD_EXECUTE({ deleteVariationList(m_selectedVariationIndex); });

					ImGui::PopStyleColor(3);
				}
			}
		}
	}
}
