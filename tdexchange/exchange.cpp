#include "exchange.h"
#include "logger.h"

#include <fmt/core.h>
#include <cassert>
#include <string>
#include <set>

market::ticker::ticker()
{
    throw std::runtime_error("not implemented");
}

market::ticker::ticker(string name, int id)
    : m_alias(name), m_id(id), m_asks(), m_bids(), m_valuation(0)
{
}

auto market::ticker::match(const order &aggressor, id_system &id) const->vector<transaction>
{
    logger::log(fmt::format("matching ticker {}", m_alias));

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

auto market::ticker::has_order(const order &ord) -> bool
{
    if (ord.wish == side::ASK)
    {
        if (!m_asks.contains(ord.price))
            return false;

        auto it = std::find_if(m_asks[ord.price].begin(), m_asks[ord.price].end(), [&](const order &val)
        {
            return val.id == ord.id;
        });

        return it != m_asks[ord.price].end();
    }
    else
    {
        if (!m_bids.contains(ord.price))
            return false;

        auto it = std::find_if(m_bids[ord.price].begin(), m_bids[ord.price].end(), [&](const order &val)
        {
            return val.id == ord.id;
        });

        return it != m_bids[ord.price].end();
    }
}

auto market::ticker::get_alias() const -> string
{
    return m_alias;
}

auto market::ticker::get_id() const -> int
{
    return m_id;
}

auto market::ticker::get_valuation() const -> int
{
    return m_valuation;
}

auto market::ticker::repr_orderbook() const -> string
{
    string repr;

    // header
    repr += fmt::format("{:8}|{:8}|{:8}", "Bids", "Price", "Asks") + "\n";
    repr += "--------------------------\n";

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


        repr += fmt::format("{:8}|{:^8.1f}|{:<8}", bid_vol, price / 100.0, ask_vol) + "\n";
    }

    return repr;
}

auto market::ticker::get_orderbook() const -> orderbook
{
    orderbook book;

    for (const auto &[price, orders] : m_bids)
    {
        int volume = 0;
        for (const auto &ord : orders)
        {
            volume += ord.volume;
        }
        book.bids[price] = volume;
    }

    for (const auto &[price, orders] : m_asks)
    {
        int volume = 0;
        for (const auto &ord : orders)
        {
            volume += ord.volume;
        }
        book.asks[price] = volume;
    }

    return book;
}

market::user::user()
{
    throw std::runtime_error("not implemented");
}

market::user::user(string name, int id, string passphase)
    : m_alias(name), m_id(id), m_passphase(passphase), m_holdings(), m_cash(0), m_is_admin(false)
{
}

market::user::user(string name, int id)
    : m_alias(name), m_id(id), m_passphase(name), m_holdings(), m_cash(0), m_is_admin(false)
{
}

market::user::user(string name, int id, bool is_admin)
    : m_alias(name), m_id(id), m_passphase(name), m_holdings(), m_cash(0), m_is_admin(is_admin)
{
}

auto market::user::add_order(const order &ord) -> void
{
    assert(!m_orders.contains(ord.id));
    assert(m_id == ord.user_id);

    logger::log(fmt::format("user {} added order {}", m_id, ord.id));

    m_orders[ord.id] = ord;
}

auto market::user::remove_order(const order &ord) -> void
{
    assert(m_orders.contains(ord.id));

    logger::log(fmt::format("user {} removed order {}", m_id, ord.id));

    m_orders.erase(ord.id);
}

auto market::user::fill_order(const order &ord, int price, int volume, side type) -> void
{
    assert(m_orders.contains(ord.id));

    logger::log(fmt::format("user {} filled a {} order {} of {} @ {}",
        m_id, side_repr[static_cast<int>(type)], ord.id, volume, price));


    if (type == side::BID)
    {
        // we've brought assets
        m_cash -= price * volume;
        m_holdings[ord.ticker_id] += volume;
    }
    else
    {
        // we've sold assets
        m_cash += price * volume;
        m_holdings[ord.ticker_id] -= volume;
    }

    // update order
    m_orders[ord.id].volume -= volume;
    assert(m_orders[ord.id].volume >= 0);
    if (m_orders[ord.id].volume == 0)
    {
        m_orders.erase(ord.id);
    }
}

auto market::user::view_order(int order_id) const -> const order &
{
    assert(m_orders.contains(order_id));

    return m_orders.at(order_id);
}

auto market::user::get_orders() const -> vector<int>
{
    vector<int> orderids;
    for (const auto &ord : m_orders)
    {
        orderids.push_back(ord.first);
    }

    return orderids;
}

auto market::user::has_order(const order &ord) -> bool
{
    return m_orders.contains(ord.id);
}

auto market::user::get_assets(const map<int, int> &valuations) const -> int
{
    int total = m_cash;
    for (const auto &key : m_holdings)
    {
        assert(valuations.contains(key.first));

        total += key.second * valuations.at(key.first);
    }

    return total;
}

auto market::user::get_holdings() const -> const map<int, int>
{
    return m_holdings;
}

auto market::user::get_cash() const -> int
{
    return m_cash;
}

auto market::user::get_admin() const -> bool
{
    return m_is_admin;
}

auto market::user::get_alias() const -> const string &
{
    return m_alias;
}

auto market::user::match(const std::string &name, const std::string &passphase) const -> bool
{
    return m_alias == name && m_passphase == passphase;
}

auto market::user::repr() const -> string
{
    string repr;

    repr += fmt::format("name {}, id {}, passphase {}\n", m_alias, m_id, m_passphase);
    repr += fmt::format("cash {}\n", m_cash);
    repr += fmt::format("holdings:\n");

    for (const auto &key : m_holdings)
    {
        repr += fmt::format("    {}: {}\n", key.first, key.second);
    }
    if (m_holdings.size() == 0)
    {
        repr += "none\n";
    }

    repr += "orders:\n";
    for (const auto &ord : m_orders)
    {
        repr += ord.second.repr() + "\n";
    }
    if (m_orders.size() == 0)
    {
        repr += "none\n";
    }

    return repr;
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

auto market::exchange::user_order(side _side, int userid, int tickerid, int price, int volume, bool ioc) -> void
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

    order neworder{ m_id.get("order"), userid, tickerid, _side, price, volume };
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

auto market::exchange::user_cancel(int userid) -> void
{
    assert(m_users.contains(userid));

    logger::log(fmt::format("cancelling all orders for user {}", userid));

    auto &user = m_users[userid];
    vector<int> ords = user.get_orders();
    for (int ord : ords)
    {
        const order &o = user.view_order(ord);

        m_tickers[o.ticker_id].cancel_order(o);
        user.remove_order(o);

        logger::log(fmt::format("cancelled order {}", o.id));
    }


}

auto market::exchange::user_cancel_ticker(int userid, int tickerid) -> void
{
    assert(m_users.contains(userid));
    assert(m_tickers.contains(tickerid));

    logger::log(fmt::format("cancelling all orders on {} for user {}", tickerid, userid));

    auto &user = m_users[userid];
    vector<int> ords = user.get_orders();
    for (int ord : ords)
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
    map<int, int> valuations;
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

auto market::exchange::get_tickers() const -> const map<int, ticker> &
{
    return m_tickers;
}

auto market::exchange::get_user(int id) const -> const user &
{
    assert(m_users.contains(id));

    return m_users.at(id);
}

auto market::exchange::get_users() const -> const map<int, user> &
{
    return m_users;
}

auto market::exchange::get_ticker(int id) const -> const ticker &
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

auto market::exchange::get_valuations() const -> map<int, int>
{
    map<int, int> valuations;
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

auto market::exchange::process_order(const order &aggressor) -> void
{
    assert(m_tickers.contains(aggressor.ticker_id));
    assert(m_users.contains(aggressor.user_id));


    vector<transaction> transactions = m_tickers[aggressor.ticker_id].match(aggressor, m_id);

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

        int ticker = trans.ticker_id;
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

market::id_system::id_system()
    : m_ids()
{
}

auto market::id_system::get(const string &type) -> int
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
    return fmt::format(
        "Transaction {}, {} aggressor between {} and {} on {} of {} @ {}, orders {} and {}",
        id,
        market::side_repr[static_cast<int>(aggressor)],
        bidder_id, asker_id,
        ticker_id,
        volume, price,
        bid_id, ask_id
    );
}

auto market::order::repr() const -> string
{
    return fmt::format(
        "Order {}, type {} by {} on {} of {} @ {}",
        id,
        market::side_repr[static_cast<int>(wish)],
        user_id,
        ticker_id,
        volume,
        price
    );
}
