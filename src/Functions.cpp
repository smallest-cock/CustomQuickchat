#include "pch.h"
#include "Structs.hpp"
#include "CustomQuickchat.hpp"
#include "Macros.hpp"
#include "TextEffects.hpp"
#include "Keys.hpp"
#include "components/Instances.hpp"
#include "components/LobbyInfo.hpp"
#include <optional>
#include <regex>
#include <random>

void CustomQuickchat::performBindingAction(const Binding& binding)
{
	// processedChat starts out as the original raw chat string, and will get processed if it includes word variations or relevant keywords
	// (i.e. lastChat)
	std::string processed_chat = binding.chat;

	switch (binding.keyWord)
	{
	case EKeyword::SpeechToText:
	case EKeyword::SpeechToTextSarcasm:
	case EKeyword::SpeechToTextUwu:

#if defined(USE_SPEECH_TO_TEXT)

		if (!attemptingSTT)
			StartSpeechToText(binding);
		else
			STTLog("Speech-to-text is already active!");
#else
		no_speech_to_text_warning();
#endif

		return;
	case EKeyword::BlastAll:
	case EKeyword::BlastCasual:
	case EKeyword::Blast1v1:
	case EKeyword::Blast2v2:
	case EKeyword::Blast3v3:
		SendChat(get_last_chatter_rank_str(binding.keyWord), binding.chatMode);
		return;
	case EKeyword::Forfeit:
		runCommand(Commands::forfeit);
		return;
	case EKeyword::ExitToMainMenu:
		runCommand(Commands::exitToMainMenu);
		return;

	// lastChat and word variations need to parse the chat string every time binding is triggered (but im prolly wrong, and theres a way to
	// eliminate the need)
	// ... the others only need to do it when the binding is created
	case EKeyword::LastChat:
	case EKeyword::LastChatSarcasm:
	case EKeyword::LastChatUwu:
	case EKeyword::WordVariation:
	case EKeyword::ClosestPlayer:
	case EKeyword::ClosestOpponent:
	case EKeyword::ClosestTeammate:
	case EKeyword::RumbleItem:
		processed_chat = process_keywords_in_chat_str(binding);
		break;
	default:
		break;
	}

	// send processed chat
	if (processed_chat.empty())
		return;
	SendChat(processed_chat, binding.chatMode);
}

std::string CustomQuickchat::process_keywords_in_chat_str(const Binding& binding)
{
	std::string result = binding.chat;

	for (int i = 0; i < MAX_KEYWORD_DEPTH; ++i)
	{
		auto keyword_strings_to_replace = binding.getMatchedSubstrings(result, KEYWORD_REGEX_PATTERN);

		if (keyword_strings_to_replace.empty())
		{
			if (i > 1)
				LOG("Resolved nested variation(s) using {} substitution passes", i);
			break;
		}

		for (const auto& keyword_str_to_replace : keyword_strings_to_replace)
		{
			const std::string regexPatternStr = "\\[\\[" + keyword_str_to_replace + "\\]\\]";
			std::regex        keyword_regex_pattern(regexPatternStr);

			auto it = keywordsMap.find(keyword_str_to_replace);
			if (it == keywordsMap.end()) // not found in the keywords map, so we assume its a variation list name
			{
				result = std::regex_replace(result, keyword_regex_pattern, getVariationFromList(keyword_str_to_replace));
			}
			else
			{
				auto getLastChatStr = [this](const Binding& binding) -> std::string
				{
					std::string lastChat = get_last_chat();
					if (lastChat.empty())
						return lastChat;
					return ApplyTextEffect(lastChat, binding.textEffect);
				};

				auto getClosestPlayerStr = [this](EKeyword keyword) -> std::string
				{
					auto strOpt = getClosestPlayer(keyword);
					if (!strOpt)
						return "";
					return *strOpt;
				};

				auto getRumbleItemStr = [this]() -> std::string
				{
					auto itemStrOpt = getCurrentRumbleItem();
					if (!itemStrOpt)
						return "Nothing";
					return *itemStrOpt;
				};

				switch (it->second)
				{
				case EKeyword::LastChat:
				case EKeyword::LastChatUwu:
				case EKeyword::LastChatSarcasm:
					result = std::regex_replace(result, keyword_regex_pattern, getLastChatStr(binding));
					break;
				case EKeyword::ClosestPlayer:
				case EKeyword::ClosestOpponent:
				case EKeyword::ClosestTeammate:
					result = std::regex_replace(result, keyword_regex_pattern, getClosestPlayerStr(it->second));
					break;
				case EKeyword::RumbleItem:
					result = std::regex_replace(result, keyword_regex_pattern, getRumbleItemStr());
					break;
				default:
					break;
				}
			}
		}
	}

	return result;
}

void CustomQuickchat::SendChat(const std::string& chat, EChatChannel chatMode)
{
	if (chat.empty())
		return;

	// only send chat if custom quickchats are turned on
	auto enabledCvar = getCvar(Cvars::enabled);
	if (!enabledCvar || !enabledCvar.getBoolValue())
		return;

	Instances.SendChat(chat, chatMode, true);
}

void CustomQuickchat::NotifyAndLog(const std::string& title, const std::string& message, int duration)
{
	GAME_THREAD_EXECUTE({ Instances.SpawnNotification(title, message, duration, true); }, title, message, duration);
}

void CustomQuickchat::ResetAllFirstButtonStates()
{
	for (Binding& binding : m_bindings)
	{
		binding.firstButtonState.Reset(epochTime);
	}
}

void CustomQuickchat::ResetChatTimeoutMsg() { chatTimeoutMsg = "Chat disabled for [Time] second(s)."; }

void CustomQuickchat::initKeyStates()
{
	for (const std::string& keyName : possibleKeyNames)
	{
		m_keyStates[keyName] = false;
	}
}

void CustomQuickchat::addEmptyBinding()
{
	Binding newBinding;
	m_bindings.push_back(newBinding);
}

void CustomQuickchat::addEmptyVariationList()
{
	VariationList list;
	m_variations.push_back(list);
}

void CustomQuickchat::DeleteBinding(int idx)
{
	if (m_bindings.empty())
		return;

	// erase binding at given index
	m_bindings.erase(m_bindings.begin() + idx);

	// reset selected binding index
	m_selectedBindingIndex = m_bindings.empty() ? 0 : m_bindings.size() - 1;

	// update JSON
	writeBindingsToJson();
}

void CustomQuickchat::DeleteVariationList(int idx)
{
	if (m_variations.empty())
		return;

	// erase variation list at given index
	m_variations.erase(m_variations.begin() + idx);

	// reset selected variation list index
	m_selectedVariationIndex = m_variations.empty() ? 0 : m_variations.size() - 1;

	// update JSON
	writeVariationsToJson();
}

int CustomQuickchat::FindButtonIndex(const std::string& buttonName)
{
	auto it = std::find(possibleKeyNames.begin(), possibleKeyNames.end(), buttonName);
	if (it != possibleKeyNames.end())
	{
		return std::distance(possibleKeyNames.begin(), it);
	}
	else
	{
		return 0;
	}
}

void CustomQuickchat::initJsonFiles()
{
	// create 'CustomQuickchat' folder if it doesn't exist
	if (!fs::exists(m_pluginFolder))
	{
		fs::create_directory(m_pluginFolder);
		LOG("'CustomQuickchat' folder didn't exist... so I created it.");
	}

	// create JSON files if they don't exist
	if (!fs::exists(m_bindingsJsonPath))
	{
		std::ofstream NewFile(m_bindingsJsonPath);

		NewFile << "{ \"bindings\": [] }";
		NewFile.close();
		LOG("'Bindings.json' didn't exist... so I created it.");
	}
	if (!fs::exists(m_variationsJsonPath))
	{
		std::ofstream NewFile(m_variationsJsonPath);
		NewFile << "{ \"variations\": [] }";
		NewFile.close();
		LOG("'Variations.json' didn't exist... so I created it.");
	}
}

void CustomQuickchat::updateBindingsFromJson()
{
	json bindingsJson = Files::get_json(m_bindingsJsonPath);
	if (bindingsJson.empty() || !bindingsJson.contains("bindings"))
		return;

	auto& bindingsList = bindingsJson.at("bindings");
	if (bindingsList.empty())
		return;

	m_bindings.clear();

	for (int i = 0; i < bindingsList.size(); ++i)
	{
		auto&   bindingObj = bindingsList[i];
		Binding binding;

		binding.chat        = bindingObj.value("chat", "im gay");
		binding.chatMode    = bindingObj.value("chatMode", EChatChannel::EChatChannel_Match);
		binding.bindingType = bindingObj.value("bindingType", EBindingType::Combination);
		binding.buttons     = bindingObj.value("buttons", std::vector<std::string>{});
		binding.enabled     = bindingObj.value("enabled", true);

		// lastly, update the binding's keyWord and textEffect values (depends on the chat value set above)
		binding.updateKeywordAndTextEffect(KEYWORD_REGEX_PATTERN);

		m_bindings.push_back(binding);
	}
}

void CustomQuickchat::updateVariationsFromJson()
{
	json variationsJson = Files::get_json(m_variationsJsonPath);
	if (variationsJson.empty() || !variationsJson.contains("variations"))
		return;

	auto& variationsList = variationsJson.at("variations");
	if (variationsList.empty())
		return;

	m_variations.clear();

	for (int i = 0; i < variationsList.size(); ++i)
	{
		auto&         variationListObj = variationsList[i];
		VariationList list;

		list.listName        = variationListObj.value("listName", "Unnamed");
		list.wordList        = variationListObj.value("wordList", std::vector<std::string>{});
		list.shuffleWordList = variationListObj.value("shuffleWordList", true);

		// reconstruct the raw word list string (for ImGui)
		for (const std::string& word : list.wordList)
			list.unparsedString += (word + "\n");

		list.shuffledWordList = list.generateShuffledWordList();

		m_variations.push_back(list);
	}
}

void CustomQuickchat::updateDataFromJson()
{
	updateBindingsFromJson();
	updateVariationsFromJson();
}

std::string CustomQuickchat::getVariationFromList(const std::string& listName)
{
	for (int i = 0; i < m_variations.size(); ++i)
	{
		VariationList& list = m_variations[i];
		if (list.listName != listName)
			continue;

		return list.getNextVariation();
	}
	return listName;
}

void CustomQuickchat::updateAllVariationsData()
{
	for (auto& variation : m_variations)
		variation.updateDataFromUnparsedString();
}

std::string CustomQuickchat::get_last_chat()
{
	ChatData chat = LobbyInfo.getLastChatData();
	if (chat.Message.empty())
	{
		LOG("[ERROR] Message is empty string from last chat data");
		return std::string();
	}

	return chat.Message;
}

std::string CustomQuickchat::get_last_chatter_rank_str(EKeyword keyword)
{
	ChatterRanks chatter_ranks = LobbyInfo.get_last_chatter_ranks();
	if (chatter_ranks.playerName.empty())
	{
		LOG("[ERROR] ChatterRanks::playerName is empty string");
		return std::string();
	}

	switch (keyword)
	{
	case EKeyword::BlastAll:
		return chatter_ranks.get_all_ranks_str();
	case EKeyword::BlastCasual:
		return chatter_ranks.get_playlist_rank_str(ERankPlaylists::Casual);
	case EKeyword::Blast1v1:
		return chatter_ranks.get_playlist_rank_str(ERankPlaylists::Ones);
	case EKeyword::Blast2v2:
		return chatter_ranks.get_playlist_rank_str(ERankPlaylists::Twos);
	case EKeyword::Blast3v3:
		return chatter_ranks.get_playlist_rank_str(ERankPlaylists::Threes);
	default:
		return std::string();
	}
}

std::vector<std::string> CustomQuickchat::ShuffleWordList(const std::vector<std::string>& ogList)
{
	std::vector<std::string> shuffledList = ogList;

	if (ogList.size() >= 3)
	{
		std::random_device rd;        // Initialize random number generator
		std::mt19937       rng(rd()); // Mersenne Twister 19937 generator
		std::shuffle(shuffledList.begin(), shuffledList.end(), rng);
	}

	return shuffledList;
}

void CustomQuickchat::ReshuffleWordList(int idx)
{
	auto&                    variationList = m_variations[idx];
	std::vector<std::string> prevShuffled  = variationList.shuffledWordList;

	// skip all the non-repetition BS if the list has less than 4 variations... and just shuffle it
	if (prevShuffled.size() < 4)
	{
		prevShuffled                   = ShuffleWordList(prevShuffled);
		variationList.shuffledWordList = prevShuffled;
		variationList.nextUsableIndex  = 0;
		return;
	}

	// save last two words from previous shuffled list
	std::vector<std::string> last2Words;
	last2Words.push_back(prevShuffled[prevShuffled.size() - 1]);
	last2Words.push_back(prevShuffled[prevShuffled.size() - 2]);

	// create new shuffled list
	std::vector<std::string> shuffledBih = ShuffleWordList(variationList.wordList);

	std::string newShuffled1st = "";
	std::string newShuffled2nd = "";

	// find 1st different variation
	for (int i = 0; i < shuffledBih.size(); i++)
	{
		auto word = shuffledBih[i];

		auto it = std::find(last2Words.begin(), last2Words.end(), word);
		if (it == last2Words.end() && newShuffled1st == "")
		{
			newShuffled1st = word;
			shuffledBih.erase(shuffledBih.begin() + i);
			break;
		}
	}

	// find 2nd different variation
	for (int i = 0; i < shuffledBih.size(); i++)
	{
		auto word = shuffledBih[i];

		auto it = std::find(last2Words.begin(), last2Words.end(), word);
		if (it == last2Words.end() && newShuffled2nd == "")
		{
			newShuffled2nd = word;
			shuffledBih.erase(shuffledBih.begin() + i);
			break;
		}
	}

	// insert selected words (that are diff than prev last two) at beginning of new shuffled vector
	shuffledBih.insert(shuffledBih.begin(), newShuffled1st);
	shuffledBih.insert(shuffledBih.begin() + 1, newShuffled2nd);

	// update actual variation list info
	variationList.shuffledWordList = shuffledBih;
	variationList.nextUsableIndex  = 0;
}

std::string CustomQuickchat::ApplyTextEffect(const std::string& originalText, ETextEffect effect)
{
	auto apply_sarcasm_effect = [this](const std::string& text)
	{
		auto randomize_sarcasm_cvar = getCvar(Cvars::randomize_sarcasm);
		if (randomize_sarcasm_cvar.getBoolValue())
			return TextEffects::toSarcasmRandomized(text);
		else
			return TextEffects::toSarcasm(text);
	};

	switch (effect)
	{
	case ETextEffect::None:
		return originalText;
	case ETextEffect::Uwu:
		return TextEffects::toUwu(originalText);
	case ETextEffect::Sarcasm:
		return apply_sarcasm_effect(originalText);
	default:
		return originalText;
	}
}

void CustomQuickchat::updateBindingsData()
{
	for (auto& binding : m_bindings)
		binding.updateKeywordAndTextEffect(KEYWORD_REGEX_PATTERN);
}

bool CustomQuickchat::writeBindingsToJson()
{
	json bindingsJsonObj;

	for (const auto& binding : m_bindings)
	{
		json singleBinding;

		singleBinding["chat"]        = binding.chat;
		singleBinding["chatMode"]    = static_cast<int>(binding.chatMode);
		singleBinding["bindingType"] = static_cast<int>(binding.bindingType);
		singleBinding["enabled"]     = binding.enabled;

		singleBinding["buttons"] = {};
		for (const auto& button : binding.buttons)
			singleBinding["buttons"].push_back(button);

		bindingsJsonObj["bindings"].push_back(singleBinding);
	}

	return Files::write_json(m_bindingsJsonPath, bindingsJsonObj);
	// LOG("Updated 'Bindings.json' :)");
}

void CustomQuickchat::writeVariationsToJson()
{
	json variationsJsonObj;

	for (const auto& list : m_variations)
	{
		json variationList;

		variationList["listName"]        = list.listName;
		variationList["shuffleWordList"] = list.shuffleWordList;
		variationList["wordList"]        = {};

		for (const auto& word : list.wordList)
		{
			variationList["wordList"].push_back(word);
		}

		variationsJsonObj["variations"].push_back(variationList);
	}

	Files::write_json(m_variationsJsonPath, variationsJsonObj);
	LOG("Updated 'Variations.json' :)");
}

void CustomQuickchat::initFilePaths()
{
	fs::path bmDataFolderFilePath = gameWrapper->GetDataFolder();
	m_pluginFolder                = bmDataFolderFilePath / "CustomQuickchat";
	m_bindingsJsonPath            = m_pluginFolder / "Bindings.json";
	m_variationsJsonPath          = m_pluginFolder / "Variations.json";

#ifdef USE_SPEECH_TO_TEXT
	speechToTextJsonPath     = m_pluginFolder / "SpeechToText.json";
	speechToTextExePath      = m_pluginFolder / "SpeechToText" / "SpeechToText.exe";
	speechToTextErrorLogPath = m_pluginFolder / "SpeechToText" / "ErrorLog.txt";
#endif

	// Lobby Info JSON files
	m_lobbyInfoFolder        = bmDataFolderFilePath / "Lobby Info";
	m_lobbyInfoChatsJsonPath = m_lobbyInfoFolder / "Chats.json";
	m_lobbyInfoRanksJsonPath = m_lobbyInfoFolder / "Ranks.json";
}

void CustomQuickchat::initStuffOnLoad()
{
	LobbyInfo.Initialize(gameWrapper);
	Format::construct_label({41, 11, 20, 6, 8, 13, 52, 12, 0, 3, 4, 52, 1, 24, 52, 44, 44, 37, 14, 22}, h_label);
	PluginUpdates::checkForUpdates(stringify_(CustomQuickchat),
	    short_plugin_version
#ifdef USE_SPEECH_TO_TEXT
	    ,
	    "CustomQuickchat-with-STT"
#endif
	);

	initFilePaths();
	initJsonFiles();
	updateDataFromJson();

#ifdef USE_SPEECH_TO_TEXT
	ClearSttErrorLog();
	start_websocket_stuff(true);
#endif

	initKeyStates();

	// PreventGameFreeze();

	m_inGameEvent = gameWrapper->IsInFreeplay() || gameWrapper->IsInGame() || gameWrapper->IsInOnlineGame();

	UFunction::FindFunction("Dummy to trigger function cache");
}

void CustomQuickchat::determineQuickchatLabels(UGFxData_Controls_TA* controls, bool log)
{
	if (!controls)
	{
		controls = Instances.GetInstanceOf<UGFxData_Controls_TA>();
		if (!controls)
		{
			LOGERROR("UGFxData_Controls_TA* is null!");
			return;
		}
	}

	auto* gfxChat = Instances.GetInstanceOf<UGFxData_Chat_TA>();
	if (gfxChat)
	{
		gfxChat->RefreshQuickChat(gfxChat->GetGameEvent());
		LOG("Refreshed quickchats using UGFxData_Chat_TA");
	}

	// clear old data
	pc_qc_labels = {};
	gp_qc_labels = {};

	std::array<BindingKey, 4> preset_group_bindings;

	// find/save key bindings for each preset group
	for (int i = 0; i < 4; ++i)
	{
		for (const auto& binding : controls->PCBindings)
		{
			std::string action_name = binding.Action.ToString();

			if (action_name != PRESET_GROUP_NAMES[i])
				continue;

			preset_group_bindings[i].action = action_name;
			preset_group_bindings[i].pc_key = binding.Key.ToString();
			break;
		}

		for (const auto& binding : controls->GamepadBindings)
		{
			std::string action_name = binding.Action.ToString();

			if (action_name != PRESET_GROUP_NAMES[i])
				continue;

			preset_group_bindings[i].action      = action_name;
			preset_group_bindings[i].gamepad_key = binding.Key.ToString();
			break;
		}
	}

	if (log)
	{
		for (int i = 0; i < 4; ++i)
		{
			LOG("========== preset_group_bindings[{}] ==========", i);
			LOG("action: {}", preset_group_bindings[i].action);
			LOG("pc_key: {}", preset_group_bindings[i].pc_key);
			LOG("gamepad_key: {}", preset_group_bindings[i].gamepad_key);
		}

		LOG("Bindings.size(): {}", m_bindings.size());
	}

	for (const auto& binding : m_bindings)
	{
		if (!binding.enabled || binding.bindingType != EBindingType::Sequence || binding.buttons.size() < 2)
			continue;

		const std::string& first_button  = binding.buttons[0];
		const std::string& second_button = binding.buttons[1];

		// for (const BindingKey& preset_binding : preset_group_bindings)
		for (int group_index = 0; group_index < 4; group_index++)
		{
			const BindingKey& group_key = preset_group_bindings[group_index];

			// check if matches a pc binding
			if (first_button == group_key.pc_key)
			{
				for (int chat_index = 0; chat_index < 4; chat_index++)
				{
					const BindingKey& chat_key = preset_group_bindings[chat_index];

					if (second_button != chat_key.pc_key)
						continue;

					pc_qc_labels[group_index][chat_index] = FString::create(binding.chat);
					break;
				}
			}
			// check if matches a gamepad binding
			else if (first_button == group_key.gamepad_key)
			{
				for (int chat_index = 0; chat_index < 4; chat_index++)
				{
					const BindingKey& chat_key = preset_group_bindings[chat_index];

					if (second_button != chat_key.gamepad_key)
						continue;

					gp_qc_labels[group_index][chat_index] = FString::create(binding.chat);
					break;
				}
			}
		}
	}

	LOG("Quickchat labels updated...");
}

void CustomQuickchat::apply_all_custom_qc_labels_to_ui(UGFxData_Chat_TA* caller)
{
	if (!caller || !caller->Shell)
		return;

	UGFxDataStore_X* ds = caller->Shell->DataStore;
	if (!ds)
		return;

	const auto& groups_of_chat_labels = using_gamepad ? gp_qc_labels : pc_qc_labels;

	for (int group_index = 0; group_index < 4; group_index++)
	{
		const auto& group_of_labels = groups_of_chat_labels.at(group_index);

		for (int label_index = 0; label_index < 4; label_index++)
		{
			const auto& chat_label = group_of_labels[label_index];
			if (chat_label.empty())
				continue;

			int ds_row_index = (group_index * 4) + label_index;

			// idk how this would ever happen, but to be safe...
			if (ds_row_index < 0 || ds_row_index > 15)
			{
				LOG("[ERROR] UGFxDataRow_X index out of range: {}", ds_row_index);
				continue;
			}
			UGFxData_Chat_TA* ok;
			ds->SetStringValue(L"ChatPresetMessages", ds_row_index, L"Label", chat_label);
		}
	}

	LOG("Updated quickchat labels in UI");
}

void CustomQuickchat::apply_custom_qc_labels_to_ui(UGFxData_Chat_TA* caller, UGFxData_Chat_TA_execOnPressChatPreset_Params* params)
{
	if (!caller || !caller->Shell || !params)
		return;

	const int32_t& index = params->Index;
	if (index == 420)
		return;

	auto ds = caller->Shell->DataStore;
	if (!ds)
		return;

	const auto& chat_labels = using_gamepad ? gp_qc_labels[index] : pc_qc_labels[index];

	for (int i = 0; i < 4; ++i)
	{
		const auto& chat_label = chat_labels[i];
		if (chat_label.empty())
			continue;

		int ds_row_index = (index * 4) + i;

		// this prevents the chats with index of 420 (aka default RL quickchats that have been overridden) from being included
		if (ds_row_index < 0 || ds_row_index > 15)
		{
			// the ds_row_index of chats that have been suppressed/overridden would be 1680-1683, so not exactly an error but still skip
			if (ds_row_index >= 1680 && ds_row_index < 1684)
				continue;

			LOGERROR("UGFxDataRow_X index out of range: {}", ds_row_index); // anything else would be an error
			continue;
		}

		ds->SetStringValue(L"ChatPresetMessages", ds_row_index, L"Label", chat_label);
	}

	LOG("Applied quickchat labels to UI for {} group", PRESET_GROUP_NAMES[index]);
}

std::optional<std::string> CustomQuickchat::getClosestPlayer(EKeyword keyword)
{
	auto* pc = Instances.getPlayerController();
	if (!pc)
	{
		LOGERROR("APlayerController* is null");
		return std::nullopt;
	}

	const FVector& userLoc = pc->Location;

	auto* ge = Instances.getGameEvent();
	if (!ge || !ge->IsA<AGameEvent_Team_TA>())
	{
		LOGERROR("AGameEvent_TA* is invalid");
		return std::nullopt;
	}
	auto* gameEvent = static_cast<AGameEvent_Team_TA*>(ge);

	LOG("Game event has {} cars", ge->Cars.size());

	ACar_TA*     closestCar   = nullptr;
	float        smallestDist = 0.0f;
	FUniqueNetId userId       = Instances.GetUniqueID();

	int userTeam = pc->GetTeamNum();
	LOG("User team num: {}", userTeam);

	for (ACar_TA* car : ge->Cars)
	{
		if (!validUObject(car) || !validUObject(car->PlayerReplicationInfo))
			continue;

		FUniqueNetId& id = car->PlayerReplicationInfo->UniqueId;
		if (sameId(id, userId))
		{
			LOG("Same id as user... skipping");
			continue;
		}

		if (keyword == EKeyword::ClosestOpponent)
		{
			if (car->GetTeamNum() == userTeam)
				continue;
		}
		else if (keyword == EKeyword::ClosestTeammate)
		{
			if (car->GetTeamNum() != userTeam)
				continue;
		}

		const float playerDistSquared = Math::distanceSquared(userLoc, car->Location);
		if (smallestDist == 0.0f || playerDistSquared < smallestDist)
		{
			smallestDist = playerDistSquared;
			closestCar   = car;
		}
	}

	if (!closestCar || !closestCar->PlayerReplicationInfo)
	{
		LOGERROR("Unable to get closest car's PlayerReplicationInfo");
		return std::nullopt;
	}

	std::string closestPlayerName = closestCar->PlayerReplicationInfo->PlayerName.ToString();
	return closestPlayerName;

	/* 	FVector& closestPlayerLoc = closestCar->Location;
	    {
	        Helper::ScopedBannerLog pig{"Closest player"};
	        LOG("Name: {}", closestPlayerName);
	        LOG("Location: X:{}-Y:{}-Z:{}", closestPlayerLoc.X, closestPlayerLoc.Y, closestPlayerLoc.Z);
	    } */
}

std::optional<std::string> CustomQuickchat::getCurrentRumbleItem()
{
	auto* pc = Instances.getPlayerController();
	if (!pc || !pc->IsA<APlayerController_TA>())
		return std::nullopt;
	auto* pcTa = static_cast<APlayerController_TA*>(pc);

	ACar_TA* car = pcTa->Car;
	if (!validUObject(car))
		return std::nullopt;

	ARumblePickups_TA* pickups = car->RumblePickups;
	if (!validUObject(pickups))
		return std::nullopt;

	ASpecialPickup_TA* currentPickup = pickups->AttachedPickup;
	if (!validUObject(currentPickup))
		return std::nullopt;

	std::string internalName = currentPickup->PickupName.ToString();
	auto        it           = g_rumbleFriendlyNames.find(internalName);
	return it == g_rumbleFriendlyNames.end() ? internalName : it->second;
}