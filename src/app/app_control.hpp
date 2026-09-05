#pragma once

#include <cstdint>

#include "../../hal/interface/i_sensor_hal.hpp"
#include "../../hal/interface/i_motor_hal.hpp"
#include "../../hal/interface/i_monotonic_clock.hpp"
#include "../pid/pid_control.hpp"

class AppControl
{
public:
    AppControl(ISensorHal& sensor,
               IMotorHal& motor,
               IMonotonicClock& clock,
               BalancePD balance,
               VelocityPI velocity,
               TurnPD turn);

    void update(float move_x = 0.0f, float move_z = 0.0f);
    void reset();

private:
    ISensorHal& sensor_;
    IMotorHal&  motor_;
    IMonotonicClock& clock_;
    BalancePD   balance_;
    VelocityPI  velocity_;
    TurnPD      turn_;

    bool          faulted_ {false};
    int           telemetry_tick_ {0};
    std::uint32_t max_poll_ms_ {0};

    // Stale-data shutdown
    std::uint32_t last_fresh_ms_ {0};
    int           consecutive_fresh_ {0};
    bool          stale_ {false};

    static constexpr int TELEMETRY_DECIMATION {20};
    static constexpr std::uint32_t STALE_TIMEOUT_MS {25};
    static constexpr int RECOVERY_FRESH_SAMPLES {3};
};
