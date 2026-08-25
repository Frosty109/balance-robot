#include "sim_monotonic_clock.hpp"

std::uint32_t SimMonotonicClock::nowMs() const
{
    return now_ms_;
}

void SimMonotonicClock::advance(std::uint32_t elapsed_ms)
{
    now_ms_ += elapsed_ms;
}
