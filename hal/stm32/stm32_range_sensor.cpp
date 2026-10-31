#include "stm32_range_sensor.hpp"

namespace
{
    // Nominal HC-SR04 limits
    constexpr std::uint32_t MIN_DISTANCE_MM {20};
    constexpr std::uint32_t MAX_DISTANCE_MM {4000};
}

Stm32RangeSensor::Stm32RangeSensor(IMonotonicClock& clock)
    : clock_(clock)
{
}

void Stm32RangeSensor::init()
{
    if (initialised_)
    {
        return;
    }

    sensor_.init();
    initialised_ = true;
}

bool Stm32RangeSensor::poll()
{
    if (!initialised_)
    {
        return false;
    }

    bool published = false;

    if (awaiting_result_)
    {
        EchoCapture capture {};
        if (sensor_.poll(clock_.nowMs(), capture))
        {
            RangeSample sample {};
            sample.sequence = latest_.sequence + 1u;
            sample.started_ms = started_ms_;
            sample.completed_ms =
                started_ms_ + capture.completed_after_us / 1000u;
            sample.status = RangeStatus::capture_error;

            switch (capture.status)
            {
            case EchoStatus::captured:
            {
                // Round-trip conversion at nominal 340 m/s, matching Yahboom.
                const std::uint32_t scaled_mm = capture.pulse_us * 170u;

                if (scaled_mm >= MIN_DISTANCE_MM * 1000u &&
                    scaled_mm <= MAX_DISTANCE_MM * 1000u)
                {
                    sample.distance_mm = scaled_mm / 1000u;
                    sample.status = RangeStatus::valid;
                }
                else
                {
                    sample.status = RangeStatus::out_of_range;
                }
                break;
            }

            case EchoStatus::no_echo:
                sample.status = RangeStatus::no_echo;
                break;

            case EchoStatus::timeout:
                sample.status = RangeStatus::timeout;
                break;

            case EchoStatus::capture_error:
                break;
            }

            latest_ = sample;
            awaiting_result_ = false;
            published = true;
        }
    }

    if (!awaiting_result_)
    {
        const std::uint32_t now_ms = clock_.nowMs();
        if (sensor_.start(now_ms))
        {
            started_ms_ = now_ms;
            awaiting_result_ = true;
        }
    }

    return published;
}

RangeSample Stm32RangeSensor::latest() const
{
    return latest_;
}
