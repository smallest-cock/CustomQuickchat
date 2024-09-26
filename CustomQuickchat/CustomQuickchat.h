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

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);
constexpr auto pretty_plugin_version = "v" stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH);


constexpr double BLOCK_DEFAULT_QUICKCHAT_WINDOW = 0.1;		// maybe turn into a cvar w slider in settings


class CustomQuickchat : public BakkesMod::Plugin::BakkesModPlugin
	,public SettingsWindowBase
	,public PluginWindowBase
{
	//Boilerplate
	void onLoad() override;
	void onUnload() override;


	// bools
	bool onLoadComplete = false;
	bool gamePaused = false;
	bool matchEnded = false;
	bool inGameEvent = false;


	// plugin init
	void InitStuffOnLoad();
	void InitKeyStates();
	void CheckJsonFiles();
	void GetFilePaths();
	void UpdateData();
	void PreventGameFreeze();	// hacky solution to prevent game freezing for few seconds on 1st chat sent


	// sending chat stuff
	void SendChat(const std::string& chat, EChatChannel chatMode);
	ETextEffect GetTextEffect(EKeyword keyword);
	std::string ApplyTextEffect(const std::string& originalText, ETextEffect effect);
	std::vector<std::string> GetSubstringsUsingRegexPattern(const std::string& inputStr, const std::string& patternRawStr);


	// chat timeout stuff
	std::string chatTimeoutMsg = "Chat disabled for [Time] second(s).";
	void ResetChatTimeoutMsg();


	// speech-to-text stuff
	void StartSpeechToText(EChatChannel chatMode, ETextEffect effect = ETextEffect::None, bool test = false, bool calibrateMic = false);
	void STTWaitAndProbe(EChatChannel chatMode, ETextEffect effect, const std::string& attemptID, bool test);
	void STTLog(const std::string& message);
	void ResetJsonFile(float micCalibration = 69420);
	bool ClearTranscriptionJson();
	std::string ActiveSTTAttemptID = "420_blz_it_lmao";		// magic string... needs to be refactored
	double micEnergyThreshold = 420;
	void UpdateMicCalibration(float timeOut);

	// speech-to-text python interpreter stuff
	fs::path findPythonInterpreter();
	fs::path findInterpreterUsingSearchPathW(const wchar_t* fileName);
	fs::path manuallySearchPathDirectories(const std::string& fileName);
	std::vector<std::string> getPathsFromEnvironmentVariable();


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


	// JSON stuff
	std::string readContent(const fs::path& FileName);
	void writeJsonToFile(const fs::path& filePath, const json& jsonData);
	json getJsonFromFile(const fs::path& filePath);


	// CustomQuickchat filepaths
	fs::path customQuickchatFolder;
	fs::path bindingsFilePath;
	fs::path variationsFilePath;
	fs::path speechToTextFilePath;
	fs::path speechToTextPyScriptFilePath;
	fs::path pyInterpreter;
	fs::path cfgPath;

	// Lobby Info filepaths
	fs::path lobbyInfoFolder;
	fs::path lobbyInfoChatsFilePath;
	fs::path lobbyInfoRanksFilePath;



	// cvar helper stuff
	CVarWrapper RegisterCvar_Bool(const Cvars::CvarData& cvar, bool startingValue);
	CVarWrapper RegisterCvar_String(const Cvars::CvarData& cvar, const std::string& startingValue);
	CVarWrapper RegisterCvar_Number(const Cvars::CvarData& cvar, float startingValue, bool hasMinMax = false, float min = 0, float max = 0);
	CVarWrapper RegisterCvar_Color(const Cvars::CvarData& cvar, const std::string& startingValue);
	CVarWrapper GetCvar(const Cvars::CvarData& cvar);
	void RegisterCommand(const Cvars::CvarData& cvar, std::function<void(std::vector<std::string>)> callback);
	void RunCommand(const Cvars::CvarData& command, float delaySeconds = 0);
	void RunCommandInterval(const Cvars::CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand = false);


	// commands
	void cmd_toggleEnabled(std::vector<std::string> args);
	void cmd_showPathDirectories(std::vector<std::string> args);
	void cmd_listBindings(std::vector<std::string> args);
	void cmd_exitToMainMenu(std::vector<std::string> args);
	void cmd_forfeit(std::vector<std::string> args);
	void cmd_test(std::vector<std::string> args);

	// cvar change callbacks
	void changed_enabled(std::string cvarName, CVarWrapper updatedCvar);
	void changed_autoDetectInterpreterPath(std::string cvarName, CVarWrapper updatedCvar);
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
	void Event_HUDDestroyed(ActorWrapper caller, void* params, std::string eventName);
	void Event_PushMenu(ActorWrapper caller, void* params, std::string eventName);
	void Event_PopMenu(ActorWrapper caller, void* params, std::string eventName);
	void Event_LoadingScreenStart(std::string eventName);

public:
	// GUI
	void RenderSettings() override;
	void RenderWindow() override;

	void GeneralSettings();
	void ChatTimeoutSettings();
	void SpeechToTextSettings();

	void RenderAllBindings();
	void RenderBindingDetails();
	
	void RenderAllVariationListNames();
	void RenderVariationListDetails();
};
