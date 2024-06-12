#pragma once

#include <string>
#include <map>

namespace ids
{
using transaction_id = unsigned long long;
using order_id = unsigned long long;
using ticker_id = unsigned long long;
using user_id = int;

};

// id singleton generator
template <typename T>
class id_system
{
protected:
    // next id containers
    std::map<std::string, T> m_ids;


public:
    id_system()
        : m_ids()
    {}

    // get an unique id for an container
    auto get(const std::string &type) -> T
    {
        if (!m_ids.contains(type))
        {
            m_ids[type] = 0;
        }

        m_ids[type] += 1;
        return m_ids[type];
    }
};
