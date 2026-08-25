#pragma once

#include "../../hal/interface/i_monotonic_clock.hpp"
#include "../../bsp/clock/tim2_millisecond_timer.hpp"

class Stm32MonotonicClock final : public IMonotonicClock
{
public:
    void init();
    std::uint32_t nowMs() const override;

private:
    Tim2MillisecondTimer timer_;
};
