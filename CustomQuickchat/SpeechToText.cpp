#include "pch.h"
#include "CustomQuickchat.h"


// search for Python interpreter filepath
std::string findPythonInterpreter() {
	// Get the value of the PATH environment variable
	const char* pathEnv = getenv("PATH");
	if (pathEnv == nullptr) {
		LOG("PATH environment variable not found :(");
		return "";
	}

	// Split the PATH string into individual directories
	std::istringstream iss(pathEnv);
	std::vector<std::string> directories;
	std::string directory;
	while (std::getline(iss, directory, ';')) {
		directories.push_back(directory);
	}

	// Search for Python executable in each directory
	const std::string pythonExecutableName = "pythonw.exe";
	for (const auto& dir : directories) {
		std::filesystem::path directoryFromPATH = std::filesystem::path(dir);
		if (!std::filesystem::exists(directoryFromPATH)) { continue; }

		std::filesystem::path pythonPath = directoryFromPATH / pythonExecutableName;
		if (std::filesystem::exists(pythonPath)) {
			return pythonPath.string();  // Convert path to string
		}
	}

	LOG("Python interpreter not found in PATH directories :(");
	return "";
}



void CustomQuickchat::STTLog(const std::string& message) {
	CVarWrapper speechToTextNotificationsOnCvar = cvarManager->getCvar("customQuickchat_speechToTextNotificationsOn");
	if (!speechToTextNotificationsOnCvar) { return; }

	if (speechToTextNotificationsOnCvar.getBoolValue()) {
		CVarWrapper popupNotificationDurationCvar = cvarManager->getCvar("customQuickchat_popupNotificationDuration");
		if (!popupNotificationDurationCvar) { return; }

		PopupNotification(message, "speech-to-text", popupNotificationDurationCvar.getFloatValue());
	}

	LOG("[SPEECH-TO-TEXT] " + message);
}


void CustomQuickchat::STTWaitAndProbe(const std::string& chatMode, const std::string& effect, const std::string& attemptID, bool test) {
	CVarWrapper processSpeechTimeoutCvar = cvarManager->getCvar("customQuickchat_processSpeechTimeout");
	if (!processSpeechTimeoutCvar) { return; }
	int processSpeechTimeout = processSpeechTimeoutCvar.getIntValue();



	// probe JSON file ...
	for (int i = 0; i < (((processSpeechTimeout - 2) * 5) + 1); i++) {

		gameWrapper->SetTimeout([this, chatMode, effect, attemptID, test, i](GameWrapper* gw) {

			if (ActiveSTTAttemptID == attemptID) {

				std::string jsonFileRawStr = readContent(speechToTextFilePath);

				// prevent crash on reading invalid JSON data
				try {
					auto transcriptionData = nlohmann::json::parse(jsonFileRawStr);
					auto transcription = transcriptionData["transcription"];

					if (transcription.empty()) { return; }		// return if still processing

					// make sure JSON data is from the same attempt
					std::string jsonAttemptID = transcriptionData["transcription"]["ID"];
					if (jsonAttemptID != attemptID) { return; }


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

		if (ActiveSTTAttemptID == attemptID && !test) {
			STTLog("Processing reached timeout of " + std::to_string(processSpeechTimeout) + " seconds... aborting");

			// clear active attempt ID
			ActiveSTTAttemptID = "420_blz_it_lmao";
		}

		}, processSpeechTimeout);

}


void CustomQuickchat::StartSpeechToText(const std::string& chatMode, const std::string& effect, bool test) {

	// reset transcription data
	if (!ClearTranscriptionJson()) {
		STTLog("[ERROR] 'SpeechToText.json' cannot be found");
		return;
	}


	std::string pyInterpreter = findPythonInterpreter();

	if (pyInterpreter == "") {
		STTLog("[ERROR] Couldn't find pythonw.exe interpreter in PATH variable");
		return;
	}


	// get CVar for start speech timeout
	CVarWrapper waitForSpeechTimeoutCvar = cvarManager->getCvar("customQuickchat_waitForSpeechTimeout");
	if (!waitForSpeechTimeoutCvar) { return; }
	float waitForSpeechTimeout = waitForSpeechTimeoutCvar.getFloatValue() - 1.1;	// an additional ~1.1 seconds is added in py script due to pause/phrase thresholds

	// generate unique attempt ID
	std::string attemptID = generateRandomString(10);
	ActiveSTTAttemptID = attemptID;		// store in global variable (so other attempts can see/compare to it)
	LOG("ID for current speech-to-text attempt: {}", ActiveSTTAttemptID);


	std::string pathToPyInterpreter = "\"" + pyInterpreter + "\"";
	std::string pathToPywScript = "\"" + speechToTextPyScriptFilePath.string() + "\"";
	std::string pathToJsonFile = "\"" + speechToTextFilePath.string() + "\"";

	// Command-line to execute Python script with JSON filepath as 1st arg
	std::string command = pathToPyInterpreter + " " + pathToPywScript + " " + pathToJsonFile + " " + std::to_string(waitForSpeechTimeout) + " " + attemptID;

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

		if (!test) {
			// prompt user for speech
			STTLog("listening......");
		}

		// wait for speech, and probe JSON file for response
		STTWaitAndProbe(chatMode, effect, attemptID, test);

	}
	else {
		// Failed to create process
		DWORD error = GetLastError();

		STTLog("Error executing Python script with CreateProcess. Error code: " + std::to_string(error));
	}

}


bool CustomQuickchat::ClearTranscriptionJson() {

	if (!std::filesystem::exists(speechToTextFilePath)) {
		return false;
	}

	// reset transcription data
	std::ofstream NewFile(speechToTextFilePath);
	NewFile << "{ \"transcription\": {} }";
	NewFile.close();
	DEBUGLOG("cleared STT JSON file...");

	return true;
}