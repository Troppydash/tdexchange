#include "transaction.h"

#include <fmt/core.h>

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