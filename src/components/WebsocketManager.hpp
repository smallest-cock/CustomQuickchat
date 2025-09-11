#pragma once
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_NO_BOOST
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

using websocketpp::connection_hdl;

class WebsocketClientManager
{
	using PluginClient = websocketpp::client<websocketpp::config::asio_client>;

public:
	WebsocketClientManager(std::function<void(json serverResponse)> serverResponseCallback, std::atomic<bool>& connectingToWsServerBool);
	WebsocketClientManager() = delete;
	~WebsocketClientManager() { stopClient(); };

public:
	bool startClient(int port); // Connect to the WebSocket server
	bool stopClient();          // Disconnect from the WebSocket server

	void        sendEvent(const std::string& eventName, const json& dataJson);
	inline void setbUseBase64(bool val) { m_bUseBase64 = val; }
	inline bool isConnectedToServer() { return m_isConnected.load(); }

	inline std::string getPortStr() const { return m_portNumStr; }
	inline void        setConnectedStatus(bool val) { m_isConnected = val; }

private:
	int         m_portNum    = 42069;
	std::string m_portNumStr = std::to_string(m_portNum);
	std::string m_serverURI  = "ws://localhost:" + m_portNumStr; // WebSocket server URI
	bool        m_bUseBase64 = false;

	PluginClient   m_wsClient;           // The WebSocket client instance
	connection_hdl m_wsConnectionHandle; // Handle for the active connection
	std::thread    m_wsClientThread;     // Thread for the WebSocket client

	std::mutex         m_connectionMutex;
	std::atomic<bool>& m_connectingToWsServer;
	std::atomic<bool>  m_isConnected = false;
	std::atomic<bool>  m_shouldStop  = false;

	std::function<void(json serverResponse)> m_serverResponseCallback;

	void onWsOpen(connection_hdl hdl);
	void onWsClose(connection_hdl hdl);
	void onWsMessage(connection_hdl hdl, PluginClient::message_ptr msg);
};
