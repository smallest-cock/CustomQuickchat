#include "pch.h"
#include "CustomQuickchat.h"



// search for pythonw.exe filepath
fs::path CustomQuickchat::findPythonInterpreter()
{
	auto autoDetectInterpreterPath_cvar = GetCvar(Cvars::autoDetectInterpreterPath);
	if (!autoDetectInterpreterPath_cvar) return fs::path();

	fs::path searchResult;

	if (!autoDetectInterpreterPath_cvar.getBoolValue())
	{
		auto pythonInterpreterPath_cvar = GetCvar(Cvars::pythonInterpreterPath);
		if (!pythonInterpreterPath_cvar) return fs::path();

		std::string pythonInterpreterPath = pythonInterpreterPath_cvar.getStringValue();
		searchResult = fs::path(pythonInterpreterPath);

		if (fs::exists(searchResult))
		{ 
			STTLog("Updated python interpreter filepath");
			return searchResult;
		}
		else if (searchResult.empty())
		{
			STTLog("[ERROR] Filepath is empty!");
		}
		else {
			STTLog("[ERROR] Filepath doesn't exist!");
			LOG("[ERROR] Filepath doesnt exist: {}", pythonInterpreterPath);
		}
	}

	searchResult = findInterpreterUsingSearchPathW(L"pythonw.exe");		// 1st option

	if (!fs::exists(searchResult))
	{
		// find interpreter by manually checking each directory in PATH
		searchResult = manuallySearchPathDirectories("pythonw.exe");	// 2nd option (fallback)
	}

	return searchResult;
}


fs::path CustomQuickchat::findInterpreterUsingSearchPathW(const wchar_t* fileName)
{
	wchar_t buffer[MAX_PATH];
	DWORD result = SearchPath(NULL, fileName, NULL, MAX_PATH, buffer, NULL);

	if (result > 0 && result < MAX_PATH)
	{
		fs::path foundPath = fs::path(buffer);
		LOG("found pythonw.exe filepath using SearchPathW: {}", foundPath.string());
		return foundPath;
	}
	else {
		return fs::path(); // return empty path (same as "")
	}
}


fs::path CustomQuickchat::manuallySearchPathDirectories(const std::string& fileName)
{
	std::vector<std::string> paths = getPathsFromEnvironmentVariable();

	for (const auto& path : paths)
	{
		std::filesystem::path fullPath = path;
		fullPath /= fileName;

		if (fs::exists(fullPath))
		{
			return fullPath;
		}
	}

	return fs::path();	// return empty path (same as "")
}


std::vector<std::string> CustomQuickchat::getPathsFromEnvironmentVariable()
{
	std::vector<std::string> paths;
	char* pathEnv = nullptr;
	size_t pathLen = 0;
	_dupenv_s(&pathEnv, &pathLen, "PATH");

	if (pathEnv) {
		std::string pathStr(pathEnv);
		size_t pos = 0;
		std::string delimiter = ";";
		while ((pos = pathStr.find(delimiter)) != std::string::npos) {
			paths.push_back(pathStr.substr(0, pos));
			pathStr.erase(0, pos + delimiter.length());
		}
		paths.push_back(pathStr); // Add the last path segment
		free(pathEnv);
	}

	return paths;
}


void CustomQuickchat::STTLog(const std::string& message)
{
	if (!onLoadComplete) return;	// to prevent crash on startup (bc threaded spawnnotification crash bs)

	auto enableSTTNotificationsCvar = GetCvar(Cvars::enableSTTNotifications);
	if (!enableSTTNotificationsCvar) return;

	if (enableSTTNotificationsCvar.getBoolValue())
	{
		auto notificationDurationCvar = GetCvar(Cvars::notificationDuration);
		if (!notificationDurationCvar) return;

		Instances.SpawnNotification("Speech-To-Text", message, notificationDurationCvar.getFloatValue());
	}
}


void CustomQuickchat::STTWaitAndProbe(const std::string& chatMode, const std::string& effect, const std::string& attemptID, bool test)
{
	auto speechProcessingTimeoutCvar = GetCvar(Cvars::speechProcessingTimeout);
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

	auto searchForPyInterpreter_cvar = GetCvar(Cvars::searchForPyInterpreter);
	if (!searchForPyInterpreter_cvar) return;
	bool searchForPyInterpreter = searchForPyInterpreter_cvar.getBoolValue();

	if (searchForPyInterpreter)
	{
		// search for pythonw.exe once more if it's not already found & stored
		if (pyInterpreter.empty() || pyInterpreter.string() == "")
		{
			pyInterpreter = findPythonInterpreter();
		}

		if (pyInterpreter.empty() || pyInterpreter.string() == "")
		{
			STTLog("[ERROR] Couldn't find pythonw.exe in PATH directories");
			return;
		}
	}


	// get cvar for timeout to start speech
	auto beginSpeechTimeout_cvar = GetCvar(Cvars::beginSpeechTimeout);
	if (!beginSpeechTimeout_cvar) return;
	float beginSpeechTimeout = beginSpeechTimeout_cvar.getFloatValue() - 1.1;	// an additional ~1.1 seconds is added in py script due to pause/phrase thresholds

	// generate unique attempt ID
	std::string attemptID = Format::GenRandomString(10);
	ActiveSTTAttemptID = attemptID;		// store in global variable (so other attempts can see/compare to it)
	LOG("ID for current speech-to-text attempt: {}", ActiveSTTAttemptID);

	// determine python interpreter argument
	std::string pyInterpreterArg = searchForPyInterpreter ? ("\"" + pyInterpreter.string() + "\"") : "pythonw";

	std::string pathToPywScript = "\"" + speechToTextPyScriptFilePath.string() + "\"";
	std::string pathToJsonFile = "\"" + speechToTextFilePath.string() + "\"";

	// command to start speech-to-text python script
	std::string command = pyInterpreterArg + " " + pathToPywScript + " " + pathToJsonFile + " " + std::to_string(beginSpeechTimeout) + " " + attemptID;

	if (calibrateMic)
	{
		command += " --calibrate";
	}

	LOG("STT command: {}", command);

	// CreateProcess variables
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// Initialize STARTUPINFO
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Create the process to start python script
	if (CreateProcess(
		NULL,								// Application name (set NULL to use command)
		Format::ToWcharString(command),		// Command
		NULL,								// Process security attributes
		NULL,								// Thread security attributes
		FALSE,								// Inherit handles from the calling process
		CREATE_NEW_CONSOLE,					// Creation flags (use CREATE_NEW_CONSOLE for asynchronous execution)
		NULL,								// Use parent's environment block
		NULL,								// Use parent's starting directory
		&si,								// Pointer to STARTUPINFO
		&pi									// Pointer to PROCESS_INFORMATION
	))
	{
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
		else
		{
			// reset active attempt ID
			ActiveSTTAttemptID = "420_blz_it_lmao";
		}
	}
	else
	{
		// Failed to create process
		DWORD error = GetLastError();

		STTLog("Error starting python script with CreateProcess. Error code: " + std::to_string(error));
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