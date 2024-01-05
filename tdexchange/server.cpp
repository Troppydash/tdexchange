#include "server.h"
#include "logger.h"

#include <chrono>
#include <format>
#include <functional>



namespace ws_opcode = websocketpp::frame::opcode;

template <typename T>
static std::optional<T> wrap_optional(std::function<T(void)> fn)
{
	try
	{
		return fn();
	}
	catch (...)
	{
		return std::nullopt;
	}
}

static std::optional<json> try_parse_json(const std::string &j)
{
	std::function<json(void)> fn = [&]() {return json::parse(j); };
	return wrap_optional(fn);
}

//template <typename T, typename D>
//static auto execute_with_timeout(T fn, D duration)
//{
//	std::atomic<bool> timeout = false;
//	std::thread execute(fn);
//
//	std::this_thread::sleep_for(duration);
//	execute.
//}


network::server::server(unsigned short port)
	: m_port(port), m_nextid(0), m_pool(8)
{

	// exchange setup


}

auto network::server::start() -> void
{
	// TODO: make this shutdown gracefully

	using std::placeholders::_1;
	using std::placeholders::_2;

	// start exchange in new thread
	std::thread exchange{ &network::server::start_exchange, this };

	try {
		m_ws.init_asio();
		m_ws.set_open_handler(std::bind(&network::server::on_open, this, _1));
		m_ws.set_close_handler(std::bind(&network::server::on_close, this, _1));
		m_ws.set_message_handler(std::bind(&network::server::on_message, this, _1, _2));

		logger::log(std::format("server started on port {}", m_port));
		m_ws.listen(m_port);
		m_ws.start_accept();
		m_ws.run();
	}
	catch (const ws::exception &ex)
	{
		logger::log(std::format("ws error {}", ex.what()), logger::mode::ERR);
	}
	/*catch (std::exception &ex)
	{
		logger::log(std::format("other exception occurred, stopping, {}", ex.what()));
	}*/

	logger::log("stopping exchange...");
	stop_exchange();
	exchange.join();
	logger::log("...exchange stopped");
}

auto network::server::on_open(ws::connection_hdl hdl) -> void
{
	m_connection_lock.lock();
	int id = m_nextid++;
	m_connections[hdl] = id;
	m_rconnections[id] = hdl;
	m_connection_lock.unlock();

	// attempt to authorize user
	m_pool.submit_task([&, id]()
	{
		std::this_thread::sleep_for(std::chrono::seconds(2));
		if (!m_user_map.contains(id))
		{
			logger::log(std::format("stopping user {} due to lack of authentication", id));

			// terminate the handle
			force_close_user(id);
		}
	});
}

auto network::server::on_close(ws::connection_hdl hdl) -> void
{
	m_connection_lock.lock();
	int id = m_connections.at(hdl);

	if (m_user_map.contains(id))
	{
		logger::log(std::format("user {} disconnected", id));

		// we only erase the reverse user exchange id if it corresponds with the connection id,
		// otherwise leave it unchanged towards the new connection id
		int user_exchange_id = m_user_map.at(id);
		if (m_r_user_map.at(user_exchange_id) == id)
			m_r_user_map.erase(user_exchange_id);

		m_user_map.erase(id);
	}
	else
	{
		logger::log(std::format("connection {} disconnected", id));
	}

	m_connections.erase(hdl);
	m_rconnections.erase(id);
	m_connection_lock.unlock();
}

auto network::server::on_message(ws::connection_hdl hdl, websocket::message_ptr ptr) -> void
{
	int id = rand() % 100;
	logger::log(std::format("id {}, received message {} with code {}", id, ptr->get_payload(), static_cast<int>(ptr->get_opcode())));

	if (ptr->get_opcode() == ws_opcode::text)
	{
		// checks if the user exists for the message just in case
		if (!m_connections.contains(hdl))
		{
			logger::log(std::format("id {}, no user for the message exists", id));
			return;
		}

		// try parsing json
		std::optional<json> payload = try_parse_json(ptr->get_payload());
		if (!payload)
		{
			logger::log(std::format("id {}, unknown message payload", id));
			return;
		}

		// parse payload
		parse_payload(payload.value(), id, m_connections.at(hdl));
		return;
	}

	logger::log(std::format("id {}, unknown message code {}", id, static_cast<int>(ptr->get_opcode())), logger::mode::WARN);
	return;
}

auto network::server::start_exchange() -> void
{
	m_exchange_flag = false;

	while (!m_exchange_flag)
	{
		// once a second
		std::this_thread::sleep_for(std::chrono::seconds(1));

		logger::log("TICK");
	}
}

auto network::server::stop_exchange() -> void
{
	m_exchange_flag = true;
}

auto network::server::parse_payload(const json &payload, int id, int user) -> void
{
	if (!payload.contains("type"))
	{
		logger::log(std::format("{} message no type", id));
		return;
	}

	if (payload["type"] == "auth")
	{
		if (!(payload.contains("name") && payload.contains("passphase")))
		{
			json pl = {
				{"type", "auth"},
				{"ok", false},
				{"message", "misformed auth payload"}
			};
			send_json(pl, user);

			logger::log(std::format("id {}, misformed auth payload", id));
			return;
		}

		if (is_user_auth(user))
		{
			json pl = {
				{"type", "auth"},
				{"ok", false},
				{"message", "user already authed"}
			};
			send_json(pl, user);

			return;
		}

		bool ok = user_auth(user, payload["name"], payload["passphase"]);
		if (ok)
		{
			json pl = {
				{"type", "auth"},
				{"ok", true},
				{"message", "auth success"}
			};
			send_json(pl, user);
			logger::log(std::format("id {}, user {} authorized", id, static_cast<std::string>(payload["name"])));
		}
		else
		{
			json pl = {
				{"type", "auth"},
				{"ok", false},
				{"message", "incorrect auth details"}
			};
			send_json(pl, user);
			logger::log(std::format("id {}, unauthorized", id));
		}
	}

	return;
}

auto network::server::send_json(const json &message, int user) -> void
{
	if (!m_rconnections.contains(user))
	{
		logger::log(std::format("sending to user {} failed, no user found", user));
		return;
	}

	logger::log(std::format("sending to user {} of message {}", user, message.dump()));
	m_ws.send(m_rconnections.at(user), message.dump(), ws_opcode::text);
}

auto network::server::force_close_user(int id) -> void
{
	m_ws.close(m_rconnections.at(id), 1000, "forced closure");
}

auto network::server::user_auth(int id, const std::string &name, const std::string &passphase) -> bool
{
	std::optional<int> value = m_exchange.user_auth(name, passphase);

	if (!value.has_value())
		return false;

	int user = value.value();
	if (m_r_user_map.contains(user))
	{
		// first log the current one out
		force_close_user(m_r_user_map.at(user));
	}

	m_connection_lock.lock();
	m_r_user_map[user] = id;
	m_user_map[id] = user;
	m_connection_lock.unlock();

	return true;
}

auto network::server::is_user_auth(int user) const -> bool
{
	return m_user_map.contains(user);
}


