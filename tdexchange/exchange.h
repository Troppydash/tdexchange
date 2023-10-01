#pragma once


#include <string>
#include <vector>
#include <map>
#include <mutex>


namespace market
{

// i am lazy
using namespace std;

// id singleton
class id_system
{
protected:
	map<string, int> m_ids;


public:
	id_system();

	auto get(string type) -> int;
};



enum class side
{
	BID = 0,
	ASK = 1
};
static const char *side_repr[] = { "BID", "ASK" };

struct transaction
{
	// transaction id
	int id;

	// whether the aggressor is bidding
	side aggressor;


	// transacted price and volume
	int volume;
	int price;

	// id of the bid and ask order
	// may be 0 if it is an IOC order
	int bid_id;
	int ask_id;

	int bidder_id;
	int asker_id;

	int ticker_id;

	/// DISPLAY ///
	auto repr() const->string;
};

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


class user
{
protected:
	// user name
	string m_alias;

	// login details
	int m_id;
	string m_passphase;

	// FINANCIALS
	int m_cash;
	// holdings mapping tickers to amounts
	map<int, int> m_holdings;
	map<int, order> m_orders;


public:
	user();

	// setup a user
	user(string name, int id, string passphase);

	// setup a user, using the name as a passphase (unsafe)
	user(string name, int id);

	auto add_order(const order &ord) -> void;
	auto remove_order(const order &ord) -> void;
	auto fill_order(const order &ord, int price, int volume, side type) -> void;
	auto view_order(int order_id) const->const order &;

	auto get_orders() const->vector<int>;

	/**
	 * @brief Returns the net wealth of the user, including cash and holdings
	 *
	 * @param valuations
	 * @return
	*/
	auto get_assets(const map<int, int> &valuations) const->int;

	auto repr() const->string;
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
	map<int, vector<order>> m_bids;
	map<int, vector<order>> m_asks;

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

	auto cancel_order(const order &ord) -> void;

	auto get_alias() const->string;

	auto get_valuation() const-> int;

	/// DISPLAYING ///

	auto repr_orderbook() const->string;

};


/**
 * @brief A synced exchange containing a list of tickers and users
*/
class exchange
{
protected:
	map<int, ticker> m_tickers;
	map<int, user> m_users;
	vector<transaction> m_transactions;

	mutex m_update;

	id_system m_id;

public:
	exchange();


	//// USER UPDATING FUNCTIONS ////

	auto user_order(side _side, int userid, int tickerid, int price, int volume, bool ioc) -> void;

	auto user_cancel(int userid) -> void;

	/// DISPLAYS ///
	auto repr_tickers() const->string;
	auto repr_users() const->string;
	auto repr_transactions() const->string;

protected:
	auto process_order(const order &aggressor) -> void;

};



}


