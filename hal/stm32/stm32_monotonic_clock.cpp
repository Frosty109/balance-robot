#include "stm32_monotonic_clock.hpp"

void Stm32MonotonicClock::init()
{
    timer_.init();
}

std::uint32_t Stm32MonotonicClock::nowMs() const
{
    return timer_.nowMs();
}
