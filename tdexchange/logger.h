#pragma once

#include <string>
#include <iostream>


namespace logger
{

// logging mode
enum class mode
{
    INFO = 0,
    WARN = 1,
    ERR = 2,
};
static std::string mode_text[] = { "(INFO)", "(WARN)", "(ERROR)" };

static mode global_mode = mode::WARN;

/**
 * @brief Log a message with mode
 * @param text Message
 * @param m Optional mode
 * @return 
*/
static auto log(const std::string &text, mode m = mode::INFO) -> void
{
    if (static_cast<int>(m) >= static_cast<int>(global_mode))
    {
        std::cout << mode_text[static_cast<int>(m)] << " " << text << "\n";
    }
}


}