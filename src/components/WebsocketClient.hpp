#pragma once
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_NO_BOOST
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

using websocketpp::connection_hdl;

// TODO: maybe use this ConnectionMetadata?
class ConnectionMetadata
{
	using client = websocketpp::client<websocketpp::config::asio_client>;

public:
	typedef websocketpp::lib::shared_ptr<ConnectionMetadata> ptr;

	ConnectionMetadata(int id, websocketpp::connection_hdl hdl, std::string uri)
	    : m_id(id), m_hdl(hdl), m_status("Connecting"), m_uri(uri), m_server("N/A")
	{
	}

	void on_open(client* c, websocketpp::connection_hdl hdl)
	{
		m_status = "Open";

		client::connection_ptr con = c->get_con_from_hdl(hdl);
		m_server                   = con->get_response_header("Server");
	}

	void on_fail(client* c, websocketpp::connection_hdl hdl)
	{
		m_status = "Failed";

		client::connection_ptr con = c->get_con_from_hdl(hdl);
		m_server                   = con->get_response_header("Server");
		m_error_reason             = con->get_ec().message();
	}

	friend std::ostream& operator<<(std::ostream& out, ConnectionMetadata const& data);

private:
	int                         m_id;
	websocketpp::connection_hdl m_hdl;
	std::string                 m_status;
	std::string                 m_uri;
	std::string                 m_server;
	std::string                 m_error_reason;
};

class WebsocketClientManager
{
	using WebsocketClient = websocketpp::client<websocketpp::config::asio_client>;
	using JsonMsgHandler  = std::function<void(json)>;

public:
	WebsocketClientManager() = delete;
	WebsocketClientManager(std::atomic<bool>& bConnecting, std::atomic<bool>& bPyServerRunning);
	~WebsocketClientManager();

public:
	bool connect(const std::string& uri, JsonMsgHandler msgHandler);
	bool disconnect();

	inline bool        isConnected() const { return m_isConnected.load(); }
	inline std::string getCurrentURI() const { return m_uri; }

	void sendMessage(const std::string& msg); // generic, reusable, just send some text (any serialization should've alr happened)

private:
	WebsocketClient m_endpoint;
	std::thread     m_runThread;

	JsonMsgHandler m_serverResponseHandler; // from main plugin class

	// connection metadata (maybe use metadata class like tutorial)
	connection_hdl     m_connectionHandle; // handle for the active connection
	std::string        m_uri;
	std::atomic<bool>  m_isConnected = false;
	std::atomic<bool>& m_isConnecting;
	std::atomic<bool>& m_pyServerRunning;

private:
	// LOG overloads
	template <typename... Args> static void LOG(std::string_view format_str, Args&&... args)
	{
		::LOG("[WebsocketClient] " + std::string(format_str), std::forward<Args>(args)...);
	}

	template <typename... Args> static void LOGERROR(std::string_view format_str, Args&&... args)
	{
		::LOG("[WebsocketClient] ERROR: " + std::string(format_str), std::forward<Args>(args)...);
	}
};
