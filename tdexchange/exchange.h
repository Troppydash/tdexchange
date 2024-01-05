#pragma once


#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <optional>


namespace market
{

// i am lazy
using namespace std;

// id singleton generator
class id_system
{
protected:
    // next id containers
    map<string, int> m_ids;


public:
    id_system();

    // get an unique id for an container
    auto get(const string &type) -> int;
};

// the order type
enum class side
{
    BID = 0,
    ASK = 1
};
static const char *side_repr[] = { "BID", "ASK" };  // macro string


// represents a transaction in an order book
struct transaction
{
    // transaction id
    // TODO: change this to an unsigned long long
    int id;

    // whether the aggressor is bidding or asking
    side aggressor;

    // transacted price and volume
    int volume;
    int price;

    // id of the bid and ask order that enacted the transaction
    int bid_id;
    int ask_id;

    // bidder and asker id
    int bidder_id;
    int asker_id;

    // id of the ticker it operated on
    int ticker_id;

    /// DISPLAY ///
    auto repr() const->string;
};


// represents an order in the order book
struct order
{
    // order id
    int id;

    // foreign key of the user
    int user_id;
    int ticker_id;

    // whether the limit order is a bid or ask
    side wish;

    // volume and price of the order
    int price;
    int volume;

    /// DISPLAY ///
    auto repr() const->string;
};

// represents a user in the system
class user
{
protected:
    // user name
    string m_alias;

    // login details
    int m_id;
    string m_passphase;

    /// FINANCIALS ///

    // cash money held
    int m_cash;

    // holdings mapping tickers to amounts
    map<int, int> m_holdings;
    // mapping order ids to the order instances
    map<int, order> m_orders;


public:
    user();

    // setup a user
    user(string name, int id, string passphase);

    // setup a user, using the name as a passphase (unsafe)
    user(string name, int id);

    /// user operations

    // link an order with the user
    auto add_order(const order &ord) -> void;

    // remove the order from the user
    auto remove_order(const order &ord) -> void;

    // process the order for the user
    auto fill_order(const order &ord, int price, int volume, side type) -> void;

    // view the order given the id
    auto view_order(int order_id) const->const order &;

    // get a list of order ids
    auto get_orders() const->vector<int>;

    // return if the user has the order
    auto has_order(const order &ord) -> bool;

    /**
     * @brief Returns the net wealth of the user, including cash and holdings
     *
     * @param valuations
     * @return
    */
    auto get_assets(const map<int, int> &valuations) const->int;

    auto get_holdings() const->const map<int, int>;

    auto get_cash() const -> int;

    // returns whether the user info matches a given info
    auto match(const std::string &name, const std::string &passphase) const -> bool;

    /// DISPLAY ///
    auto repr() const->string;
};


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
    int m_id;

    // bids and asks, price are the keys, values are the list of orders at that price
    // the vector is sorted so that orders at the front are processed first
    map<int, vector<order>> m_bids;
    map<int, vector<order>> m_asks;

    // valuation for the ticker
    int m_valuation;

public:
    ticker();

    // setup a ticker
    ticker(string name, int id);

    /**
     * @brief Computes the transactions made upon inserting a new order
     *
     * Assuming that before the aggressor's order, no transactions are possible
     *
     * @param aggressor The aggressor's order
     * @param id Id system
     * @return A list of transactions
    */
    auto match(const order &aggressor, id_system &id) const->vector<transaction>;

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

    auto get_id() const -> int;

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


/**
 * @brief A synced exchange containing a list of tickers and users
*/
class exchange
{
protected:
    // tickerid to ticker objects
    map<int, ticker> m_tickers;

    // userid to user objects
    map<int, user> m_users;

    // a list of transactions, chronologically
    vector<transaction> m_transactions;

    // mutex lock
    mutex m_update;

    // id system
    id_system m_id;

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
    auto user_order(side _side, int userid, int tickerid, int price, int volume, bool ioc = false) -> void;

    // cancels all orders of the user
    auto user_cancel(int userid) -> void;

    auto user_cancel_ticker(int userid, int tickerid) -> void;

    // returns if the user is authenticated (a part of the exchange)
    auto user_auth(const std::string &name, const std::string &passphase) const->std::optional<int>;

    /// DISPLAYS ///
    auto repr_tickers() const->string;
    auto repr_users() const->string;
    auto repr_transactions() const->string;

    /// GETTERS ///
    auto get_tickers() const-> const map<int, ticker> &;
    auto get_user(int id) const -> const user &;
    auto get_ticker(int id) const -> const ticker &;
    auto get_ticker(const std::string &name) const -> const ticker &;
    auto has_ticker(const std::string &name) const -> bool;
    auto get_valuations() const->map<int, int>;

protected:
    // attempt to match any order given the new aggressor order
    auto process_order(const order &aggressor) -> void;
};



}


