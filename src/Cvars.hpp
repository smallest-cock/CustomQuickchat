#pragma once

#define CVAR(name, desc)                                                                                                                   \
	CvarData { "cqc_" name, desc }

struct CvarData {
	const char *name;
	const char *description;
};

namespace Cvars {
	// bools
	constexpr auto enabled                    = CVAR("enabled", "Toggle custom quick chats on or off");
	constexpr auto blockDefaultQuickchats     = CVAR("block_default_quickchats", "block default quickchats (without unbinding them)");
	constexpr auto useCustomChatTimeoutMsg    = CVAR("enable_custom_chat_timeout_msg", "enable custom chat timeout message");
	constexpr auto removeTimestamps           = CVAR("remove_timestamps", "remove default chat timestamps");
	constexpr auto randomizeSarcasm           = CVAR("randomize_sarcasm", "randomize the sarcasm text effect");
	constexpr auto disablePostMatchQuickchats = CVAR("disable_post_match_quickchats", "disable custom quickchats in the post match screen");
	constexpr auto overrideDefaultQuickchats  = CVAR(
        "override_default_quickchats", "override default quickchat with custom one if they share the same binding");

	// numbers
	constexpr auto sequenceTimeWindow = CVAR("button_sequence_time_window", "time window given for button sequence macros");
	constexpr auto minBindingDelay = CVAR("min_binding_delay", "minimum delay between binding activation (prevents accidental activation)");

	// strings
	constexpr auto customChatTimeoutMsg = CVAR("custom_chat_timeout_msg", "custom chat timeout message");

	// === lobby info settings (bools) ===
	constexpr auto userChatsInLastChat     = CVAR("user_chats_in_last_chat", "include user chats in search for last chat");
	constexpr auto teammateChatsInLastChat = CVAR("teammate_chats_in_last_chat", "include teammate chats in search for last chat");
	constexpr auto quickchatsInLastChat    = CVAR("quickchats_in_last_chat", "include quickchats in search for last chat");
	constexpr auto partyChatsInLastChat    = CVAR("party_chats_in_last_chat", "include party chats in search for last chat");
	constexpr auto teamChatsInLastChat     = CVAR("team_chats_in_last_chat", "include team chats in search for last chat");

#ifdef USE_SPEECH_TO_TEXT
	constexpr auto enableSTTNotifications = CVAR("enable_stt_notifications", "Toggle speech-to-text notifications on or off");
	constexpr auto autoCalibrateMic       = CVAR("auto_calibrate_mic", "automatically calibrate microphone on each speech-to-text attempt");
	constexpr auto beginSpeechTimeout     = CVAR("begin_speech_timeout", "timeout for starting speech");
	constexpr auto notificationDuration   = CVAR("notification_duration", "how long a popup notification will stay on the screen");
	constexpr auto speechProcessingTimeout = CVAR("speech_processing_timeout", "timeout for processing speech");
	constexpr auto micCalibrationTimeout   = CVAR("mic_calibration_timeout", "max time to spend calibrating microphone before aborting");
	constexpr auto micEnergyThreshold      = CVAR("mic_energy_threshold", "minimum audio energy to consider for recording speech");
	constexpr auto websocketPort           = CVAR("websocket_port", "port number used for speech-to-text websocket connection");
#endif
}

namespace Commands {
	constexpr auto toggleEnabled        = CVAR("toggle_enabled", "toggle turning custom quidchats on/off");
	constexpr auto listCustomChatLabels = CVAR("list_custom_chat_labels", "list all custom chat labels that will show up in quickchat UI");
	constexpr auto listPlaylistInfo     = CVAR("list_playlist_info", "list info for all playlists in the game");
	constexpr auto listBindings         = CVAR("list_bindings", "show a list of all current bindings");
	constexpr auto exitToMainMenu       = CVAR("exit_to_main_menu", "exit to the main menu");
	constexpr auto forfeit              = CVAR("forfeit", "forfeit the current match");
	constexpr auto sendChatMatch        = CVAR("send_chat_match", "Send match chat");
	constexpr auto sendChatTeam         = CVAR("send_chat_team", "Send team chat");
	constexpr auto sendChatParty        = CVAR("send_chat_party", "Send party chat");
	constexpr auto test                 = CVAR("test", "test");
	constexpr auto test2                = CVAR("test2", "test 2");
}

#undef CVAR
