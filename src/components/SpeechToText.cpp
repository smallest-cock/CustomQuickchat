#include "pch.h"
#include "WebsocketManager.hpp"
#include "ModUtils/gui/GuiTools.hpp"
#include "SpeechToText.hpp"
#include "Macros.hpp"
#include "components/Instances.hpp"

// ##############################################################################################################
// ###############################################    INIT    ###################################################
// ##############################################################################################################

void STTComponent::init(const std::shared_ptr<CustomQuickchat>& mainPluginClass, const std::shared_ptr<GameWrapper>& gw)
{
	gameWrapper   = gw;
	m_pluginClass = mainPluginClass;

	initFilepaths();
	initCvars();
	initHooks();

	clearSttErrorLog();
	m_websocketClient = std::make_unique<WebsocketClientManager>(m_connectingToWsServer);

	// small delay to allow cvar values to load from config.cfg (websocket port num) before starting connection
	DELAY(1.0f, { startConnection(); });
}

void STTComponent::initFilepaths()
{
	fs::path bmDataFolderFilePath = gameWrapper->GetDataFolder();
	fs::path pluginFolder         = bmDataFolderFilePath / "CustomQuickchat";

	m_jsonPath     = pluginFolder / "SpeechToText.json";
	m_exePath      = pluginFolder / "SpeechToText" / "SpeechToText.exe";
	m_errorLogPath = pluginFolder / "SpeechToText" / "ErrorLog.txt";

	bool missingFile = false;
	if (!fs::exists(m_exePath))
	{
		sttLog("ERROR: SpeechToText.exe not found");
		missingFile = true;
	}

	m_allFilesExist = !missingFile;
}

void STTComponent::initCvars()
{
	// bools
	registerCvar_bool(Cvars::enableSTTNotifications, true).bindTo(m_enableSTTNotifications);
	registerCvar_bool(Cvars::autoCalibrateMic, true).bindTo(m_autoCalibrateMic);

	// ints
	registerCvar_number(Cvars::websocketPort, 8003, true, 0, 65535).bindTo(m_websocketPort);
	registerCvar_number(Cvars::micEnergyThreshold, 420).bindTo(m_micEnergyThreshold);

	// floats
	registerCvar_number(Cvars::notificationDuration, 3, true, 1.5, 10).bindTo(m_notificationDuration);
	registerCvar_number(Cvars::beginSpeechTimeout, 3, true, 1.5, 10).bindTo(m_beginSpeechTimeout);
	registerCvar_number(Cvars::speechProcessingTimeout, 10, true, 3, 500).bindTo(m_speechProcessingTimeout);
	registerCvar_number(Cvars::micCalibrationTimeout, 10, true, 3, 20).bindTo(m_micCalibrationTimeout);
}

void STTComponent::initHooks() {}

void STTComponent::onUnload()
{
	stopWebsocketServer();

	// explicitly release resource here (in BM's dedicated onUnload, as opposed to leaving it up to RAII destructor)
	// to prevent weird behavior where m_websocketClient state persists across plugin reloads, fricking many things up
	m_websocketClient.reset();
}

// ##############################################################################################################
// ###############################################    FUNCTIONS    ##############################################
// ##############################################################################################################

void STTComponent::triggerSTT(const Binding& binding)
{
	if (!m_allFilesExist)
	{
		sttLog("ERROR: Missing required files. Check your installation");
		return;
	}

	if (!m_attemptingSTT)
		startSTT(binding);
	else
		sttLog("Speech-to-text is already active!");
}

void STTComponent::startSTT(const Binding& binding)
{
	// update active attempt data
	m_activeAtempt.binding   = binding;
	m_activeAtempt.attemptID = generateAttemptId();

	json data = generateDataForSTTAttempt();
	if (data.empty())
	{
		sttLog("Error generating JSON data for speech-to-text attempt");
		return;
	}

	m_websocketClient->sendMessage("start_speech_to_text", data);
	m_attemptingSTT.store(true);
}

void STTComponent::startConnection()
{
	if (*m_websocketPort != m_lastUsedServerPort)
		startServerThenConnectClient();
	else
	{
		if (m_startedWebsocketServer)
			connectClientToServer();
		else
			startServerThenConnectClient();
	}
}

void STTComponent::endConnection()
{
	disconnectClientFromServer();

	DELAY(0.5f, {
		// terminate existing websocket server process (if necessary)
		if (m_pythonServerProcess.is_active())
			Process::terminate_created_process(m_pythonServerProcess);

		m_startedWebsocketServer = false;
	});
}

void STTComponent::startServerThenConnectClient()
{
	if (startWebsocketServer())
	{
		LOG("Started websocket server");
		DELAY(START_WS_CLIENT_DELAY, { connectClientToServer(); });
	}
	else
		sttLog("ERROR: Unable to start python websocket server");
}

void STTComponent::connectClientToServer()
{
	if (!m_startedWebsocketServer)
	{
		LOGERROR("Unable to connect client. Websocket server hasn't been started!");
		return;
	}
	if (m_websocketClient->isConnected())
	{
		sttLog("Websocket client is already connected to server!");
		return;
	}

	std::function<void(json)> jsonResponseHandler = std::bind(&STTComponent::processWsResponse, this, std::placeholders::_1);
	bool                      success             = m_websocketClient->connect(buildServerUri(), jsonResponseHandler);
	LOG("Connecting websocket client was {}", success ? "successful" : "unsuccessful");
}

void STTComponent::disconnectClientFromServer()
{
	if (!m_startedWebsocketServer)
	{
		LOGERROR("Unable to disconnect client. Websocket server hasn't been started!");
		return;
	}
	if (!m_websocketClient->isConnected())
	{
		sttLog("Websocket client is already disconnected from server!");
		return;
	}

	bool success = m_websocketClient->disconnect();
	LOG("Connecting websocket client was {}", success ? "successful" : "unsuccessful");
}

bool STTComponent::startWebsocketServer()
{
	if (!m_allFilesExist)
	{
		sttLog("ERROR: Missing required files. Check your installation");
		return false;
	}
	if (m_startedWebsocketServer)
	{
		sttLog("Websocket server has already started!");
		return false;
	}

	// start websocket sever (spawn python process)
	// args: py exe, websocket port
	std::string command = CreateCommandString(m_exePath.string(), {std::to_string(*m_websocketPort)});

	// terminate existing websocket server process (if necessary)
	if (m_pythonServerProcess.is_active())
		Process::terminate_created_process(m_pythonServerProcess);

	m_connectingToWsServer.store(true);

	auto process_info = Process::create_process_from_command(command);
	LOG("Status code after attempting to create process (0 is good): {}", process_info.status_code);

	if (process_info.status_code != 0)
	{
		LOGERROR("Unable to create process using command (status code: {}): {}", process_info.status_code, command);
		m_connectingToWsServer.store(false);
		return false;
	}

	LOG("Created process using command: {}", command);
	m_pythonServerProcess    = process_info.handles; // save handle to created process, so we can close it later
	m_startedWebsocketServer = true;
	m_lastUsedServerPort     = *m_websocketPort;
	return true;
}

void STTComponent::stopWebsocketServer()
{
	if (!m_allFilesExist)
	{
		sttLog("ERROR: Missing required files. Check your installation");
		return;
	}

	Process::terminate_created_process(m_pythonServerProcess);
	LOG("Stopped websocket server using TerminateProcess...");
}

json STTComponent::generateDataForSTTAttempt()
{
	json data;

	// an additional ~1.1 seconds is added in py script due to pause/phrase thresholds
	float beginSpeechTimeout = *m_beginSpeechTimeout - 1.1;

	data["args"] = {{"beginSpeechTimeout", beginSpeechTimeout},
	    {"processSpeechTimeout", *m_speechProcessingTimeout},
	    {"autoCalibrateMic", *m_autoCalibrateMic},
	    {"micEnergyThreshold", *m_micEnergyThreshold},
	    {"attemptId", m_activeAtempt.attemptID}};

	return data;
}

json STTComponent::generateDataForMicCalibrationAttempt() const { return {{"attemptId", m_activeAtempt.attemptID}}; }

std::string STTComponent::generateAttemptId()
{
	std::string id = Format::GenRandomString(10);
	LOG("Generated ID for current speech-to-text attempt: {}", id);
	return id;
}

// large gross function, may benefit from a redesign
void STTComponent::processWsResponse(const json& res)
{
	if (!res.contains("event"))
	{
		sttLog("[ERROR] Missing 'event' field in response JSON from websocket server");
		return;
	}

	std::string event = res["event"];

	// TODO: maybe abstract the repeated code below into a function that takes event as an arg (DRY)
	if (event == "speech_to_text_result")
	{
		if (!res.contains("data"))
		{
			sttLog("[ERROR] Missing 'data' field in speech-to-text response JSON");
			return;
		}

		auto stt_result_data = res["data"];

		processSTTResult(stt_result_data);
	}
	else if (event == "mic_calibration_result")
	{
		if (!res.contains("data"))
		{
			sttLog("[ERROR] Missing 'data' field in mic calibration response JSON");
			return;
		}

		auto calibrationResult = res["data"];
		processMicCalibrationResult(calibrationResult);
	}
	else if (event == "test_response")
	{
		auto responseData = res["data"];

		std::string message = responseData["message"];

		sttLog(message);
	}
	else if (event == "notify_mic_listening")
	{
		if (!res.contains("data"))
		{
			sttLog("[ERROR] Missing 'data' field in error response JSON from websocket server");
			return;
		}

		auto responseData = res["data"];

		if (!responseData.contains("attemptId"))
		{
			sttLog("[ERROR] Missing 'attemptId' field in notify_mic_listening response JSON");
			return;
		}

		if (responseData["attemptId"] != m_activeAtempt.attemptID)
		{
			LOG("[ERROR] Attempt ID in response JSON doesn't match active STT attempt ID");
			return;
		}

		sttLog("listening......");
	}
	// else if (event == "shutdown_response")
	// {
	// 	std::string message = res["data"]["message"];

	// 	sttLog(message);

	// 	m_websocketClient->setConnectedStatus(false); // wtf is this?
	// }
	else if (event == "error_response")
	{
		if (!res.contains("data"))
		{
			sttLog("[ERROR] Missing 'data' field in error response JSON from websocket server");
			return;
		}

		auto error_data = res["data"];

		// IDEA: maybe check for attempt ID, and do specific things here if it matches the active attempt ID

		if (error_data.contains("errorMsg"))
			sttLog(std::format("[ERROR] {}", error_data["errorMsg"].get<std::string>()));
		else
			sttLog("[ERROR] Missing \"errorMsg\" field in error response JSON");
	}
	else
		sttLog("[ERROR] Unknown event type in response JSON");
}

void STTComponent::processSTTResult(const json& res)
{
	if (!res.contains("attemptId"))
	{
		sttLog("[ERROR] Missing 'attemptId' field in speech-to-text response JSON");
		return;
	}

	if (res["attemptId"] != m_activeAtempt.attemptID)
	{
		LOGERROR("Attempt ID in response JSON doesn't match active STT attempt ID");
		return;
	}

	if (res.contains("success"))
	{
		if (res["success"])
		{
			if (res.contains("transcription"))
			{
				std::string    text    = res["transcription"];
				const Binding& binding = m_activeAtempt.binding;

				// apply text effect if necessary
				text = m_pluginClass->ApplyTextEffect(text, binding.textEffect);

				// send chat
				GAME_THREAD_EXECUTE({ m_pluginClass->SendChat(text, binding.chatMode); }, text, binding);
			}
			else
				sttLog("[ERROR] No transcription data found in speech-to-text response JSON");
		}
		else
		{
			if (res.contains("errorMsg"))
				sttLog(res["errorMsg"]);
			else
				sttLog("Unknown error occurred during speech-to-text processing");
		}
	}
	else
		sttLog("[ERROR] Missing 'success' field in speech-to-text response JSON");

	m_attemptingSTT.store(false);
}

void STTComponent::processMicCalibrationResult(const json& res)
{
	if (!res.contains("attemptId"))
	{
		sttLog("[ERROR] Missing 'attemptId' field in speech-to-text response JSON");
		return;
	}

	if (res["attemptId"] != m_activeAtempt.attemptID)
	{
		LOGERROR("Attempt ID in response JSON doesn't match active STT attempt ID");
		return;
	}

	if (res.contains("success"))
	{
		if (res["success"])
		{
			if (res.contains("mic_energy_threshold"))
			{
				auto micEnergyThreshold_cvar = getCvar(Cvars::micEnergyThreshold);
				if (!micEnergyThreshold_cvar)
					return;

				int new_energy_threshold = res["mic_energy_threshold"];

				micEnergyThreshold_cvar.setValue(new_energy_threshold);

				LOG("Updated mic energy threshold: {}", new_energy_threshold);
			}
			else
				sttLog("[ERROR] Missing 'mic_energy_threshold' field in mic calibration response JSON");
		}
		else
		{
			if (res.contains("errorMsg"))
				sttLog(res["errorMsg"]);
			else
				sttLog("[ERROR] Unknown error occurred during microphone calibration");
		}
	}
	else
		sttLog("[ERROR] Missing 'success' field in speech-to-text response JSON");

	m_calibratingMicLevel.store(false);
}

// ======================================== MIC CALIBRATION ========================================

void STTComponent::calibrateMicrophone()
{
	if (!m_allFilesExist)
	{
		sttLog("ERROR: Missing required files. Check your installation");
		return;
	}

	if (m_calibratingMicLevel)
	{
		sttLog("Mic calibration is already active!");
		return;
	}

	m_activeAtempt.attemptID = generateAttemptId();

	json data = generateDataForMicCalibrationAttempt();
	if (data.empty())
	{
		sttLog("Error generating JSON data for mic calibration attempt");
		return;
	}

	m_websocketClient->sendMessage("calibrate_microphone", data);
	LOG("Sent calibrate_microphone event");

	m_calibratingMicLevel.store(true);

	std::string attemptId = m_activeAtempt.attemptID;
	DELAY(
	    *m_micCalibrationTimeout,
	    {
		    if (attemptId == m_activeAtempt.attemptID)
			    m_calibratingMicLevel.store(false);
	    },
	    attemptId);
}

// ========================================= SPEECH-TO-TEXT ========================================

std::string STTComponent::CreateCommandString(const fs::path& executablePath, const std::vector<std::string>& args)
{
	std::string commandStr = "\"" + executablePath.string() + "\"";

	for (const std::string& arg : args)
		commandStr += " \"" + arg + "\"";

	return commandStr;
}

void STTComponent::clearSttErrorLog()
{
	// Open file in write mode to clear its contents
	std::ofstream ofs(m_errorLogPath, std::ofstream::out | std::ofstream::trunc);
	ofs.close();

	LOG("Cleared STT error log at \"{}\"", m_errorLogPath.string());
}

void STTComponent::sttLog(const std::string& message)
{
	auto enableSTTNotifications_cvar = getCvar(Cvars::enableSTTNotifications);
	auto notificationDuration_cvar   = getCvar(Cvars::notificationDuration);
	if (!enableSTTNotifications_cvar || !notificationDuration_cvar)
		return;

	if (enableSTTNotifications_cvar.getBoolValue())
		notifyAndLog("Speech-To-Text", message, notificationDuration_cvar.getFloatValue());
}

void STTComponent::notifyAndLog(const std::string& title, const std::string& message, int duration)
{
	GAME_THREAD_EXECUTE({ Instances.SpawnNotification(title, message, duration, true); }, title, message, duration);
}

void STTComponent::test()
{
	if (!m_allFilesExist)
	{
		sttLog("ERROR: Missing required files. Check your installation");
		return;
	}

	m_websocketClient->sendMessage("test", {{"data", "test"}});
}

// ##############################################################################################################
// ##########################################   DISPLAY FUNCTIONS    ############################################
// ##############################################################################################################

void STTComponent::display_settings()
{
#ifndef USE_SPEECH_TO_TEXT
	GUI::Spacing(4);
	ImGui::Text("This version of the plugin doesnt support speech-to-text. You can find that version on the github Releases page:");
	GUI::Spacing(2);
	GUI::ClickableLink("Releases", "https://github.com/smallest-cock/CustomQuickchat/releases/latest", ImVec4(1, 1, 0, 1));
#else
	auto enableSTTNotifications_cvar  = getCvar(Cvars::enableSTTNotifications);
	auto speechProcessingTimeout_cvar = getCvar(Cvars::speechProcessingTimeout);
	auto beginSpeechTimeout_cvar      = getCvar(Cvars::beginSpeechTimeout);
	auto notificationDuration_cvar    = getCvar(Cvars::notificationDuration);
	auto autoCalibrateMic_cvar        = getCvar(Cvars::autoCalibrateMic);
	auto micCalibrationTimeout_cvar   = getCvar(Cvars::micCalibrationTimeout);
	auto micEnergyThreshold_cvar      = getCvar(Cvars::micEnergyThreshold);
	auto websocketPort_cvar           = getCvar(Cvars::websocketPort);
	if (!micEnergyThreshold_cvar)
		return;

	GUI::Spacing(2);

	// display websocket connection status
	bool bConnectedToWsServer = m_websocketClient ? m_websocketClient->isConnected() : false;

	std::string connectionStatusStr;
	if (!m_connectingToWsServer)
		connectionStatusStr = bConnectedToWsServer ? std::format("Connected ({})", m_websocketClient->getCurrentURI()) : "Not connected";
	else
		connectionStatusStr = "Connecting....";

	std::string wsStatusStr = "Websocket status:\t" + connectionStatusStr;
	ImGui::Text("%s", wsStatusStr.c_str());

	GUI::SameLineSpacing_relative(50.0f);

	if (!m_connectingToWsServer && bConnectedToWsServer)
	{
		if (ImGui::Button("Test##websocketConnection"))
			GAME_THREAD_EXECUTE({ test(); });
		GUI::ToolTip("Send a test message to check the websocket connection");
	}

	GUI::Spacing();

	ImGui::SetNextItemWidth(100);
	int websocketPort = websocketPort_cvar.getIntValue();
	if (ImGui::InputInt("Port number", &websocketPort))
		websocketPort_cvar.setValue(websocketPort);

	GUI::Spacing();

	if (!bConnectedToWsServer && !m_connectingToWsServer)
	{
		if (ImGui::Button("Connect##websocket"))
			GAME_THREAD_EXECUTE({ startConnection(); });
	}
	else
	{
		if (ImGui::Button("Disconnect##websocket"))
			GAME_THREAD_EXECUTE({ endConnection(); });
	}

	GUI::Spacing(2);

	bool autoCalibrateMic = autoCalibrateMic_cvar.getBoolValue();
	int  radioButtonVal   = autoCalibrateMic ? 0 : 1;

	if (ImGui::RadioButton("Auto calibrate microphone on every listen", &radioButtonVal, 0))
		autoCalibrateMic_cvar.setValue(true);
	GUI::ToolTip("Briefly calibrates mic energy level before you start speaking (reliable)");

	if (ImGui::RadioButton("Manually calibrate microphone", &radioButtonVal, 1))
		autoCalibrateMic_cvar.setValue(false);
	GUI::ToolTip("Uses a stored calibration value, eliminating the need to calibrate mic before every attempt (can be faster)");

	if (!autoCalibrateMic)
	{
		GUI::Spacing(4);

		std::string thresholdStr = std::format(
		    "Mic energy threshold: {}", m_calibratingMicLevel ? "calibrating....." : std::to_string(*m_micEnergyThreshold));
		ImGui::Text("%s", thresholdStr.c_str());

		GUI::Spacing(2);

		// calibrate mic button
		if (ImGui::Button("Calibrate Microphone"))
			GAME_THREAD_EXECUTE({ calibrateMicrophone(); });
		GUI::ToolTip("Calibrate microphone sensitivity level (for the plugin) based on a sample of background noise");

		GUI::Spacing(2);

		int micCalibrationTimeout = micCalibrationTimeout_cvar.getIntValue();
		if (ImGui::SliderInt("mic calibration timeout", &micCalibrationTimeout, 1.0f, 20.0f, "%.0f seconds"))
			micCalibrationTimeout_cvar.setValue(micCalibrationTimeout);
		GUI::ToolTip("Max amount of time to spend on a calibration attempt before aborting");
	}

	GUI::Spacing(4);

	// chat notifications
	bool speechToTextNotificationsOn = enableSTTNotifications_cvar.getBoolValue();
	if (ImGui::Checkbox("Enable speech-to-text notifications", &speechToTextNotificationsOn))
		enableSTTNotifications_cvar.setValue(speechToTextNotificationsOn);

	if (speechToTextNotificationsOn)
	{
		GUI::Spacing(2);

		// popup notification duration
		float notificationDuration = notificationDuration_cvar.getFloatValue();
		if (ImGui::SliderFloat("notification duration", &notificationDuration, 1.5f, 10.0f, "%.1f seconds"))
			notificationDuration_cvar.setValue(notificationDuration);

		GUI::SameLineSpacing_relative(10);

		// test popup notifications
		if (ImGui::Button("Test"))
		{
			GAME_THREAD_EXECUTE(
			    {
				    Instances.SpawnNotification("Terry A Davis",
				        "You can see 'em if you're driving. You just run them over. That's what you do.",
				        notificationDuration);
			    },
			    notificationDuration);
		}
	}

	// start speech timeout
	float waitForSpeechTimeout = beginSpeechTimeout_cvar.getFloatValue();
	if (ImGui::SliderFloat("timeout to start speaking", &waitForSpeechTimeout, 1.5f, 10.0f, "%.1f seconds"))
		beginSpeechTimeout_cvar.setValue(waitForSpeechTimeout);
	GUI::ToolTip("max time to wait for start of speech");

	// processing timeout
	int processSpeechTimeout = speechProcessingTimeout_cvar.getFloatValue();
	if (ImGui::SliderInt("timeout for processing speech", &processSpeechTimeout, 3.0f, 30.0f, "%.0f seconds"))
		speechProcessingTimeout_cvar.setValue(processSpeechTimeout);
	GUI::ToolTip("max time to spend processing speech\t(will abort speech-to-text attempt if exceeded)");
#endif // USE_SPEECH_TO_TEXT

	GUI::Spacing(2);
}

class STTComponent SpeechToText{};
