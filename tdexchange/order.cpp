#include "order.h"

#include <fmt/core.h>
#include "side.h"

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