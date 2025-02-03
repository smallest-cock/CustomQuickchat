#include "pch.h"
#include "CustomQuickchat.h"



#if !defined(USE_SPEECH_TO_TEXT)

void CustomQuickchat::no_speech_to_text_warning()
{
	std::string message = "This version doesnt support speech-to-text. You can find that version on the github Releases page";
	NotifyAndLog("Speech-To-Text", message, 5);
}

#else


void CustomQuickchat::StartSpeechToText(const Binding& binding)
{

	if (attemptingSTT)
	{
		STTLog("Speech-to-text is already active!");
		return;
	}

	// update Active_STT_Attempt data
	Active_STT_Attempt.binding = binding;
	Active_STT_Attempt.attemptID = generate_STT_attempt_id();

	json data = generate_data_for_STT_attempt();
	if (data.empty())
	{
		STTLog("Error generating JSON data for speech-to-text attempt");
		return;
	}

	Websocket->SendEvent("start_speech_to_text", data);

	attemptingSTT = true;
}


void CustomQuickchat::start_websocket_stuff(bool onLoad)
{
	if (!onLoad && Websocket && Websocket->IsConnectedToServer())
	{
		LOG("[ERROR] Failed to start websocket stuff... we're already connected on port {}!", Websocket->get_port_str());
		return;
	}

	auto ws_client_setup = [this, onLoad]()
	{
		auto websocket_port_cvar = GetCvar(Cvars::websocket_port);
		if (!websocket_port_cvar)
		{
			LOG("[ERROR] websocket_port cvar invalid");
			*connecting_to_ws_server = false;
			return;
		}
		int websocket_port = websocket_port_cvar.getIntValue();

		if (onLoad)
		{
			// create websocket object
			std::function<void(json serverResponse)> ws_response_callback = std::bind(&CustomQuickchat::process_ws_response, this, std::placeholders::_1);
			Websocket = std::make_shared<WebsocketClientManager>(ws_response_callback, connecting_to_ws_server);
		}

		const float delay_s = onLoad ? START_WS_CLIENT_DELAY + 3.0f : START_WS_CLIENT_DELAY;	// wait 3 more seconds when plugin loads


		auto start_the_client = [this, websocket_port]()
		{
			bool success = Websocket->StartClient(websocket_port);
			LOG(success ? "Starting websocket client was successful" : "Starting websocket client was unsuccessful");

			if (!success)
			{
				*connecting_to_ws_server = false;
			}
		};

		// wait x seconds after python websocket server has started to start client
		DELAY_CAPTURE(delay_s,

			start_the_client();

		, start_the_client);
	};

	// start websocket sever (spawn python process)
	bool success = start_websocket_server();
	LOG("[onLoad] Ran start_websocket_server()");
	if (!success) return;

	if (onLoad)
	{
		// wait 1 second before starting websocket client stuff (so websocket_port cvar value gets a chance to load)
		DELAY_CAPTURE(1.0f, ws_client_setup(); , ws_client_setup);
	}
	else
	{
		// start client (includes delay)
		ws_client_setup();
	}
}


bool CustomQuickchat::start_websocket_server()
{
	auto websocket_port_cvar = GetCvar(Cvars::websocket_port);
	if (!websocket_port_cvar) return false;
	int websocket_port = websocket_port_cvar.getIntValue();

	*connecting_to_ws_server = true;

	// start websocket sever (spawn python process)
	std::string command = CreateCommandString(speechToTextExePath.string(), { std::to_string(websocket_port) });	// args: py exe, websocket port
	
	auto process_info = Process::create_process_from_command(command);
	LOG("Status code after attempting to create process (0 is good): {}", process_info.status_code);

	if (process_info.status_code == 0)
	{
		LOG("Created process using command: {}", command);
		stt_python_server_process = process_info.handles;	// save handle to created process, so we can close it later
		return true;
	}
	else
	{
		LOG("[ERROR] Unable to create process using command: {}", command);
		*connecting_to_ws_server = false;
		return false;
	}
}


void CustomQuickchat::stop_websocket_server()
{
	Process::terminate_created_process(stt_python_server_process);
	LOG("Stopped websocket server using TerminateProcess...");
}


json CustomQuickchat::generate_data_for_STT_attempt()
{
	json data;

	auto beginSpeechTimeout_cvar = GetCvar(Cvars::beginSpeechTimeout);
	auto speechProcessingTimeout_cvar = GetCvar(Cvars::speechProcessingTimeout);
	auto autoCalibrateMic_cvar = GetCvar(Cvars::autoCalibrateMic);
	auto micEnergyThreshold_cvar = GetCvar(Cvars::micEnergyThreshold);

	if (!beginSpeechTimeout_cvar || !speechProcessingTimeout_cvar) return data;

	float beginSpeechTimeout = beginSpeechTimeout_cvar.getFloatValue() - 1.1;	// an additional ~1.1 seconds is added in py script due to pause/phrase thresholds
	float processSpeechTimeout = speechProcessingTimeout_cvar.getFloatValue();
	float micEnergyThreshold = micEnergyThreshold_cvar.getFloatValue();
	bool autoCalibrateMic = autoCalibrateMic_cvar.getBoolValue();

	data["args"] =
	{
		{ "beginSpeechTimeout",		beginSpeechTimeout },
		{ "processSpeechTimeout",	processSpeechTimeout },
		{ "autoCalibrateMic",		autoCalibrateMic },
		{ "micEnergyThreshold",		micEnergyThreshold },
		{ "attemptId",				Active_STT_Attempt.attemptID }
	};

	return data;
}


json CustomQuickchat::generate_data_for_mic_calibration_attempt()
{
	json data;

	data["attemptId"] = Active_STT_Attempt.attemptID;

	return data;
}


std::string CustomQuickchat::generate_STT_attempt_id()
{
	std::string id = Format::GenRandomString(10);
	LOG("Generated ID for current speech-to-text attempt: {}", id);
	return id;
}


void CustomQuickchat::process_ws_response(const json& response)
{
	if (!response.contains("event"))
	{
		STTLog("[ERROR] Missing 'event' field in response JSON from websocket server");
		return;
	}

	std::string event = response["event"];

	// TODO: maybe abstract the repeated code below into a function that takes event as an arg (DRY)
	if (event == "speech_to_text_result")
	{
		if (!response.contains("data"))
		{
			STTLog("[ERROR] Missing 'data' field in speech-to-text response JSON");
			return;
		}

		auto stt_result_data = response["data"];

		process_STT_result(stt_result_data);
	}
	else if (event == "mic_calibration_result")
	{
		if (!response.contains("data"))
		{
			STTLog("[ERROR] Missing 'data' field in mic calibration response JSON");
			return;
		}

		auto mic_calibration_result_data = response["data"];

		process_mic_calibration_result(mic_calibration_result_data);
	}
	else if (event == "test_response")
	{
		auto responseData = response["data"];

		std::string message = responseData["message"];

		STTLog(message);
	}
	else if (event == "notify_mic_listening")
	{
		if (!response.contains("data"))
		{
			STTLog("[ERROR] Missing 'data' field in error response JSON from websocket server");
			return;
		}

		auto responseData = response["data"];

		if (!responseData.contains("attemptId"))
		{
			STTLog("[ERROR] Missing 'attemptId' field in notify_mic_listening response JSON");
			return;
		}

		if (responseData["attemptId"] != Active_STT_Attempt.attemptID)
		{
			LOG("[ERROR] Attempt ID in response JSON doesn't match active STT attempt ID");
			return;
		}

		STTLog("listening......");
	}
	else if (event == "shutdown_response")
	{
		std::string message = response["data"]["message"];

		STTLog(message);

		Websocket->set_connected_status(false);
	}
	else if (event == "error_response")
	{
		if (!response.contains("data"))
		{
			STTLog("[ERROR] Missing 'data' field in error response JSON from websocket server");
			return;
		}

		auto error_data = response["data"];

		// TODO: check for attempt ID, and do specific things based on if it matches the active attempt ID

		if (error_data.contains("errorMsg"))
		{
			STTLog("[ERROR] " + error_data["errorMsg"]);
		}
		else
		{
			STTLog("[ERROR] Missing 'errorMsg' field in error response JSON");
		}
	}
	else
	{
		STTLog("[ERROR] Unknown event type in response JSON");
	}
}


void CustomQuickchat::process_STT_result(const json& response_data)
{
	if (!response_data.contains("attemptId"))
	{
		STTLog("[ERROR] Missing 'attemptId' field in speech-to-text response JSON");
		return;
	}

	if (response_data["attemptId"] != Active_STT_Attempt.attemptID)
	{
		LOG("[ERROR] Attempt ID in response JSON doesn't match active STT attempt ID");
		return;
	}

	if (response_data.contains("success"))
	{
		if (response_data["success"])
		{
			if (response_data.contains("transcription"))
			{
				std::string text = response_data["transcription"];
				const Binding& binding = Active_STT_Attempt.binding;

				// apply text effect if necessary
				text = ApplyTextEffect(text, binding.textEffect);

				// send chat
				GAME_THREAD_EXECUTE_CAPTURE(
					SendChat(text, binding.chatMode);
				, text, binding);
			}
			else
			{
				STTLog("[ERROR] No transcription data found in speech-to-text response JSON");
			}
		}
		else
		{
			if (response_data.contains("errorMsg"))
			{
				STTLog(response_data["errorMsg"]);
			}
			else
			{
				STTLog("Unknown error occurred during speech-to-text processing");
			}
		}
	}
	else
	{
		STTLog("[ERROR] Missing 'success' field in speech-to-text response JSON");
	}

	attemptingSTT = false;
}


void CustomQuickchat::process_mic_calibration_result(const json& response_data)
{
	if (!response_data.contains("attemptId"))
	{
		STTLog("[ERROR] Missing 'attemptId' field in speech-to-text response JSON");
		return;
	}

	if (response_data["attemptId"] != Active_STT_Attempt.attemptID)
	{
		LOG("[ERROR] Attempt ID in response JSON doesn't match active STT attempt ID");
		return;
	}

	if (response_data.contains("success"))
	{
		if (response_data["success"])
		{
			if (response_data.contains("mic_energy_threshold"))
			{
				auto micEnergyThreshold_cvar = GetCvar(Cvars::micEnergyThreshold);
				if (!micEnergyThreshold_cvar) return;

				int new_energy_threshold = response_data["mic_energy_threshold"];

				micEnergyThreshold_cvar.setValue(new_energy_threshold);

				LOG("Updated mic energy threshold: {}", new_energy_threshold);
			}
			else
			{
				STTLog("[ERROR] Missing 'mic_energy_threshold' field in mic calibration response JSON");
			}
		}
		else
		{
			if (response_data.contains("errorMsg"))
			{
				STTLog(response_data["errorMsg"]);
			}
			else
			{
				STTLog("[ERROR] Unknown error occurred during microphone calibration");
			}
		}
	}
	else
	{
		STTLog("[ERROR] Missing 'success' field in speech-to-text response JSON");
	}

	calibratingMicLevel = false;
}


// ======================================== MIC CALIBRATION ========================================

void CustomQuickchat::CalibrateMicrophone()
{
	if (calibratingMicLevel)
	{
		STTLog("Mic calibration is already active!");
		return;
	}

	// update Active_STT_Attempt ID
	Active_STT_Attempt.attemptID = generate_STT_attempt_id();

	json data = generate_data_for_mic_calibration_attempt();
	if (data.empty())
	{
		STTLog("Error generating JSON data for mic calibration attempt");
		return;
	}

	Websocket->SendEvent("calibrate_microphone", data);
	LOG("Sent calibrate_microphone event");

	calibratingMicLevel = true;

	auto micCalibrationTimeout_cvar = GetCvar(Cvars::micCalibrationTimeout);
	if (!micCalibrationTimeout_cvar) return;

	DELAY(micCalibrationTimeout_cvar.getFloatValue(),
		calibratingMicLevel = false;
	);
}


// ========================================= SPEECH-TO-TEXT ========================================

std::string CustomQuickchat::CreateCommandString(const fs::path& executablePath, const std::vector<std::string>& args)
{
	std::string commandStr = "\"" + executablePath.string() + "\"";

	for (const std::string& arg : args)
	{
		commandStr += " \"" + arg + "\"";
	}

	return commandStr;
}


void CustomQuickchat::ClearSttErrorLog()
{
	// Open file in write mode to clear its contents
	std::ofstream ofs(speechToTextErrorLogPath, std::ofstream::out | std::ofstream::trunc);
	ofs.close();

	LOG("Cleared '{}'", speechToTextErrorLogPath.string());
}


void CustomQuickchat::STTLog(const std::string& message)
{
	auto enableSTTNotifications_cvar =	GetCvar(Cvars::enableSTTNotifications);
	auto notificationDuration_cvar =	GetCvar(Cvars::notificationDuration);
	if (!enableSTTNotifications_cvar || !notificationDuration_cvar) return;

	if (enableSTTNotifications_cvar.getBoolValue())
	{
		NotifyAndLog("Speech-To-Text", message, notificationDuration_cvar.getFloatValue());
	}
}

#endif // USE_SPEECH_TO_TEXT

