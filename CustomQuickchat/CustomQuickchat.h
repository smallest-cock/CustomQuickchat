#pragma once

#include "pch.h"
#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "Structs.hpp"
#include "Keys.h"
#include "TextEffects.h"

#include "version.h"

#include "Macros.hpp"
#include "Events.hpp"
#include "Cvars.hpp"
#include "Components/Includes.hpp"



#define USE_SPEECH_TO_TEXT



constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

#if !defined(USE_SPEECH_TO_TEXT)
constexpr auto pretty_plugin_version = "v" stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH);
#else
constexpr auto pretty_plugin_version = "v" stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "\t(with speech-to-text)";
#endif


#ifdef USE_SPEECH_TO_TEXT

struct SpeechToTextResult
{
	bool success = false;
	bool error = false;
	std::string outputStr;
};

struct MicCalibrationResult
{
	bool success = false;
	bool error = false;
	int energyThreshold = 420;
	std::string errorMsg;
};

#endif


class CustomQuickchat : public BakkesMod::Plugin::BakkesModPlugin
	,public SettingsWindowBase
	,public PluginWindowBase
{
	//Boilerplate
	void onLoad() override;
	void onUnload() override;


	// constants
	static constexpr const char* keywordRegexPattern = R"(\[\[(.*?)\]\])";
	static constexpr double BLOCK_DEFAULT_QUICKCHAT_WINDOW = 0.1;		// maybe turn into a cvar w slider in settings


	// flags
	bool onLoadComplete =	false;
	bool gamePaused =		false;
	bool matchEnded =		false;
	bool inGameEvent =		false;

	
	// CustomQuickchat filepaths
	fs::path customQuickchatFolder;
	fs::path bindingsFilePath;
	fs::path variationsFilePath;

	// Lobby Info filepaths
	fs::path lobbyInfoFolder;
	fs::path lobbyInfoChatsFilePath;
	fs::path lobbyInfoRanksFilePath;


	// plugin init
	void InitStuffOnLoad();
	void InitKeyStates();
	void CheckJsonFiles();
	void GetFilePaths();
	void ReadDataFromJson();
	void PreventGameFreeze();	// hacky solution to prevent game freezing for few seconds on 1st chat sent


	// bindings & variations stuff
	std::vector<Binding> Bindings;
	std::vector<VariationList> Variations;

	int selectedBindingIndex = 0;
	int selectedVariationIndex = 0;

	std::unordered_map<std::string, bool> keyStates;

	std::chrono::steady_clock::time_point epochTime = std::chrono::steady_clock::time_point();
	std::chrono::steady_clock::time_point lastBindingActivated;

	void ResetAllFirstButtonStates();
	int FindButtonIndex(const std::string& buttonName);

	void UpdateBindingsData();
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


	// Lobby Info stuff (blast ranks & last chat)
	std::string LastChat();
	std::string GetRankStr(EKeyword keyword);
	std::string AllRanks();
	std::string SpecificRank(const std::string& playlist);
	std::string GetRankStr(const Rank& rank);
	ChatterRanks FindLastChattersRanks();


	// sending chat stuff
	void SendChat(const std::string& chat, EChatChannel chatMode);
	std::string ApplyTextEffect(const std::string& originalText, ETextEffect effect);


	// chat timeout stuff
	std::string chatTimeoutMsg = "Chat disabled for [Time] second(s).";
	void ResetChatTimeoutMsg();


	// JSON stuff (can be moved to Utils, like Files::)
	std::string readContent(const fs::path& FileName);
	void writeJsonToFile(const fs::path& filePath, const json& jsonData);
	json getJsonFromFile(const fs::path& filePath);


	// misc functions
	void NotifyAndLog(const std::string& title, const std::string& message, int duration = 3);
	DWORD CreateProcessUsingCommand(const std::string& commandStr);

	
	// ------------------------------------ Speech-to-Text ------------------------------------

	void StartSpeechToText(const Binding& binding);

#ifdef USE_SPEECH_TO_TEXT

	// constants
	static constexpr float PROBE_JSON_FREQUENCY = 0.2f;		// in seconds
	
	// thread-safe flags
	std::atomic<bool> attemptingSTT =			false;
	std::atomic<bool> calibratingMicLevel =		false;

	// filepaths
	fs::path speechToTextExePath;
	fs::path speechToTextJsonPath;
	fs::path speechToTextErrorLogPath;

	// mutable values
	std::string ActiveSTTAttemptID;

	// mic calibration
	void CalibrateMicrophone();
	void MicCalibrationWaitAndProbe(int micCalibrationTimeout);
	void StartProbingJsonForMicCalibrationResult(int micCalibrationTimeout);
	MicCalibrationResult CheckJsonForCalibrationResult();

	// speech-to-text
	std::string GenerateSTTCommand(bool calibrateMic);
	std::string CreateSTTCommandString(const fs::path& executablePath, const std::vector<std::string>& args);
	void STTWaitAndProbe(const Binding& binding);
	void StartProbingJsonForSTTResult(const Binding& binding);
	SpeechToTextResult CheckJsonForSTTResult();

	// misc
	void ClearSttErrorLog();
	void ResetSTTJsonFile();

	void STTLog(const std::string& message);

#endif // USE_SPEECH_TO_TEXT

	// ----------------------------------------------------------------------------------------


	// commands
	void cmd_toggleEnabled(std::vector<std::string> args);
	void cmd_listBindings(std::vector<std::string> args);
	void cmd_exitToMainMenu(std::vector<std::string> args);
	void cmd_forfeit(std::vector<std::string> args);
	void cmd_test(std::vector<std::string> args);
	void cmd_test2(std::vector<std::string> args);

	// cvar change callbacks
	void changed_enabled(std::string cvarName, CVarWrapper updatedCvar);
	void changed_enableSTTNotifications(std::string cvarName, CVarWrapper updatedCvar);
	void changed_overrideDefaultQuickchats(std::string cvarName, CVarWrapper updatedCvar);
	void changed_blockDefaultQuickchats(std::string cvarName, CVarWrapper updatedCvar);
	void changed_useCustomChatTimeoutMsg(std::string cvarName, CVarWrapper updatedCvar);
	void changed_customChatTimeoutMsg(std::string cvarName, CVarWrapper updatedCvar);

	// hook callbacks
	void Event_KeyPressed(ActorWrapper caller, void* params, std::string eventName);
	void Event_ChatPresetPressed(ActorWrapper caller, void* params, std::string eventName);
	void Event_ApplyChatSpamFilter(ActorWrapper caller, void* params, std::string eventName);
	void Event_NotifyChatDisabled(ActorWrapper caller, void* params, std::string eventName);
	void Event_OnChatMessage(ActorWrapper caller, void* params, std::string eventName);
	void Event_PushMenu(ActorWrapper caller, void* params, std::string eventName);
	void Event_PopMenu(ActorWrapper caller, void* params, std::string eventName);
	void Event_LoadingScreenStart(std::string eventName);

	
	// cvar helper stuff
	CVarWrapper RegisterCvar_Bool(const Cvars::CvarData& cvar, bool startingValue);
	CVarWrapper RegisterCvar_String(const Cvars::CvarData& cvar, const std::string& startingValue);
	CVarWrapper RegisterCvar_Number(const Cvars::CvarData& cvar, float startingValue, bool hasMinMax = false, float min = 0, float max = 0);
	CVarWrapper RegisterCvar_Color(const Cvars::CvarData& cvar, const std::string& startingValue);
	CVarWrapper GetCvar(const Cvars::CvarData& cvar);
	void RegisterCommand(const Cvars::CvarData& cvar, std::function<void(std::vector<std::string>)> callback);
	void RunCommand(const Cvars::CvarData& command, float delaySeconds = 0);
	void RunCommandInterval(const Cvars::CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand = false);


public:

	// GUI
	void RenderSettings() override;
	void RenderWindow() override;

	void GeneralSettings();
	void ChatTimeoutSettings();
	void SpeechToTextSettings();

	void RenderAllBindings();
	void RenderBindingDetails();
	void RenderChatDetails(Binding& selectedBinding);
	void RenderBindingTriggerDetails(Binding& selectedBinding);
	
	void RenderAllVariationListNames();
	void RenderVariationListDetails();
};
