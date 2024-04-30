#include "pch.h"
#include "CustomQuickchat.h"



FString StrToFString(const std::string& s) {
	wchar_t* p = new wchar_t[s.size() + 1];
	for (std::string::size_type i = 0; i < s.size(); ++i)
		p[i] = s[i];
	p[s.size()] = '\0';
	return FString(p);
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


// -------------------------------------------------------------------------------------------------------------------



void CustomQuickchat::SendChat(const std::string& chat, const std::string& chatMode) {

	// .... something here prolly causing the one-time few second hang


	// only send chat if custom quickchats are turned on
	CVarWrapper chatsOnCvar = cvarManager->getCvar("customQuickchat_chatsOn");
	if (!chatsOnCvar) { return; }
	if (!chatsOnCvar.getBoolValue()) { return; }

	UGFxData_Chat_TA* chatBox = plugin::instances::GetInstanceOf<UGFxData_Chat_TA>();		// maybe this?

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


void CustomQuickchat::DeleteBinding(int idx) {

	if (Bindings.empty()) { return; }

	// erase binding at given index
	Bindings.erase(Bindings.begin() + idx);

	// reset selected binding index
	selectedBindingIndex = Bindings.empty() ? 0 : Bindings.size() - 1;

	// update JSON
	WriteBindingsToJson();
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


	// TODO: read in variations JSON and update Variations data

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