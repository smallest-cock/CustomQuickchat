#include "pch.h"
#include "WebsocketManager.hpp"
#include "websocketpp/roles/client_endpoint.hpp"

// ##############################################################################################################
// ###############################################    INIT    ###################################################
// ##############################################################################################################

WebsocketClientManager::WebsocketClientManager(std::atomic<bool>& bConnecting) : m_isConnecting(bConnecting)
{
	m_endpoint.init_asio();
	m_endpoint.start_perpetual();

	m_runThread = std::thread(
	    [this]()
	    {
		    LOG("Event loop starting...");
		    try
		    {
			    m_endpoint.run();
		    }
		    catch (const std::exception& e)
		    {
			    LOGERROR("Exception in event loop: {}", e.what());
		    }
		    LOG("Event loop exiting");
	    });
}

WebsocketClientManager::~WebsocketClientManager()
{
	LOG("Shutting down WebSocket client...");
	m_endpoint.stop_perpetual();

	if (m_isConnected && m_connectionHandle.lock())
	{
		std::error_code ec;
		m_endpoint.close(m_connectionHandle, websocketpp::close::status::normal, "", ec);
		if (ec)
			LOGERROR("Error closing WebSocket connection: {}", ec.message());
	}

	if (m_runThread.joinable())
		m_runThread.join();

	m_isConnected.store(false);
	LOG("Client shutdown complete");
}

// ##############################################################################################################
// ###############################################    FUNCTIONS    ##############################################
// ##############################################################################################################

bool WebsocketClientManager::connect(const std::string& uri, JsonMsgHandler msgHandler)
{
	std::error_code ec;
	auto            con = m_endpoint.get_connection(uri, ec);
	if (ec)
	{
		LOGERROR("Failed to create connection: {}", ec.message());
		return false;
	}

	// set/store connection metadata
	m_connectionHandle      = con->get_handle();
	m_uri                   = uri;
	m_serverResponseHandler = msgHandler;

	// From docs: Either the fail handler or the open handler will be called for each WebSocket connection attempt
	con->set_open_handler(
	    [this](connection_hdl hdl)
	    {
		    LOG("Connected to websocket server");
		    m_isConnected.store(true);
		    m_isConnecting.store(false);
	    });
	con->set_fail_handler(
	    [this](connection_hdl hdl)
	    {
		    auto connection = m_endpoint.get_con_from_hdl(hdl);
		    LOGERROR("Failed to connect to server. Error code: {}", connection->get_ec().message());
		    m_isConnected.store(false);
		    m_isConnecting.store(false);
	    });

	/**
	 * The close handler is called once for every successfully established
	 * connection after it is no longer capable of sending or receiving new messages
	 *
	 * The close handler will be called exactly once for every connection for which
	 * the open handler was called.
	 */
	con->set_close_handler(
	    [this](connection_hdl hdl)
	    {
		    auto connection = m_endpoint.get_con_from_hdl(hdl);
		    LOG("Closed connection to server. Error code: {}", connection->get_ec().message());
		    m_isConnected.store(false);
		    m_isConnecting.store(false);
	    });

	con->set_message_handler(
	    [this](connection_hdl hdl, WebsocketClient::message_ptr msg)
	    {
		    m_isConnected.store(true);
		    m_isConnecting.store(false);

		    try
		    {
			    std::string payload = msg->get_payload();
			    LOG("Message received: {}", payload);

			    json response = json::parse(payload);
			    m_serverResponseHandler(response);
		    }
		    catch (const std::exception& e)
		    {
			    LOGERROR("Failed to process message: {}", e.what());
		    }
	    });

	LOG("Set handlers new connection");

	m_endpoint.connect(con);
	return true;
}

bool WebsocketClientManager::disconnect()
{
	std::error_code ec;
	m_endpoint.close(m_connectionHandle, websocketpp::close::status::normal, "Client disconnect", ec);
	if (ec)
	{
		LOGERROR("Failed to close connection: {}", ec.message().c_str());
		return false;
	}

	m_uri.clear();
	return true;
}

void WebsocketClientManager::sendMessage(const std::string& eventName, const json& dataJson)
{
	if (!m_connectionHandle.lock())
	{
		LOGERROR("Unable to send message. No active connection!");
		m_isConnected.store(false);
		m_isConnecting.store(false);
		return;
	}

	std::string message;
	try
	{
		json payload = {{"event", eventName}, {"data", dataJson}};
		message      = payload.dump(); // serialize JSON payload
	}
	catch (const std::exception& e)
	{
		LOGERROR("Exception while parsing message JSON: {}", e.what());
	}

	std::error_code ec;
	m_endpoint.send(m_connectionHandle, message, websocketpp::frame::opcode::text, ec);
	if (ec)
		LOGERROR("Failed to send message: {}", ec.message());
	else
		LOG("Message sent: \"{}\"", message);
}

void WebsocketClientManager::sendMessage(const std::string& msg)
{
	if (!m_connectionHandle.lock()) // crashes here, ig bc m_connectionHandle is null ptr
	{
		LOGERROR("Unable to send message. No active connection!");
		m_isConnected.store(false);
		m_isConnecting.store(false);
		return;
	}

	std::error_code ec;
	m_endpoint.send(m_connectionHandle, msg, websocketpp::frame::opcode::text, ec);
	if (ec)
		LOGERROR("Failed to send message: {}", ec.message());
	else
		LOG("Message sent: \"{}\"", msg);
}