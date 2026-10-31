#pragma once

#include <cstdint>

enum class RangeStatus
{
    starting,
    valid,
    no_echo,
    timeout,
    out_of_range,
    capture_error,
};

struct RangeSample
{
    std::uint32_t sequence {0};
    std::uint32_t started_ms {0};
    std::uint32_t completed_ms {0};
    std::uint32_t distance_mm {0};
    RangeStatus status {RangeStatus::starting};
};

class IRangeSensor
{
public:
    virtual ~IRangeSensor() = default;

    // Services acquisition without waiting for an echo
    // True means a new result was published, including failed attempts

    virtual bool poll() = 0;

    virtual RangeSample latest() const = 0;
};