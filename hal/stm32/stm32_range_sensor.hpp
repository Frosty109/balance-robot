#pragma once

#include "../interface/i_range_sensor.hpp"
#include "../interface/i_monotonic_clock.hpp"
#include "../../bsp/ultrasonic/ultrasonic.hpp"

class Stm32RangeSensor final : public IRangeSensor
{
public:
    explicit Stm32RangeSensor(IMonotonicClock& clock);

    void init();

    bool poll() override;
    RangeSample latest() const override;

private:
    IMonotonicClock& clock_;
    Ultrasonic sensor_;

    RangeSample latest_ {};
    std::uint32_t started_ms_ {0};
    bool awaiting_result_ {false};
    bool initialised_ {false};
};