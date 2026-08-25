#pragma once

#include <cstdint>

class Tim2MillisecondTimer
{
public:
    void init();
    std::uint32_t nowMs() const;
};
