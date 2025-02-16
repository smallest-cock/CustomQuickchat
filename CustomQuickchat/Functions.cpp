#include "pch.h"
#include "CustomQuickchat.h"
#include <regex>



void CustomQuickchat::PerformBindingAction(const Binding& binding)
{
	// processedChat starts out as the original raw chat string, and will get processed if it includes word variations or relevant keywords (i.e. lastChat)
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
		RunCommand(Commands::forfeit);
		return;
	case EKeyword::ExitToMainMenu:
		RunCommand(Commands::exitToMainMenu);
		return;

	// lastChat and word variations need to parse the chat string every time binding is triggered (but im prolly wrong, and theres a way to eliminate the need)
	// ... the others only need to do it when the binding is created
	case EKeyword::LastChat:
	case EKeyword::LastChatSarcasm:
	case EKeyword::LastChatUwu:
	case EKeyword::WordVariation:
		processed_chat = process_keywords_in_chat_str(binding);
		break;
	default:
		break;
	}

	// send processed chat
	if (processed_chat.empty()) return;
	SendChat(processed_chat, binding.chatMode);
}


std::string CustomQuickchat::process_keywords_in_chat_str(const Binding& binding)
{
	std::string result = binding.chat;

	for (int i = 0; i < MAX_KEYWORD_DEPTH; i++)
	{
		auto keyword_strings_to_replace = binding.GetMatchedSubstrings(result, keywordRegexPattern);

		if (keyword_strings_to_replace.empty())
		{
			if (i > 1)
			{
				LOG("Resolved nested variation(s) using {} substitution passes", i);
			}

			break;
		}

		for (const auto& keyword_str_to_replace : keyword_strings_to_replace)
		{
			const std::string regexPatternStr = "\\[\\[" + keyword_str_to_replace + "\\]\\]";
			std::regex keyword_regex_pattern(regexPatternStr);

			// if a special keyword was found (not a variation list name) .... which, at this point, would be just one of the lastChat keywords
			auto it = keywordsMap.find(keyword_str_to_replace);
			if (it != keywordsMap.end())
			{
				std::string lastChat;

				switch (it->second)
				{
				case EKeyword::LastChat:
				case EKeyword::LastChatUwu:
				case EKeyword::LastChatSarcasm:
					lastChat = get_last_chat();
					if (lastChat.empty())
						return "";
					lastChat = ApplyTextEffect(lastChat, binding.textEffect);
					result = std::regex_replace(result, keyword_regex_pattern, lastChat);
					break;
				default:
					break;	// this should never get executed bc keyword should be a lastChat atp, but who knows
				}
			}
			// if something else was found, like a word variation list name
			else
			{
				result = std::regex_replace(result, keyword_regex_pattern, Variation(keyword_str_to_replace));
			}
		}
	}

	return result;
}


void CustomQuickchat::SendChat(const std::string& chat, EChatChannel chatMode)
{
	if (chat.empty()) return;

	// only send chat if custom quickchats are turned on
	auto enabledCvar = GetCvar(Cvars::enabled);
	if (!enabledCvar || !enabledCvar.getBoolValue()) return;

	Instances.SendChat(chat, chatMode, true);
}


void CustomQuickchat::NotifyAndLog(const std::string& title, const std::string& message, int duration)
{
	GAME_THREAD_EXECUTE_CAPTURE(
		Instances.SpawnNotification(title, message, duration, true);
	, title, message, duration);
}


void CustomQuickchat::ResetAllFirstButtonStates()
{
	for (Binding& binding : Bindings)
	{
		binding.firstButtonState.Reset(epochTime);
	}
}


void CustomQuickchat::ResetChatTimeoutMsg()
{
	chatTimeoutMsg = "Chat disabled for [Time] second(s).";
}


void CustomQuickchat::InitKeyStates()
{
	for (const std::string& keyName : possibleKeyNames)
	{
		keyStates[keyName] = false;
	}
}


void CustomQuickchat::AddEmptyBinding()
{
	Binding newBinding;
	Bindings.push_back(newBinding);
}


void CustomQuickchat::AddEmptyVariationList()
{
	VariationList list;
	Variations.push_back(list);
}


void CustomQuickchat::DeleteBinding(int idx)
{
	if (Bindings.empty()) return;

	// erase binding at given index
	Bindings.erase(Bindings.begin() + idx);

	// reset selected binding index
	selectedBindingIndex = Bindings.empty() ? 0 : Bindings.size() - 1;

	// update JSON
	WriteBindingsToJson();
}


void CustomQuickchat::DeleteVariationList(int idx)
{
	if (Variations.empty()) return;

	// erase variation list at given index
	Variations.erase(Variations.begin() + idx);
	
	// reset selected variation list index
	selectedVariationIndex = Variations.empty() ? 0 : Variations.size() - 1;

	// update JSON
	WriteVariationsToJson();
}


int CustomQuickchat::FindButtonIndex(const std::string& buttonName)
{
	auto it = std::find(possibleKeyNames.begin(), possibleKeyNames.end(), buttonName);
	if (it != possibleKeyNames.end())
	{
		return std::distance(possibleKeyNames.begin(), it);
	}
	else {
		return 0;
	}
}


void CustomQuickchat::CheckJsonFiles()
{
	// create 'CustomQuickchat' folder if it doesn't exist
	if (!fs::exists(customQuickchatFolder))
	{
		fs::create_directory(customQuickchatFolder);
		LOG("'CustomQuickchat' folder didn't exist... so I created it.");
	}

	// create JSON files if they don't exist
	if (!fs::exists(bindingsFilePath))
	{
		std::ofstream NewFile(bindingsFilePath);

		NewFile << "{ \"bindings\": [] }";
		NewFile.close();
		LOG("'Bindings.json' didn't exist... so I created it.");
	}
	if (!fs::exists(variationsFilePath))
	{
		std::ofstream NewFile(variationsFilePath);
		NewFile << "{ \"variations\": [] }";
		NewFile.close();
		LOG("'Variations.json' didn't exist... so I created it.");
	}
}


void CustomQuickchat::ReadDataFromJson()
{
	json bindings_json_data = Files::get_json(bindingsFilePath);

	if (!bindings_json_data.empty() && bindings_json_data.contains("bindings"))
	{
		auto bindingsList = bindings_json_data.at("bindings");

		if (bindingsList.size() > 0)
		{
			for (int i = 0; i < bindingsList.size(); i++)
			{
				// read data from each binding obj and update Bindings vector
				auto bindingObj = bindingsList[i];

				Binding binding;
				binding.chat =			bindingObj.value("chat",			"im gay");
				binding.chatMode =		bindingObj.value("chatMode",		EChatChannel::EChatChannel_Match);
				binding.bindingType =	bindingObj.value("bindingType",		EBindingType::Combination);

				// use value for "enabled" key if it exists, otherwise default to true
				// (to make backwards compatible with old json files that don't contain an "enabled" key)
				binding.enabled = bindingObj.value("enabled", true);

				if (bindingObj.contains("buttons") && bindingObj.at("buttons").is_array())
				{
					binding.buttons = bindingObj.value("buttons", std::vector<std::string>{});
				}
				else
				{
					LOG("[ERROR] Missing or invalid \"buttons\" array in JSON");
				}

				// lastly, update binding's keyWord and textEffect values (which depend on the chat value above)
				binding.UpdateKeywordAndTextEffect(keywordRegexPattern);

				Bindings.push_back(binding);
			}
		}
	}


	// ... same thing for variations
	json variations_json_data = Files::get_json(variationsFilePath);

	if (!variations_json_data.empty() && variations_json_data.contains("variations"))
	{
		auto variations_list = variations_json_data.at("variations");

		if (variations_list.size() > 0)
		{
			for (int i = 0; i < variations_list.size(); i++)
			{
				// read data from each variation list obj and update Variations vector
				auto variationListObj = variations_list[i];

				VariationList variationList;
				variationList.listName = variationListObj["listName"];

				for (const std::string& word : variationListObj["wordList"])
				{
					variationList.wordList.push_back(word);
					variationList.unparsedString += (word + "\n");
				}

				variationList.shuffledWordList = ShuffleWordList(variationList.wordList);

				Variations.push_back(variationList);
			}
		}
	}
}


std::string CustomQuickchat::Variation(const std::string& listName)
{
	for (int i = 0; i < Variations.size(); i++)
	{
		VariationList& list = Variations[i];
		
		if (list.listName == listName)
		{
			if (list.wordList.size() < 3)
			{
				LOG("** '{}' word variation list has less than 3 items... and cannot be used **", listName);
				return listName;
			}

			std::string variation = list.shuffledWordList[list.nextUsableIndex];
			
			if (list.nextUsableIndex != (list.shuffledWordList.size() - 1))
			{
				list.nextUsableIndex++;
				return variation;
			}
			else {
				ReshuffleWordList(i);
				return variation;
			}
		}
	}
	return listName;
}


void CustomQuickchat::UpdateDataFromVariationStr()
{
	for (auto& variation : Variations)	// <--- not const bc variation instances should be modified
	{
		// update word list based on parsed string
		std::vector<std::string> parsedVariations = Format::SplitStrByNewline(variation.unparsedString);
		variation.wordList = parsedVariations;

		// reset & create new shuffled word list
		variation.nextUsableIndex = 0;
		variation.shuffledWordList = ShuffleWordList(variation.wordList);
	}
}


std::string CustomQuickchat::get_last_chat()
{
	ChatData chat = LobbyInfo.get_last_chat_data();

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
		std::random_device rd;	// Initialize random number generator
		std::mt19937 rng(rd()); // Mersenne Twister 19937 generator
		std::shuffle(shuffledList.begin(), shuffledList.end(), rng);
	}

	return shuffledList;
}


 void CustomQuickchat::ReshuffleWordList(int idx)
 {
	 auto& variationList = Variations[idx];
	 std::vector<std::string> prevShuffled = variationList.shuffledWordList;

	 // skip all the non-repetition BS if the list has less than 4 variations... and just shuffle it
	 if (prevShuffled.size() < 4)
	 {
		 prevShuffled = ShuffleWordList(prevShuffled);
		 variationList.shuffledWordList = prevShuffled;
		 variationList.nextUsableIndex = 0;
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
	 variationList.nextUsableIndex = 0;
 }


std::string CustomQuickchat::ApplyTextEffect(const std::string& originalText, ETextEffect effect)
{
	switch (effect)
	{
	case ETextEffect::None:
		return originalText;
	case ETextEffect::Uwu:
		return to_uwu(originalText);
	case ETextEffect::Sarcasm:
		return to_sarcasm(originalText);
	default:
		return originalText;
	}
}


// to be called in separate thread (in onLoad)
void CustomQuickchat::PreventGameFreeze()
{
	// for sending chats
	Instances.SendChat(" ", EChatChannel::EChatChannel_Match);

	LOG("Sent dummy chat to prevent game freeze...");
}


void CustomQuickchat::UpdateBindingsData()
{
	for (auto& binding : Bindings)
	{
		binding.UpdateKeywordAndTextEffect(keywordRegexPattern);
	}
}


void CustomQuickchat::WriteBindingsToJson()
{
	json bindingsJsonObj;
	
	for (const auto& binding : Bindings)
	{
		json singleBinding;

		singleBinding["chat"] = binding.chat;
		singleBinding["chatMode"] = static_cast<int>(binding.chatMode);
		singleBinding["bindingType"] = static_cast<int>(binding.bindingType);
		singleBinding["enabled"] = binding.enabled;
		
		singleBinding["buttons"] = {};
		for (const auto& button : binding.buttons)
		{
			singleBinding["buttons"].push_back(button);
		}

		bindingsJsonObj["bindings"].push_back(singleBinding);
	}

	Files::write_json(bindingsFilePath, bindingsJsonObj);
	LOG("Updated 'Bindings.json' :)");
}


void CustomQuickchat::WriteVariationsToJson()
{
	json variationsJsonObj;

	for (const auto& list : Variations)
	{
		json variationList;

		variationList["listName"] = list.listName;
		variationList["wordList"] = {};

		for (const auto& word : list.wordList)
		{
			variationList["wordList"].push_back(word);
		}

		variationsJsonObj["variations"].push_back(variationList);
	}

	Files::write_json(variationsFilePath, variationsJsonObj);
	LOG("Updated 'Variations.json' :)");
}


void CustomQuickchat::GetFilePaths()
{
	fs::path bmDataFolderFilePath = gameWrapper->GetDataFolder();
	customQuickchatFolder =			bmDataFolderFilePath / "CustomQuickchat";
	bindingsFilePath =				customQuickchatFolder / "Bindings.json";
	variationsFilePath =			customQuickchatFolder / "Variations.json";

#ifdef USE_SPEECH_TO_TEXT
	speechToTextJsonPath =			customQuickchatFolder / "SpeechToText.json";
	speechToTextExePath =			customQuickchatFolder / "SpeechToText" / "SpeechToText.exe";
	speechToTextErrorLogPath =		customQuickchatFolder / "SpeechToText" / "ErrorLog.txt";
#endif
	
	// Lobby Info JSON files
	lobbyInfoFolder =				bmDataFolderFilePath / "Lobby Info";
	lobbyInfoChatsFilePath =		lobbyInfoFolder / "Chats.json";
	lobbyInfoRanksFilePath =		lobbyInfoFolder / "Ranks.json";
}


void CustomQuickchat::InitStuffOnLoad()
{
	LobbyInfo.Initialize(gameWrapper);

	// make sure JSON files are good to go, then read them to update data
	GetFilePaths();
	CheckJsonFiles();
	ReadDataFromJson();
	gui_footer_init();

#ifdef USE_SPEECH_TO_TEXT
	
	ClearSttErrorLog();
	start_websocket_stuff(true);

#endif

	InitKeyStates();
	PreventGameFreeze();

	inGameEvent = gameWrapper->IsInFreeplay() || gameWrapper->IsInGame() || gameWrapper->IsInOnlineGame();
}


void CustomQuickchat::determine_quickchat_labels(UGFxData_Controls_TA* controls, bool log)
{
	if (!controls)
	{
		controls = Instances.GetInstanceOf<UGFxData_Controls_TA>();
		if (!controls) {
			LOG("UGFxData_Controls_TA* is null!");
			return;
		}
	}

	std::array<BindingKey, 4> preset_group_bindings;

	// find/save key bindings for each preset group
	for (int i = 0; i < 4; i++)
	{
		for (const auto& binding : controls->PCBindings)
		{
			std::string action_name = binding.Action.ToString();

			if (action_name != preset_group_names[i]) continue;

			preset_group_bindings[i].action = action_name;
			preset_group_bindings[i].pc_key = binding.Key.ToString();
			break;
		}

		for (const auto& binding : controls->GamepadBindings)
		{
			std::string action_name = binding.Action.ToString();

			if (action_name != preset_group_names[i]) continue;

			preset_group_bindings[i].action = action_name;
			preset_group_bindings[i].gamepad_key = binding.Key.ToString();
			break;
		}
	}

	if (log)
	{
		for (int i = 0; i < 4; i++)
		{
			LOG("========== preset_group_bindings[{}] ==========", i);
			LOG("action: {}", preset_group_bindings[i].action);
			LOG("pc_key: {}", preset_group_bindings[i].pc_key);
			LOG("gamepad_key: {}", preset_group_bindings[i].gamepad_key);
		}
		
		LOG("Bindings.size(): {}", Bindings.size());
	}

	for (const auto& binding : Bindings)
	{
		if (binding.bindingType != EBindingType::Sequence || binding.buttons.size() < 2) continue;

		const std::string& first_button = binding.buttons[0];
		const std::string& second_button = binding.buttons[1];

		//for (const BindingKey& preset_binding : preset_group_bindings)
		for (int group_index = 0; group_index < 4; group_index++)
		{
			const BindingKey& group_key = preset_group_bindings[group_index];

			// check if matches a pc binding
			if (first_button == group_key.pc_key)
			{
				for (int chat_index = 0; chat_index < 4; chat_index++)
				{
					const BindingKey& chat_key = preset_group_bindings[chat_index];

					if (second_button != chat_key.pc_key) continue;

					pc_qc_labels[group_index][chat_index] = Instances.NewFString(binding.chat);
					break;
				}
			}
			// check if matches a gamepad binding
			else if (first_button == group_key.gamepad_key)
			{
				for (int chat_index = 0; chat_index < 4; chat_index++)
				{
					const BindingKey& chat_key = preset_group_bindings[chat_index];

					if (second_button != chat_key.gamepad_key) continue;

					gp_qc_labels[group_index][chat_index] = Instances.NewFString(binding.chat);
					break;
				}
			}
		}
	}

	LOG("Quickchat labels updated...");
}


void CustomQuickchat::apply_all_custom_qc_labels_to_ui(UGFxData_Chat_TA* caller)
{
	if (!caller) return;

	auto shell = caller->Shell;
	if (!shell) return;

	auto ds = shell->DataStore;
	if (!ds) return;

	const auto& groups_of_chat_labels = using_gamepad ? gp_qc_labels : pc_qc_labels;

	for (int group_index = 0; group_index < 4; group_index++)
	{
		const auto& group_of_labels = groups_of_chat_labels.at(group_index);

		for (int label_index = 0; label_index < 4; label_index++)
		{
			const auto& chat_label = group_of_labels[label_index];
			if (chat_label.empty()) continue;

			int ds_row_index = (group_index * 4) + label_index;

			// idk how this would ever happen, but to be safe...
			if (ds_row_index < 0 || ds_row_index > 15)
			{
				LOG("[ERROR] UGFxDataRow_X index out of range: {}", ds_row_index);
				continue;
			}

			ds->SetStringValue(L"ChatPresetMessages", ds_row_index, L"Label", chat_label);
		}
	}

	LOG("Updated quickchat labels in UI");
}


void CustomQuickchat::apply_custom_qc_labels_to_ui(UGFxData_Chat_TA* caller, UGFxData_Chat_TA_execOnPressChatPreset_Params* params)
{
	if (!caller || !params) return;

	const int32_t& index = params->Index;
	if (index == 420) return;

	auto shell = caller->Shell;
	if (!shell) return;

	auto ds = shell->DataStore;
	if (!ds) return;

	const auto& chat_labels = using_gamepad ? gp_qc_labels[index] : pc_qc_labels[index];

	for (int i = 0; i < 4; i++)
	{
		const auto& chat_label = chat_labels[i];
		if (chat_label.empty()) continue;

		int ds_row_index = (index * 4) + i;

		// this prevents the chats with index of 420 (aka default RL quickchats that have been overridden) from being included
		if (ds_row_index < 0 || ds_row_index > 15)
		{
			// the ds_row_index of chats that have been suppressed/overridden would be 1680-1683, so not exactly an error but still skip
			if (ds_row_index >= 1680 && ds_row_index < 1684) continue;

			LOG("[ERROR] UGFxDataRow_X index out of range: {}", ds_row_index);	// anything else would be an error
			continue;
		}

		ds->SetStringValue(L"ChatPresetMessages", ds_row_index, L"Label", chat_label);
	}


	LOG("Applied quickchat labels to UI for {} group", preset_group_names[index]);
}


void CustomQuickchat::gui_footer_init()
{
	fs::path plugin_assets_folder = gameWrapper->GetDataFolder() / "sslow_plugin_assets";
	if (!fs::exists(plugin_assets_folder))
	{
		LOG("[ERROR] Folder not found: {}", plugin_assets_folder.string());
		LOG("Will use old ugly settings footer :(");
		return;
	}

	GUI::FooterAssets assets = {
		plugin_assets_folder / "github.png",
		plugin_assets_folder / "discord.png",
		plugin_assets_folder / "youtube.png",
	};

	assets_exist = assets.all_assets_exist();

	if (assets_exist)
	{
		footer_links = std::make_shared<GUI::FooterLinks>(
			GUI::ImageLink(assets.github_img_path, github_link, github_link_tooltip, footer_img_height),
			GUI::ImageLink(assets.discord_img_path, GUI::discord_link, GUI::discord_desc, footer_img_height),
			GUI::ImageLink(assets.youtube_img_path, GUI::youtube_link, GUI::youtube_desc, footer_img_height)
		);
	}
	else
	{
		LOG("One or more plugin asset is missing... will use old ugly settings footer instead :(");
	}
}