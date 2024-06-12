#include "user.h"

#include <cassert>
#include <stdexcept>
#include <fmt/core.h>
#include "logger.h"

market::user::user()
{
    throw std::runtime_error("not implemented");
}

market::user::user(string name, ids::user_id id, string passphase)
    : m_alias(name), m_id(id), m_passphase(passphase), m_holdings(), m_cash(0), m_is_admin(false)
{
}

market::user::user(string name, ids::user_id id)
    : m_alias(name), m_id(id), m_passphase(name), m_holdings(), m_cash(0), m_is_admin(false)
{
}

market::user::user(string name, ids::user_id id, bool is_admin)
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

auto market::user::view_order(ids::order_id order_id) const -> const order &
{
    assert(m_orders.contains(order_id));

    return m_orders.at(order_id);
}

auto market::user::get_orders() const -> vector<ids::order_id>
{
    vector<ids::order_id> orderids;
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

auto market::user::get_assets(const map<ids::ticker_id, int> &valuations) const -> int
{
    int total = m_cash;
    for (const auto &key : m_holdings)
    {
        assert(valuations.contains(key.first));

        total += key.second * valuations.at(key.first);
    }

    return total;
}

auto market::user::get_holdings() const -> const map<ids::ticker_id, int>
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