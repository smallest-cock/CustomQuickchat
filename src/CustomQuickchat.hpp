#pragma once
#include "GuiBase.h"
#include <bakkesmod/plugin/bakkesmodplugin.h>
#include <bakkesmod/plugin/pluginwindow.h>
#include <bakkesmod/plugin/PluginSettingsWindow.h>

#include "version.h"
#include <ModUtils/util/Utils.hpp>
#include <memory>
#include "Structs.hpp"
#include "Bindings.hpp"
#include "Cvars.hpp"

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(
    VERSION_BUILD);

constexpr auto plugin_version_display = "v" VERSION_STR
#ifdef USE_SPEECH_TO_TEXT
                                        "\t(with speech-to-text)"
#endif
    ;

class CustomQuickchat : public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase, public PluginWindowBase
{
	// Boilerplate
	void onLoad() override;
	void onUnload() override;

private:
	// plugin init
	void initStuffOnLoad();
	void initCvars();
	void initCommands();
	void initHooks();
	void initJsonFiles();
	void initFilePaths();
	void updateVariationsFromJson();
	void updateBindingsFromJson();
	void updateDataFromJson();

private:
	// plugin boilerplate helper stuff
	CVarWrapper registerCvar_Bool(const CvarData& cvar, bool startingValue);
	CVarWrapper registerCvar_String(const CvarData& cvar, const std::string& startingValue);
	CVarWrapper registerCvar_Number(const CvarData& cvar, float startingValue, bool hasMinMax = false, float min = 0, float max = 0);
	CVarWrapper registerCvar_Color(const CvarData& cvar, const std::string& startingValue);
	void        registerCommand(const CvarData& cvar, std::function<void(std::vector<std::string>)> callback);
	CVarWrapper getCvar(const CvarData& cvar);

	void hookEvent(const char* funcName, std::function<void(std::string)> callback);
	void hookEventPost(const char* funcName, std::function<void(std::string)> callback);
	void hookWithCaller(const char* funcName, std::function<void(ActorWrapper, void*, std::string)> callback);
	void hookWithCallerPost(const char* funcName, std::function<void(ActorWrapper, void*, std::string)> callback);

	void runCommand(const CvarData& command, float delaySeconds = 0.0f);
	void autoRunCommand(const CvarData& autoRunBool, const CvarData& command, float delaySeconds = 0.0f);
	void runCommandInterval(const CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand = false);
	void autoRunCommandInterval(
	    const CvarData& autoRunBool, const CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand = false);

private:
	// cvar values
	std::shared_ptr<bool> m_enabled                    = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_overrideDefaultQuickchats  = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_blockDefaultQuickchats     = std::make_shared<bool>(false);
	std::shared_ptr<bool> m_disablePostMatchQuickchats = std::make_shared<bool>(false);
	std::shared_ptr<bool> m_disableChatTimeout         = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_useCustomChatTimeoutMsg    = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_removeTimestamps           = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_randomizeSarcasm           = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_uncensorChats              = std::make_shared<bool>(true);

	std::shared_ptr<float> m_sequenceTimeWindow = std::make_shared<float>(2.0f);
	std::shared_ptr<float> m_minBindingDelay    = std::make_shared<float>(0.05f);

	std::shared_ptr<std::string> m_customChatTimeoutMsg = std::make_shared<std::string>("Wait [Time] seconds lil bro");

private:
	std::string h_label;

	// constants
	static constexpr const char* KEYWORD_REGEX_PATTERN          = R"(\[\[(.*?)\]\])";
	static constexpr double      BLOCK_DEFAULT_QUICKCHAT_WINDOW = 0.1; // maybe turn into a cvar w slider in settings
	static constexpr int         MAX_KEYWORD_DEPTH              = 10;
	static constexpr auto        DEFAULT_CHAT_TIMEOUT_MSG       = "Chat disabled for [Time] second(s).";

	// flags
	bool m_onLoadComplete = false;
	bool m_gamePaused     = false;
	bool m_matchEnded     = false;
	bool m_inGameEvent    = false;
	bool m_chatboxOpen    = false;

	// filepaths
	fs::path m_pluginFolder;
	fs::path m_bindingsJsonPath;
	fs::path m_variationsJsonPath;

	// bindings & variations stuff
	std::vector<std::shared_ptr<Binding>> m_bindings;
	std::vector<VariationList>            m_variations;

	int m_selectedBindingIndex   = 0;
	int m_selectedVariationIndex = 0;

	BindingDetectionManager m_bindingManager;

	// modify quickchat UI stuff
	static constexpr std::array<const char*, 4> PRESET_GROUP_NAMES = {"ChatPreset1", "ChatPreset2", "ChatPreset3", "ChatPreset4"};
	std::array<std::array<FString, 4>, 4>       m_pcQcLabels;
	std::array<std::array<FString, 4>, 4>       m_gamepadQcLabels;
	bool                                        m_usingGamepad = false;

private:
	void updateBindingsData();
	bool writeBindingsToJson();
	void writeVariationsToJson();

	void addEmptyBinding();
	void addEmptyVariationList();

	void deleteBinding(int idx);
	void DeleteVariationList(int idx);
	void updateAllVariationsData();

	std::string              getVariationFromList(const std::string& listName);
	std::vector<std::string> ShuffleWordList(const std::vector<std::string>& ogList);
	void                     ReshuffleWordList(int idx);

	void        performBindingAction(const Binding& binding);
	std::string process_keywords_in_chat_str(const Binding& binding);

	/*
	// recieving chats
	FString m_censoredChatSave;
	*/

	// chat timeout
	std::string getChatTimeoutMsg() { return *m_useCustomChatTimeoutMsg ? *m_customChatTimeoutMsg : DEFAULT_CHAT_TIMEOUT_MSG; }
	void        determineQuickchatLabels(UGFxData_Controls_TA* controls = nullptr, bool log = false);
	void        apply_custom_qc_labels_to_ui(UGFxData_Chat_TA* caller, UGFxData_Chat_TA_execOnPressChatPreset_Params* params = nullptr);
	void        apply_all_custom_qc_labels_to_ui(UGFxData_Chat_TA* caller);

	// misc functions
	void                       NotifyAndLog(const std::string& title, const std::string& message, int duration = 3);
	std::optional<std::string> getClosestPlayer(EKeyword keyword = EKeyword::ClosestPlayer);
	std::optional<std::string> getCurrentRumbleItem();

	void no_speech_to_text_warning();

public:
	// sending chats
	void        SendChat(const std::string& chat, EChatChannel chatMode);
	std::string ApplyTextEffect(const std::string& originalText, ETextEffect effect);

	// gui
public:
	void RenderSettings() override;
	void RenderWindow() override;

private:
	void display_generalSettings();
	void display_chatTimeoutSettings();

	void display_bindingsList();
	void display_bindingDetails();
	void display_bindingChatDetails(const std::shared_ptr<Binding>& selectedBinding);
	void display_bindingTriggerDetails(const std::shared_ptr<Binding>& selectedBinding);

	void display_variationListList();
	void display_variationListDetails();

	// header/footer stuff
	static constexpr float FOOTER_HEIGHT = 40.0f;
};
