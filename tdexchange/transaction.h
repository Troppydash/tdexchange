#pragma once

#include <string>
#include <map>
#include "id.h"
#include "side.h"

namespace market
{

using namespace std;

// represents a transaction in an order book
struct transaction
{
    // transaction id
    ids::transaction_id id;

    // whether the aggressor is bidding or asking
    side aggressor;

    // transacted price and volume
    int volume;
    int price;

    // id of the bid and ask order that enacted the transaction
    ids::order_id bid_id;
    ids::order_id ask_id;

    // bidder and asker id
    ids::user_id bidder_id;
    ids::user_id asker_id;

    // id of the ticker it operated on
    ids::ticker_id ticker_id;

    /// DISPLAY ///
    auto repr() const->string;
};

};
