#pragma once

#include <string>
#include <map>
#include "id.h"
#include "side.h"

namespace market
{
using namespace std;

// represents an order in the order book
struct order
{
    // order id
    ids::order_id id;

    // foreign key of the user
    ids::user_id user_id;
    ids::ticker_id ticker_id;

    // whether the limit order is a bid or ask
    side wish;

    // volume and price of the order
    int price;
    int volume;

    /// DISPLAY ///
    auto repr() const->string;
};
};