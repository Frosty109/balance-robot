#pragma once

#include <cstdint>

class IMonotonicClock
{
public:
    virtual ~IMonotonicClock() = default;
    virtual std::uint32_t nowMs() const = 0;
};
