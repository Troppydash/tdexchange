#pragma once

#include <atomic>
#include <set>
#include <string>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <BS_thread_pool.hpp>

#include "exchange.h"


using websocket = websocketpp::server<websocketpp::config::asio>;
namespace ws = websocketpp;

#include <nlohmann/json.hpp>
using namespace nlohmann::json_literals;
using nlohmann::json;

namespace network
{

// server representing an websocket interface with the exchange
class server
{

public:
    /**
     * @brief Default constructor
     * @param port Port to open the websocket at
    */
    explicit server(unsigned short port = 8080);

    /**
     * @brief Start the websocket at the specified port
     * @return
    */
    auto start() -> void;

protected:  // raw connection callbacks
    /**
     * @brief Handles when a new connection is opened, by adding it into the connection map/queue
     * @param hdl The new connection handle
     * @return
    */
    auto on_open(ws::connection_hdl hdl) -> void;
    /**
     * @brief Handles when a connection is closed by removing it from the map
     * @param hdl The closing connection handle
     * @return
    */
    auto on_close(ws::connection_hdl hdl) -> void;
    /**
     * @brief Handles when a payload arrives from a given handle
     * @param hdl The handle of the origin
     * @param ptr Message ptr
     * @return
    */
    auto on_message(ws::connection_hdl hdl, websocket::message_ptr ptr) -> void;

protected:  // exchange related stuff
    /**
     * @brief Start an exchange loop that processes exchange stuff
     * @return
    */
    auto start_exchange() -> void;
    /**
     * @brief Terminates the exchange loop
     * @return
    */
    auto stop_exchange() -> void;

protected:  // user related stuff
    /**
     * @brief Terminate a user's connection
     * @param id The user id
     * @return
    */
    auto force_close_user(int id) -> void;
    /**
     * @brief Attempt to authorize a user, if successful, update the authorized user list
     * @param id The user id
     * @param name Client supplied name
     * @param passphase Client supplied passphase
     * @return Whether it is successfully
    */
    auto user_auth(int id, const std::string &name, const std::string &passphase) -> bool;
    /**
     * @brief Returns if the user is currently authorized
     * @param user The user id
     * @return
    */
    auto is_user_auth(int user) const -> bool;


protected:  // server logic
    /**
     * @brief Execute the payload instruction given from onmessage
     * @param payload The payload json
     * @param id Randomized Id of the message
     * @param user The user the message came from
     * @return
    */
    auto parse_payload(const json &payload, int id, int user) -> void;

    auto send_json(const json &message, int user) -> void;

protected:
    using connections = std::map<ws::connection_hdl, int, std::owner_less<ws::connection_hdl>>;
    using r_connections = std::map<int, ws::connection_hdl>;
    using user_map = std::map<int, int>;
    using r_user_map = std::map<int, int>;

    // port and websocket instance
    unsigned short m_port;
    websocket m_ws;

    // the connection/user id generator
    int m_nextid;
    // forward and reverse connection maps
    connections m_connections;
    r_connections m_rconnections;
    // mutex lock on modifying the connection maps
    std::mutex m_connection_lock;

    // mapping from exchange user id to connection user id
    r_user_map m_r_user_map;
    // mapping from connection user id to exchange user id
    user_map m_user_map;


    // exchange instance
    market::exchange m_exchange;
    // flag to indicate if the exchange should continue to process
    std::atomic<bool> m_exchange_flag;
    // lock to interface the exchange
    std::mutex m_exchange_lock;

    // task runner
    BS::thread_pool m_pool;
};



}


