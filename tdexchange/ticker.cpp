#include "ticker.h"
#include "logger.h"

#include <fmt/core.h>
#include <cassert>
#include <string>
#include <set>

market::ticker::ticker()
{
    throw std::runtime_error("not implemented");
}

market::ticker::ticker(string name, ids::ticker_id id)
    : m_alias(name), m_id(id), m_asks(), m_bids(), m_valuation(0)
{
}

auto market::ticker::match(const order &aggressor, id_system<ids::transaction_id> &id) const->vector<transaction>
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

auto market::ticker::get_id() const -> ids::ticker_id
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