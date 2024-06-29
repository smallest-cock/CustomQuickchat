#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "Keys.h"
#include "TextEffects.h"
#include "Utils.hpp"
#include "nlohmann.hpp"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);



FString StrToFString(const std::string& str);


// ------------------------------------------- RLSDK shit ---------------------------------------------------------------------------

static constexpr int32_t INSTANCES_INTERATE_OFFSET = 100;

bool CheckNotInName(UObject* obj, const std::string& str);

namespace plugin {
	namespace instances {
		// Get an object instance by it's name and class type. Example: UTexture2D* texture = FindObject<UTexture2D>("WhiteSquare");
		template<typename T> T* FindObject(const std::string& objectName, bool bStrictFind = false);

		// Get the instance of a class using an index for the GObjects array. Example: UEngine* engine = GetInstanceOf<UEngine>(420);
		template<typename T> T* GetInstanceFromIndex(int index);

		// Get all active instances of a class type. Example: std::vector<APawn*> pawns = GetAllInstancesOf<APawn>();
		template<typename T> std::vector<T*> GetAllInstancesOf();

		// Get the most current/active instance of a class. Example: UEngine* engine = GetInstanceOf<UEngine>();
		template<typename T> T* GetInstanceOf();

	}
	namespace memory {
		uintptr_t FindPattern(HMODULE module, const unsigned char* pattern, const char* mask);
	}
	namespace globals {
		uintptr_t fnGObjects();
		uintptr_t fnGNames();
		void Init();
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------


const std::vector<std::string> possibleBindingTypes = {
	"button combination",
	"button sequence"
};

const std::vector<std::string> possibleChatModes = {
	"lobby",
	"team",
	"party"
};

enum ChatMode {
	Lobby = 0,
	Team = 1,
	Party = 2
};


struct Binding {
	std::vector<int> buttonNameIndexes;
	std::string chat;
	int typeNameIndex;
	int chatMode;
};

// variation stuff
struct VariationList {
	std::string listName;
	std::string unparsedString;
	std::vector<std::string> wordList;
	std::vector<std::string> shuffledWordList;
	int nextUsableIndex;
};

// rank stuff
struct Rank {
	int matches;
	std::string div;
	std::string tier;
	int mmr;
};

struct ChatterRanks {
	std::string playerName;
	std::unordered_map <std::string, Rank> ranks;
};


struct ButtonPress {
	std::string buttonName;
	std::chrono::steady_clock::time_point pressedTime;
};



class CustomQuickchat : public BakkesMod::Plugin::BakkesModPlugin
	,public SettingsWindowBase
	,public PluginWindowBase
{

	//Boilerplate
	void onLoad() override;
	void onUnload() override;

	void SendChat(const std::string& chat, const std::string& chatMode);
	void TestShit();
	void PopupNotification(const std::string& message, const std::string& title, float duration);

	// speech-to-text stuff
	void StartSpeechToText(const std::string& chatMode, const std::string& effect = "", bool test = false);
	void STTWaitAndProbe(const std::string& chatMode, const std::string& effect, const std::string& attemptID, bool test);
	void STTLog(const std::string& message);
	bool ClearTranscriptionJson();
	std::string ActiveSTTAttemptID = "420_blz_it_lmao";


	bool CheckGlobals();
	bool AreGObjectsValid();
	bool AreGNamesValid();

	void InitKeyStates();
	bool Sequence(const std::string& button1, const std::string& button2);
	bool Combine(const std::vector<std::string>& buttons);

	int FindButtonIndex(const std::string& buttonName);

	void ResetFirstButtonPressed(const std::string& scope = "global");
	
	void CheckJsonFiles();
	void GetFilePaths();
	void FilterLinesInFile(const std::filesystem::path& filePath, const std::string& startString);

	void WriteBindingsToJson();
	void WriteVariationsToJson();

	void AddEmptyBinding();
	void AddEmptyVariationList();

	void DeleteBinding(int idx);
	void DeleteVariationList(int idx);
	void UpdateDataFromVariationStr();


	std::string Variation(const std::string& listName);
	std::vector<std::string> ShuffleWordList(const std::vector<std::string>& ogList);
	void ReshuffleWordList(int idx);

	void PerformBindingAction(const Binding& binding);

	std::string ReplacePatternInStr(const std::string& inputStr, const std::vector<std::string>& substrings);
	std::vector<std::string> GetSubstringsUsingRegexPattern(const std::string& inputStr, const std::string& patternRawStr);

	std::string LastChat();
	std::string AllRanks();
	std::string SpecificRank(const std::string& playlist);
	std::string GetRankStr(const Rank& rank);
	ChatterRanks FindLastChattersRanks();


	// JSON stuff
	std::string readContent(const std::filesystem::path& FileName);
	void writeContent(const std::filesystem::path& FileName, const std::string& Buffer);

	void UpdateData();
	void PreventGameFreeze();	// hacky solution to prevent game hanging for few seconds on 1st chat sent

	std::vector<Binding> Bindings;
	std::vector<VariationList> Variations;

	int selectedBindingIndex = 0;
	int selectedVariationIndex = 0;

	UClass* notificationClass = nullptr;

	std::unordered_map<std::string, bool> keyStates;
	std::unordered_map<std::string, ButtonPress> sequenceStoredButtonPresses;

	const std::string KeyPressedEvent = "Function TAGame.GameViewportClient_TA.HandleKeyPress";
	void HandleKeyPress(ActorWrapper caller, void* params, std::string eventName);

	std::filesystem::path findPythonInterpreter();

	// CustomQuickchat filepaths
	std::filesystem::path customQuickchatFolder;
	std::filesystem::path bindingsFilePath;
	std::filesystem::path variationsFilePath;
	std::filesystem::path speechToTextFilePath;
	std::filesystem::path speechToTextPyScriptFilePath;
	std::filesystem::path pyInterpreter;
	std::filesystem::path cfgPath;

	// Lobby Info filepaths
	std::filesystem::path lobbyInfoFolder;
	std::filesystem::path lobbyInfoChatsFilePath;
	std::filesystem::path lobbyInfoRanksFilePath;

	// bindings GUI
	void RenderAllBindings();
	void RenderBindingDetails();

	// word variations GUI
	void RenderAllVariationListNames();
	void RenderVariationListDetails();

public:

	void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	void RenderWindow() override; // Uncomment if you want to render your own plugin window
};
