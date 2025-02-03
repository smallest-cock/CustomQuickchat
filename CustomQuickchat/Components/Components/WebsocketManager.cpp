#include "pch.h"
#include "WebsocketManager.hpp"


WebsocketClientManager::WebsocketClientManager(std::function<void(json serverResponse)> response_callback, std::shared_ptr<bool> connecting_to_ws_server):
	handle_server_response(response_callback),
	connecting_to_server(connecting_to_ws_server)
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


bool WebsocketClientManager::StartClient(int port)
{
	//if (is_connected)
	//{
	//	LOG("[WebsocketManager] Client already started.");
	//	return false;
	//}

	// update stored port info
	port_number = port;
	port_num_str = std::to_string(port);
	server_uri = "ws://localhost:" + port_num_str;
	LOG("[WebsocketManager] Updated port to {}", port_num_str);

	ws_client.reset();		// Reset to a clean state
	LOG("[WebsocketManager] Reset client to a clean state...");

	//ws_client.init_asio();	// Reinitialize ASIO

	// Create a connection to the server
	websocketpp::lib::error_code ec;
	auto connection = ws_client.get_connection(server_uri, ec);
	if (ec)
	{
		LOG("[WebsocketManager] Connection error: {}", ec.message().c_str());

		is_connected = false;
		*connecting_to_server = false;
		return false;
	}

	// Save connection handle
	ws_connection_handle = connection->get_handle();
	LOG("[WebsocketManager] Saved connection handle...");

	// Start the connection
	ws_client.connect(connection);
	LOG("[WebsocketManager] Started the connection...");

	// Run the ASIO event loop in a separate thread
	ws_client_thread = std::thread([this]() {
		try
		{
			LOG("[WebsocketManager] Running websocket client...");
			ws_client.run();
		}
		catch (const std::exception& e)
		{
			LOG("[WebsocketManager] Exception in ASIO event loop: {}", e.what());
		}
	});

	ws_client_thread.detach();
	LOG("[WebsocketManager] Detached websocket client thread...");

	return true;
}


bool WebsocketClientManager::StopClient()
{
	//if (!is_connected)
	//{
	//	LOG("[WebsocketManager] Client already stopped.");
	//	return false;
	//}

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
		is_connected = false;
		*connecting_to_server = false;
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
	*connecting_to_server = false;
}


void WebsocketClientManager::OnWsClose(connection_hdl hdl)
{
	LOG("[WebsocketManager] Disconnected from WebSocket server");
	is_connected = false;
	*connecting_to_server = false;	// this can be misleading bc the client often "disconnects" right before it actually connects to the ws server
}


void WebsocketClientManager::OnWsMessage(connection_hdl hdl, PluginClient::message_ptr msg)
{
	is_connected = true;
	*connecting_to_server = false;
	
	LOG("[WebsocketManager] Message received: {}", msg->get_payload().c_str());

	// Process the message payload (e.g., parse JSON)
	try
	{
		json response = json::parse(msg->get_payload());
		handle_server_response(response);
	}
	catch (const json::parse_error& e)
	{
		LOG("[WebsocketManager] JSON parse error: {}", e.what());
	}
}


void WebsocketClientManager::set_connected_status(bool connected)
{
	is_connected = connected;
}

std::string WebsocketClientManager::get_port_str() const
{
	return port_num_str;
}
