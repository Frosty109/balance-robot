#include "balance_car_bridge/serial_link.hpp"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdlib>
#include <string>
#include <string_view>

namespace
{

using balance_car::bridge::SerialLink;
constexpr auto leave = SerialLink::ControlLines::leave;

// A pseudo-terminal: the test writes to the master end, SerialLink reads the other end as a port.
class Pty
{
public:
    // On any failure path_ stays empty, which each test checks first.
    Pty()
    {
        master_ = ::posix_openpt(O_RDWR | O_NOCTTY);
        if (master_ < 0)
        {
            return;
        }
        if (::grantpt(master_) != 0)
        {
            return;
        }
        if (::unlockpt(master_) != 0)
        {
            return;
        }
        path_ = ::ptsname(master_);
    }
    ~Pty() { close_master(); }
    Pty(const Pty&) = delete;
    Pty& operator=(const Pty&) = delete;

    const std::string& path() const { return path_; }

    bool write(std::string_view text) const
    {
        const ssize_t written = ::write(master_, text.data(), text.size());
        return written == static_cast<ssize_t>(text.size());
    }

    void close_master()
    {
        if (master_ >= 0)
        {
            ::close(master_);
            master_ = -1;
        }
    }

private:
    int master_ {-1};
    std::string path_;
};

bool wait_readable(const SerialLink& link)
{
    constexpr nfds_t descriptor_count = 1;
    constexpr int timeout_ms = 1000;
    pollfd fds {link.native_handle(), POLLIN, 0};
    const int ready_count = ::poll(&fds, descriptor_count, timeout_ms);
    return ready_count == 1;
}

}  // namespace

TEST(SerialLinkTest, DefaultOpenSetsControlLines)
{
    Pty pty;
    ASSERT_FALSE(pty.path().empty());
    // A pty has no modem lines, so ENOTTY proves the default reaches the DTR/RTS step.
    const auto link = SerialLink::open_read_only(pty.path());
    ASSERT_FALSE(link.has_value());
    EXPECT_EQ(link.error().value(), ENOTTY);
}

TEST(SerialLinkTest, SecondOpenIsRefused)
{
    Pty pty;
    ASSERT_FALSE(pty.path().empty());
    const auto first = SerialLink::open_read_only(pty.path(), leave);
    ASSERT_TRUE(first.has_value()) << first.error().message();
    const auto second = SerialLink::open_read_only(pty.path(), leave);
    ASSERT_FALSE(second.has_value());
    EXPECT_EQ(second.error().value(), EBUSY);
}

TEST(SerialLinkTest, ReadsArrivingBytesAndReturnsZeroWhenIdle)
{
    Pty pty;
    ASSERT_FALSE(pty.path().empty());
    auto link = SerialLink::open_read_only(pty.path(), leave);
    ASSERT_TRUE(link.has_value()) << link.error().message();

    std::array<char, 64> buffer {};
    const auto idle = link->read_some(buffer);
    ASSERT_TRUE(idle.has_value());
    EXPECT_EQ(*idle, 0u);

    ASSERT_TRUE(pty.write("RANGE seq=1\n"));
    ASSERT_TRUE(wait_readable(*link));
    const auto got = link->read_some(buffer);
    ASSERT_TRUE(got.has_value()) << got.error().message();
    EXPECT_EQ(std::string_view(buffer.data(), *got), "RANGE seq=1\n");
}

