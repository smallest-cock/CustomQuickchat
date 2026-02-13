#pragma once
#include "boilerplate/GuiBase.hpp"
#include "boilerplate/PluginHelperBase.hpp"
#include <bakkesmod/plugin/bakkesmodplugin.h>
#include <bakkesmod/plugin/pluginwindow.h>
#include <bakkesmod/plugin/PluginSettingsWindow.h>
#include "version.h"
#include <atomic>
#include <memory>

#include "RLSDK/RLSDK_w_pch_includes/GameDefines.hpp"
#include <ModUtils/util/Utils.hpp>
#include "Structs.hpp"
#include "Bindings.hpp"

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(
    VERSION_BUILD);

constexpr auto plugin_version_display = "v" VERSION_STR
#ifdef USE_SPEECH_TO_TEXT
                                        "\t(with speech-to-text)"
#endif
    ;

struct FNameCache {
	FNameMemo midgameMenuMovie;
};

class CustomQuickchat : public BakkesMod::Plugin::BakkesModPlugin,
                        public SettingsWindowBase,
                        public PluginWindowBase,
                        public PluginHelperBase {
	void                                onLoad() override;
	void                                onUnload() override;
	BakkesMod::Plugin::BakkesModPlugin *getPlugin() override;

private:
	// plugin init
	void initOtherPluginStuff();
	void initCvars();
	void initCommands();
	void initHooks();
	void initJsonFiles();
	void initFilePaths();
	void updateVariationsFromJson();
	void updateBindingsFromJson();
	void updateDataFromJson();

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
	// constants
	static constexpr const char *KEYWORD_REGEX_PATTERN          = R"(\[\[(.*?)\]\])";
	static constexpr double      BLOCK_DEFAULT_QUICKCHAT_WINDOW = 0.1; // maybe turn into a cvar w slider in settings
	static constexpr int         MAX_KEYWORD_DEPTH              = 10;
	static constexpr auto        DEFAULT_CHAT_TIMEOUT_MSG       = "Chat disabled for [Time] second(s).";

	// flags
	std::atomic_bool m_blockNextDefaultQuickchat = false;
	bool             m_onLoadComplete            = false;
	bool             m_gamePaused                = false;
	bool             m_matchEnded                = false;
	bool             m_inGameEvent               = false;
	bool             m_chatboxOpen               = false;

	// filepaths
	fs::path m_pluginFolder;
	fs::path m_bindingsJsonPath;
	fs::path m_variationsJsonPath;

	// misc
	std::string h_label;
	FNameCache  m_fnameCache;

	// bindings & variations stuff
	std::vector<std::shared_ptr<Binding>> m_bindings;
	std::vector<VariationList>            m_variations;

	int m_selectedBindingIndex   = 0;
	int m_selectedVariationIndex = 0;

	BindingDetectionManager m_bindingManager;

	// modify quickchat UI stuff
	std::array<FName, 4>                  m_chatPresetGroupNames;
	std::array<std::array<FString, 4>, 4> m_pcQcLabels;
	std::array<std::array<FString, 4>, 4> m_gamepadQcLabels;
	bool                                  m_usingGamepad             = false;
	std::atomic_bool                      m_shouldBlockNextQuickchat = false;

private:
	void updateBindingsData();
	bool writeBindingsToJson();
	void writeVariationsToJson();

	void addEmptyBinding();
	void addEmptyVariationList();

	void deleteBinding(int idx);
	void deleteVariationList(int idx);
	void updateAllVariationsData();

	std::string              getVariationFromList(const std::string &listName);
	std::vector<std::string> shuffleWordList(const std::vector<std::string> &ogList);
	void                     reshuffleWordList(int idx);

	void        performBindingAction(const Binding &binding);
	std::string processKeywordsInChatStr(const Binding &binding);

	// chat timeout
	std::string getChatTimeoutMsg() { return *m_useCustomChatTimeoutMsg ? *m_customChatTimeoutMsg : DEFAULT_CHAT_TIMEOUT_MSG; }
	void        setChatTimeoutMsg(const std::string &newMsg, AGFxHUD_TA *hud = nullptr);
	void        determineQuickchatLabels(UGFxData_Controls_TA *controls = nullptr, bool log = false);
	void        applyCustomQcLabelsToUi(UGFxData_Chat_TA *caller, UGFxData_Chat_TA_execOnPressChatPreset_Params *params = nullptr);
	void        applyAllCustomQcLabelsToUi(UGFxData_Chat_TA *caller);

	// misc functions
	void                       notifyAndLog(const std::string &title, const std::string &message, int duration = 3);
	std::optional<std::string> getClosestPlayer(EKeyword keyword = EKeyword::ClosestPlayer);
	std::optional<std::string> getCurrentRumbleItem();

#ifndef USE_SPEECH_TO_TEXT
	void warnNoSTT();
#endif

public:
	// sending chats
	void        sendChat(const std::string &chat, EChatChannel chatMode);
	std::string applyTextEffect(const std::string &originalText, ETextEffect effect);

	// gui
public:
	void RenderSettings() override;
	void RenderWindow() override;

private:
	static constexpr float FOOTER_HEIGHT = 40.0f;

	void display_generalSettings();
	void display_chatTimeoutSettings();

	void display_bindingsList();
	void display_bindingDetails();
	void display_bindingChatDetails(const std::shared_ptr<Binding> &selectedBinding);
	void display_bindingTriggerDetails(const std::shared_ptr<Binding> &selectedBinding);

	void display_variationListList();
	void display_variationListDetails();
};
