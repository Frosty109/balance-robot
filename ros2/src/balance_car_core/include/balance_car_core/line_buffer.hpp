#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace balance_car::core
{

enum class LineEventKind
{
    none,
    line,
    too_long,
};

struct LineEvent
{
    LineEventKind kind {LineEventKind::none};
    std::string_view line {};
};

template <std::size_t Capacity = 256>
class LineBuffer
{
    static_assert(Capacity > 0);

public:
    // Capacity excludes LF and an optional final CR.
    // Consume a returned line before the next feed(), reset(), or destruction.
    [[nodiscard]] LineEvent feed(char byte) noexcept
    {
        if (discarding_)
        {
            if (byte == '\n')
            {
                reset();
            }
            return {};
        }

        if (byte == '\n')
        {
            std::size_t length = used_;
            if (length > 0 && storage_[length - 1] == '\r')
            {
                --length;
            }
            used_ = 0;
            return {LineEventKind::line, {storage_.data(), length}};
        }

        if (used_ < Capacity || (used_ == Capacity && byte == '\r'))
        {
            storage_[used_++] = byte;
            return {};
        }

        // Discard through LF: the suffix must never become a new message.
        used_ = 0;
        discarding_ = true;
        return {LineEventKind::too_long, {}};
    }

    void reset() noexcept
    {
        used_ = 0;
        discarding_ = false;
    }

private:
    // One spare byte lets a capacity-length payload also end in CRLF.
    std::array<char, Capacity + 1> storage_ {};
    std::size_t used_ {0};
    bool discarding_ {false};
};

}  // namespace balance_car::core