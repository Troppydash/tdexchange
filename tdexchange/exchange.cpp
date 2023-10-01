#include "exchange.h"
#include "logger.h"

#include <format>
#include <cassert>
#include <string>
#include <set>

market::ticker::ticker()
{
	throw std::exception("not implemented");
}

market::ticker::ticker(string name, int id)
	: m_alias(name), m_id(id), m_asks(), m_bids()
{
}

auto market::ticker::match(const order &aggressor, id_system &id) const->vector<transaction>
{
	logger::log(std::format("matching ticker {}", m_alias));

	vector<transaction> transactions;

	// if the aggressor is bidding
	if (aggressor.wish == side::BID)
	{
		// no transactions when no relevent asks
		if (m_asks.empty())
			return {};


		// find the bid order
		const vector<order> &bids = m_bids.at(aggressor.price);
		assert(bids.size() >= 1);

		// if there are multiple bids, we cannot possibility make any transactions
		if (bids.size() > 1)
			return {};

		const order &bid = bids[0];

		// fill up from the best ask
		int vol = aggressor.volume;
		for (const auto &ask : m_asks)
		{
			int ask_price = ask.first;
			const vector<order> &orders = ask.second;

			// only transact when bid price is higher or equal than ask
			if (ask_price > aggressor.price)
			{
				// no matches any more
				break;
			}

			// match orders, prio the first orders
			// filled price will always be the ask price
			int filled_price = ask_price;
			for (const order &ord : orders)
			{
				if (vol > 0)
				{
					int filled_volume = std::min(vol, ord.volume);
					transactions.push_back(
						{ id.get("transaction"), side::BID, filled_volume, filled_price, bid.id, ord.id, bid.user_id, ord.user_id, m_id }
					);
					vol -= filled_volume;

					assert(vol >= 0);
				}

				// stop after we fill everything
				if (vol == 0)
				{
					break;
				}
			}

			// stop checking higher asks if we are done
			if (vol == 0)
			{
				break;
			}

		}

	}
	else if (aggressor.wish == side::ASK)
	{
		// else if the aggressor is asking

		if (m_bids.empty())
			return {};

		// find the ask order
		const vector<order> &asks = m_asks.at(aggressor.price);
		assert(asks.size() >= 1);

		if (asks.size() > 1)
			return {};

		const order &ask = asks[0];

		// fill up from the best bid
		int vol = aggressor.volume;
		for (auto bid = m_bids.rbegin(); bid != m_bids.rend(); ++bid)
		{
			int bid_price = bid->first;
			const vector<order> &orders = bid->second;

			// only transact when ask price is lower or equal than bid
			if (bid_price < aggressor.price)
			{
				// no matches any more
				break;
			}

			// match orders, prio the first orders
			// filled price will always be the bid price
			int filled_price = bid_price;
			for (const order &ord : orders)
			{
				if (vol > 0)
				{
					int filled_volume = std::min(vol, ord.volume);
					transactions.push_back(
						{ id.get("transaction"), side::ASK, filled_volume, filled_price, ord.id, ask.id, ord.user_id, ask.user_id,  m_id }
					);
					vol -= filled_volume;

					assert(vol >= 0);
				}

				// stop after we fill everything
				if (vol == 0)
				{
					break;
				}
			}

			// stop checking higher asks if we are done
			if (vol == 0)
			{
				break;
			}
		}
	}

	return transactions;
}

auto market::ticker::get_alias() const -> string
{
	return m_alias;
}

auto market::ticker::repr_orderbook() const -> string
{
	string repr;

	// header
	repr += std::format("{:8}|{:8}|{:8}", "Bids", "Price", "Asks") + "\n";
	repr += "==========================\n";

	set<int> prices;
	for (const auto &val : m_asks)
		prices.insert(val.first);
	for (const auto &val : m_bids)
		prices.insert(val.first);

	// highest to lowest asks
	for (auto it = prices.rbegin(); it != prices.rend(); ++it)
	{
		int price = *it;

		// compute total ask volume
		int ask_vol = 0;
		int bid_vol = 0;

		if (m_asks.contains(price))
		{
			for (const order &ord : m_asks.at(price))
				ask_vol += ord.volume;
		}
		
		if (m_bids.contains(price))
		{
			for (const order &ord : m_bids.at(price))
				bid_vol += ord.volume;
		}


		repr += std::format("{:8}|{:8.1f}|{:8}", bid_vol, price / 10.0, ask_vol) + "\n";
	}

	return repr;
}

market::user::user()
{
	throw std::exception("not implemented");
}

market::user::user(string name, int id, string passphase)
	: m_alias(name), m_id(id), m_passphase(passphase), m_holdings()
{
}

market::user::user(string name, int id)
	: m_alias(name), m_id(id), m_passphase(name), m_holdings()
{
}

market::exchange::exchange()
{
	// create fake users and tickers

	// fake tickers
	m_tickers = {
		{
			1,
			{ "PHILIPS_A", 1 }
		},
		{
			2,
			{ "PHILIPS_B", 2 }
		}
	};

	// fake users
	m_users = {
		{
			1,
			{"bot-01", 1},
		},
		{
			2,
			{"bot-02", 2}
		}
	};
}

auto market::exchange::user_order(side _side, int userid, int tickerid, int price, int volume, bool ioc) -> void
{
	logger::log(std::format("user {} ordered on {} of {} @ {}", userid, tickerid, volume, price));

	assert(m_tickers.contains(tickerid));

	order neworder{ m_id.get("order"), userid, tickerid, _side, price, volume };
	m_tickers[tickerid].add_order(neworder);

	// proccess/match order
	match_order(neworder);
}

auto market::exchange::repr_tickers() const -> string
{
	string repr = "=== Tickers ===\n";


	for (auto ticker : m_tickers)
	{
		repr += std::format("name {}, id {}", ticker.second.get_alias(), ticker.first) + "\n";
		repr += ticker.second.repr_orderbook();
		repr += "\n";
	}

	return repr;
}

auto market::exchange::match_order(const order &aggressor) -> void
{
	assert(m_tickers.contains(aggressor.ticker_id));

	vector<transaction> transactions = m_tickers[aggressor.ticker_id].match(aggressor, m_id);
	if (transactions.size() == 0)
	{
		logger::log("matched no transactions");
	}
	else
	{
		logger::log("matched the transactions:");
	}
	for (transaction &trans : transactions)
	{
		logger::log(std::format("    {}", trans.repr()));
		m_transactions.push_back(trans);
	}

	// perform transaction
	for (const transaction &trans : transactions)
	{
		int ticker = trans.ticker_id;
		m_tickers[ticker].process_transaction(aggressor, trans);
	}

	// also remove these from the users map
	// TODO:!

}

auto market::ticker::process_transaction(const order &aggressor, const transaction &trans) -> void
{

	if (trans.aggressor == side::BID)
	{
		// if the aggressor is bid, we process the asks

		// filled at the ask price
		int ask_price = trans.price;
		int vol = trans.volume;
		int ask_id = trans.ask_id;

		assert(m_asks.contains(ask_price));
		vector<order> &orders = m_asks[ask_price];

		auto it = std::find_if(orders.begin(), orders.end(), [&](const order &val)
		{
			return val.id == ask_id;
		});
		assert(it != orders.end());

		// remove said order if it is completely filled
		if (vol == it->volume)
		{
			orders.erase(it);
		}
		else
		{
			// otherwise we just reduce the volumne left
			it->volume -= vol;
		}


		// update aggressor bid order
		assert(m_bids.contains(aggressor.price) && m_bids[aggressor.price].size() == 1);
		order &aggressor_order = m_bids[aggressor.price][0];

		// remove order if competely filled
		if (aggressor_order.volume == vol)
		{
			m_bids[aggressor.price].clear();
		}
		else
		{
			aggressor_order.volume -= vol;
		}

	}
	else if (trans.aggressor == side::ASK)
	{
		// if the aggressor is ask, we focus on updating the bids

		// filled at the bid price
		int bid_price = trans.price;
		int vol = trans.volume;
		int bid_id = trans.bid_id;

		assert(m_bids.contains(bid_price));
		vector<order> &orders = m_bids[bid_price];

		auto it = std::find_if(orders.begin(), orders.end(), [&](const order &val)
		{
			return val.id == bid_id;
		});
		assert(it != orders.end());

		// remove said order if it is completely filled
		if (vol == it->volume)
		{
			orders.erase(it);
		}
		else
		{
			// otherwise we just reduce the volumne left
			it->volume -= vol;
		}

		// update aggressor ask order
		assert(m_asks.contains(aggressor.price) && m_asks[aggressor.price].size() == 1);
		order &aggressor_order = m_asks[aggressor.price][0];

		// remove order if competely filled
		if (aggressor_order.volume == vol)
		{
			m_asks[aggressor.price].clear();
		}
		else
		{
			aggressor_order.volume -= vol;
		}
	}
}

auto market::ticker::add_order(const order &aggressor) -> void
{
	if (aggressor.wish == side::BID)
	{
		m_bids[aggressor.price].push_back(aggressor);
	}
	else
	{
		m_asks[aggressor.price].push_back(aggressor);
	}
}

market::id_system::id_system()
	: m_ids()
{
}

auto market::id_system::get(string type) -> int
{
	if (!m_ids.contains(type))
	{
		m_ids[type] = 0;
	}

	m_ids[type] += 1;
	return m_ids[type];
}

auto market::transaction::repr() const -> string
{
	return std::format(
		"{} aggressor between {} and {} on {} of {} @ {}, orders {} and {}",
		market::side_repr[static_cast<int>(aggressor)], bidder_id, asker_id, ticker_id, volume, price, bid_id, ask_id
	);
}
