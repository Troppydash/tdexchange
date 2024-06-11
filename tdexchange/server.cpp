#include "stdafx.h"

#include "server.h"
#include "logger.h"

#include <fmt/core.h>
#include <httplib.h>

#include <chrono>
#include <functional>
#include <ranges>
#include <algorithm>
#include <random>

/**
 * @brief Wraps a throwable functor into an optional
 * @tparam T Type of the functor result
 * @param fn Functor
 * @return An optional result
*/
template <typename T>
static std::optional<T> wrap_optional(std::function<T(void)> fn) noexcept
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

/**
 * @brief Try parsing a json string
 * @param j The potential json string
 * @return An optional json object
*/
static std::optional<json> try_parse_json(const std::string &j)
{
    std::function<json(void)> fn = [&]() {return json::parse(j); };
    return wrap_optional(fn);
}

network::server::server(unsigned short port)
    : m_port(port), m_nextid(0), m_pool(8), m_ws(), m_exchange_next_transaction(0)
{
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

        // disable logging
        m_ws.set_access_channels(websocketpp::log::alevel::none);

        logger::log(fmt::format("server started on port {}", m_port));
        m_ws.listen(m_port);
        m_ws.start_accept();
        m_ws.run();
    }
    catch (const ws::exception &ex)
    {
        logger::log(fmt::format("ws error {}", ex.what()), logger::mode::ERR);
    }
    /*catch (std::exception &ex)
    {
        logger::log(fmt::format("other exception occurred, stopping, {}", ex.what()));
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
    std::future<void> _ = m_pool.submit_task([&, id]()
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        if (!m_rconnections.contains(id))
        {
            logger::log(fmt::format("user {} already disconnected prior to auth timeout", id));
            return;
        }

        if (!m_user_map.contains(id))
        {
            logger::log(fmt::format("stopping user {} due to lack of authentication", id));

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
        logger::log(fmt::format("user {} disconnected", id));

        // we only erase the reverse user exchange id if it corresponds with the connection id,
        // otherwise leave it unchanged towards the new connection id
        int user_exchange_id = m_user_map.at(id);
        if (m_r_user_map.at(user_exchange_id) == id)
            m_r_user_map.erase(user_exchange_id);

        m_user_map.erase(id);
    }
    else
    {
        logger::log(fmt::format("connection {} disconnected", id));
    }

    m_connections.erase(hdl);
    m_rconnections.erase(id);
    m_connection_lock.unlock();
}

auto network::server::on_message(ws::connection_hdl hdl, websocket::message_ptr ptr) -> void
{
    int id = rand() % 100;
    logger::log(fmt::format("id {}, received message {} with code {}", id, ptr->get_payload(), static_cast<int>(ptr->get_opcode())));

    // check if user the authorized
    m_connection_lock.lock();
    if (!m_connections.contains(hdl))
    {
        m_connection_lock.unlock();
        logger::log(fmt::format("id {}, no user for the message exists", id));
        return;
    }

    if (ptr->get_opcode() == ws_opcode::text)
    {
        // try parsing json
        std::optional<json> payload = try_parse_json(ptr->get_payload());
        if (!payload)
        {
            m_connection_lock.unlock();
            logger::log(fmt::format("id {}, unknown message payload", id));
            return;
        }

        // parse payload
        parse_payload(payload.value(), id, m_connections.at(hdl));
        m_connection_lock.unlock();
        return;
    }

    m_connection_lock.unlock();
    logger::log(fmt::format("id {}, unknown message code {}", id, static_cast<int>(ptr->get_opcode())), logger::mode::WARN);
    return;
}

auto network::server::start_exchange() -> void
{
    m_exchange_flag = false;

    // initial exchange stuff
   /* m_exchange_lock.lock();
    m_exchange.user_order(market::side::BID, 1, 1, 10000, 10);
    m_exchange.user_order(market::side::ASK, 1, 1, 10100, 100);
    m_exchange.user_order(market::side::BID, 2, 1, 10100, 3);
    m_exchange_lock.unlock();*/

    std::default_random_engine rng;
    int ms = 40;
    int tickid = 0;
    int adminclock = 1000 / ms;
    int admintick = 0;

    while (!m_exchange_flag)
    {
        // once a second
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));

        // tick
        logger::log(fmt::format("TICK {}", tickid));
        tickid++;

        // process queue
        m_action_lock.lock();
        while (!m_actions.empty())
        {
            const action &act = m_actions.front();

            if (const action_order *order = std::get_if<action_order>(&act))
            {
                // when action is to order, process the order
                const auto &[ticker, ioc, bid, price, volume, user] = *order;

                if (!m_exchange.has_ticker(ticker))
                {
                    goto next;
                }

                int tickerid = m_exchange.get_ticker(ticker).get_id();
                m_exchange.user_order(
                    bid ? market::side::BID : market::side::ASK,
                    user,
                    tickerid,
                    price,
                    volume,
                    ioc
                );
            }
            else if (const delete_order *order = std::get_if<delete_order>(&act))
            {
                const auto &[ticker, user] = *order;

                if (!m_exchange.has_ticker(ticker))
                {
                    goto next;
                }

                int tickerid = m_exchange.get_ticker(ticker).get_id();
                m_exchange.user_cancel_ticker(user, tickerid);
            }
            else
            {
                logger::log("unknown action encountered", logger::mode::WARN);
            }

        next:
            m_actions.pop();
        }
        m_action_lock.unlock();


        m_connection_lock.lock();
        // preparing data to send tick updates
        const std::map<int, int> &valuations = m_exchange.get_valuations();
        json orderbook = generate_orderbook();

        // compute transactions
        json ts = json::array();
        const auto &transactions = m_exchange.get_transactions();
        for (auto it = transactions.begin() + m_exchange_next_transaction; it < transactions.end(); ++it)
        {
            const market::transaction &trans = *it;
            json trans_json = {
                {"id", trans.id},
                {"bidder", m_exchange.get_user(trans.bidder_id).get_alias()},
                {"asker", m_exchange.get_user(trans.asker_id).get_alias()},
                {"bid_order", trans.bid_id},
                {"ask_order", trans.ask_id},
                {"ticker", m_exchange.get_ticker(trans.ticker_id).get_alias()},
                {"aggressor_bid", trans.aggressor == market::side::BID},
                {"price", trans.price},
                {"volume", trans.volume}
            };
            ts.push_back(trans_json);
        }
        m_exchange_next_transaction += ts.size();


        // randomize the user order that the ticks are sent to
        auto kv = std::views::keys(m_user_map);
        std::vector<int> ids{ kv.begin(), kv.end() };
        std::shuffle(ids.begin(), ids.end(), rng);

        // for each user, send its customized update
        for (const auto &id : ids)
        {
            int userid = m_user_map.at(id);

            // get user holdings
            const market::user &user = m_exchange.get_user(userid);
            const std::map<int, int> &holdings = user.get_holdings();

            json position = generate_user_position(userid);

            json usr = {
                {"wealth", user.get_assets(valuations)},
                {"cash", user.get_cash()},
            };

            json payload = {
                {"type", "tick"},
                {"id", tickid},
                {"position", position},
                {"orderbook", orderbook},
                {"user", usr},
                {"transactions", ts}
            };
            send_json(payload, id);
        }


        // create admin message
        if (admintick <= 0)
        {
            admintick = adminclock;
            json admin = {
                {"type", "admin-tick"},
                {"id", tickid},
                {"users", json::object()}
            };
            for (const auto &[id, user] : m_exchange.get_users())
            {
                json user_json = {
                    {"cash", user.get_cash()},
                    {"wealth", user.get_assets(valuations)},
                    {"holdings", generate_user_position(id)}
                };

                admin["users"][user.get_alias()] = user_json;
            }

            // check for admin
            for (const auto &id : ids)
            {
                int userid = m_user_map.at(id);

                if (m_exchange.get_user(userid).get_admin())
                {
                    send_json(admin, id);
                }
            }
        }
        else
        {
            admintick -= 1;
        }
        

        m_connection_lock.unlock();
    }
}

auto network::server::stop_exchange() -> void
{
    m_exchange_flag = true;
}

auto network::server::generate_orderbook() const -> json
{
    json prices = json::object();
    for (const auto &[id, ticker] : m_exchange.get_tickers())
    {
        market::orderbook book = ticker.get_orderbook();
        json bids = json::array();
        for (const auto &[price, volume] : book.bids)
        {
            bids.push_back({ {"price", price}, {"volume", volume} });
        }
        std::sort(bids.begin(), bids.end(), [&](const auto &v1, const auto &v2)
        {
            return v1["price"] > v2["price"];
        });

        json asks = json::array();
        for (const auto &[price, volume] : book.asks)
        {
            asks.push_back({ {"price", price}, {"volume", volume} });
        }
        std::sort(asks.begin(), asks.end(), [&](const auto &v1, const auto &v2)
        {
            return v1["price"] < v2["price"];
        });

        prices[ticker.get_alias()] = { {"bids", bids}, {"asks", asks}, {"last_price", ticker.get_valuation() } };
    }

    return prices;
}

auto network::server::generate_user_position(int userid) const -> json
{
    json holdings_json = json::object();

    const auto &user = m_exchange.get_user(userid);
    const std::map<int, int> &holdings = user.get_holdings();
    for (const auto &[id, ticker] : m_exchange.get_tickers())
    {
        int amount = 0;
        if (holdings.contains(id))
        {
            amount = holdings.at(id);
        }
        holdings_json[ticker.get_alias()] = amount;
    }
    return holdings_json;
}

auto network::server::parse_payload(const json &payload, int id, int user) -> void
{
    if (!payload.contains("type"))
    {
        logger::log(fmt::format("{} message no type", id));
        return;
    }

    std::string type = payload["type"];
    if (type != "auth" && !is_user_auth(user))
    {
        json pl = {
                {"type", "auth"},
                {"ok", false},
                {"message", "unauthorized action"}
        };
        send_json(pl, user);

        logger::log(fmt::format("id {}, unauthorized user", id));
        return;
    }

    if (type == "auth")
    {
        // process the authentication

        // check if the payload is well formed
        if (!(payload.contains("name") && payload.contains("passphase")))
        {
            json pl = {
                {"type", "auth"},
                {"ok", false},
                {"message", "misformed auth payload"}
            };
            send_json(pl, user);

            logger::log(fmt::format("id {}, misformed auth payload", id));
            return;
        }

        // ignore if already authorized
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
            logger::log(fmt::format("id {}, user {} authorized", id, static_cast<std::string>(payload["name"])));
        }
        else
        {
            json pl = {
                {"type", "auth"},
                {"ok", false},
                {"message", "incorrect auth details"}
            };
            send_json(pl, user);
            logger::log(fmt::format("id {}, unauthorized", id));
        }
    }
    else if (type == "order")
    {
        // process the order

        // check if the payload is well formed
        if (!(payload.contains("ticker")
            && payload.contains("price")
            && payload.contains("volume")
            && payload.contains("ioc")
            && payload.contains("bid")))
        {
            json pl = {
                {"type", "order"},
                {"ok", false},
                {"message", "misformed order payload"}
            };
            send_json(pl, user);

            logger::log(fmt::format("id {}, misformed order payload", id));
            return;
        }

        std::string ticker = payload["ticker"];
        int price = payload["price"];
        int volume = payload["volume"];
        bool ioc = payload["ioc"];
        bool bid = payload["bid"];

        m_action_lock.lock();
        m_actions.emplace(action_order{ ticker, ioc, bid, price, volume, m_user_map.at(user) });
        m_action_lock.unlock();


        json pl = {
                {"type", "order"},
                {"ok", true},
                {"message", "successfully queued order"}
        };
        send_json(pl, user);

        logger::log(fmt::format("id {}, queued order on {} with {} @ {}", id, ticker, volume, price));
    }
    else if (type == "delete")
    {
        if (!payload.contains("ticker"))
        {
            json pl = {
               {"type", "delete"},
               {"ok", false},
               {"message", "misformed delete payload"}
            };
            send_json(pl, user);

            logger::log(fmt::format("id {}, misformed delete payload", id));
            return;
        }

        std::string ticker = payload["ticker"];

        m_action_lock.lock();
        m_actions.emplace(delete_order{ ticker, m_user_map.at(user) });
        m_action_lock.unlock();

        json pl = {
                {"type", "delete"},
                {"ok", true},
                {"message", "successfully queued deletion"}
        };
        send_json(pl, user);

        logger::log(fmt::format("id {}, queued deletion on {}", id, ticker));
    }
    else
    {
        logger::log(fmt::format("id {}, unknown payload type {}", id, static_cast<std::string>(payload["type"])));
    }
}

auto network::server::send_json(const json &message, int user) -> void
{
    // check if the user exists or not
    if (!m_rconnections.contains(user))
    {
        logger::log(fmt::format("sending to user {} failed, no user found", user));
        return;
    }

    // if we can't send because the handle was closed before we process on_close,
    // too bad and just fail here whatever
    try
    {
        logger::log(fmt::format("sending to user {} of message {}", user, message.dump()));
        m_ws.send(m_rconnections.at(user), message.dump(), ws_opcode::text);
    }
    catch (const std::exception &ex)
    {
        logger::log(fmt::format("error in sending, reason: {}", ex.what()), logger::mode::ERR);
    }
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

    // no locking here because we've locked in parse_payload
    // TODO: this is scuffed
    m_r_user_map[user] = id;
    m_user_map[id] = user;

    return true;
}

auto network::server::is_user_auth(int user) const -> bool
{
    return m_user_map.contains(user);
}



network::file_server::file_server(unsigned short port, const std::string &path)
    : m_port(port), m_path(path)
{
}

auto network::file_server::start() -> void
{
    httplib::Server server;
    
    auto ret = server.set_mount_point("/", m_path);
    if (!ret)
    {
        throw std::runtime_error("no public directory found when serving static content");
    }

    std::cout << "listening\n";
    server.listen("127.0.0.1", m_port);
}
