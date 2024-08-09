#pragma once
#include <string>


namespace CvarNames
{
	const std::string prefix = "cqc_";

	// ========================= vars ========================

	// bools
	const std::string enabled = prefix +							"enabled";
	const std::string enableSTTNotifications = prefix +				"enable_stt_notifications";
	const std::string autoDetectInterpreterPath = prefix +			"auto_detect_interpreter_path";
	const std::string overrideDefaultQuickchats = prefix +			"override_default_quickchats";
	const std::string blockDefaultQuickchats = prefix +				"block_default_quickchats";

	// numbers
	const std::string sequenceTimeWindow = prefix +					"button_sequence_time_window";
	const std::string beginSpeechTimeout = prefix +					"begin_speech_timeout";
	const std::string notificationDuration = prefix +				"notification_duration";
	const std::string speechProcessingTimeout = prefix +			"speech_processing_timeout";

	// strings
	const std::string pythonInterpreterPath = prefix +				"python_interpreter_path";


	// ======================= commands ======================

	const std::string toggleEnabled = prefix +						"toggle_enabled";
	const std::string showPathDirectories = prefix +				"show_path_directories";
	const std::string listBindings = prefix +						"list_bindings";
	const std::string test = prefix +								"test";
}	