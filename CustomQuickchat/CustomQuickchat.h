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

#include "Events.hpp"
#include "CvarNames.hpp"
#include "Components/Includes.hpp"

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);
constexpr auto pretty_plugin_version = "v" stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH);


const double BLOCK_DEFAULT_QUICKCHAT_WINDOW = 0.05;


class CustomQuickchat : public BakkesMod::Plugin::BakkesModPlugin
	,public SettingsWindowBase
	,public PluginWindowBase
{

	//Boilerplate
	void onLoad() override;
	void onUnload() override;

	bool onLoadComplete = false;

	void SendChat(const std::string& chat, const std::string& chatMode);

	// speech-to-text
	void StartSpeechToText(const std::string& chatMode, const std::string& effect = "", bool test = false, bool calibrateMic = false);
	void STTWaitAndProbe(const std::string& chatMode, const std::string& effect, const std::string& attemptID, bool test);
	void STTLog(const std::string& message);
	void ResetJsonFile(float micCalibration = 69420);
	bool ClearTranscriptionJson();
	std::string ActiveSTTAttemptID = "420_blz_it_lmao";
	double micEnergyThreshold = 420;
	void UpdateMicCalibration(float timeOut);

	void InitKeyStates();
	bool Sequence(const std::string& button1, const std::string& button2);
	bool Combine(const std::vector<std::string>& buttons);

	int FindButtonIndex(const std::string& buttonName);

	void ResetFirstButtonPressed(const std::string& scope = "global");
	
	void CheckJsonFiles();
	void GetFilePaths();

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
	std::string readContent(const fs::path& FileName);
	void writeJsonToFile(const fs::path& filePath, const json& jsonData);
	json getJsonFromFile(const fs::path& filePath);

	void UpdateData();
	void PreventGameFreeze();	// hacky solution to prevent game hanging for few seconds on 1st chat sent

	std::vector<Binding> Bindings;
	std::vector<VariationList> Variations;

	int selectedBindingIndex = 0;
	int selectedVariationIndex = 0;

	std::unordered_map<std::string, bool> keyStates;
	std::unordered_map<std::string, ButtonPress> sequenceStoredButtonPresses;

	fs::path findPythonInterpreter();
	fs::path findInterpreterUsingSearchPathW(const wchar_t* fileName);
	fs::path manuallySearchPathDirectories(const std::string& fileName);
	std::vector<std::string> getPathsFromEnvironmentVariable();

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


	std::chrono::steady_clock::time_point lastCustomChatSent;

	// commands
	void cmd_toggleEnabled(std::vector<std::string> args);
	void cmd_showPathDirectories(std::vector<std::string> args);
	void cmd_listBindings(std::vector<std::string> args);
	void cmd_test(std::vector<std::string> args);

	// cvar change callbacks
	void changed_enabled(std::string cvarName, CVarWrapper updatedCvar);
	void changed_autoDetectInterpreterPath(std::string cvarName, CVarWrapper updatedCvar);
	void changed_enableSTTNotifications(std::string cvarName, CVarWrapper updatedCvar);
	void changed_overrideDefaultQuickchats(std::string cvarName, CVarWrapper updatedCvar);
	void changed_blockDefaultQuickchats(std::string cvarName, CVarWrapper updatedCvar);

	// hook callbacks
	void Event_KeyPressed(ActorWrapper caller, void* params, std::string eventName);
	void Event_ChatPresetPressed(ActorWrapper caller, void* params, std::string eventName);
	void Event_ApplyChatSpamFilter(ActorWrapper caller, void* params, std::string eventName);

public:
	// GUI
	void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	void RenderWindow() override; // Uncomment if you want to render your own plugin window

	void RenderAllBindings();
	void RenderBindingDetails();
	
	void RenderAllVariationListNames();
	void RenderVariationListDetails();
};
