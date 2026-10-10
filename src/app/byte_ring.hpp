#pragma once
#include <cstdint>

// SPSC byte ring - the ISR writes head_, the main loop writes tail_

template <std::uint16_t N>
class ByteRing
{
    static_assert((N & (N - 1)) == 0, "N must be a power of two");
public:
    static constexpr std::uint16_t ERROR_BIT = 0x100;

    bool push(std::uint16_t entry)          // ISR  side; false = full
    {
        const std::uint16_t next = (head_ + 1) & (N - 1);
        if (next == tail_) { return false; }
        buf_[head_] = entry;
        head_ = next;
        return true;
    }

    bool pop(std::uint16_t& entry)          // main-loop side; false = empty
    {
        if (tail_ == head_) { return false; }
        entry = buf_[tail_];
        tail_ = (tail_ + 1) & (N - 1);
        return true;
    }

    void clear() { tail_ = head_; }        // call with RX interrupt masked

private:
    volatile std::uint16_t buf_[N] {};
    volatile std::uint16_t head_ {0};
    volatile std::uint16_t tail_ {0};
};
