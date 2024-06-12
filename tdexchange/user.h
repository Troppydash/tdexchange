#pragma once

#include <string>
#include <map>
#include <vector>

#include "id.h"
#include "order.h"

namespace market {

using namespace std;
// represents a user in the system
class user
{
protected:
    // user name
    string m_alias;

    // login details
    ids::user_id m_id;
    string m_passphase;

    bool m_is_admin;

    /// FINANCIALS ///

    // cash money held
    int m_cash;

    // holdings mapping tickers to amounts
    map<ids::ticker_id, int> m_holdings;
    // mapping order ids to the order instances
    map<ids::order_id, order> m_orders;


public:
    user();

    // setup a user
    user(string name, ids::user_id id, string passphase);

    // setup a user, using the name as a passphase (unsafe)
    user(string name, ids::user_id id);

    user(string name, ids::user_id id, bool is_admin);

    /// user operations

    // link an order with the user
    auto add_order(const order &ord) -> void;

    // remove the order from the user
    auto remove_order(const order &ord) -> void;

    // process the order for the user
    auto fill_order(const order &ord, int price, int volume, side type) -> void;

    // view the order given the id
    auto view_order(ids::order_id order_id) const->const order &;

    // get a list of order ids
    auto get_orders() const->vector<ids::order_id>;

    // return if the user has the order
    auto has_order(const order &ord) -> bool;

    /**
     * @brief Returns the net wealth of the user, including cash and holdings
     *
     * @param valuations
     * @return
    */
    auto get_assets(const map<ids::ticker_id, int> &valuations) const->int;
    auto get_holdings() const->const map<ids::ticker_id, int>;
    auto get_cash() const -> int;
    auto get_admin() const -> bool;
    auto get_alias() const -> const string &;

    // returns whether the user info matches a given info
    auto match(const std::string &name, const std::string &passphase) const -> bool;

    /// DISPLAY ///
    auto repr() const->string;
};
}

