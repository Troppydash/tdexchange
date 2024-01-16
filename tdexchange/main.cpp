#include "stdafx.h"

#include <iostream>
#include <string>
#include <format>
#include <functional>

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
    network::server server;
    server.start();

    return 0;
}