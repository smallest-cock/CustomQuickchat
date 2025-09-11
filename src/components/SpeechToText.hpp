#pragma once
#include "Component.hpp"
#include "CustomQuickchat.hpp"
#include "Structs.hpp"
#include "components/WebsocketManager.hpp"

struct ActiveSTTAttempt
{
	std::string attemptID;
	Binding     binding;
};

struct SpeechToTextResult
{
	bool        success = false;
	bool        error   = false;
	std::string outputStr;
};

struct MicCalibrationResult
{
	bool        success         = false;
	bool        error           = false;
	int         energyThreshold = 420;
	std::string errorMsg;
};

class STTComponent : Component<STTComponent>
{
public:
	STTComponent() {}
	~STTComponent() {}

	static constexpr std::string_view componentName = "SpeechToText";

	void init(const std::shared_ptr<CustomQuickchat>& mainPluginClass, const std::shared_ptr<GameWrapper>& gw);

private:
	std::shared_ptr<CustomQuickchat> m_pluginClass;

private:
	void initFilepaths();
	void initHooks();
	void initCvars();

private:
	// cvar values
	std::shared_ptr<bool> m_enableSTTNotifications = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_autoCalibrateMic       = std::make_shared<bool>(true);

	std::shared_ptr<int> m_websocketPort      = std::make_shared<int>(8003);
	std::shared_ptr<int> m_micEnergyThreshold = std::make_shared<int>(420);

	std::shared_ptr<float> m_notificationDuration    = std::make_shared<float>(3.0f);
	std::shared_ptr<float> m_beginSpeechTimeout      = std::make_shared<float>(3.0f);
	std::shared_ptr<float> m_speechProcessingTimeout = std::make_shared<float>(10.0f);
	std::shared_ptr<float> m_micCalibrationTimeout   = std::make_shared<float>(10.0f);

private:
	// constants
	static constexpr float PROBE_JSON_FREQUENCY  = 0.2f; // in seconds
	static constexpr float START_WS_CLIENT_DELAY = 5.0f; // in seconds

	// thread-safe flags
	std::atomic<bool> m_attemptingSTT        = false;
	std::atomic<bool> m_calibratingMicLevel  = false;
	std::atomic<bool> m_connectingToWsServer = false;

	// filepaths
	bool     m_allFilesExist = false;
	fs::path m_exePath;
	fs::path m_jsonPath;
	fs::path m_errorLogPath;

	ActiveSTTAttempt m_activeAtempt;

	std::shared_ptr<WebsocketClientManager> m_websocketClient = nullptr;
	Process::ProcessHandles                 m_pythonServerProcess;

	void startWebsocketStuff(bool onLoad = false); // starts server then client
	bool startWebsocketServer();
	void stopWebsocketServer();
	void processWsResponse(const json& res);

	// speech-to-text
	void startSTT(const Binding& binding);
	json generateDataForSTTAttempt();
	void processSTTResult(const json& res);

	// mic calibration
	void calibrateMicrophone();
	json generateDataForMicCalibrationAttempt() const;
	void processMicCalibrationResult(const json& res);

	// misc
	std::string generateAttemptId();
	void        ClearSttErrorLog();

	std::string CreateCommandString(const fs::path& executablePath, const std::vector<std::string>& args);
	void        sttLog(const std::string& message);
	void        notifyAndLog(const std::string& title, const std::string& message, int duration = 3);

public:
	void triggerSTT(const Binding& binding);
	void test();

public:
	void onUnload();

public:
	void display_settings();
};

extern class STTComponent SpeechToText;
