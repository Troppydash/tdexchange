#include <iostream>
#include <string>
#include <format>
#include <functional>

#include <nlohmann/json.hpp>
using namespace nlohmann::json_literals;
using namespace nlohmann;


#include "logger.h"
#include "exchange.h"
#include "server.h"




auto start() -> void
{
    network::server server;
    server.start();
}


auto main() -> int
{
    start();




    /*market::exchange exch;

    exch.user_order(market::side::BID, 1, 1, 1000, 10);
    exch.user_order(market::side::ASK, 1, 1, 1010, 10);
    std::cout << exch.repr_tickers();

    exch.user_order(market::side::BID, 2, 1, 1020, 15, true);
    std::cout << exch.repr_tickers();

    std::cout << exch.repr_transactions();*/

    return 0;
}