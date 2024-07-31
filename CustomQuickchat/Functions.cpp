#include "pch.h"
#include "CustomQuickchat.h"
#include <regex>



void CustomQuickchat::PerformBindingAction(const Binding& binding)
{
	// get keywords in chat string
	std::string pattern = R"(\[\[(.*?)\]\])"; // Regex pattern to match double brackets [[...]] ... maybe turn into CVar and make editable?
	std::vector<std::string> keywords = GetSubstringsUsingRegexPattern(binding.chat, pattern);

	// start speech-to-text if necessary
	for (const std::string& keyword : keywords)
	{
		if (keyword == "speechToText")
		{
			if (ActiveSTTAttemptID == "420_blz_it_lmao")
			{
				StartSpeechToText(possibleChatModes[binding.chatMode]);
			}
			else {
				STTLog("Speech-to-text is already active!");
			}
			return;
		}
		else if (keyword == "speechToText sarcasm")
		{
			if (ActiveSTTAttemptID == "420_blz_it_lmao")
			{
				StartSpeechToText(possibleChatModes[binding.chatMode], "sarcasm");
			}
			else {
				STTLog("Speech-to-text is already active!");
			}
			return;
		}
		else if (keyword == "speechToText uwu")
		{
			if (ActiveSTTAttemptID == "420_blz_it_lmao")
			{
				StartSpeechToText(possibleChatModes[binding.chatMode], "uwu");
			}
			else {
				STTLog("Speech-to-text is already active!");
			}
			return;
		}
	}

	// replace keywords with appropriate text ...
	std::string processedChat = ReplacePatternInStr(binding.chat, keywords);
	if (processedChat == "") return;

	// send processed chat
	SendChat(processedChat, possibleChatModes[binding.chatMode]);
}


void CustomQuickchat::SendChat(const std::string& chat, const std::string& chatMode)
{
	if (chat == "") return;

	// only send chat if custom quickchats are turned on
	auto enabledCvar = cvarManager->getCvar(CvarNames::enabled);
	if (!enabledCvar || !enabledCvar.getBoolValue()) return;

	EChatChannel chatChannel = EChatChannel::EChatChannel_Match;

	if (chatMode == "lobby")
	{
		chatChannel = EChatChannel::EChatChannel_Match;
	}
	else if (chatMode == "team")
	{
		chatChannel = EChatChannel::EChatChannel_Team;
	}
	else if (chatMode == "party")
	{
		chatChannel = EChatChannel::EChatChannel_Party;
	}

	Instances.SendChat(chat, chatChannel);
}


bool CustomQuickchat::Sequence(const std::string& button1, const std::string& button2)
{
	bool button1Pressed = keyStates[button1];
	bool button2Pressed = keyStates[button2];

	if (button1Pressed || button2Pressed)
	{
		// get current time
		auto functionCallTime = std::chrono::steady_clock::now();

		if (sequenceStoredButtonPresses["global"].buttonName == "poopfart")
		{
			if (button1Pressed)
			{
				sequenceStoredButtonPresses["global"].buttonName = button1;
				sequenceStoredButtonPresses["global"].pressedTime = functionCallTime;
			}
		}
		else {
			// convert float timeWindow cvar into something addable to a chrono time_point
			auto sequenceTimeWindowCvar = cvarManager->getCvar(CvarNames::sequenceTimeWindow);
			if (!sequenceTimeWindowCvar) return false;

			double timeWindowRaw = sequenceTimeWindowCvar.getFloatValue();
			auto timeWindow = std::chrono::duration<double>(timeWindowRaw);

			// so sequence bindings with the same 1st & 2nd button aren't accidentally triggered (bc firstButtonPressed is global & the fact all bindings are attempted in for loop)
			double tooFastTimeWindowRaw = 0.05;
			auto tooFastTimeWindow = std::chrono::duration<double>(tooFastTimeWindowRaw);

			if (functionCallTime > sequenceStoredButtonPresses["global"].pressedTime + timeWindow)
			{
				if (button1Pressed)
				{
					sequenceStoredButtonPresses["global"].buttonName = button1;
					sequenceStoredButtonPresses["global"].pressedTime = functionCallTime;
				}
				else {
					ResetFirstButtonPressed();
				}
			}
			else {
				bool correct1stButtonPressed = sequenceStoredButtonPresses["global"].buttonName == button1;
				bool button2PressedWithinTimeWindow = functionCallTime > sequenceStoredButtonPresses["global"].pressedTime + tooFastTimeWindow;

				if (button2Pressed && correct1stButtonPressed && button2PressedWithinTimeWindow)
				{
					ResetFirstButtonPressed();
					return true;
				}
			}
		}
	}
	return false;
}


bool CustomQuickchat::Combine(const std::vector<std::string>& buttons)
{
	for (const std::string& button : buttons)
	{
		if (!keyStates[button]) return false;
	}

	ResetFirstButtonPressed();
	return true;
}


void CustomQuickchat::ResetFirstButtonPressed(const std::string& scope)
{
	sequenceStoredButtonPresses[scope].buttonName = "poopfart";
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
	newBinding.typeNameIndex = 0;
	newBinding.chat = "";
	newBinding.chatMode = ChatMode::Lobby;

	Bindings.push_back(newBinding);
}


void CustomQuickchat::AddEmptyVariationList()
{
	VariationList list;
	list.listName = "";
	list.unparsedString = "";
	list.wordList = {};

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


std::string CustomQuickchat::ReplacePatternInStr(const std::string& inputStr, const std::vector<std::string>& substrings)
{
	std::string newString = inputStr;

	for (const std::string& substring : substrings)
	{
		std::string specificPattern = "\\[\\[" + substring + "\\]\\]";
		std::regex specificRegexPattern(specificPattern);

		if (substring == "blast all")
		{
			return AllRanks();
		}
		else if (substring == "blast 1v1")
		{
			return SpecificRank("1v1");
		}
		else if (substring == "blast 2v2")
		{
			return SpecificRank("2v2");
		}
		else if (substring == "blast 3v3")
		{
			return SpecificRank("3v3");
		}
		else if (substring == "blast casual")
		{
			return SpecificRank("casual");
		}
		else if (substring == "lastChat")
		{
			std::string lastChat = LastChat();
			if (lastChat == "") return "";

			newString = std::regex_replace(newString, specificRegexPattern, lastChat);
		}
		else if (substring == "lastChat sarcasm")
		{
			std::string lastChat = LastChat();
			if (lastChat == "") return "";

			std::string sarcasmChat = toSarcasm(lastChat);
			newString = std::regex_replace(newString, specificRegexPattern, sarcasmChat);
		}
		else if (substring == "lastChat uwu")
		{
			std::string lastChat = LastChat();
			if (lastChat == "") return "";

			std::string uwuChat = toUwu(lastChat);
			newString = std::regex_replace(newString, specificRegexPattern, uwuChat);
		}
		else {
			newString = std::regex_replace(newString, specificRegexPattern, Variation(substring));
		}
	}

	return newString;
}


void CustomQuickchat::UpdateDataFromVariationStr()
{
	for (auto& list : Variations)	// not const bc list should get modified
	{
		std::vector<std::string> parsedVariations = Format::SplitStrByNewline(list.unparsedString);
		list.wordList = parsedVariations;

		list.shuffledWordList = ShuffleWordList(list.wordList);
		list.nextUsableIndex = 0;
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
				binding.typeNameIndex = bindingObj["typeNameIndex"];
				binding.chatMode = bindingObj["chatMode"];

				for (int i = 0; i < bindingObj["buttonNameIndexes"].size(); i++)
				{
					binding.buttonNameIndexes.push_back(bindingObj["buttonNameIndexes"][i]);
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
		auto variationsList = variationsJsonData["variationLists"];

		if (variationsList.size() > 0)
		{
			for (int i = 0; i < variationsList.size(); i++)
			{
				// read data from each variation list obj and update Variations vector
				auto variationListObj = variationsList[i];

				VariationList variationList;
				variationList.listName = variationListObj["listName"];
				variationList.unparsedString = variationListObj["unparsedString"];
				variationList.nextUsableIndex = 0;

				for (int i = 0; i < variationListObj["wordList"].size(); i++)
				{
					variationList.wordList.push_back(variationListObj["wordList"][i]);
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


// to be called in separate thread (in onLoad)
void CustomQuickchat::PreventGameFreeze()
{
	// for sending chats
	Instances.SendChat("custom quickchats activated", EChatChannel::EChatChannel_Match);

	LOG("Attempted to prevent game freeze on 1st chat...");

	// for notifications
	//Instances.SpawnNotification("custom quickchat", "onload dummy notification", 3);
}


void CustomQuickchat::WriteBindingsToJson()
{
	json bindingsJsonObj;
	bindingsJsonObj["bindings"] = {};
	
	for (const auto& binding: Bindings)
	{
		json singleBinding;

		singleBinding["chat"] = binding.chat;
		singleBinding["typeNameIndex"] = binding.typeNameIndex;
		singleBinding["chatMode"] = binding.chatMode;
		singleBinding["buttonNameIndexes"] = {};

		for (int buttonIndex : binding.buttonNameIndexes)
		{
			singleBinding["buttonNameIndexes"].push_back(buttonIndex);
		}

		bindingsJsonObj["bindings"].push_back(singleBinding);
	}

	writeJsonToFile(bindingsFilePath, bindingsJsonObj);
	LOG("Updated 'Bindings.json' :)");
}


void CustomQuickchat::WriteVariationsToJson()
{
	json variationsJsonObj;
	variationsJsonObj["variationLists"] = {};

	for (const auto& list : Variations)
	{
		json variationList;

		variationList["listName"] = list.listName;
		variationList["unparsedString"] = list.unparsedString;
		variationList["nextUsableIndex"] = list.nextUsableIndex;
		variationList["wordList"] = {};
		variationList["shuffledWordList"] = {};

		for (const auto& variation : list.wordList)
		{
			variationList["wordList"].push_back(variation);
		}
		
		for (const auto& variation : list.shuffledWordList)
		{
			variationList["shuffledWordList"].push_back(variation);
		}

		variationsJsonObj["variationLists"].push_back(variationList);
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
