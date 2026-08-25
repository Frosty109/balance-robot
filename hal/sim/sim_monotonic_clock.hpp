#pragma once

#include "../../hal/interface/i_monotonic_clock.hpp"

class SimMonotonicClock : public IMonotonicClock
{
public:
    std::uint32_t nowMs() const override;
    void advance(std::uint32_t elapsed_ms);

private:
    std::uint32_t now_ms_ {0};
};
