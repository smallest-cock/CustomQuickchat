#pragma once
#include <string>


namespace CvarNames
{
	const std::string prefix = "customquickchat_";

	// ========================= vars ========================

	// bools
	const std::string enabled = prefix +							"enabled";
	const std::string enableSTTNotifications = prefix +				"enable_stt_notifications";

	// numbers
	const std::string sequenceTimeWindow = prefix +					"button_sequence_time_window";
	const std::string beginSpeechTimeout = prefix +					"begin_speech_timeout";
	const std::string notificationDuration = prefix +				"notification_duration";
	const std::string speechProcessingTimeout = prefix +			"speech_processing_timeout";


	// ======================= commands ======================

	const std::string toggleEnabled = prefix +						"toggle_enabled";
	const std::string test = prefix +								"test";
}	