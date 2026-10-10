#pragma once
#include <cstddef>
#include <cstdint>

struct MotionTarget
{
    float forward_mm_s { 0.0f };
    float yaw_rate_dps { 0.0f };
};

// CRC-8/SMBUS: poly 0x07, init 0, no reflection; check("123456789") == 0xF4
constexpr std::uint8_t crc8(const char* data, std::size_t length)
{
    std::uint8_t crc = 0;

    for (std::size_t i = 0; i < length; ++i)
    {
        crc ^= static_cast<std::uint8_t>(data[i]);
        for (int bit = 0; bit < 8; ++bit)
        {
            crc = (crc & 0x80) ? static_cast<std::uint8_t>((crc << 1) ^ 0x07)
                               : static_cast<std::uint8_t>(crc << 1);
        }
    }
    return crc;
}

struct LinkEvent
{
    enum class Kind { None, Arm, Disarm, Motion, FrameError };
    enum class Cause { None, Key, Byte, RxError, Frame }; // Set only on Disarm
    Kind kind { Kind::None };
    Cause cause { Cause::None };
    std::uint8_t byte { 0 };        // byte that caused disarm
    std::uint8_t seq  { 0 };
    MotionTarget target {};
};

constexpr const char* causeName(LinkEvent::Cause cause)
{
    switch (cause)
    {
        case LinkEvent::Cause::Key:      return "key";
        case LinkEvent::Cause::Byte:     return "byte";
        case LinkEvent::Cause::RxError:  return "rxerr";
        case LinkEvent::Cause::Frame:    return "frame";
        default:                         return "none";
    }
}

class FrameParser
{
public:
    LinkEvent feed(std::uint8_t byte, bool receive_error);

private:
    static constexpr std::size_t MAX_FRAME { 32 };
    bool in_frame_ { false };
    char buf_[MAX_FRAME] {};
    std::size_t len_ {0};

    LinkEvent finishFrame();
};

class MotionGovernor
{
public:
    struct Limits
    {
        float max_forward_mm_s;
        float max_yaw_dps;
        float forward_slew_mm_s2;
        float yaw_slew_dps2;
        std::uint32_t timeout_ms;
    };

    explicit MotionGovernor(Limits limits) : limits_(limits) {}

    bool onCommand(std::uint8_t seq, MotionTarget requested, std::uint32_t now_ms); // false = repeat/old seq
    MotionTarget step(bool armed, std::uint32_t now_ms);
    bool linkAlive(std::uint32_t now_ms) const;
    void reset();

private:
    Limits        limits_;
    MotionTarget  requested_ {};
    MotionTarget  output_ {};
    bool          have_command_ {false};
    bool          have_step_ {false};
    std::uint8_t  last_seq_ {0};
    std::uint32_t last_command_ms_ {0};
    std::uint32_t last_step_ms_ {0};
};

// mm/s -> VelocityPI move_x: summed counts of both wheels per 5.01ms sample
constexpr float MOVE_X_PER_MM_S { 2.0f * 6.27f * 0.00501f };
constexpr float toMoveX(float forward_mm_s) { return forward_mm_s * MOVE_X_PER_MM_S; }
