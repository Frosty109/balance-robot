#include "motion_link.hpp"
#include <algorithm>

namespace
{
    constexpr std::uint32_t MAX_STEP_DT_MS { 50 };   // a stalled loop must not release a full jump

    bool inFrameAlphabet(std::uint8_t b)
    {
        return b == 'M' || b == ',' || b == '-' || b == '*' || b == '\r'
            || (b >= '0' && b <= '9') || (b >= 'A' && b <= 'F');
    }

    // Parses a signed decimal from [p, end) and advances p; false on no digits or overflow.
    bool parseInt(const char*& p, const char* end, long& out)
    {
        bool negative = false;
        if (p < end && *p == '-') { negative = true; ++p; }
        if (p >= end || *p < '0' || *p > '9') { return false; }

        long value = 0;
        while (p < end && *p >= '0' && *p <= '9')
        {
            value = value * 10 + (*p - '0');
            if (value > 100000) { return false; }
            ++p;
        }
        out = negative ? -value : value;
        return true;
    }

    int hexValue(char c)
    {
        if (c >= '0' && c <= '9') { return c - '0'; }
        if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
        return -1;
    }

    float slewToward(float current, float target, float max_step)
    {
        return current + std::clamp(target - current, -max_step, max_step);
    }

    LinkEvent disarm(LinkEvent::Cause cause, std::uint8_t byte)
    {
        LinkEvent event {LinkEvent::Kind::Disarm};
        event.cause = cause;
        event.byte  = byte;
        return event;
    }
}

LinkEvent FrameParser::feed(std::uint8_t byte, bool receive_error)
{
    using Cause = LinkEvent::Cause;

    if (!in_frame_)
    {
        if (receive_error)  { return disarm(Cause::RxError, byte); }
        if (byte == '$')    { in_frame_ = true; len_ = 0; return {}; }
        if (byte == 'a')    { return {LinkEvent::Kind::Arm}; }
        return disarm(byte == ' ' ? Cause::Key : Cause::Byte, byte);
    }

    if (receive_error)            { in_frame_ = false; return disarm(Cause::RxError, byte); }
    if (byte == '$')              { len_ = 0; return {}; }
    if (byte == '\n')             { in_frame_ = false; return finishFrame(); }
    if (!inFrameAlphabet(byte))   { in_frame_ = false; return disarm(Cause::Frame, byte); }
    if (byte == '\r')             { return {}; }
    if (len_ >= MAX_FRAME)        { in_frame_ = false; return {LinkEvent::Kind::FrameError}; }

    buf_[len_++] = static_cast<char>(byte);
    return {};
}

LinkEvent FrameParser::finishFrame()
{
    const LinkEvent error {LinkEvent::Kind::FrameError};

    // Expect "M,<seq>,<fwd>,<yaw>*HH"
    const char* star = nullptr;
    for (std::size_t i = 0; i < len_; ++i)
    {
        if (buf_[i] == '*') { star = &buf_[i]; break; }
    }
    if (star == nullptr || (buf_ + len_) - star != 3)
    {
        return error;
    }

    const int hi = hexValue(star[1]);
    const int lo = hexValue(star[2]);
    if (hi < 0 || lo < 0) { return error; }
    if (crc8(buf_, static_cast<std::size_t>(star - buf_)) != static_cast<std::uint8_t>(hi * 16 + lo))
    {
        return error;
    }

    const char* p = buf_;
    if (p >= star || *p++ != 'M') { return error; }
    if (p >= star || *p++ != ',') { return error; }

    long seq = 0, fwd = 0, yaw = 0;
    if (!parseInt(p, star, seq) || seq < 0 || seq > 255) { return error; }
    if (p >= star || *p++ != ',')                        { return error; }
    if (!parseInt(p, star, fwd))                         { return error; }
    if (p >= star || *p++ != ',')                        { return error; }
    if (!parseInt(p, star, yaw))                         { return error; }
    if (p != star)                                       { return error; }

    LinkEvent event {LinkEvent::Kind::Motion};
    event.seq = static_cast<std::uint8_t>(seq);
    event.target.forward_mm_s = static_cast<float>(fwd);
    event.target.yaw_rate_dps = static_cast<float>(yaw) / 10.0f;
    return event;
}

bool MotionGovernor::onCommand(std::uint8_t seq, MotionTarget requested, std::uint32_t now_ms)
{
    if (have_command_)
    {
        const auto ahead = static_cast<std::uint8_t>(seq - last_seq_);
        if (ahead == 0 || ahead >= 128) { return false; }   // duplicate, or behind in the wrapped space
    }
    requested_       = requested;
    last_seq_        = seq;
    last_command_ms_ = now_ms;
    have_command_    = true;
    return true;
}

MotionTarget MotionGovernor::step(bool armed, std::uint32_t now_ms)
{
    if (!armed) { reset(); return {}; }

    const std::uint32_t dt_ms = have_step_ ? std::min(now_ms - last_step_ms_, MAX_STEP_DT_MS) : 0;
    last_step_ms_ = now_ms;
    have_step_    = true;
    const float dt_s = static_cast<float>(dt_ms) / 1000.0f;

    MotionTarget target {};
    if (linkAlive(now_ms))
    {
        target.forward_mm_s = std::clamp(requested_.forward_mm_s, -limits_.max_forward_mm_s, limits_.max_forward_mm_s);
        target.yaw_rate_dps = std::clamp(requested_.yaw_rate_dps, -limits_.max_yaw_dps, limits_.max_yaw_dps);
    }

    output_.forward_mm_s = slewToward(output_.forward_mm_s, target.forward_mm_s, limits_.forward_slew_mm_s2 * dt_s);
    output_.yaw_rate_dps = slewToward(output_.yaw_rate_dps, target.yaw_rate_dps, limits_.yaw_slew_dps2 * dt_s);
    return output_;
}

bool MotionGovernor::linkAlive(std::uint32_t now_ms) const
{
    return have_command_ && (now_ms - last_command_ms_ < limits_.timeout_ms);
}

void MotionGovernor::reset()
{
    requested_    = {};
    output_       = {};
    have_command_ = false;
    have_step_    = false;
}
