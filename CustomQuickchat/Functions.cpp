#include "pch.h"
#include "CustomQuickchat.h"
#include "nlohmann.hpp"
#include <regex>
#include <random>


FString StrToFString(const std::string& s) {
	wchar_t* p = new wchar_t[s.size() + 1];
	for (std::string::size_type i = 0; i < s.size(); ++i)
		p[i] = s[i];
	p[s.size()] = '\0';
	return FString(p);
}



std::vector<std::string> splitStringByNewline(const std::string& input) {
	std::vector<std::string> lines;
	std::istringstream iss(input);
	std::string line;

	// Read each line using std::getline with '\n' as delimiter
	while (std::getline(iss, line)) {
		lines.push_back(line);
	}

	return lines;
}


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



void CustomQuickchat::SendChat(const std::string& chat, const std::string& chatMode) {

	// only send chat if custom quickchats are turned on
	CVarWrapper chatsOnCvar = cvarManager->getCvar("customQuickchat_chatsOn");
	if (!chatsOnCvar) { return; }
	if (!chatsOnCvar.getBoolValue()) { return; }

	UGFxData_Chat_TA* chatBox = plugin::instances::GetInstanceOf<UGFxData_Chat_TA>();

	if (chatBox) {

		FString message = StrToFString(ReplacePatternInStr(chat));

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

		if (sequenceStoredButtonPresses["global"].buttonName == "poopfart" && button1Pressed) {
			sequenceStoredButtonPresses["global"].buttonName = button1;
			sequenceStoredButtonPresses["global"].pressedTime = functionCallTime;
		}
		else {
			// convert float timeWindow cvar into something addable to a chrono time_point
			CVarWrapper timeWindowCvar = cvarManager->getCvar("customQuickchat_macroTimeWindow");
			if (!timeWindowCvar) { return false; }
			double timeWindowRaw = timeWindowCvar.getFloatValue();
			auto timeWindow = std::chrono::duration<double>(timeWindowRaw);

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
				if (button2Pressed && (sequenceStoredButtonPresses["global"].buttonName == button1) && (functionCallTime > sequenceStoredButtonPresses["global"].pressedTime)) {
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


std::string CustomQuickchat::ReplacePatternInStr(const std::string& inputStr) {
	
	std::string pattern = R"(\[\[(.*?)\]\])"; // Regex pattern to match double brackets [[...]]
	std::regex regexPattern(pattern);

	std::vector<std::string> matchedSubstrings;
	std::sregex_iterator it(inputStr.begin(), inputStr.end(), regexPattern);
	std::sregex_iterator end;

	while (it != end) {
		std::string matchedSubstring = (*it)[1].str();
		matchedSubstrings.push_back(matchedSubstring);
		++it;
	}

	std::string newString = inputStr;
	for (std::string substring : matchedSubstrings) {
		std::string specificPattern = "\\[\\[" + substring + "\\]\\]";
		std::regex specificRegexPattern(specificPattern);

		newString = std::regex_replace(newString, specificRegexPattern, Variation(substring));
	}

	return newString;
}


void CustomQuickchat::UpdateDataFromVariationStr() {
	for (auto& list : Variations) {		// reference used to modify the actual element itself (instead of just a local copy)
		std::vector<std::string> parsedVariations = splitStringByNewline(list.unparsedString);
		list.wordList = parsedVariations;

		list.shuffledWordList = ShuffleWordList(list.wordList);
		list.nextUsableIndex = 0;
	}
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
	UGFxData_Chat_TA* chatbox = plugin::instances::GetInstanceOf<UGFxData_Chat_TA>();

	if (chatbox) {
		FString message = StrToFString("custom quickchats activated");		// never actually gets chatted, prolly bc not in game thread

		chatbox->SendChatMessage(message, 0);

		LOG("called template function to have it compile and/or load instances in memory to prevent game freeze on 1st chat");
	}
	else {
		LOG("(onload) UGFxData_Chat_TA ptr is NULL");
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

	writeContent(bindingsFilePath, bindingsJsonObj.dump(4));
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

	writeContent(variationsFilePath, variationsJsonObj.dump(4));
	LOG("Updated 'Variations.json' :)");
}


void CustomQuickchat::GetFilePaths() {
	std::filesystem::path bmDataFolderFilePath = gameWrapper->GetDataFolder();
	customQuickchatFolder = bmDataFolderFilePath / "CustomQuickchat";
	bindingsFilePath = customQuickchatFolder / "Bindings.json";
	variationsFilePath = customQuickchatFolder / "Variations.json";
}


std::string CustomQuickchat::readContent(const std::filesystem::path& FileName) {
	std::ifstream Temp(FileName);
	std::stringstream Buffer;
	Buffer << Temp.rdbuf();
	return (Buffer.str());
}


void CustomQuickchat::writeContent(const std::filesystem::path& FileName, const std::string& Buffer) {
	std::ofstream File(FileName, std::ofstream::trunc);
	File << Buffer;
	File.close();
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