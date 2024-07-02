#include "pch.h"
#include "CustomQuickchat.h"
#include <regex>



// ---------------------------- RLSDK shit ---------------------------------------------------------------------------


bool CheckNotInName(UObject* obj, const std::string& str) {
	return obj->GetFullName().find(str) == std::string::npos;
}


namespace plugin {
	namespace instances {
		// Get an object instance by it's name and class type. Example: UTexture2D* texture = FindObject<UTexture2D>("WhiteSquare");
		template<typename T> T* FindObject(const std::string& objectName, bool bStrictFind)
		{
			if (std::is_base_of<UObject, T>::value)
			{
				int32_t numObjects = UObject::GObjObjects()->Num();
				for (int32_t i = numObjects - 1; i > 0; i--)
				{
					if (i >= numObjects)
						continue;

					UObject* uObject = UObject::GObjObjects()->At(i);

					if (uObject && uObject->IsA<T>())
					{
						std::string objectFullName = uObject->GetFullName();

						if (bStrictFind)
						{
							if (objectFullName == objectName)
							{
								return static_cast<T*>(uObject);
							}
						}
						else if (objectFullName.find(objectName) != std::string::npos)
						{
							return static_cast<T*>(uObject);
						}
					}
				}
			}

			return nullptr;
		}


		// Get the instance of a class using an index for the GObjects array. Example: UEngine* engine = GetInstanceOf<UEngine>(420);
		template<typename T> T* GetInstanceFromIndex(int index)
		{
			if (std::is_base_of<UObject, T>::value)
			{
				UObject* uObject = UObject::GObjObjects()->At(index);

				if (uObject && uObject->IsA<T>())
				{
					//if (uObject->GetFullName().find("Default__") == std::string::npos && !(uObject->ObjectFlags & EObjectFlags::RF_ClassDefaultObject))
					if (CheckNotInName(uObject, "Default") && CheckNotInName(uObject, "Archetype") && CheckNotInName(uObject, "PostGameLobby") && CheckNotInName(uObject, "Test"))
					{
						return static_cast<T*>(uObject);
					}
				}
			}

			return nullptr;
		}


		// Get all active instances of a class type. Example: std::vector<APawn*> pawns = GetAllInstancesOf<APawn>();
		template<typename T> std::vector<T*> GetAllInstancesOf()
		{
			std::vector<T*> objectInstances;

			if (std::is_base_of<UObject, T>::value)
			{
				for (int32_t i = (UObject::GObjObjects()->Num() - INSTANCES_INTERATE_OFFSET); i > 0; i--)
				{
					UObject* uObject = UObject::GObjObjects()->At(i);

					if (uObject && uObject->IsA<T>())
					{
						if (CheckNotInName(uObject, "Default") && CheckNotInName(uObject, "Archetype") && CheckNotInName(uObject, "PostGameLobby") && CheckNotInName(uObject, "Test"))
						{
							objectInstances.push_back(static_cast<T*>(uObject));
						}
					}
				}
			}
			return objectInstances;
		}


		// Get the most current/active instance of a class. Example: UEngine* engine = GetInstanceOf<UEngine>();
		template<typename T> T* GetInstanceOf()
		{
			if (std::is_base_of<UObject, T>::value)
			{
				for (int32_t i = (UObject::GObjObjects()->Num() - INSTANCES_INTERATE_OFFSET); i > 0; i--)
				{
					UObject* uObject = UObject::GObjObjects()->At(i);

					if (uObject && uObject->IsA<T>())
					{
						//if (uObject->GetFullName().find("Default__") == std::string::npos && !(uObject->ObjectFlags & EObjectFlags::RF_ClassDefaultObject))
						if (CheckNotInName(uObject, "Default") && CheckNotInName(uObject, "Archetype") && CheckNotInName(uObject, "PostGameLobby") && CheckNotInName(uObject, "Test"))
						{
							// LOG("found instance of chatbox ptr thing: {}", i);
							return static_cast<T*>(uObject);
						}
					}
				}
			}
			return nullptr;
		}

	}
	namespace memory {
		uintptr_t FindPattern(HMODULE module, const unsigned char* pattern, const char* mask)
		{
			MODULEINFO info = { };
			GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

			uintptr_t start = reinterpret_cast<uintptr_t>(module);
			size_t length = info.SizeOfImage;

			size_t pos = 0;
			size_t maskLength = std::strlen(mask) - 1;

			for (uintptr_t retAddress = start; retAddress < start + length; retAddress++)
			{
				if (*reinterpret_cast<unsigned char*>(retAddress) == pattern[pos] || mask[pos] == '?')
				{
					if (pos == maskLength)
					{
						return (retAddress - maskLength);
					}
					pos++;
				}
				else
				{
					retAddress -= pos;
					pos = 0;
				}
			}
			return NULL;
		}
	}
	namespace globals {
		uintptr_t fnGObjects() {
			unsigned char GNamesPattern[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x35\x25\x02\x00";
			char GNamesMask[] = "??????xx??xxxxxx";

			auto GNamesAddress = memory::FindPattern(GetModuleHandle(L"RocketLeague.exe"), GNamesPattern, GNamesMask);
			auto GObjectsAddress = GNamesAddress + 0x48;

			return GObjectsAddress;
		}
		uintptr_t fnGNames() {
			unsigned char GNamesPattern[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x35\x25\x02\x00";
			char GNamesMask[] = "??????xx??xxxxxx";

			auto GNamesAddress = memory::FindPattern(GetModuleHandle(L"RocketLeague.exe"), GNamesPattern, GNamesMask);
			auto GObjectsAddress = GNamesAddress + 0x48;

			return GNamesAddress;
		}
		void Init() {
			GObjects = reinterpret_cast<TArray<UObject*>*>(fnGObjects());
			GNames = reinterpret_cast<TArray<FNameEntry*>*>(fnGNames());
		}
	}
}

// explicit instatiation ... idk if this even does anything
template UGFxData_Chat_TA* plugin::instances::GetInstanceOf<UGFxData_Chat_TA>();


// -------------------------------------------------------------------------------------------------------------------



void CustomQuickchat::TestShit() {

	// ...

}


void CustomQuickchat::HandleKeyPress(ActorWrapper caller, void* params, std::string eventName)
{
	UGameViewportClient_TA_execHandleKeyPress_Params* keyPressData = (UGameViewportClient_TA_execHandleKeyPress_Params*)params;
	if (!keyPressData) { return; }

	std::string keyName = keyPressData->Key.ToString();
	uint8_t keyEventType = keyPressData->EventType;

	// update key state
	if (keyEventType == InputEvent::Pressed) {
		keyStates[keyName] = true;

		// check if any bindings triggered
		for (Binding binding : Bindings) {

			if (possibleBindingTypes[binding.typeNameIndex] == "button combination") {

				// skip if no buttons for the binding
				if (binding.buttonNameIndexes.empty()) { continue; }

				std::vector<std::string> args;
				for (int buttonIndex : binding.buttonNameIndexes) {
					args.push_back(possibleKeyNames[buttonIndex]);
				}

				if (Combine(args)) {
					PerformBindingAction(binding);
					return;
				}
			}
			else if (possibleBindingTypes[binding.typeNameIndex] == "button sequence") {

				// skip if less than 2 buttons for the binding
				if (binding.buttonNameIndexes.size() < 2) { continue; }

				if (Sequence(possibleKeyNames[binding.buttonNameIndexes[0]], possibleKeyNames[binding.buttonNameIndexes[1]])) {
					PerformBindingAction(binding);
					return;
				}
			}
		}

	}
	else if (keyEventType == InputEvent::Released) {
		keyStates[keyName] = false;
	}
}


void CustomQuickchat::PerformBindingAction(const Binding& binding) {

	// get keywords in chat string
	std::string pattern = R"(\[\[(.*?)\]\])"; // Regex pattern to match double brackets [[...]] ... maybe turn into CVar and make editable?
	std::vector<std::string> keywords = GetSubstringsUsingRegexPattern(binding.chat, pattern);


	// start speech-to-text if necessary
	for (std::string keyword : keywords) {
		if (keyword == "speechToText") {
			if (ActiveSTTAttemptID == "420_blz_it_lmao") {
				StartSpeechToText(possibleChatModes[binding.chatMode]);
			}
			else {
				STTLog("Speech-to-text is already active!");
			}
			return;
		}
		else if (keyword == "speechToText sarcasm") {
			if (ActiveSTTAttemptID == "420_blz_it_lmao") {
				StartSpeechToText(possibleChatModes[binding.chatMode], "sarcasm");
			}
			else {
				STTLog("Speech-to-text is already active!");
			}
			return;
		}
		else if (keyword == "speechToText uwu") {
			if (ActiveSTTAttemptID == "420_blz_it_lmao") {
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
	if (processedChat == "") { return; }

	// send processed chat
	SendChat(processedChat, possibleChatModes[binding.chatMode]);
}


void CustomQuickchat::SendChat(const std::string& chat, const std::string& chatMode) {
	if (chat == "") { return; }

	// only send chat if custom quickchats are turned on
	CVarWrapper chatsOnCvar = cvarManager->getCvar("customQuickchat_chatsOn");
	if (!chatsOnCvar) { return; }
	if (!chatsOnCvar.getBoolValue()) { return; }

	UGFxData_Chat_TA* chatBox = plugin::instances::GetInstanceOf<UGFxData_Chat_TA>();

	if (chatBox) {

		FString message = StrToFString(chat);

		if (chatMode == "lobby") {
			chatBox->SendChatMessage(message, 0);		// send regular (lobby) chat
		}
		else if (chatMode == "team") {
			chatBox->SendTeamChatMessage(message, 0);	// send team chat
		}
		else if (chatMode == "party") {
			chatBox->SendPartyChatMessage(message, 0);	// send party chat
		}
	}
	else {
		LOG("UGFxData_Chat_TA ptr is NULL!");
	}
}



bool CustomQuickchat::Sequence(const std::string& button1, const std::string& button2) {
	bool button1Pressed = keyStates[button1];
	bool button2Pressed = keyStates[button2];
	if (button1Pressed || button2Pressed) {
		// get current time
		auto functionCallTime = std::chrono::steady_clock::now();

		if (sequenceStoredButtonPresses["global"].buttonName == "poopfart") {
			if (button1Pressed) {
				sequenceStoredButtonPresses["global"].buttonName = button1;
				sequenceStoredButtonPresses["global"].pressedTime = functionCallTime;
			}
		}
		else {
			// convert float timeWindow cvar into something addable to a chrono time_point
			CVarWrapper timeWindowCvar = cvarManager->getCvar("customQuickchat_macroTimeWindow");
			if (!timeWindowCvar) { return false; }
			double timeWindowRaw = timeWindowCvar.getFloatValue();
			auto timeWindow = std::chrono::duration<double>(timeWindowRaw);

			// so sequence bindings with the same 1st & 2nd button aren't accidentally triggered (bc firstButtonPressed is global & the fact all bindings are attempted in for loop)
			double tooFastTimeWindowRaw = 0.05;
			auto tooFastTimeWindow = std::chrono::duration<double>(tooFastTimeWindowRaw);

			if (functionCallTime > sequenceStoredButtonPresses["global"].pressedTime + timeWindow) {
				if (button1Pressed) {
					sequenceStoredButtonPresses["global"].buttonName = button1;
					sequenceStoredButtonPresses["global"].pressedTime = functionCallTime;
				}
				else {
					ResetFirstButtonPressed();
				}
			}
			else {
				// if 2nd button pressed & correct 1st button previously pressed & 2nd button pressed within the alotted timewindow ...
				if (button2Pressed && (sequenceStoredButtonPresses["global"].buttonName == button1) && (functionCallTime > sequenceStoredButtonPresses["global"].pressedTime + tooFastTimeWindow)) {
					ResetFirstButtonPressed();
					return true;
				}
			}
		}
	}
	return false;
}


bool CustomQuickchat::Combine(const std::vector<std::string>& buttons) {
	for (const std::string& button : buttons) {
		if (!keyStates[button]) {
			return false;
		}
	}
	ResetFirstButtonPressed();
	return true;
}



void CustomQuickchat::ResetFirstButtonPressed(const std::string& scope) {
	sequenceStoredButtonPresses[scope].buttonName = "poopfart";
}


void CustomQuickchat::InitKeyStates() {
	for (std::string keyName : possibleKeyNames) {
		keyStates[keyName] = false;
	}
}


void CustomQuickchat::AddEmptyBinding() {
	Binding newBinding;
	newBinding.typeNameIndex = 0;
	newBinding.chat = "";
	newBinding.chatMode = ChatMode::Lobby;

	Bindings.push_back(newBinding);
}


void CustomQuickchat::AddEmptyVariationList() {
	VariationList list;
	list.listName = "";
	list.unparsedString = "";
	list.wordList = {};

	Variations.push_back(list);
}


void CustomQuickchat::DeleteBinding(int idx) {

	if (Bindings.empty()) { return; }

	// erase binding at given index
	Bindings.erase(Bindings.begin() + idx);

	// reset selected binding index
	selectedBindingIndex = Bindings.empty() ? 0 : Bindings.size() - 1;

	// update JSON
	WriteBindingsToJson();
}


void CustomQuickchat::DeleteVariationList(int idx) {
	if (Variations.empty()) { return; }

	// erase variation list at given index
	Variations.erase(Variations.begin() + idx);
	
	// reset selected variation list index
	selectedVariationIndex = Variations.empty() ? 0 : Variations.size() - 1;

	// update JSON
	WriteVariationsToJson();
}


int CustomQuickchat::FindButtonIndex(const std::string& buttonName) {
	auto it = std::find(possibleKeyNames.begin(), possibleKeyNames.end(), buttonName);
	if (it != possibleKeyNames.end()) {
		return std::distance(possibleKeyNames.begin(), it);
	}
	else {
		return 0;
	}
}


void CustomQuickchat::CheckJsonFiles() {
	// create 'CustomQuickchat' folder if it doesn't exist
	if (!std::filesystem::exists(customQuickchatFolder)) {
		std::filesystem::create_directory(customQuickchatFolder);
		LOG("'CustomQuickchat' folder didn't exist... so I created it.");
	}

	// create JSON files if they don't exist
	if (!std::filesystem::exists(bindingsFilePath)) {
		std::ofstream NewFile(bindingsFilePath);

		NewFile << "{ \"bindings\": [] }";
		NewFile.close();
		LOG("'Bindings.json' didn't exist... so I created it.");
	}
	if (!std::filesystem::exists(variationsFilePath)) {
		std::ofstream NewFile(variationsFilePath);
		NewFile << "{ \"variations\": [] }";
		NewFile.close();
		LOG("'Variations.json' didn't exist... so I created it.");
	}
	if (!std::filesystem::exists(speechToTextFilePath)) {
		std::ofstream NewFile(speechToTextFilePath);
		NewFile << "{ \"transcription\": {} }";
		NewFile.close();
		LOG("'SpeechToText.json' didn't exist... so I created it.");
	}
}


std::string CustomQuickchat::Variation(const std::string& listName) {

	for (int i = 0; i < Variations.size(); i++) {

		VariationList& list = Variations[i];
		
		if (list.listName == listName) {

			if (list.wordList.size() < 3) {
				LOG("** '{}' word variation list has less than 3 items... and cannot be used **", listName);
				return listName;
			}

			std::string variation = list.shuffledWordList[list.nextUsableIndex];
			
			if (list.nextUsableIndex != (list.shuffledWordList.size() - 1)) {
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


std::vector<std::string> CustomQuickchat::GetSubstringsUsingRegexPattern(const std::string& inputStr, const std::string& patternRawStr) {
	std::regex regexPattern(patternRawStr);

	std::vector<std::string> matchedSubstrings;
	std::sregex_iterator it(inputStr.begin(), inputStr.end(), regexPattern);
	std::sregex_iterator end;

	while (it != end) {
		std::string matchedSubstring = (*it)[1].str();
		matchedSubstrings.push_back(matchedSubstring);
		++it;
	}

	return matchedSubstrings;
}


std::string CustomQuickchat::ReplacePatternInStr(const std::string& inputStr, const std::vector<std::string>& substrings) {

	std::string newString = inputStr;
	for (std::string substring : substrings) {
		std::string specificPattern = "\\[\\[" + substring + "\\]\\]";
		std::regex specificRegexPattern(specificPattern);

		if (substring == "blast all") {
			return AllRanks();
		}
		else if (substring == "blast 1v1") {
			return SpecificRank("1v1");
		}
		else if (substring == "blast 2v2") {
			return SpecificRank("2v2");
		}
		else if (substring == "blast 3v3") {
			return SpecificRank("3v3");
		}
		else if (substring == "blast casual") {
			return SpecificRank("casual");
		}
		else if (substring == "lastChat") {
			std::string lastChat = LastChat();
			if (lastChat == "") { return ""; }

			newString = std::regex_replace(newString, specificRegexPattern, lastChat);
		}
		else if (substring == "lastChat sarcasm") {
			std::string lastChat = LastChat();
			if (lastChat == "") { return ""; }

			std::string sarcasmChat = toSarcasm(lastChat);
			newString = std::regex_replace(newString, specificRegexPattern, sarcasmChat);
		}
		else if (substring == "lastChat uwu") {
			std::string lastChat = LastChat();
			if (lastChat == "") { return ""; }

			std::string uwuChat = toUwu(lastChat);
			newString = std::regex_replace(newString, specificRegexPattern, uwuChat);
		}
		else {
			newString = std::regex_replace(newString, specificRegexPattern, Variation(substring));
		}
	}

	return newString;
}


void CustomQuickchat::PopupNotification(const std::string& message, const std::string& title, float duration) {
	
	if (UNotificationManager_TA* NotificationManager = plugin::instances::GetInstanceOf<UNotificationManager_TA>()) {
		if (!notificationClass)
		{
			notificationClass = UGenericNotification_TA::StaticClass();
		}

		UNotification_TA* Notification = NotificationManager->PopUpOnlyNotification(notificationClass);

		if (Notification) {
			Notification->SetTitle(StrToFString(title));
			Notification->SetBody(StrToFString(message));
			Notification->PopUpDuration = duration;
		}
	}
	else {
		LOG("UNotificationManager_TA* is NULL! ..... popup notification failed :(");
	}
}


void CustomQuickchat::UpdateDataFromVariationStr() {
	for (auto& list : Variations) {		// reference used to modify the actual element itself (instead of just a local copy)
		std::vector<std::string> parsedVariations = splitStringByNewline(list.unparsedString);
		list.wordList = parsedVariations;

		list.shuffledWordList = ShuffleWordList(list.wordList);
		list.nextUsableIndex = 0;
	}
}


std::string CustomQuickchat::LastChat() {
	
	// check if file exists 1st ...
	if (!std::filesystem::exists(lobbyInfoChatsFilePath)) {
		LOG("*** 'Lobby Info/Chats.json' doesn't exist... ***");
		return "";
	}

	std::string jsonFileRawStr = readContent(lobbyInfoChatsFilePath);

	// prevent crash on reading invalid JSON data
	try {
		auto chatsJsonData = nlohmann::json::parse(jsonFileRawStr);
		auto chatMessages = chatsJsonData["chatMessages"];

		if (!chatMessages.empty()) {
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


std::string CustomQuickchat::AllRanks() {
	ChatterRanks lastChatterRanks = FindLastChattersRanks();

	if (lastChatterRanks.ranks.empty()) { return ""; }

	// to make return line readable
	std::string onesRankStr = GetRankStr(lastChatterRanks.ranks["1v1"]);
	std::string twosRankStr = GetRankStr(lastChatterRanks.ranks["2v2"]);
	std::string threesRankStr = GetRankStr(lastChatterRanks.ranks["3v3"]);

	return lastChatterRanks.playerName + ": [1s] " + onesRankStr + " [2s] " + twosRankStr + " [3s] " + threesRankStr;
}


std::string CustomQuickchat::SpecificRank(const std::string& playlist) {
	ChatterRanks lastChatterRanks = FindLastChattersRanks();

	if (lastChatterRanks.ranks.empty()) { return ""; }

	Rank specificRank = lastChatterRanks.ranks[playlist];

	if (playlist != "casual") {
		if (specificRank.tier != "n/a" || specificRank.div != "n/a") {
			if (specificRank.matches != 0) {
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


ChatterRanks CustomQuickchat::FindLastChattersRanks() {

	ChatterRanks lastChattersRanks;
	lastChattersRanks.playerName = "";
	lastChattersRanks.ranks = {};

	// check if file exists 1st ...
	if (!std::filesystem::exists(lobbyInfoChatsFilePath)) {
		LOG("*** 'Lobby Info/Chats.json' doesn't exist... ***");
		return lastChattersRanks;
	}

	std::string jsonFileRawStr = readContent(lobbyInfoChatsFilePath);

	// prevent crash on reading invalid JSON data
	try {
		auto chatsJsonData = nlohmann::json::parse(jsonFileRawStr);
		auto chatMessages = chatsJsonData["chatMessages"];

		if (!chatMessages.empty()) {
			std::string playerName = chatMessages[chatMessages.size() - 1]["playerName"];

			jsonFileRawStr = readContent(lobbyInfoRanksFilePath);
			
			auto ranksJsonData = nlohmann::json::parse(jsonFileRawStr);
			auto playersRanks = ranksJsonData["lobbyRanks"];

			// check if ranks for given player name exist
			auto it = playersRanks.find(playerName);
			if (it != playersRanks.end()) {
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


				// TODO: filter non-ascii characters out of player name
				lastChattersRanks.playerName = cleanString(playerName);


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


std::string CustomQuickchat::GetRankStr(const Rank& rank) {
	if (rank.div == "n/a" || rank.tier == "n/a" || rank.matches == 0) {
		return "--";
	}
	return rank.tier + "..div" + rank.div;
}


std::vector<std::string> CustomQuickchat::ShuffleWordList(const std::vector<std::string>& ogList) {
	std::vector<std::string> shuffledList = ogList;

	if (ogList.size() >= 3) {
		std::random_device rd;	// Initialize random number generator
		std::mt19937 rng(rd()); // Mersenne Twister 19937 generator
		std::shuffle(shuffledList.begin(), shuffledList.end(), rng);
	}

	return shuffledList;
}


 void CustomQuickchat::ReshuffleWordList(int idx) {
	 auto& variationList = Variations[idx];
	 std::vector<std::string> prevShuffled = variationList.shuffledWordList;

	 // skip all the non-repetition BS if the list has less than 4 variations... and just shuffle it
	 if (prevShuffled.size() < 4) {
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
	 for (int i = 0; i < shuffledBih.size(); i++) {

		 auto word = shuffledBih[i];

		 auto it = std::find(last2Words.begin(), last2Words.end(), word);
		 if (it == last2Words.end()) {
		
			 if (newShuffled1st == "") {
				 newShuffled1st = word;
				 shuffledBih.erase(shuffledBih.begin() + i);
				 break;
			 }
		 }
	 }
	 
	 // find 2nd different variation
	 for (int i = 0; i < shuffledBih.size(); i++) {

		 auto word = shuffledBih[i];

		 auto it = std::find(last2Words.begin(), last2Words.end(), word);
		 if (it == last2Words.end()) {
		
			 if (newShuffled2nd == "") {
				 newShuffled2nd = word;
				 shuffledBih.erase(shuffledBih.begin() + i);
				 break;
			 }
		 }
	 }
	 
	 // insert selected words (that are diff than prev last two) at beginning of new shuffled vector
	 shuffledBih.insert(shuffledBih.begin(), newShuffled1st);
	 shuffledBih.insert(shuffledBih.begin() + 1, newShuffled2nd);

	 // update actual variation list info
	 variationList.shuffledWordList = shuffledBih;
	 variationList.nextUsableIndex = 0;
 }



void CustomQuickchat::UpdateData() {
	std::string jsonFileRawStr = readContent(bindingsFilePath);

	// prevent crash on reading invalid JSON data
	try {
		auto bindingsJsonData = nlohmann::json::parse(jsonFileRawStr);
		auto bindingsList = bindingsJsonData["bindings"];

		if (bindingsList.size() > 0) {
			for (int i = 0; i < bindingsList.size(); i++) {

				// read data from each binding obj and update Bindings vector
				auto bindingObj = bindingsList[i];
				
				Binding binding;
				binding.chat = bindingObj["chat"];
				binding.typeNameIndex = bindingObj["typeNameIndex"];
				binding.chatMode = bindingObj["chatMode"];

				for (int i = 0; i < bindingObj["buttonNameIndexes"].size(); i++) {
					
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
		auto variationsJsonData = nlohmann::json::parse(jsonFileRawStr);
		auto variationsList = variationsJsonData["variationLists"];

		if (variationsList.size() > 0) {
			for (int i = 0; i < variationsList.size(); i++) {

				// read data from each variation list obj and update Variations vector
				auto variationListObj = variationsList[i];

				VariationList variationList;
				variationList.listName = variationListObj["listName"];
				variationList.unparsedString = variationListObj["unparsedString"];
				variationList.nextUsableIndex = 0;

				for (int i = 0; i < variationListObj["wordList"].size(); i++) {
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


// to be called in separate thread (in onLoad function)
void CustomQuickchat::PreventGameFreeze() {
	// for sending chats
	UGFxData_Chat_TA* chatbox = plugin::instances::GetInstanceOf<UGFxData_Chat_TA>();

	if (chatbox) {
		FString message = StrToFString("custom quickchats activated");		// never actually gets chatted, prolly bc not in game thread

		chatbox->SendChatMessage(message, 0);

		LOG("called template function to have it compile and/or load instances in memory to prevent game freeze on 1st chat");
	}
	else {
		LOG("(onload) UGFxData_Chat_TA ptr is NULL");
	}

	// for popup notifications
	if (UNotificationManager_TA* NotificationManager = plugin::instances::GetInstanceOf<UNotificationManager_TA>()) {

		UClass* notiClass = UGenericNotification_TA::StaticClass();

		UNotification_TA* Notification = NotificationManager->PopUpOnlyNotification(notiClass);		// doesn't get shown, separate thread

		if (Notification) {
			Notification->SetTitle(StrToFString("Custom Quickchat"));
			Notification->SetBody(StrToFString("Ready to cook"));
			Notification->PopUpDuration = 3;
		}
	}
	else {
		LOG("UNotificationManager_TA* is NULL! ..... popup notification failed :(");
	}
}


void CustomQuickchat::WriteBindingsToJson() {
	nlohmann::json bindingsJsonObj;
	bindingsJsonObj["bindings"] = {};
	
	for (Binding binding: Bindings) {
		nlohmann::json singleBinding;

		singleBinding["chat"] = binding.chat;
		singleBinding["typeNameIndex"] = binding.typeNameIndex;
		singleBinding["chatMode"] = binding.chatMode;
		singleBinding["buttonNameIndexes"] = {};

		for (int buttonIndex : binding.buttonNameIndexes) {
			singleBinding["buttonNameIndexes"].push_back(buttonIndex);
		}

		bindingsJsonObj["bindings"].push_back(singleBinding);
	}

	writeJsonToFile(bindingsFilePath, bindingsJsonObj);
	LOG("Updated 'Bindings.json' :)");
}


void CustomQuickchat::WriteVariationsToJson() {
	nlohmann::json variationsJsonObj;
	variationsJsonObj["variationLists"] = {};

	for (auto& list : Variations) {
		nlohmann::json variationList;

		variationList["listName"] = list.listName;
		variationList["unparsedString"] = list.unparsedString;
		variationList["nextUsableIndex"] = list.nextUsableIndex;
		variationList["wordList"] = {};
		variationList["shuffledWordList"] = {};

		for (auto& variation : list.wordList) {
			variationList["wordList"].push_back(variation);
		}
		
		for (auto& variation : list.shuffledWordList) {
			variationList["shuffledWordList"].push_back(variation);
		}

		variationsJsonObj["variationLists"].push_back(variationList);
	}

	writeJsonToFile(variationsFilePath, variationsJsonObj);
	LOG("Updated 'Variations.json' :)");
}


void CustomQuickchat::GetFilePaths() {
	std::filesystem::path bmDataFolderFilePath = gameWrapper->GetDataFolder();
	customQuickchatFolder = bmDataFolderFilePath / "CustomQuickchat";
	bindingsFilePath = customQuickchatFolder / "Bindings.json";
	variationsFilePath = customQuickchatFolder / "Variations.json";
	speechToTextFilePath = customQuickchatFolder / "SpeechToText.json";
	speechToTextPyScriptFilePath = customQuickchatFolder / "speechToText.pyw";
	
	// Lobby Info JSON files
	lobbyInfoFolder = bmDataFolderFilePath / "Lobby Info";
	lobbyInfoChatsFilePath = lobbyInfoFolder / "Chats.json";
	lobbyInfoRanksFilePath = lobbyInfoFolder / "Ranks.json";

	std::string bmPath = gameWrapper->GetBakkesModPath().string();

	LOG("bmPath: {}", bmPath);

	// .cfg file
	cfgPath = gameWrapper->GetBakkesModPath() / "cfg" / "customQuickchat.cfg";
}


void CustomQuickchat::FilterLinesInFile(const std::filesystem::path& filePath, const std::string& startString)
{
	// Open the file for reading and writing
	std::fstream file(filePath, std::ios::in | std::ios::out);

	if (!file.is_open()) {
		LOG("Error: Unable to open file {}", filePath.string());
		return;
	}

	std::string line;
	std::ofstream tempFile("temp.txt"); // Temporary file to store filtered lines

	if (!tempFile.is_open()) {
		LOG("Error: Unable to create temporary file");
		return;
	}

	while (std::getline(file, line)) {
		if (line.substr(0, startString.length()) == startString) {
			// Write the line to the temporary file if it starts with the given string
			tempFile << line << '\n';
		}
	}

	// Close both files
	file.close();
	tempFile.close();

	// Replace the original file with the temporary file
	std::filesystem::remove(filePath); // Remove the original file
	std::filesystem::rename("temp.txt", filePath); // Rename the temporary file to the original file

	LOG("Filtered lines saved to {}", filePath.string());
}


std::string CustomQuickchat::readContent(const std::filesystem::path& FileName) {
	std::ifstream Temp(FileName);
	std::stringstream Buffer;
	Buffer << Temp.rdbuf();
	return (Buffer.str());
}


nlohmann::json CustomQuickchat::getJsonFromFile(const std::filesystem::path& filePath)
{
	nlohmann::json contents;

	if (!std::filesystem::exists(filePath)) { return contents; }
	
	std::string jsonFileRawStr = readContent(filePath);
	try {
		contents = nlohmann::json::parse(jsonFileRawStr);
	}
	catch (...) {
		LOG("[ERROR] Couldn't read '{}' Make sure it contains valid JSON!", filePath.string());
	}

	return contents;
}


void CustomQuickchat::writeJsonToFile(const std::filesystem::path& filePath, const nlohmann::json& jsonData) {
	std::ofstream file(filePath);
	if (file.is_open()) {
		file << jsonData.dump(4); // Pretty-print with 4 spaces indentation
		file.close();
	}
	else {
		LOG("Could not open file for writing");
	}
}


bool CustomQuickchat::CheckGlobals() 
{
	if (!GObjects || !GNames || !AreGObjectsValid() || !AreGNamesValid()) {
		LOG("(onLoad) Error: RLSDK classes are wrong... need to fix them :(");
		LOG(std::to_string(!GObjects) + ", " + std::to_string(!GNames));
		return false;
	}
		
	LOG("Globals Initailized");
	return true;
}


bool CustomQuickchat::AreGObjectsValid() {
	if (UObject::GObjObjects()->Num() > 0 && UObject::GObjObjects()->Max() > UObject::GObjObjects()->Num())
	{
		if (UObject::GObjObjects()->At(0)->GetFullName() == "Class Core.Config_ORS")
		{
			return true;
		}
	}
	return false;
}

bool CustomQuickchat::AreGNamesValid() {
	if (FName::Names()->Num() > 0 && FName::Names()->Max() > FName::Names()->Num())
	{
		if (FName(0).ToString() == "None")
		{
			return true;
		}
	}
	return false;
}