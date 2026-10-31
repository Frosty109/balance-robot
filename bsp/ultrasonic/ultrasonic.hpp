#pragma once

#include <cstdint>

enum class EchoStatus
{
    captured,
    no_echo,
    timeout,
    capture_error
};

struct EchoCapture
{
    std::uint32_t pulse_us {0};

    // Conservative completion offset - zero if timing is unknown. 
    std::uint32_t completed_after_us {0};

    EchoStatus status {EchoStatus::capture_error};
};

class Ultrasonic
{
public:
    Ultrasonic() = default;

    Ultrasonic(const Ultrasonic&) = delete;
    Ultrasonic& operator=(const Ultrasonic&) = delete;
    Ultrasonic(Ultrasonic&&) = delete;
    Ultrasonic& operator=(Ultrasonic&&) = delete;

    void init();

    // Main-loop only. False if uninitialised, not the active owner,
    // busy (including an unread result), or within the trigger interval.
    bool start(std::uint32_t now_ms);

    // Returns a completed attempt once, without waiting for an echo.
    bool poll(std::uint32_t now_ms, EchoCapture& result);

    void handleInterrupt();

private:
    enum class State
    {
        idle,
        waiting_rise,
        waiting_fall,
        complete
    };

    static constexpr std::uint32_t CAPTURE_TIMEOUT_US {30000};
    static constexpr std::uint32_t MIN_TRIGGER_INTERVAL_MS {100};
    volatile State state_ {State::idle};
    volatile std::uint16_t rise_ticks_ {0};
    volatile std::uint32_t pulse_us_ {0};
    volatile std::uint32_t completed_after_us_ {0};
    volatile EchoStatus status_ {EchoStatus::capture_error};

    std::uint32_t started_ms_ {0};
    std::uint32_t last_started_ms_ {0};
    bool has_started_ {false};
    bool initialised_ {false};

    void finishCapture(EchoStatus status, std::uint32_t pulse_us);
};

