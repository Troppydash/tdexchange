

#include "stdafx.h"

#include <iostream>
#include <string>
#include <functional>

#include "logger.h"
#include "exchange.h"
#include "server.h"


auto main() -> int
{
    std::cout << "Starting TDEXCHANGE" << std::endl;
    network::server server;
    server.start();

    return 0;
}