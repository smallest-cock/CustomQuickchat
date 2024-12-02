#include "pch.h"
#include "CustomQuickchat.h"



void CustomQuickchat::StartSpeechToText(const Binding& binding)
{
#if !defined(USE_SPEECH_TO_TEXT)

	std::string message = "This version of the plugin doesnt support speech-to-text. You can find that version on the github Releases page";

	GAME_THREAD_EXECUTE_CAPTURE(
		Instances.SpawnNotification("Speech-To-Text", message, 5);
	, message);

	LOG("[ERROR] " + message);

#else

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

	// prompt user for speech
	STTLog("listening......");

#endif // USE_SPEECH_TO_TEXT
}


json CustomQuickchat::generate_data_for_STT_attempt()
{
	json data;

	auto beginSpeechTimeout_cvar =			GetCvar(Cvars::beginSpeechTimeout);
	auto speechProcessingTimeout_cvar =		GetCvar(Cvars::speechProcessingTimeout);
	auto autoCalibrateMic_cvar =			GetCvar(Cvars::autoCalibrateMic);
	auto micEnergyThreshold_cvar =			GetCvar(Cvars::micEnergyThreshold);

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
}


std::string CustomQuickchat::generate_STT_attempt_id()
{
	std::string id = Format::GenRandomString(10);
	LOG("Generated ID for current speech-to-text attempt: {}", id);
	return id;
}

#ifdef USE_SPEECH_TO_TEXT

void CustomQuickchat::start_websocket_server()
{
	// start websocket sever (spawn python process)
	std::string command = CreateCommandString(speechToTextExePath.string(), { stringify(WS_PORT) });	// args: py exe, websocket port
	
	CreateProcessUsingCommand(command);

	LOG("Created process using command: {}", command);
}


void CustomQuickchat::process_ws_response(const json& response)
{
	if (response.contains("testResponse"))
	{
		auto testResponse = response["testResponse"];

		std::string message = testResponse["message"];

		STTLog(message);
	}
	else if (response["event"] == "speech_to_text_result")
	{
		if (!response.contains("data"))
		{
			STTLog("[ERROR] Missing 'data' field in speech-to-text response JSON");
			return;
		}

		auto stt_result_data = response["data"];

		if (!stt_result_data.contains("attemptId"))
		{
			STTLog("[ERROR] Missing 'attemptId' field in speech-to-text response JSON");
			return;
		}

		if (stt_result_data["attemptId"] != Active_STT_Attempt.attemptID)
		{
			LOG("[ERROR] Attempt ID in response JSON doesn't match active STT attempt ID");
			return;
		}

		if (stt_result_data.contains("success"))
		{
			if (stt_result_data["success"])
			{
				if (stt_result_data.contains("transcription"))
				{
					std::string text = stt_result_data["transcription"];
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
				if (stt_result_data.contains("errorMsg"))
				{
					STTLog(stt_result_data["errorMsg"]);
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
	}
	else if (response.contains("micCalibrationResponse"))
	{
		auto micCalibrationResponse = response["micCalibrationResponse"];

		// ...
	}
	else
	{
		STTLog("[ERROR] Unknown response JSON from websocket server");
	}
}


void CustomQuickchat::websocket_thread()
{
	client ws_client;

	try
	{
		ws_client.init_asio();
		ws_client.set_message_handler(std::bind(&CustomQuickchat::on_ws_message, this, std::placeholders::_1, std::placeholders::_2));

		websocketpp::lib::error_code ec;
		client::connection_ptr con = ws_client.get_connection(ws_url, ec);
		if (ec)
		{
			STTLog("WebSocket connection error: " + ec.message());
			return;
		}

		ws_client.connect(con);

		// Run the WebSocket loop
		ws_client.run();
	}
	catch (const std::exception& e)
	{
		STTLog("WebSocket exception: " + std::string(e.what()));
	}
}


void CustomQuickchat::on_ws_message(websocketpp::connection_hdl, client::message_ptr msg)
{
	std::lock_guard<std::mutex> lock(mtx);
	received_message = msg->get_payload();
	message_ready = true;
	cv.notify_all();
}


// ======================================== MIC CALIBRATION ========================================

void CustomQuickchat::CalibrateMicrophone()
{
	if (calibratingMicLevel)
	{
		STTLog("Mic calibration is already active!");
		return;
	}

	ResetSTTJsonFile();

	auto micCalibrationTimeout_cvar = GetCvar(Cvars::micCalibrationTimeout);
	if (!micCalibrationTimeout_cvar) return;
	int micCalibrationTimeout = micCalibrationTimeout_cvar.getIntValue();

	std::string command = GenerateSTTCommand(true);		// also updates ActiveSTTAttemptID

	DWORD error = CreateProcessUsingCommand(command);
	if (error != 0)
	{
		STTLog("Error starting mic calibration using CreateProcess. Error code: " + std::to_string(error));
		return;
	}

	calibratingMicLevel = true;

	DELAY(micCalibrationTimeout,
		calibratingMicLevel = false;
	);

	// notify user that mic is listening for audio
	STTLog("listening......");

	MicCalibrationWaitAndProbe(micCalibrationTimeout);
}


void CustomQuickchat::MicCalibrationWaitAndProbe(int micCalibrationTimeout)
{
	// start loop in separate thread to probe JSON file
	SEPARATE_THREAD_CAPTURE(

		Sleep(1000);	// wait 1s before starting to probe JSON (avoid some unnecessary probing while mic is still listening)

	StartProbingJsonForMicCalibrationResult(micCalibrationTimeout);	// <--- made into separate function bc syntax highlighting inside macro sucks

	, micCalibrationTimeout);
}


void CustomQuickchat::StartProbingJsonForMicCalibrationResult(int micCalibrationTimeout)
{
	float elapsedSeconds = 0;

	while (calibratingMicLevel && elapsedSeconds < micCalibrationTimeout)
	{
		MicCalibrationResult result = CheckJsonForCalibrationResult();

		if (result.success)
		{
			auto micEnergyThreshold_cvar = GetCvar(Cvars::micEnergyThreshold);
			if (!micEnergyThreshold_cvar) return;

			micEnergyThreshold_cvar.setValue(result.energyThreshold);
			calibratingMicLevel = false;
			return;
		}
		else
		{
			if (result.errorMsg.empty())
			{
				Sleep(PROBE_JSON_FREQUENCY * 1000);
				elapsedSeconds += PROBE_JSON_FREQUENCY;
			}
			else
			{
				STTLog(result.errorMsg);
				calibratingMicLevel = false;
				return;
			}
		}
	}

	if (!calibratingMicLevel)
	{
		STTLog("Mic calibration reached timeout of " + std::to_string(micCalibrationTimeout) + " seconds... aborting");
	}
	else
	{
		calibratingMicLevel = false;
	}
}


MicCalibrationResult CustomQuickchat::CheckJsonForCalibrationResult()
{
	MicCalibrationResult result;

	if (!fs::exists(speechToTextJsonPath)) return result;

	std::string jsonFileRawStr = readContent(speechToTextJsonPath);

	// prevent crash on reading invalid JSON data
	try
	{
		auto jsonData = json::parse(jsonFileRawStr);
		auto calibrationData = jsonData["microphoneCalibration"];

		if (calibrationData.empty()) return result;		// return default result if still processing

		// make sure JSON data is from the same attempt
		if (calibrationData.contains("ID"))
		{
			if (calibrationData["ID"] == ActiveSTTAttemptID)
			{
				if (calibrationData.contains("error") && calibrationData["error"])
				{
					result.errorMsg = calibrationData["errorMessage"];
				}
				else if (calibrationData.contains("energyThreshold"))
				{
					result.success = true;
					result.energyThreshold = calibrationData["energyThreshold"];
				}
			}
			else
			{
				result.errorMsg = "Invalid mic calibration attempt ID";
				return result;
			}
		}
	}
	catch (...)
	{
		result.errorMsg = "Couldn't read '" + speechToTextJsonPath.filename().string() + "'... Make sure it contains valid JSON";
	}

	return result;
}



// ========================================= SPEECH-TO-TEXT ========================================

void CustomQuickchat::STTWaitAndProbe(const Binding& binding)
{
	// start loop in separate thread to probe JSON file
	SEPARATE_THREAD_CAPTURE(

		Sleep(2000);	// wait 2s before starting to probe JSON (avoid some unnecessary probing while user is speaking)

		StartProbingJsonForSTTResult(binding);	// <--- made into separate function bc syntax highlighting inside macro sucks

	, binding);
}


void CustomQuickchat::StartProbingJsonForSTTResult(const Binding& binding)
{
	auto speechProcessingTimeout_cvar = GetCvar(Cvars::speechProcessingTimeout);
	if (!speechProcessingTimeout_cvar) return;
	int speechProcessingTimeout = speechProcessingTimeout_cvar.getIntValue();

	float elapsedSeconds = 0;

	while (attemptingSTT && elapsedSeconds < speechProcessingTimeout)
	{
		SpeechToTextResult result = CheckJsonForSTTResult();

		if (result.error)
		{
			STTLog(result.outputStr);
			attemptingSTT = false;
			return;
		}
		else if (result.success)
		{
			std::string text = result.outputStr;

			ApplyTextEffect(text, binding.textEffect);	// apply text effect if necessary

			GAME_THREAD_EXECUTE_CAPTURE(
				SendChat(text, binding.chatMode);
			, text, binding);
			
			attemptingSTT = false;
			return;
		}
		else
		{
			Sleep(PROBE_JSON_FREQUENCY * 1000);
			LOG("slept for 0.2s...");
			elapsedSeconds += PROBE_JSON_FREQUENCY;
		}
	}

	STTLog("Processing reached timeout of " + std::to_string(speechProcessingTimeout) + " seconds... aborting");
	attemptingSTT = false;
}


SpeechToTextResult CustomQuickchat::CheckJsonForSTTResult()
{
	SpeechToTextResult result;

	if (!fs::exists(speechToTextJsonPath)) return result;

	std::string jsonFileRawStr = readContent(speechToTextJsonPath);
	
	try		// prevent crash on reading invalid JSON data
	{
		auto transcriptionDataJson = json::parse(jsonFileRawStr);
		auto transcriptionData = transcriptionDataJson["transcription"];

		if (transcriptionData.empty()) return result;		// return default result if still processing

		if (transcriptionData.contains("ID"))
		{
			if (transcriptionData["ID"] == ActiveSTTAttemptID)
			{
				if (transcriptionData.contains("error") && transcriptionData["error"])
				{
					result.error = true;
					result.outputStr = transcriptionData["errorMessage"];
				}
				else if (transcriptionData.contains("text"))
				{
					result.success = true;
					result.outputStr = transcriptionData["text"];
				}
				else
				{
					result.error = true;
					result.outputStr = "ERROR: No transcription data found in '" + speechToTextJsonPath.filename().string() + "'";
				}
			}
			else
			{
				result.error = true;
				result.outputStr = "Invalid speech-to-text attempt ID";
			}
		}
	}
	catch (...)
	{
		result.error = true;
		result.outputStr = "Couldn't read '" + speechToTextJsonPath.filename().string() + "'... Make sure it contains valid JSON";
	}

	return result;
}


std::string CustomQuickchat::GenerateSTTCommand(bool calibrateMic)
{
	std::string command;

	auto beginSpeechTimeout_cvar =			GetCvar(Cvars::beginSpeechTimeout);
	auto speechProcessingTimeout_cvar =		GetCvar(Cvars::speechProcessingTimeout);
	auto autoCalibrateMic_cvar =			GetCvar(Cvars::autoCalibrateMic);

	if (!beginSpeechTimeout_cvar || !speechProcessingTimeout_cvar) return command;

	float beginSpeechTimeout = beginSpeechTimeout_cvar.getFloatValue() - 1.1;	// an additional ~1.1 seconds is added in py script due to pause/phrase thresholds
	float processSpeechTimeout = speechProcessingTimeout_cvar.getFloatValue();

	// generate unique attempt ID
	ActiveSTTAttemptID = Format::GenRandomString(10);		// store in global variable so other attempts can see/compare to it
	LOG("ID for current speech-to-text attempt: {}", ActiveSTTAttemptID);

	// construct command to start speech-to-text python script
	std::vector<std::string> args = {
		speechToTextJsonPath.string(),
		std::to_string(beginSpeechTimeout),
		std::to_string(processSpeechTimeout),
		ActiveSTTAttemptID
	};

	if (calibrateMic)
	{
		args.emplace_back("--calibrate");
	}
	else if (autoCalibrateMic_cvar && !autoCalibrateMic_cvar.getBoolValue())
	{
		args.emplace_back("--use-saved-calibration-value");
	}

	command = CreateCommandString(speechToTextExePath, args);

	LOG("STT command: {}", command);

	return command;
}


std::string CustomQuickchat::CreateCommandString(const fs::path& executablePath, const std::vector<std::string>& args)
{
	std::string commandStr = "\"" + executablePath.string() + "\"";

	for (const std::string& arg : args)
	{
		commandStr += " \"" + arg + "\"";
	}

	return commandStr;
}


void CustomQuickchat::ResetSTTJsonFile()
{
	/*
	* JSON file structure:
	* 
		"transcription": {
			"ID": "sfu384g39g3wfd",
			"text": "okay buddy",
			"error": false
		},
		"microphoneCalibration": {
			"energyThreshold": 212.35357783178569,
			"ID": "9wf82ys8edfsyf",
			"error": true,
			"errorMsg": "Bad thing happend"
		}
	*/

	json jsonData;
	jsonData["transcription"] = json::object();
	jsonData["microphoneCalibration"] = json::object();

	auto autoCalibrateMic_cvar = GetCvar(Cvars::autoCalibrateMic);
	if (autoCalibrateMic_cvar && !autoCalibrateMic_cvar.getBoolValue())
	{
		auto micEnergyThreshold_cvar = GetCvar(Cvars::micEnergyThreshold);

		jsonData["microphoneCalibration"]["energyThreshold"] = micEnergyThreshold_cvar.getIntValue();
	}

	try {
		writeJsonToFile(speechToTextJsonPath, jsonData);
	}
	catch (const std::exception& e) {
		LOG("Error writing JSON file: {}", e.what());
	}

	LOG("[Speech-To-Text] Cleared '{}' ...", speechToTextJsonPath.filename().string());
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

