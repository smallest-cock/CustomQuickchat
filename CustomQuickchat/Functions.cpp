#include "pch.h"
#include "CustomQuickchat.h"
#include <regex>



void CustomQuickchat::PerformBindingAction(const Binding& binding)
{
	// processedChat starts out as the original raw chat string, and will get processed if it includes word variations or relevant keywords (i.e. lastChat)
	std::string processedChat = binding.chat;

	bool shouldProcessChatStr = false;
	std::string stt_message;

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
		SendChat(GetRankStr(binding.keyWord), binding.chatMode);
		return;
	case EKeyword::Forfeit:
		RunCommand(Cvars::forfeit);
		return;
	case EKeyword::ExitToMainMenu:
		RunCommand(Cvars::exitToMainMenu);
		return;

	// lastChat and word variations need to parse the chat string every time binding is triggered (but im prolly wrong, and theres a way to eliminate the need)
	// ... the others only need to do it when the binding is created
	case EKeyword::LastChat:
	case EKeyword::LastChatSarcasm:
	case EKeyword::LastChatUwu:
	case EKeyword::WordVariation:
		shouldProcessChatStr = true;
		break;
	default:
		break;
	}

	// parse the chat string for relevant keywords/variations and replace the text as necessary
	if (shouldProcessChatStr)
	{
		auto stringsToReplace = binding.GetMatchedSubstrings(keywordRegexPattern);

		for (const auto& strToBeReplaced : stringsToReplace)
		{
			const std::string regexPatternStr = "\\[\\[" + strToBeReplaced + "\\]\\]";
			std::regex regexPattern(regexPatternStr);

			auto it = keywordsMap.find(strToBeReplaced);

			// if a special keyword was found (not a variation list name) .... which, at this point, would be just one of the lastChat keywords
			if (it != keywordsMap.end())
			{
				std::string lastChat;

				switch (it->second)
				{
				case EKeyword::LastChat:
				case EKeyword::LastChatUwu:
				case EKeyword::LastChatSarcasm:
					lastChat = LastChat();
					if (lastChat == "") return;
					lastChat = ApplyTextEffect(lastChat, binding.textEffect);
					processedChat = std::regex_replace(processedChat, regexPattern, lastChat);
					break;
				default:
					break;	// this should never get executed bc keyword should be a lastChat atp, but who knows
				}
			}
			// if something else was found, like a word variation list name
			else
			{
				processedChat = std::regex_replace(processedChat, regexPattern, Variation(strToBeReplaced));
			}
		}
	}

	// send processed chat
	SendChat(processedChat, binding.chatMode);
}


void CustomQuickchat::SendChat(const std::string& chat, EChatChannel chatMode)
{
	if (chat == "") return;

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


std::string CustomQuickchat::GetRankStr(EKeyword keyword)
{
	switch (keyword)
	{
	case EKeyword::BlastAll:
		return AllRanks();
	case EKeyword::BlastCasual:
		return SpecificRank("casual");
	case EKeyword::Blast1v1:
		return SpecificRank("1v1");
	case EKeyword::Blast2v2:
		return SpecificRank("2v2");
	case EKeyword::Blast3v3:
		return SpecificRank("3v3");
	default:
		return "";
	}
}


std::string CustomQuickchat::LastChat()
{
	// check if file exists 1st ...
	if (!fs::exists(lobbyInfoChatsFilePath))
	{
		LOG("*** 'Lobby Info/Chats.json' doesn't exist... ***");
		return "";
	}

	std::string jsonFileRawStr = readContent(lobbyInfoChatsFilePath);

	// prevent crash on reading invalid JSON data
	try {
		auto chatsJsonData = json::parse(jsonFileRawStr);
		auto chatMessages = chatsJsonData["chatMessages"];

		if (!chatMessages.empty())
		{
			std::string lastChat = chatMessages[chatMessages.size() - 1]["chat"];
			return lastChat;
		}
		else {
			LOG("*** 'Lobby Info/Chats.json' has no chats... ***");
		}
	}
	catch (...) {
		LOG("*** Couldn't read the 'Lobby Info/Chats.json' file! Make sure it contains valid JSON... ***");
		return "";
	}

	return "";
}


std::string CustomQuickchat::AllRanks()
{
	ChatterRanks lastChatterRanks = FindLastChattersRanks();

	if (lastChatterRanks.ranks.empty()) return "";

	// to make return line readable
	std::string onesRankStr = GetRankStr(lastChatterRanks.ranks["1v1"]);
	std::string twosRankStr = GetRankStr(lastChatterRanks.ranks["2v2"]);
	std::string threesRankStr = GetRankStr(lastChatterRanks.ranks["3v3"]);

	return lastChatterRanks.playerName + ": [1s] " + onesRankStr + " [2s] " + twosRankStr + " [3s] " + threesRankStr;
}


std::string CustomQuickchat::SpecificRank(const std::string& playlist)
{
	ChatterRanks lastChatterRanks = FindLastChattersRanks();

	if (lastChatterRanks.ranks.empty()) return "";

	Rank specificRank = lastChatterRanks.ranks[playlist];

	if (playlist != "casual")
	{
		if (specificRank.tier != "n/a" || specificRank.div != "n/a")
		{
			if (specificRank.matches != 0)
			{
				return lastChatterRanks.playerName + " [" + playlist + "] ** " + specificRank.tier + " div" + specificRank.div + " ** (" + std::to_string(specificRank.matches) + " matches)";
			}
			else {
				return lastChatterRanks.playerName + " [" + playlist + "] ** " + specificRank.tier + " div" + specificRank.div + " ** (prev season MMR)";
			}
		}
		else {
			return lastChatterRanks.playerName + " [" + playlist + "] ** doesnt play ** (" + std::to_string(specificRank.matches) + " matches)";
		}
	}
	else {
		return lastChatterRanks.playerName + " [casual] ** " + std::to_string(specificRank.matches) + " matches played **";
	}
}


ChatterRanks CustomQuickchat::FindLastChattersRanks()
{
	ChatterRanks lastChattersRanks;
	lastChattersRanks.playerName = "";
	lastChattersRanks.ranks = {};

	// check if file exists 1st ...
	if (!fs::exists(lobbyInfoChatsFilePath))
	{
		LOG("*** 'Lobby Info/Chats.json' doesn't exist... ***");
		return lastChattersRanks;
	}

	std::string jsonFileRawStr = readContent(lobbyInfoChatsFilePath);

	// prevent crash on reading invalid JSON data
	try {
		auto chatsJsonData = json::parse(jsonFileRawStr);
		auto chatMessages = chatsJsonData["chatMessages"];

		if (!chatMessages.empty())
		{
			std::string playerName = chatMessages[chatMessages.size() - 1]["playerName"];

			jsonFileRawStr = readContent(lobbyInfoRanksFilePath);
			
			auto ranksJsonData = json::parse(jsonFileRawStr);
			auto playersRanks = ranksJsonData["lobbyRanks"];

			// check if ranks for given player name exist
			auto it = playersRanks.find(playerName);
			if (it != playersRanks.end())
			{
				auto playerRanksObj = playersRanks[playerName];
				auto onesData = playerRanksObj["1v1"];
				auto twosData = playerRanksObj["2v2"];
				auto threesData = playerRanksObj["3v3"];
				auto casData = playerRanksObj["casual"];

				Rank onesRank;
				onesRank.mmr = 0;
				onesRank.div = onesData["rank"]["div"];
				onesRank.tier = onesData["rank"]["tier"];
				onesRank.matches = onesData["matches"];
				
				Rank twosRank;
				twosRank.mmr = 0;
				twosRank.div = twosData["rank"]["div"];
				twosRank.tier = twosData["rank"]["tier"];
				twosRank.matches = twosData["matches"];

				Rank threesRank;
				threesRank.mmr = 0;
				threesRank.div = threesData["rank"]["div"];
				threesRank.tier = threesData["rank"]["tier"];
				threesRank.matches = threesData["matches"];

				Rank casRank;
				casRank.mmr = casData["mmr"];
				casRank.matches = casData["matches"];
				casRank.div = "";
				casRank.tier = "";


				lastChattersRanks.playerName = Format::ToASCIIString(playerName);

				lastChattersRanks.ranks["1v1"] = onesRank;
				lastChattersRanks.ranks["2v2"] = twosRank;
				lastChattersRanks.ranks["3v3"] = threesRank;
				lastChattersRanks.ranks["casual"] = casRank;
			}
			else {
				LOG("*** Error: player '{}' wasn't found in 'Lobby Info/Ranks.json' ***", playerName);
			}
		}
		else {
			LOG("*** 'Lobby Info/Chats.json' has no chats... ***");
		}
	}
	catch (...) {
		LOG("*** Couldn't read the 'Lobby Info/Chats.json' file! Make sure it contains valid JSON... ***");
	}

	return lastChattersRanks;
}


std::string CustomQuickchat::GetRankStr(const Rank& rank)
{
	if (rank.div == "n/a" || rank.tier == "n/a" || rank.matches == 0)
	{
		return "--";
	}
	return rank.tier + "..div" + rank.div;
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


void CustomQuickchat::ReadDataFromJson()
{
	std::string jsonFileRawStr = readContent(bindingsFilePath);

	// prevent crash on reading invalid JSON data
	try {
		auto bindingsJsonData = json::parse(jsonFileRawStr);
		auto bindingsList = bindingsJsonData["bindings"];

		if (bindingsList.size() > 0)
		{
			for (int i = 0; i < bindingsList.size(); i++)
			{
				// read data from each binding obj and update Bindings vector
				auto bindingObj = bindingsList[i];
				
				Binding binding;
				binding.chat = bindingObj["chat"];
				binding.chatMode = static_cast<EChatChannel>(bindingObj["chatMode"]);
				binding.bindingType = static_cast<EBindingType>(bindingObj["bindingType"]);

				for (const std::string& buttonName : bindingObj["buttons"])
				{
					binding.buttons.push_back(buttonName);
				}

				// lastly, update binding's keyWord and textEffect values (which depend on the chat value above)
				binding.UpdateKeywordAndTextEffect(keywordRegexPattern);

				Bindings.push_back(binding);
			}
		}
	}
	catch (...) {
		LOG("*** Couldn't read the 'Bindings.json' file! Make sure it contains valid JSON... ***");
	}


	// ... same thing for variations
	jsonFileRawStr = readContent(variationsFilePath);

	try {
		auto variationsJsonData = json::parse(jsonFileRawStr);
		auto variationsList = variationsJsonData["variations"];

		if (variationsList.size() > 0)
		{
			for (int i = 0; i < variationsList.size(); i++)
			{
				// read data from each variation list obj and update Variations vector
				auto variationListObj = variationsList[i];

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
	catch (...) {
		LOG("*** Couldn't read the 'Variations.json' file! Make sure it contains valid JSON... ***");
	}
}


std::string CustomQuickchat::ApplyTextEffect(const std::string& originalText, ETextEffect effect)
{
	switch (effect)
	{
	case ETextEffect::None:
		return originalText;
	case ETextEffect::Uwu:
		return toUwu(originalText);
	case ETextEffect::Sarcasm:
		return toSarcasm(originalText);
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
		
		singleBinding["buttons"] = {};
		for (const auto& button : binding.buttons)
		{
			singleBinding["buttons"].push_back(button);
		}

		bindingsJsonObj["bindings"].push_back(singleBinding);
	}

	writeJsonToFile(bindingsFilePath, bindingsJsonObj);
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

	writeJsonToFile(variationsFilePath, variationsJsonObj);
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
	// make sure JSON files are good to go, then read them to update data
	GetFilePaths();
	CheckJsonFiles();
	ReadDataFromJson();

#ifdef USE_SPEECH_TO_TEXT
	
	ClearSttErrorLog();

	// start websocket sever (spawn python process)
	start_websocket_server();

	// create websocket object
	std::function<void(json serverResponse)> ws_response_callback = std::bind(&CustomQuickchat::process_ws_response, this, std::placeholders::_1);
	
	Websocket = std::make_shared<WebsocketClientManager>(cvarManager, ws_url, ws_response_callback);

	// wait x seconds after python websocket server has started to start client
	DELAY(start_ws_client_delay,
		Websocket->StartClient();
	);

#endif

	InitKeyStates();
	PreventGameFreeze();

	inGameEvent = gameWrapper->IsInFreeplay() || gameWrapper->IsInGame() || gameWrapper->IsInOnlineGame();
}


DWORD CustomQuickchat::CreateProcessUsingCommand(const std::string& command)
{
	// CreateProcess variables
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// Initialize STARTUPINFO
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Create the process to start python script
	if (CreateProcess(
		NULL,									// Application name (set NULL to use command)
		Format::ToWcharString(command),			// Command
		NULL,									// Process security attributes
		NULL,									// Thread security attributes
		FALSE,									// Inherit handles from the calling process
		CREATE_NEW_CONSOLE,						// Creation flags (use CREATE_NEW_CONSOLE for asynchronous execution)
		NULL,									// Use parent's environment block
		NULL,									// Use parent's starting directory
		&si,									// Pointer to STARTUPINFO
		&pi										// Pointer to PROCESS_INFORMATION
	))
	{
		// After successfully starting process, close handle to allow it to run asynchronously
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return 0;
	}
	else
	{
		// Failed to create process
		return GetLastError();
	}
}


std::string CustomQuickchat::readContent(const fs::path& FileName)
{
	std::ifstream Temp(FileName);
	std::stringstream Buffer;
	Buffer << Temp.rdbuf();
	return Buffer.str();
}


json CustomQuickchat::getJsonFromFile(const fs::path& filePath)
{
	json contents;

	if (!fs::exists(filePath)) return contents;
	
	std::string jsonFileRawStr = readContent(filePath);
	try {
		contents = json::parse(jsonFileRawStr);
	}
	catch (...) {
		LOG("[ERROR] Couldn't read '{}' Make sure it contains valid JSON!", filePath.string());
	}

	return contents;
}


void CustomQuickchat::writeJsonToFile(const fs::path& filePath, const json& jsonData)
{
	std::ofstream file(filePath);
	if (file.is_open())
	{
		file << jsonData.dump(4); // pretty-print with 4 spaces indentation
		file.close();
	}
	else {
		LOG("[ERROR] Couldn't open file for writing: {}", filePath.string());
	}
}
