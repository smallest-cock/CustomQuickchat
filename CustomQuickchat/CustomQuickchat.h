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
#include "GuiTools.hpp"
#include "Components/Includes.hpp"


#define USE_SPEECH_TO_TEXT


constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

#if !defined(USE_SPEECH_TO_TEXT)
constexpr auto pretty_plugin_version = "v" stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH);
#else
constexpr auto pretty_plugin_version = "v" stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "\t(with speech-to-text)";
#endif


#ifdef USE_SPEECH_TO_TEXT

struct ActiveSTTAttempt
{
	std::string attemptID;
	Binding binding;
};

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
	static constexpr int MAX_KEYWORD_DEPTH = 10;

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
	std::string process_keywords_in_chat_str(const Binding& binding);


	// Lobby Info stuff (last chat & blast ranks)
	std::string get_last_chat();
	std::string get_last_chatter_rank_str(EKeyword keyword);
	ChatterRanks get_last_chatter_ranks();


	// sending chat stuff
	void SendChat(const std::string& chat, EChatChannel chatMode);
	std::string ApplyTextEffect(const std::string& originalText, ETextEffect effect);


	// chat timeout stuff
	std::string chatTimeoutMsg = "Chat disabled for [Time] second(s).";
	void ResetChatTimeoutMsg();


	// modify quickchat UI stuff
	static constexpr std::array<const char*, 4> preset_group_names = { "ChatPreset1", "ChatPreset2", "ChatPreset3", "ChatPreset4" };
	std::array<std::array<FString, 4>, 4>	pc_qc_labels;
	std::array<std::array<FString, 4>, 4>	gp_qc_labels;
	bool using_gamepad = false;

	void determine_quickchat_labels(UGFxData_Controls_TA* controls = nullptr, bool log = false);
	void apply_custom_qc_labels_to_ui(UGFxData_Chat_TA* caller, UGFxData_Chat_TA_execOnPressChatPreset_Params* params = nullptr);
	void apply_all_custom_qc_labels_to_ui(UGFxData_Chat_TA* caller);


	// misc functions
	void NotifyAndLog(const std::string& title, const std::string& message, int duration = 3);
	


#if defined(USE_SPEECH_TO_TEXT)

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
	ActiveSTTAttempt Active_STT_Attempt;

	// websocket stuff
	static constexpr float START_WS_CLIENT_DELAY = 5.0f;	// in seconds

	std::atomic<bool> connecting_to_ws_server{false};
	std::shared_ptr<WebsocketClientManager> Websocket = nullptr;
	Process::ProcessHandles stt_python_server_process;
	
	void start_websocket_stuff(bool onLoad = false);
	bool start_websocket_server();
	void stop_websocket_server();
	void process_ws_response(const json& response);


	// speech-to-text
	void StartSpeechToText(const Binding& binding);
	json generate_data_for_STT_attempt();
	void process_STT_result(const json& response_data);

	
	// mic calibration
	void CalibrateMicrophone();
	json generate_data_for_mic_calibration_attempt();
	void process_mic_calibration_result(const json& response_data);


	// misc
	std::string generate_STT_attempt_id();
	void ClearSttErrorLog();

	std::string CreateCommandString(const fs::path& executablePath, const std::vector<std::string>& args);
	void STTLog(const std::string& message);


#else

	void no_speech_to_text_warning();

#endif



	// commands
	void cmd_toggleEnabled(std::vector<std::string> args);
	void cmd_listBindings(std::vector<std::string> args);
	void cmd_list_custom_chat_labels(std::vector<std::string> args);
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
	void Event_GFxHUD_TA_ChatPreset(ActorWrapper caller, void* params, std::string eventName);
	void Event_OnPressChatPreset(ActorWrapper caller, void* params, std::string eventName);
	void Event_InitUIBindings(ActorWrapper caller, void* params, std::string eventName);
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


	// header/footer stuff
	void gui_footer_init();
	bool assets_exist =	false;
	std::shared_ptr<GUI::FooterLinks> footer_links;

	static constexpr float header_height =					80.0f;
	static constexpr float footer_height =					40.0f;
	static constexpr float footer_img_height =				25.0f;

	static constexpr const wchar_t* github_link =			L"https://github.com/smallest-cock/CustomQuickchat";
	static constexpr const char* github_link_tooltip =		"See the sauce";
};
