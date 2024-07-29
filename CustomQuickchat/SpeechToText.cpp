#include "pch.h"
#include "CustomQuickchat.h"



// search for pythonw.exe filepath
fs::path CustomQuickchat::findPythonInterpreter()
{
	wchar_t buffer[MAX_PATH];
	DWORD result = SearchPath(NULL, L"pythonw.exe", NULL, MAX_PATH, buffer, NULL);

	if (result > 0 && result < MAX_PATH)
	{
		fs::path foundPath = fs::path(buffer);
		LOG("found filepath to pythonw.exe: {}", foundPath.string());
		return fs::path(buffer);
	}
	else {
		return fs::path(); // return empty path (same as "")
	}
}


void CustomQuickchat::STTLog(const std::string& message)
{
	auto enableSTTNotificationsCvar = cvarManager->getCvar(CvarNames::enableSTTNotifications);
	if (!enableSTTNotificationsCvar) return;

	if (enableSTTNotificationsCvar.getBoolValue())
	{
		auto notificationDurationCvar = cvarManager->getCvar(CvarNames::notificationDuration);
		if (!notificationDurationCvar) return;

		Instances.SpawnNotification("Speech-To-Text", message, notificationDurationCvar.getFloatValue());
	}
}


void CustomQuickchat::STTWaitAndProbe(const std::string& chatMode, const std::string& effect, const std::string& attemptID, bool test)
{
	auto speechProcessingTimeoutCvar = cvarManager->getCvar(CvarNames::speechProcessingTimeout);
	if (!speechProcessingTimeoutCvar) return;
	int processSpeechTimeout = speechProcessingTimeoutCvar.getIntValue();


	// probe JSON file ...
	for (int i = 0; i < (((processSpeechTimeout - 2) * 5) + 1); i++)
	{
		gameWrapper->SetTimeout([this, chatMode, effect, attemptID, test, i](GameWrapper* gw) {

			if (ActiveSTTAttemptID == attemptID)
			{
				std::string jsonFileRawStr = readContent(speechToTextFilePath);

				// prevent crash on reading invalid JSON data
				try {
					auto transcriptionData = json::parse(jsonFileRawStr);
					auto transcription = transcriptionData["transcription"];

					if (transcription.empty()) return;		// return if still processing

					// make sure JSON data is from the same attempt
					std::string jsonAttemptID = transcriptionData["transcription"]["ID"];
					if (jsonAttemptID != attemptID) return;


					bool error = transcriptionData["transcription"]["error"];

					// clear active attempt ID
					ActiveSTTAttemptID = "420_blz_it_lmao";

					if (!test) {
						if (!error) {
							std::string text = transcription["text"];

							// apply text effect if necessary
							if (effect == "sarcasm") {
								text = toSarcasm(text);
							}
							else if (effect == "uwu") {
								text = toUwu(text);
							}

							SendChat(text, chatMode);
						}
						else {
							std::string errorMsg = transcriptionData["transcription"]["errorMessage"];
							LOG("[SPEECH-TO-TEXT] Error: {}", errorMsg);
							STTLog("[ERROR] " + errorMsg);
						}
					}

				}
				catch (...) {
					// clear active attempt ID
					ActiveSTTAttemptID = "420_blz_it_lmao";

					STTLog("[ERROR] Couldn't read 'SpeechToText.json'... Make sure it contains valid JSON");
				}
			}

		}, ((i + 1) * 0.2) + 2);	// wait 2 seconds before probing (to help avoid unnecessary probing while user still speaking)
	}


	gameWrapper->SetTimeout([this, processSpeechTimeout, attemptID, test](GameWrapper* gw) {

		if (ActiveSTTAttemptID == attemptID && !test)
		{
			STTLog("Processing reached timeout of " + std::to_string(processSpeechTimeout) + " seconds... aborting");

			// clear active attempt ID
			ActiveSTTAttemptID = "420_blz_it_lmao";
		}

	}, processSpeechTimeout);

}


void CustomQuickchat::StartSpeechToText(const std::string& chatMode, const std::string& effect, bool test, bool calibrateMic)
{
	// reset transcription data
	if (!ClearTranscriptionJson())
	{
		STTLog("[ERROR] 'SpeechToText.json' cannot be found");
		return;
	}

	// search for pythonw.exe once more if it's not already found & stored
	if (pyInterpreter.empty())
	{
		pyInterpreter = findPythonInterpreter();
	}

	if (pyInterpreter.empty())
	{
		STTLog("[ERROR] Couldn't find pythonw.exe interpreter in PATH variable");
		return;
	}


	// get CVar for start speech timeout
	auto beginSpeechTimeoutCvar = cvarManager->getCvar(CvarNames::beginSpeechTimeout);
	if (!beginSpeechTimeoutCvar) return;
	float beginSpeechTimeout = beginSpeechTimeoutCvar.getFloatValue() - 1.1;	// an additional ~1.1 seconds is added in py script due to pause/phrase thresholds

	// generate unique attempt ID
	std::string attemptID = Format::GenRandomString(10);
	ActiveSTTAttemptID = attemptID;		// store in global variable (so other attempts can see/compare to it)
	LOG("ID for current speech-to-text attempt: {}", ActiveSTTAttemptID);


	std::string pathToPyInterpreter = "\"" + pyInterpreter.string() + "\"";
	std::string pathToPywScript = "\"" + speechToTextPyScriptFilePath.string() + "\"";
	std::string pathToJsonFile = "\"" + speechToTextFilePath.string() + "\"";

	// Command-line to execute Python script with JSON filepath as 1st arg
	std::string command = pathToPyInterpreter + " " + pathToPywScript + " " + pathToJsonFile + " " + std::to_string(beginSpeechTimeout) + " " + attemptID;

	if (calibrateMic) {
		command += " --calibrate";
	}

	std::wstring wCommand(command.begin(), command.end());		// convert to wide string


	// CreateProcess variables
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// Initialize STARTUPINFO
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Create the process
	if (CreateProcess(
		NULL,                   // Application name (use NULL to use command line)
		const_cast<wchar_t*>(wCommand.c_str()),  // Command line
		NULL,                   // Process security attributes
		NULL,                   // Thread security attributes
		FALSE,                  // Inherit handles from the calling process
		CREATE_NEW_CONSOLE,    // Creation flags (use CREATE_NEW_CONSOLE for asynchronous execution)
		NULL,                   // Use parent's environment block
		NULL,                   // Use parent's starting directory
		&si,                    // Pointer to STARTUPINFO
		&pi                     // Pointer to PROCESS_INFORMATION
	)) {

		// -------------- after successfully starting the process -----------

		// Close process handle to allow it to run asynchronously
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		if (!test && !calibrateMic)
		{
			// prompt user for speech
			STTLog("listening......");

			// wait for speech, and probe JSON file for response
			STTWaitAndProbe(chatMode, effect, attemptID, test);
		}
		else {
			// reset active attempt ID
			ActiveSTTAttemptID = "420_blz_it_lmao";
		}

	}
	else {
		// Failed to create process
		DWORD error = GetLastError();

		STTLog("Error executing Python script with CreateProcess. Error code: " + std::to_string(error));
	}
}


void CustomQuickchat::ResetJsonFile(float micCalibration)
{
	if (!fs::exists(speechToTextFilePath)) return;

	json jsonData;
	jsonData["transcription"] = json::object();

	if (micCalibration != 69420)
	{
		jsonData["micNoiseCalibration"] = micCalibration;
	}

	try {
		writeJsonToFile(speechToTextFilePath, jsonData);
	}
	catch (const std::exception& e) {
		LOG("Error writing JSON file: {}", e.what());
	}

	DEBUGLOG("cleared STT JSON file...");
}


void CustomQuickchat::UpdateMicCalibration(float timeOut)
{
	// get value from JSON after calibration finished
	gameWrapper->SetTimeout([this](GameWrapper* gw) {

		// read file
		auto jsonData = getJsonFromFile(speechToTextFilePath);

		if (jsonData.contains("micNoiseCalibration") && !jsonData["micNoiseCalibration"].is_null())
		{
			micEnergyThreshold = jsonData["micNoiseCalibration"];	// update value of micEnergyThreshold
		}

	}, timeOut);
}


bool CustomQuickchat::ClearTranscriptionJson()
{
	if (!fs::exists(speechToTextFilePath)) return false;

	// read file
	auto jsonData = getJsonFromFile(speechToTextFilePath);

	if (jsonData.contains("micNoiseCalibration") && !jsonData["micNoiseCalibration"].is_null())
	{
		auto micCalibration = jsonData["micNoiseCalibration"];

		if (!micCalibration.empty())
		{
			ResetJsonFile(micCalibration);
			return true;
		}
	}

	ResetJsonFile();
	return true;
}