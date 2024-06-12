#pragma once

namespace market
{

enum class side
{
    BID = 0,
    ASK = 1
};
static const char *side_repr[] = { "BID", "ASK" };  // macro string

}