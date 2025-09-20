#pragma once
#include "WebsocketClient.hpp"
#include <memory>

enum class EWebsocketError
{
	NoError,
	MissingRequiredFiles,
	ServerAlreadyStarted,
	ServerHasntBeenStarted,
	UnableToStartServer,
	UnableToConnectToServer,
	UnableToDisconnectFromServer,
	ClientAlreadyDisconnected,
};

struct PythonServerProcess
{
public:
	PythonServerProcess() = default;
	~PythonServerProcess() { terminate(); } // might get the RAII bug where it needs to happen in onUnload, idk if it would matter tho

	void init(const fs::path& exePath);

public:
	EWebsocketError start(int port);
	bool            spawn(const std::string& args = "", const std::string& workingDir = "");
	void            terminate(std::function<void()> onDone = nullptr);

	// getters
	inline std::atomic<bool>& getRunningBool() { return m_running; }
	inline bool               isRunning() const { return m_running.load(); }

	inline bool getAllFilesExist() const { return m_allFilesExist; }

private:
	bool              m_allFilesExist = false;
	fs::path          m_exePath;
	std::atomic<bool> m_running{false};

	PROCESS_INFORMATION m_pi{};
};

// oink
using JsonResCallback = std::function<void(json)>;

class WebsocketConnectionManager
{
public:
	WebsocketConnectionManager() = delete;
	WebsocketConnectionManager(std::atomic<bool>& bConnecting) : m_isConnecting(bConnecting) {}
	~WebsocketConnectionManager() {}

	void init(const std::shared_ptr<GameWrapper>& gw, const std::shared_ptr<int>& port, const fs::path& serverExePath);

public:
	EWebsocketError connect(JsonResCallback callback);
	EWebsocketError disconnect();

	bool isConnected() const { return m_client->isConnected(); }
	void sendMessage(const std::string& eventName, const json& dataJson);
	void onPluginUnload();

	inline std::string getCurrentURI() const { return m_client->getCurrentURI(); }
	inline bool        serverExeExists() const { return m_pythonServer.getAllFilesExist(); }

private:
	void            connectClientToServer(JsonResCallback callback);
	EWebsocketError disconnectClientFromServer();
	void            stopWebsocketServer();

	inline std::string buildServerUri() const { return std::format("ws://localhost:{}", *m_port); }

private:
	static constexpr float START_WS_CLIENT_DELAY = 5.0f; // in seconds

	std::shared_ptr<GameWrapper> gameWrapper;
	std::shared_ptr<int>         m_port;

	std::atomic<bool>& m_isConnecting;

	PythonServerProcess                     m_pythonServer;
	std::unique_ptr<WebsocketClientManager> m_client;

public:
	void display_debugInfo();
};
