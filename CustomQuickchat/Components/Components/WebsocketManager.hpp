#pragma once

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>


using websocketpp::connection_hdl;
class CVarManagerWrapper;


class WebsocketClientManager
{
	using PluginClient = websocketpp::client<websocketpp::config::asio_client>;

public:
	WebsocketClientManager(std::shared_ptr<CVarManagerWrapper> InCvarManager, const std::string& serverUri, std::function<void(json serverResponse)> response_callback);

	bool StartClient();						// Connect to the WebSocket server
	bool StopClient();						// Disconnect from the WebSocket server

	void SendEvent(const std::string& eventName, const json& dataJson);
	void SetbUseBase64(bool bNewValue) { bUseBase64 = bNewValue; }
	bool IsConnectedToServer() { return is_connected; }

private:
	WebsocketClientManager() = delete;			// No default constructor
	//~WebsocketClientManager() { StopClient(); };

	std::shared_ptr<CVarManagerWrapper> cvarManager;
	std::string server_uri;					// WebSocket server URI
	bool bUseBase64 = false;

	PluginClient ws_client;					// The WebSocket client instance
	connection_hdl ws_connection_handle;	// Handle for the active connection
	std::thread ws_client_thread;			// Thread for the WebSocket client
	bool is_connected = false;				// Whether the client is connected to the server

	std::function<void(json serverResponse)> handle_server_response;

	void OnWsOpen(connection_hdl hdl);
	void OnWsClose(connection_hdl hdl);
	void OnWsMessage(connection_hdl hdl, PluginClient::message_ptr msg);
};
