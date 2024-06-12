#include "exchange.h"
#include "logger.h"

#include <fmt/core.h>
#include <cassert>
#include <string>
#include <set>



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
    m_users = {};

    // create bots
    for (int i = 0; i < 20; ++i)
    {
        m_users.insert({ i, { fmt::format("bot-{:02}", i), i } });
    }

    // create trading accounts
    for (char c = 'a'; c <= 'p'; ++c)
    {
        m_users.insert({ (int)c, { fmt::format("trading-{}", c), (int)c } });
    }

    // create admin
    m_users.insert({ 1000, {"terry", 1000, true} });
}

auto market::exchange::user_order(side _side, ids::user_id userid, ids::ticker_id tickerid, int price, int volume, bool ioc) -> void
{
    if (ioc)
    {
        logger::log(fmt::format("user {} ordered IOC on {} of {} @ {}", userid, tickerid, volume, price));
    }
    else
    {
        logger::log(fmt::format("user {} ordered LIM on {} of {} @ {}", userid, tickerid, volume, price));
    }



    assert(m_tickers.contains(tickerid));
    assert(m_users.contains(userid));

    order neworder{ m_order_id.get("order"), userid, tickerid, _side, price, volume };
    // add order to user and ticker
    m_tickers[tickerid].add_order(neworder);
    m_users[userid].add_order(neworder);

    // proccess/match order
    process_order(neworder);

    // if is IOC order
    if (ioc)
    {
        // immediate cancel it from both the tickers and users
        if (m_tickers[tickerid].has_order(neworder))
        {
            m_tickers[tickerid].cancel_order(neworder);
        }

        if (m_users[userid].has_order(neworder))
        {
            m_users[userid].remove_order(neworder);
        }
    }
}

auto market::exchange::user_cancel(ids::user_id userid) -> void
{
    assert(m_users.contains(userid));

    logger::log(fmt::format("cancelling all orders for user {}", userid));

    auto &user = m_users[userid];
    vector<ids::order_id> ords = user.get_orders();
    for (ids::order_id ord : ords)
    {
        const order &o = user.view_order(ord);

        m_tickers[o.ticker_id].cancel_order(o);
        user.remove_order(o);

        logger::log(fmt::format("cancelled order {}", o.id));
    }


}

auto market::exchange::user_cancel_ticker(ids::user_id userid, ids::ticker_id tickerid) -> void
{
    assert(m_users.contains(userid));
    assert(m_tickers.contains(tickerid));

    logger::log(fmt::format("cancelling all orders on {} for user {}", tickerid, userid));

    auto &user = m_users[userid];
    vector<ids::order_id> ords = user.get_orders();
    for (ids::order_id ord : ords)
    {
        const order &o = user.view_order(ord);

        // but we skip the non-matching tickers
        if (o.ticker_id != tickerid)
            continue;

        m_tickers[o.ticker_id].cancel_order(o);
        user.remove_order(o);

        logger::log(fmt::format("cancelled order {}", o.id));
    }
}

auto market::exchange::user_auth(const std::string &name, const std::string &passphase) const -> std::optional<int>
{
    for (const auto &[id, u] : m_users)
    {
        if (u.match(name, passphase))
        {
            return { id };
        }
    }

    return std::nullopt;
}

auto market::exchange::repr_tickers() const -> string
{
    string repr = "=== Tickers ===\n";


    for (const auto &ticker : m_tickers)
    {
        repr += fmt::format("name {}, id {}", ticker.second.get_alias(), ticker.first) + "\n";
        repr += ticker.second.repr_orderbook();
        repr += "\n";
    }

    return repr;
}

auto market::exchange::repr_users() const -> string
{
    string repr = "=== Users ===\n";

    // compute valuations
    map<ids::ticker_id, int> valuations;
    for (const auto &ticker : m_tickers)
    {
        valuations[ticker.first] = ticker.second.get_valuation();
    }

    for (const auto &user : m_users)
    {
        repr += user.second.repr();
        repr += fmt::format("user assets {}\n", user.second.get_assets(valuations));
        repr += "\n";
    }

    return repr;
}

auto market::exchange::repr_transactions() const -> string
{
    string repr = "=== Transactions ===\n";

    for (const auto &trans : m_transactions)
    {
        repr += trans.repr() + "\n";
    }

    return repr;
}

auto market::exchange::get_tickers() const -> const map<ids::ticker_id, ticker> &
{
    return m_tickers;
}

auto market::exchange::get_user(ids::user_id id) const -> const user &
{
    assert(m_users.contains(id));

    return m_users.at(id);
}

auto market::exchange::get_users() const -> const map<ids::user_id, user> &
{
    return m_users;
}

auto market::exchange::get_ticker(ids::ticker_id id) const -> const ticker &
{
    assert(m_tickers.contains(id));

    return m_tickers.at(id);
}

auto market::exchange::get_ticker(const std::string &name) const -> const ticker &
{
    // TODO: cache this ticker alias lookup

    for (const auto &[_, ticker] : m_tickers)
    {
        if (ticker.get_alias() == name)
        {
            return ticker;
        }
    }

    throw std::runtime_error(fmt::format("cannot find ticker {} in exchange", name));
}

auto market::exchange::has_ticker(const std::string &name) const -> bool
{
    for (const auto &[_, ticker] : m_tickers)
    {
        if (ticker.get_alias() == name)
        {
            return true;
        }
    }

    return false;
}

auto market::exchange::get_valuations() const -> map<ids::ticker_id, int>
{
    map<ids::ticker_id, int> valuations;
    for (const auto &[id, ticker] : m_tickers)
    {
        valuations[id] = ticker.get_valuation();
    }
    return valuations;
}

auto market::exchange::get_transactions() const -> const vector<transaction> &
{
    return m_transactions;
}

auto market::exchange::consume_transactions() -> vector<transaction>
{
    auto trans = m_transactions;
    m_transactions.clear();
    return trans;
}

auto market::exchange::process_order(const order &aggressor) -> void
{
    assert(m_tickers.contains(aggressor.ticker_id));
    assert(m_users.contains(aggressor.user_id));


    vector<transaction> transactions = m_tickers[aggressor.ticker_id].match(aggressor, m_transaction_id);

    // add to transaction history && logging
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
        logger::log(fmt::format("    {}", trans.repr()));
        m_transactions.push_back(trans);
    }

    // perform transaction
    int total_volume = 0;
    for (const transaction &trans : transactions)
    {
        total_volume += trans.volume;

        ids::ticker_id ticker = trans.ticker_id;
        m_tickers[ticker].process_transaction(aggressor, trans);

        // update users' orders
        if (aggressor.wish == side::BID)
        {
            // get the order reference for the other side of the transaction
            const order &ord = m_users[trans.asker_id].view_order(trans.ask_id);

            // update both users' order references
            m_users[trans.asker_id].fill_order(ord, trans.price, trans.volume, side::ASK);
            m_users[aggressor.user_id].fill_order(aggressor, trans.price, trans.volume, side::BID);
        }
        else if (aggressor.wish == side::ASK)
        {
            // get the order reference for the other side of the transaction
            const order &ord = m_users[trans.bidder_id].view_order(trans.bid_id);

            // update both userss order references
            m_users[trans.bidder_id].fill_order(ord, trans.price, trans.volume, side::BID);
            m_users[aggressor.user_id].fill_order(aggressor, trans.price, trans.volume, side::ASK);
        }
    }
}

auto market::ticker::process_transaction(const order &aggressor, const transaction &trans) -> void
{
    // update valuation
    m_valuation = trans.price;

    // process the transaction, namely to update the orderbooks

    if (trans.aggressor == side::BID)
    {
        // if the aggressor is bid, we process the asks

        // filled at the ask price
        int ask_price = trans.price;
        int vol = trans.volume;
        ids::order_id ask_id = trans.ask_id;

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
            if (orders.size() == 0)
            {
                m_asks.erase(ask_price);
            }
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
            // this price level must be empty, for we cannot have two aggressors on the same price level
            assert(m_bids[aggressor.price].size() == 1);
            m_bids.erase(aggressor.price);
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
        ids::order_id bid_id = trans.bid_id;

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
            if (orders.size() == 0)
            {
                m_bids.erase(bid_price);
            }
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
            assert(m_asks[aggressor.price].size() == 1);
            m_asks.erase(aggressor.price);
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

auto market::ticker::cancel_order(const order &ord) -> void
{
    // find the order

    if (ord.wish == side::BID)
    {
        assert(m_bids.contains(ord.price));

        auto it = std::find_if(m_bids[ord.price].begin(), m_bids[ord.price].end(), [&](const auto &o)
        {
            return o.id == ord.id;
        });

        // not found here
        assert(it != m_bids[ord.price].end());

        // else found, and erase it
        m_bids[ord.price].erase(it);
        if (m_bids[ord.price].size() == 0)
        {
            m_bids.erase(ord.price);
        }
    }
    else if (ord.wish == side::ASK)
    {
        assert(m_asks.contains(ord.price));

        auto it = std::find_if(
            m_asks[ord.price].begin(), m_asks[ord.price].end(),
            [&](const auto &o)
        {
            return o.id == ord.id;
        });
        // not found here
        assert(it != m_asks[ord.price].end());

        // else found, and erase it
        m_asks[ord.price].erase(it);
        if (m_asks[ord.price].size() == 0)
        {
            m_asks.erase(ord.price);
        }
    }
}


