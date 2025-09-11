#include "pch.h"
#include "WebsocketManager.hpp"

WebsocketClientManager::WebsocketClientManager(
    std::function<void(json serverResponse)> serverResponseCallback, std::atomic<bool>& connectingToWsServerBool)
    : m_serverResponseCallback(serverResponseCallback), m_connectingToWsServer(connectingToWsServerBool)
{
	// Set up client
	m_wsClient.init_asio();
	LOG("[WebsocketManager] Initialized asio");

	// Bind callbacks
	m_wsClient.set_open_handler(std::bind(&WebsocketClientManager::onWsOpen, this, std::placeholders::_1));
	m_wsClient.set_close_handler(std::bind(&WebsocketClientManager::onWsClose, this, std::placeholders::_1));
	m_wsClient.set_message_handler(std::bind(&WebsocketClientManager::onWsMessage, this, std::placeholders::_1, std::placeholders::_2));
	LOG("[WebsocketManager] Set callbacks for websocket client");
}

bool WebsocketClientManager::startClient(int port)
{
	std::lock_guard<std::mutex> lock(m_connectionMutex);

	// update stored port info
	m_portNum    = port;
	m_portNumStr = std::to_string(port);
	m_serverURI  = "ws://localhost:" + m_portNumStr;
	LOG("[WebsocketManager] Updated port to {}", m_portNumStr);

	// Reset to a clean state
	m_wsClient.reset();
	m_shouldStop.store(false);
	LOG("[WebsocketManager] Reset client to a clean state...");

	// Create a connection to the server
	websocketpp::lib::error_code ec;
	auto                         connection = m_wsClient.get_connection(m_serverURI, ec);
	if (ec)
	{
		LOG("[WebsocketManager] Connection error: {}", ec.message().c_str());

		m_isConnected.store(false);
		m_connectingToWsServer.store(false);
		return false;
	}

	// Save connection handle
	m_wsConnectionHandle = connection->get_handle();
	LOG("[WebsocketManager] Saved connection handle...");

	// Start the connection
	m_wsClient.connect(connection);
	LOG("[WebsocketManager] Started the connection...");

	// Run the ASIO event loop in a separate thread
	m_wsClientThread = std::thread(
	    [this]()
	    {
		    LOG("[WebsocketManager] Running websocket client...");

		    constexpr int MAX_RETRY_ATTEMPTS = 5;
		    int           retryCount         = 0;
		    while (!m_shouldStop.load() && retryCount < MAX_RETRY_ATTEMPTS)
		    {
			    try
			    {
				    DEBUGLOG("[WebsocketManager] Processing a websocket event...");
				    m_wsClient.run_one(); // Run one iteration instead of blocking

				    // Small sleep to prevent tight loop
				    std::this_thread::sleep_for(std::chrono::milliseconds(50));
			    }
			    catch (const std::exception& e)
			    {
				    LOG("[WebsocketManager] Exception in ASIO event loop: {}", e.what());
				    m_isConnected.store(false);
				    m_connectingToWsServer.store(false);

				    retryCount++;
				    if (retryCount >= MAX_RETRY_ATTEMPTS)
					    LOG("[WebsocketManager] Max retry attempts reached, stopping websocket client");

				    break;
			    }
		    }
		    LOG("[WebsocketManager] Event loop thread exiting");
	    });

	m_wsClientThread.detach();
	LOG("[WebsocketManager] Detached websocket client thread...");

	return true;
}

bool WebsocketClientManager::stopClient()
{
	std::lock_guard<std::mutex> lock(m_connectionMutex);

	m_shouldStop.store(true); // Signal the thread to stop

	// Close the connection gracefully first
	websocketpp::lib::error_code ec;
	m_wsClient.close(m_wsConnectionHandle, websocketpp::close::status::normal, "Client disconnect", ec);
	if (ec)
	{
		LOG("[WebsocketManager] Error closing connection: {}", ec.message().c_str());
		return false;
	}

	// Stop the ASIO event loop
	try
	{
		m_wsClient.stop();
	}
	catch (const std::exception& e)
	{
		LOG("[WebsocketManager] Error stopping client: {}", e.what());
	}

	m_isConnected.store(false);
	return true;
}

void WebsocketClientManager::sendEvent(const std::string& eventName, const json& dataJson)
{
	if (!m_wsConnectionHandle.lock()) // crashes here, ig bc m_wsConnectionHandle is null ptr
	{
		LOG("[WebsocketManager] ERROR: No active WebSocket connection!");
		m_isConnected.store(false);
		m_connectingToWsServer.store(false);
		return;
	}

	try
	{
		// Serialize JSON payload
		json        payload = {{"event", eventName}, {"data", dataJson}};
		std::string message = payload.dump();

		websocketpp::lib::error_code ec;
		m_wsClient.send(m_wsConnectionHandle, message, websocketpp::frame::opcode::text, ec);
		if (ec)
			LOG("[WebsocketManager] Error sending message: {}", ec.message());
		else
			LOG("[WebsocketManager] Message sent: {}", message);
	}
	catch (const std::exception& e)
	{
		LOG("[WebsocketManager] Exception while sending message: {}", e.what());
	}
}

void WebsocketClientManager::onWsOpen(connection_hdl hdl)
{
	LOG("[WebsocketManager] Connected to WebSocket server");
	m_isConnected.store(true);
	m_connectingToWsServer.store(false);
}

void WebsocketClientManager::onWsClose(connection_hdl hdl)
{
	LOG("[WebsocketManager] Disconnected from WebSocket server");
	m_isConnected.store(false);
	m_connectingToWsServer.store(false);
}

void WebsocketClientManager::onWsMessage(connection_hdl hdl, PluginClient::message_ptr msg)
{
	m_isConnected.store(true);
	m_connectingToWsServer.store(false);

	// Process the message payload (e.g., parse JSON)
	try
	{
		std::string payload = msg->get_payload();
		LOG("[WebsocketManager] Message received: {}", payload);

		json response = json::parse(payload);
		m_serverResponseCallback(response);
	}
	catch (const std::exception& e)
	{
		LOG("[WebsocketManager] Error processing message: {}", e.what());
	}
}
