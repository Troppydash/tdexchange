#pragma once


#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <optional>

#include "id.h"
#include "user.h"
#include "ticker.h"
#include "transaction.h"
#include "order.h"

namespace market
{

/**
 * @brief A synced exchange containing a list of tickers and users
*/
class exchange
{
protected:
    // tickerid to ticker objects
    map<ids::ticker_id, ticker> m_tickers;

    // userid to user objects
    map<ids::user_id, user> m_users;

    // a list of transactions, chronologically
    vector<transaction> m_transactions;

    // mutex lock
    mutex m_update;

    // id system
    id_system<ids::order_id> m_order_id;
    id_system<ids::transaction_id> m_transaction_id;

public:
    exchange();


    //// USER UPDATING FUNCTIONS ////

    /**
     * @brief Process a user order by updating the respective user, ticker orderbooks, and/or order fillings.
     *
     * Defaults to a limit that fills at the price or better.
     * An IOC acts as a limit order where the unfilled portion is immediately cancelled.
     *
     * @param _side
     * @param userid
     * @param tickerid
     * @param price
     * @param volume
     * @param ioc
     * @return
    */
    auto user_order(side _side, ids::user_id userid, ids::ticker_id tickerid, int price, int volume, bool ioc = false) -> void;

    // cancels all orders of the user
    auto user_cancel(ids::user_id userid) -> void;

    auto user_cancel_ticker(ids::user_id userid, ids::ticker_id tickerid) -> void;

    // returns if the user is authenticated (a part of the exchange)
    auto user_auth(const std::string &name, const std::string &passphase) const->std::optional<int>;

    /// DISPLAYS ///
    auto repr_tickers() const->string;
    auto repr_users() const->string;
    auto repr_transactions() const->string;

    /// GETTERS ///
    auto get_tickers() const-> const map<ids::ticker_id, ticker> &;
    auto get_user(ids::user_id id) const -> const user &;
    auto get_users() const -> const map<ids::user_id, user> &;
    auto get_ticker(ids::ticker_id id) const -> const ticker &;
    auto get_ticker(const std::string &name) const -> const ticker &;
    auto has_ticker(const std::string &name) const -> bool;
    auto get_valuations() const->map<ids::ticker_id, int>;

    auto get_transactions() const->const vector<transaction> &;
    auto consume_transactions() -> vector<transaction>;
protected:
    // attempt to match any order given the new aggressor order
    auto process_order(const order &aggressor) -> void;
};

};


