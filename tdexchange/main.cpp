

#include "stdafx.h"

#include <iostream>
#include <string>
#include <functional>
#include <thread>

#include "logger.h"
#include "exchange.h"
#include "server.h"


auto main() -> int
{
    // start file server
    std::thread file([]()
    {
        network::file_server s{ 8081 };
        std::cout << "starting file server on port 8081" << std::endl;
        s.start();
    });


    network::server server{ 8080 };
    std::cout << "Starting exchange on port 8080" << std::endl;
    server.start();

    return 0;
}