#include "pch.h"
#include "WebsocketConnection.hpp"
#include "util/Macros.hpp"
#include <ModUtils/gui/GuiTools.hpp>

// ##############################################################################################################
// ###############################################    INIT    ###################################################
// ##############################################################################################################

void WebsocketConnectionManager::init(
    const std::shared_ptr<GameWrapper> &gw, const std::shared_ptr<int> &port, const fs::path &serverExePath) {
	gameWrapper = gw;
	m_port      = port;

	m_pythonServer.init(serverExePath);
	m_client = std::make_unique<WebsocketClientManager>(m_isConnecting, m_pythonServer.getRunningBool());
}

// ##############################################################################################################
// ############################################    FUNCTIONS    #################################################
// ##############################################################################################################

EWebsocketError WebsocketConnectionManager::connect(JsonResCallback callback) {
	EWebsocketError err = m_pythonServer.start(*m_port);
	if (err == EWebsocketError::NoError) {
		m_isConnecting.store(true);
		DELAY(START_WS_CLIENT_DELAY, { connectClientToServer(callback); }, callback);
	} else
		LOGERROR("Unable to start python websocket server");

	return err;
}

EWebsocketError WebsocketConnectionManager::disconnect() {
	auto err = disconnectClientFromServer();

	DELAY(0.5f, { stopWebsocketServer(); });
	return err;
}

void WebsocketConnectionManager::connectClientToServer(JsonResCallback callback) {
	if (!m_pythonServer.isRunning()) {
		LOGERROR("Unable to connect client. Websocket server hasn't been started!");
		m_isConnecting.store(false);
		return;
	}
	if (m_client->isConnected()) {
		LOGERROR("Websocket client is already connected to server!");
		m_isConnecting.store(false);
		return;
	}

	bool success = m_client->connect(buildServerUri(), callback);
	LOG("Connecting websocket client was {}", success ? "successful" : "unsuccessful");
}

EWebsocketError WebsocketConnectionManager::disconnectClientFromServer() {
	m_isConnecting.store(false);

	if (!m_pythonServer.isRunning()) {
		LOGERROR("Unable to disconnect client. Websocket server hasn't been started!");
		return EWebsocketError::ServerHasntBeenStarted;
	}
	if (!m_client->isConnected()) {
		LOGERROR("Websocket client is already disconnected from server!");
		return EWebsocketError::ClientAlreadyDisconnected;
	}

	if (bool success = m_client->disconnect()) {
		LOG("Successfully disconnected client from server");
		return EWebsocketError::NoError;
	} else {
		LOGERROR("Failed to disconnect client from server");
		return EWebsocketError::UnableToDisconnectFromServer;
	}
}

void WebsocketConnectionManager::stopWebsocketServer() {
	// Terminate server in background
	m_pythonServer.terminate([this]() {
		// Called when process is fully dead
		// m_startedWebsocketServer = false;
		m_isConnecting.store(false);
		LOG("Python websocket server terminated");
	});
}

void WebsocketConnectionManager::sendMessage(const std::string &eventName, const json &dataJson) {
	std::string message;
	try {
		json payload = {{"event", eventName}, {"data", dataJson}};
		message      = payload.dump(); // serialize JSON payload
	} catch (const std::exception &e) {
		LOGERROR("Exception while parsing message JSON: {}", e.what());
	}

	m_client->sendMessage(message);
}

void WebsocketConnectionManager::onPluginUnload() {
	// explicitly release resource here (in BM's dedicated onUnload, as opposed to leaving it up to RAII destructor)
	// to prevent weird behavior where m_websocketClient state persists across plugin reloads, fricking many things up
	m_client.reset();

	m_pythonServer.terminate();
}

// ##############################################################################################################
// #########################################    DISPLAY FUNCTIONS    ############################################
// ##############################################################################################################

void WebsocketConnectionManager::display_debugInfo() {
	if (ImGui::CollapsingHeader("debug websockets")) {
		ImGui::Text("m_pythonServer.isRunning(): %d", m_pythonServer.isRunning());
		ImGui::Text("m_connectingToWsServer: %d", m_isConnecting.load());
		GUI::Spacing(2);
	}
}

// ##############################################################################################################
// #######################################    PythonServerProcess    ############################################
// ##############################################################################################################

void PythonServerProcess::init(const fs::path &exePath) {
	m_exePath       = exePath;
	m_allFilesExist = fs::exists(exePath);
}

EWebsocketError PythonServerProcess::start(int port) {
	if (!m_allFilesExist) {
		LOGERROR("Missing required files. Check your installation");
		return EWebsocketError::MissingRequiredFiles;
	}
	if (isRunning()) {
		LOGERROR("Websocket server has already started!");
		return EWebsocketError::ServerAlreadyStarted;
	}

	if (spawn(std::to_string(port)))
		return EWebsocketError::NoError;
	else {
		LOGERROR("Failed to start Python websocket server");
		return EWebsocketError::UnableToStartServer;
	}
}

bool PythonServerProcess::spawn(const std::string &args, const std::string &workingDir) {
	terminate(); // ensure previous process is gone

	STARTUPINFO si{};
	si.cb = sizeof(si);

	std::wstring cmd = std::format(L"\"{}\" {}", m_exePath.wstring(), StringUtils::ToWideString(args));

	BOOL success = CreateProcessW(m_exePath.wstring().c_str(),
	    cmd.data(),
	    nullptr,
	    nullptr,
	    FALSE,
	    CREATE_NO_WINDOW,
	    nullptr,
	    workingDir.empty() ? nullptr : StringUtils::ToWideString(workingDir).c_str(),
	    &si,
	    &m_pi);

	if (!success) {
		LOGERROR("CreateProcess failed: {}", GetLastError());
		return false;
	}

	LOG("Started python server process...");
	m_running.store(true);
	return true;
}

// Terminate process in background thread
void PythonServerProcess::terminate(std::function<void()> onDone) {
	if (!m_running.load())
		return;

	// Capture handles
	HANDLE hProcess = m_pi.hProcess;
	HANDLE hThread  = m_pi.hThread;

	// Clear state immediately
	m_pi = {};
	m_running.store(false);

	if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
		std::thread([hProcess, hThread, onDone]() {
			if (!TerminateProcess(hProcess, 1))
				LOGERROR("TerminateProcess failed: {}", GetLastError());

			WaitForSingleObject(hProcess, INFINITE);

			if (!CloseHandle(hProcess))
				LOGERROR("CloseHandle on hProcess failed: {}", GetLastError());
			if (!CloseHandle(hThread))
				LOGERROR("CloseHandle on hThread failed: {}", GetLastError());

			if (onDone)
				onDone();
		}).detach();
	}
}
