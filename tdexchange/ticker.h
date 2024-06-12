#pragma once

#include <string>
#include <vector>
#include <map>
#include "id.h"
#include "order.h"
#include "user.h"
#include "transaction.h"

namespace market
{

using namespace std;

struct orderbook
{
    map<int, int> bids;
    map<int, int> asks;
};

/**
 * @brief A market ticker refering to a specific stock
*/
class ticker
{
protected:
    // ticker name
    string m_alias;

    // ticker id
    ids::ticker_id m_id;

    // bids and asks, price are the keys, values are the list of orders at that price
    // the vector is sorted so that orders at the front are processed first
    map<int, vector<market::order>> m_bids;
    map<int, vector<market::order>> m_asks;

    // valuation for the ticker
    int m_valuation;

public:
    ticker();

    // setup a ticker
    ticker(string name, ids::ticker_id id);

    /**
     * @brief Computes the transactions made upon inserting a new order
     *
     * Assuming that before the aggressor's order, no transactions are possible
     *
     * @param aggressor The aggressor's order
     * @param id Id system
     * @return A list of transactions
    */
    auto match(const order &aggressor, id_system<ids::transaction_id> &id) const->vector<transaction>;

    /**
     * @brief Updates the order book using a transaction and the aggressor orders
     *
     * Updates both side of the order book (one under the transaction, and the other under the aggressor)
     *
     * @param aggressor The aggressor order
     * @param trans The transaction resulting from that order, compute by this.match
     * @return
    */
    auto process_transaction(const order &aggressor, const transaction &trans) -> void;

    /**
     * @brief Adds an order to the order book
     *
     * @param aggresor
     * @return
    */
    auto add_order(const order &aggresor) -> void;

    /**
     * @brief Removes an order from the order book, raising exceptions when the order is not found
     *
     * @param ord The order containing the order id
     * @return
     */
    auto cancel_order(const order &ord) -> void;

    // returns whether the order book contains the order
    auto has_order(const order &ord) -> bool;

    /**
     * @brief Returns the string alias for the ticker
     * @return The string alias
    */
    auto get_alias() const->string;

    auto get_id() const -> ids::ticker_id;

    /**
     * @brief Returns the valuation of the ticker, using the last executed price
     * @return The valuation
    */
    auto get_valuation() const-> int;

    /// DISPLAYING ///

    /**
     * @brief Returns the string representation of the orderbook
     * @return
    */
    auto repr_orderbook() const->string;

    /// GETTERS ///
    auto get_orderbook() const->orderbook;

};

};