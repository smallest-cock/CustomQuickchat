#include "pch.h"
#include "CustomQuickchat.h"
#include <regex>



void CustomQuickchat::PerformBindingAction(const Binding& binding)
{
	// get keywords in chat string
	std::string pattern = R"(\[\[(.*?)\]\])"; // Regex pattern to match double brackets [[...]] ... maybe turn into CVar and make editable?
	std::vector<std::string> stringsFoundInBrackets = GetSubstringsUsingRegexPattern(binding.chat, pattern);

	// processedChat starts out as the original raw chat string, and will get processed one stringFoundInBrackets at a time
	std::string processedChat = binding.chat;

	// handle any words in double brackets, like special keywords or word variations
	for (const std::string& stringFoundInBrackets : stringsFoundInBrackets)
	{
		auto it = keywordsMap.find(stringFoundInBrackets);
		
		// if a special keyword was found (not a variation list name)
		if (it != keywordsMap.end())
		{
			// get keyword and text effect (if any)
			EKeyword keyword = it->second;
			ETextEffect textEffect = GetTextEffect(keyword);

			// initialize variables outside switch
			std::string rankStr;
			std::string specificPattern;
			std::regex specificRegexPattern;
			bool replaceWithLastChat = false;  // whether or not to replace keyword with last chat after switch

			switch (keyword)
			{
			case EKeyword::SpeechToText:
			case EKeyword::SpeechToTextSarcasm:
			case EKeyword::SpeechToTextUwu:
				if (ActiveSTTAttemptID == "420_blz_it_lmao")
					StartSpeechToText(binding.chatMode, textEffect);
				else
					STTLog("Speech-to-text is already active!");
				return;
			case EKeyword::BlastAll:
			case EKeyword::BlastCasual:
			case EKeyword::Blast1v1:
			case EKeyword::Blast2v2:
			case EKeyword::Blast3v3:
				rankStr = GetRankStr(keyword);
				if (rankStr.empty()) return;
				SendChat(rankStr, binding.chatMode);
				return;
			case EKeyword::Forfeit:
				RunCommand(Cvars::forfeit);
				return;
			case EKeyword::ExitToMainMenu:
				RunCommand(Cvars::exitToMainMenu);
				return;
			case EKeyword::LastChat:
			case EKeyword::LastChatSarcasm:
			case EKeyword::LastChatUwu:
				replaceWithLastChat = true;
				specificPattern = "\\[\\[" + stringFoundInBrackets + "\\]\\]";
				specificRegexPattern = std::regex(specificPattern);
				break;
			default:
				break;
			}
			
			// replace keyword with last chat (with text effect if necessary)
			if (replaceWithLastChat)
			{
				std::string lastChat = LastChat();
				if (lastChat == "") return;

				std::string chatWithEffect = ApplyTextEffect(lastChat, textEffect);
				processedChat = std::regex_replace(processedChat, specificRegexPattern, chatWithEffect);
			}
		}
		// if something else, like a variation list name (or nothing), was found inside brackets
		else
		{
			// replace keyword with word variation
			std::string specificPattern = "\\[\\[" + stringFoundInBrackets + "\\]\\]";
			std::regex specificRegexPattern(specificPattern);

			processedChat = std::regex_replace(processedChat, specificRegexPattern, Variation(stringFoundInBrackets));
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

	Instances.SendChat(chat, chatMode);
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
	if (!fs::exists(speechToTextFilePath))
	{
		std::ofstream NewFile(speechToTextFilePath);
		NewFile << "{ \"transcription\": {} }";
		NewFile.close();
		LOG("'SpeechToText.json' didn't exist... so I created it.");
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


std::vector<std::string> CustomQuickchat::GetSubstringsUsingRegexPattern(const std::string& inputStr, const std::string& patternRawStr)
{
	std::regex regexPattern(patternRawStr);

	std::vector<std::string> matchedSubstrings;
	std::sregex_iterator it(inputStr.begin(), inputStr.end(), regexPattern);
	std::sregex_iterator end;

	while (it != end)
	{
		std::string matchedSubstring = (*it)[1].str();
		matchedSubstrings.push_back(matchedSubstring);
		++it;
	}

	return matchedSubstrings;
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


void CustomQuickchat::UpdateData()
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
				binding.bindingType = static_cast<BindingType>(bindingObj["bindingType"]);

				for (const std::string& buttonName : bindingObj["buttons"])
				{
					binding.buttons.push_back(buttonName);
				}

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


ETextEffect CustomQuickchat::GetTextEffect(EKeyword keyword)
{
	switch (keyword)
	{
	case EKeyword::LastChatUwu:
	case EKeyword::SpeechToTextUwu:
		return ETextEffect::Uwu;
	case EKeyword::LastChatSarcasm:
	case EKeyword::SpeechToTextSarcasm:
		return ETextEffect::Sarcasm;
	default:
		return ETextEffect::None;
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

	// for notifications
	//Instances.SpawnNotification("custom quickchat", "onload dummy notification", 3);
}


void CustomQuickchat::WriteBindingsToJson()
{
	json bindingsJsonObj;
	
	for (const auto& binding: Bindings)
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
	customQuickchatFolder = bmDataFolderFilePath / "CustomQuickchat";
	bindingsFilePath = customQuickchatFolder / "Bindings.json";
	variationsFilePath = customQuickchatFolder / "Variations.json";
	speechToTextFilePath = customQuickchatFolder / "SpeechToText.json";
	speechToTextPyScriptFilePath = customQuickchatFolder / "speechToText.pyw";
	
	// Lobby Info JSON files
	lobbyInfoFolder = bmDataFolderFilePath / "Lobby Info";
	lobbyInfoChatsFilePath = lobbyInfoFolder / "Chats.json";
	lobbyInfoRanksFilePath = lobbyInfoFolder / "Ranks.json";

	// for .cfg file
	fs::path bmPath = gameWrapper->GetBakkesModPath();
	LOG("bmPath: {}", bmPath.string());

	cfgPath = bmPath / "cfg" / "customQuickchat.cfg";
	LOG("cfgPath: {}", cfgPath.string());
}


void CustomQuickchat::InitStuffOnLoad()
{
	InitKeyStates();

	// make sure JSON files are good to go, then read them to update data
	GetFilePaths();
	CheckJsonFiles();
	UpdateData();
	ClearTranscriptionJson();
	PreventGameFreeze();

	//// start a new thread to send a dummy 1st chat (so it wont freeze game thread)
	//std::thread newThread(std::bind(&CustomQuickchat::PreventGameFreeze, this));
	//newThread.detach();		// don't wait for thread to finish... let it be freee
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
