#pragma once


#define CVAR(name, desc) CvarData("cqc_" name, desc)	// automatically apply plugin prefix

struct CvarData
{
	const char* name;
	const char* description;
};

namespace Cvars
{
	// bools
	constexpr CvarData enabled =						CVAR("enabled",							"Toggle custom quick chats on or off");
	constexpr CvarData enableSTTNotifications =			CVAR("enable_stt_notifications",		"Toggle speech-to-text notifications on or off");
	constexpr CvarData autoDetectInterpreterPath =		CVAR("auto_detect_interpreter_path",	"Automatically detect python interpreter filepath");
	constexpr CvarData overrideDefaultQuickchats =		CVAR("override_default_quickchats",		"override default quickchat with custom one if they share the same binding");
	constexpr CvarData blockDefaultQuickchats =			CVAR("block_default_quickchats",		"block default quickchats (without unbinding them)");
	constexpr CvarData searchForPyInterpreter =			CVAR("search_for_python_interpreter",	"search for full path to pythonw.exe, instead of using the 'pythonw' command line argument");
	constexpr CvarData disablePostMatchQuickchats =		CVAR("disable_post_match_quickchats",	"disable custom quickchats in the post match screen");
	constexpr CvarData disableChatTimeout =				CVAR("disable_chat_timeout",			"disable chat timeout in freeplay");
	constexpr CvarData useCustomChatTimeoutMsg =		CVAR("enable_custom_chat_timeout_msg",	"enable custom chat timeout message");
	constexpr CvarData removeTimestamps =				CVAR("remove_timestamps",				"remove default chat timestamps");
	constexpr CvarData autoCalibrateMic =				CVAR("auto_calibrate_mic",				"automatically calibrate microphone on each speech-to-text attempt");
	constexpr CvarData user_chats_in_last_chat =		CVAR("user_chats_in_last_chat",			"include user chats in search for last chat");
	constexpr CvarData teammate_chats_in_last_chat =	CVAR("teammate_chats_in_last_chat",		"include teammate chats in search for last chat");
	constexpr CvarData quickchats_in_last_chat =		CVAR("quickchats_in_last_chat",			"include quickchats in search for last chat");
	constexpr CvarData party_chats_in_last_chat =		CVAR("party_chats_in_last_chat",		"include party chats in search for last chat");
	constexpr CvarData team_chats_in_last_chat =		CVAR("team_chats_in_last_chat",			"include team chats in search for last chat");
	constexpr CvarData randomize_sarcasm =				CVAR("randomize_sarcasm",				"include team chats in search for last chat");

	// numbers
	constexpr CvarData sequenceTimeWindow =				CVAR("button_sequence_time_window",		"time window given for button sequence macros");
	constexpr CvarData minBindingDelay =				CVAR("min_binding_delay",				"minimum delay between binding activation (prevents accidental activation)");
	constexpr CvarData beginSpeechTimeout =				CVAR("begin_speech_timeout",			"timeout for starting speech");
	constexpr CvarData notificationDuration =			CVAR("notification_duration",			"how long a popup notification will stay on the screen");
	constexpr CvarData speechProcessingTimeout =		CVAR("speech_processing_timeout",		"timeout for processing speech");
	constexpr CvarData micCalibrationTimeout =			CVAR("mic_calibration_timeout",			"max time to spend calibrating microphone before aborting");
	constexpr CvarData micEnergyThreshold =				CVAR("mic_energy_threshold",			"minimum audio energy to consider for recording speech");
	constexpr CvarData websocket_port =					CVAR("websocket_port",					"port number used for speech-to-text websocket connection");

	// strings
	constexpr CvarData pythonInterpreterPath =			CVAR("python_interpreter_path",			"filepath to python interpreter");
	constexpr CvarData customChatTimeoutMsg =			CVAR("custom_chat_timeout_msg",			"custom chat timeout message");
}


namespace Commands
{
	constexpr CvarData toggleEnabled =					CVAR("toggle_enabled",					"toggle turning custom quidchats on/off");
	constexpr CvarData list_custom_chat_labels =		CVAR("list_custom_chat_labels",			"list all custom chat labels that will show up in quickchat UI");
	constexpr CvarData list_playlist_info =				CVAR("list_playlist_info",				"list info for all playlists in the game");
	constexpr CvarData listBindings =					CVAR("list_bindings",					"show a list of all current bindings");
	constexpr CvarData exitToMainMenu =					CVAR("exit_to_main_menu",				"exit to the main menu");
	constexpr CvarData forfeit =						CVAR("forfeit",							"forfeit the current match");
	constexpr CvarData test =							CVAR("test",							"test");
	constexpr CvarData test2 =							CVAR("test2",							"test 2");
}

#undef CVAR