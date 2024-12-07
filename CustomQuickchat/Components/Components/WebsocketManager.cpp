#include "pch.h"
#include "WebsocketManager.hpp"


WebsocketClientManager::WebsocketClientManager(std::shared_ptr<CVarManagerWrapper> InCvarManager, const std::string& serverUri, std::function<void(json serverResponse)> response_callback)
	: cvarManager(InCvarManager), server_uri(serverUri), handle_server_response(response_callback)
{
	// Set up client
	ws_client.init_asio();
	LOG("[WebsocketManager] Initialized asio");

	// Bind callbacks
	ws_client.set_open_handler(     std::bind(&WebsocketClientManager::OnWsOpen, this, std::placeholders::_1));
	ws_client.set_close_handler(    std::bind(&WebsocketClientManager::OnWsClose, this, std::placeholders::_1));
	ws_client.set_message_handler(  std::bind(&WebsocketClientManager::OnWsMessage, this, std::placeholders::_1, std::placeholders::_2));
	LOG("[WebsocketManager] Set callbacks for websocket client");
}


bool WebsocketClientManager::StartClient()
{
	if (is_connected)
	{
		LOG("[WebsocketManager] Client already started.");
		return false;
	}

	ws_client.reset();		// Reset to a clean state
	//ws_client.init_asio();	// Reinitialize ASIO

	// Create a connection to the server
	websocketpp::lib::error_code ec;
	auto connection = ws_client.get_connection(server_uri, ec);
	if (ec)
	{
		LOG("[WebsocketManager] Connection error: {}", ec.message().c_str());
		return false;
	}

	// Save connection handle
	ws_connection_handle = connection->get_handle();

	// Start the connection
	ws_client.connect(connection);

	// Run the ASIO event loop in a separate thread
	ws_client_thread = std::thread([this]() {
		try
		{
			ws_client.run();
		}
		catch (const std::exception& e)
		{
			LOG("[WebsocketManager] Exception in ASIO event loop: {}", e.what());
		}
	});

	ws_client_thread.detach();

	return true;
}


bool WebsocketClientManager::StopClient()
{
	if (!is_connected)
	{
		LOG("[WebsocketManager] Client already stopped.");
		return false;
	}

	// Close the connection gracefully
	websocketpp::lib::error_code ec;
	ws_client.close(ws_connection_handle, websocketpp::close::status::normal, "Client disconnect", ec);
	if (ec)
	{
		LOG("[WebsocketManager] Error closing connection: {}", ec.message().c_str());
		return false;
	}

	// Stop the ASIO event loop
	ws_client.stop();
	
	is_connected = false;
	//if (ws_client_thread.joinable())
	//{
	//	ws_client_thread.join();
	//}

	return true;
}


void WebsocketClientManager::SendEvent(const std::string& eventName, const json& dataJson)
{
	if (!ws_connection_handle.lock())
	{
		LOG("[WebsocketManager] ERROR: No active WebSocket connection!");
		return;
	}

	// Serialize JSON payload
	json payload = { { "event", eventName }, { "data", dataJson } };
	std::string message = payload.dump();

	websocketpp::lib::error_code ec;
	ws_client.send(ws_connection_handle, message, websocketpp::frame::opcode::text, ec);
	if (ec)
	{
		LOG("[WebsocketManager] Error sending message: {}", ec.message());
	}
	else
	{
		LOG("[WebsocketManager] Message sent: {}", message);
	}
}


void WebsocketClientManager::OnWsOpen(connection_hdl hdl)
{
	LOG("[WebsocketManager] Connected to WebSocket server");
	is_connected = true;
}

void WebsocketClientManager::OnWsClose(connection_hdl hdl)
{
	LOG("[WebsocketManager] Disconnected from WebSocket server");
	is_connected = false;
}

void WebsocketClientManager::OnWsMessage(connection_hdl hdl, PluginClient::message_ptr msg)
{
	LOG("[WebsocketManager] Message received: {}", msg->get_payload().c_str());

	// Process the message payload (e.g., parse JSON)
	try
	{
		json response = json::parse(msg->get_payload());
		
		// TODO: Handle the response data ...
		handle_server_response(response);
	}
	catch (const json::parse_error& e)
	{
		LOG("[WebsocketManager] JSON parse error: {}", e.what());
	}
}
