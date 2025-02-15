#pragma once
#include <string>


namespace Cvars
{
	const std::string prefix = "cqc_";

	struct CvarData
	{
		std::string name;
		std::string description;

		// constructor to automatically add prefix to name
		CvarData(const std::string& name, const std::string& description)
			: name(prefix + name), description(description) {}
	};


	// ========================= vars ========================

	// bools
	const CvarData enabled =					{ "enabled",						"Toggle custom quick chats on or off" };
	const CvarData enableSTTNotifications =		{ "enable_stt_notifications",		"Toggle speech-to-text notifications on or off" };
	const CvarData autoDetectInterpreterPath =	{ "auto_detect_interpreter_path",	"Automatically detect python interpreter filepath" };
	const CvarData overrideDefaultQuickchats =	{ "override_default_quickchats",	"override default quickchat with custom one if they share the same binding" };
	const CvarData blockDefaultQuickchats =		{ "block_default_quickchats",		"block default quickchats (without unbinding them)" };
	const CvarData searchForPyInterpreter =		{ "search_for_python_interpreter",	"search for full path to pythonw.exe, instead of using the 'pythonw' command line argument" };
	const CvarData disablePostMatchQuickchats =	{ "disable_post_match_quickchats",	"disable custom quickchats in the post match screen" };
	const CvarData disableChatTimeout =			{ "disable_chat_timeout",			"disable chat timeout in freeplay" };
	const CvarData useCustomChatTimeoutMsg =	{ "enable_custom_chat_timeout_msg",	"enable custom chat timeout message" };
	const CvarData removeTimestamps =			{ "remove_timestamps",				"remove default chat timestamps" };
	const CvarData autoCalibrateMic =			{ "auto_calibrate_mic",				"automatically calibrate microphone on each speech-to-text attempt" };

	// numbers
	const CvarData sequenceTimeWindow =			{ "button_sequence_time_window",	"time window given for button sequence macros" };
	const CvarData minBindingDelay =			{ "min_binding_delay",				"minimum delay between binding activation (prevents accidental activation)" };
	const CvarData beginSpeechTimeout =			{ "begin_speech_timeout",			"timeout for starting speech" };
	const CvarData notificationDuration =		{ "notification_duration",			"how long a popup notification will stay on the screen" };
	const CvarData speechProcessingTimeout =	{ "speech_processing_timeout",		"timeout for processing speech" };
	const CvarData micCalibrationTimeout =		{ "mic_calibration_timeout",		"max time to spend calibrating microphone before aborting" };
	const CvarData micEnergyThreshold =			{ "mic_energy_threshold",			"minimum audio energy to consider for recording speech" };
	const CvarData websocket_port =				{ "websocket_port",					"port number used for speech-to-text websocket connection" };

	// strings
	const CvarData pythonInterpreterPath =		{ "python_interpreter_path",		"filepath to python interpreter" };
	const CvarData customChatTimeoutMsg =		{ "custom_chat_timeout_msg",		"custom chat timeout message" };


	// ======================= commands ======================

	const CvarData toggleEnabled =				{ "toggle_enabled",					"toggle turning custom quidchats on/off"};
	const CvarData list_custom_chat_labels =	{ "list_custom_chat_labels",		"list all custom chat labels that will show up in quickchat UI" };
	const CvarData listBindings =				{ "list_bindings",					"show a list of all current bindings"};
	const CvarData exitToMainMenu =				{ "exit_to_main_menu",				"exit to the main menu"};
	const CvarData forfeit =					{ "forfeit",						"forfeit the current match"};
	const CvarData test =						{ "test",							"test"};
	const CvarData test2 =						{ "test2",							"test 2"};
}
