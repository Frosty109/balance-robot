#include "balance_car_bridge/serial_link.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <utility>

namespace balance_car::bridge
{

namespace
{

std::error_code last_error() noexcept
{
    return {errno, std::system_category()};
}

}  // namespace

std::expected<SerialLink, std::error_code>
SerialLink::open_read_only(const std::string& path, ControlLines lines)
{
    const int fd = ::open(path.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
    {
        return std::unexpected(last_error());
    }

    // From here the destructor closes fd on every early return.
    SerialLink link {fd};

    // Refuses a second bridge; drive.py and stm32flash do not flock, so this alone cannot stop them.
    if (::flock(fd, LOCK_EX | LOCK_NB) != 0)
    {
        return std::unexpected(last_error());
    }

    // Kernel-enforced: a later open of this port by drive.py or stm32flash fails with EBUSY.
    if (::ioctl(fd, TIOCEXCL) != 0)
    {
        return std::unexpected(last_error());
    }

    termios attrs {};
    if (::tcgetattr(fd, &attrs) != 0)
    {
        return std::unexpected(last_error());
    }

    // Raw 8N1 exactly as drive.py sets it; HUPCL stays clear so closing the port leaves DTR and RTS alone.
    attrs.c_iflag = 0;
    attrs.c_oflag = 0;
    attrs.c_cflag = CS8 | CREAD | CLOCAL;
    attrs.c_lflag = 0;

    if (::cfsetispeed(&attrs, B115200) != 0)
    {
        return std::unexpected(last_error());
    }
    if (::cfsetospeed(&attrs, B115200) != 0)
    {
        return std::unexpected(last_error());
    }
    if (::tcsetattr(fd, TCSANOW, &attrs) != 0)
    {
        return std::unexpected(last_error());
    }

    // Discard bytes that arrived before the bridge owned the port; they may be a partial line.
    if (::tcflush(fd, TCIFLUSH) != 0)
    {
        return std::unexpected(last_error());
    }

    if (lines == ControlLines::set_run_levels)
    {
        // Linux raises DTR and RTS on open; move both to the run levels in one call, as drive.py does.
        int levels = 0;
        if (::ioctl(fd, TIOCMGET, &levels) != 0)
        {
            return std::unexpected(last_error());
        }
        levels |= TIOCM_DTR;
        levels &= ~TIOCM_RTS;
        if (::ioctl(fd, TIOCMSET, &levels) != 0)
        {
            return std::unexpected(last_error());
        }
    }

    return link;
}

std::expected<std::size_t, std::error_code>
SerialLink::read_some(std::span<char> buffer) noexcept
{
    // A zero-length read returns 0, which would look like the hang-up case below.
    if (buffer.empty())
    {
        return 0;
    }

    while (true)
    {
        const ssize_t n = ::read(fd_, buffer.data(), buffer.size());

        if (n > 0)
        {
            return static_cast<std::size_t>(n);
        }

        if (n == 0)
        {
            // End of file on a tty means the device hung up, eg. the USB adapter was unplugged.
            return std::unexpected(std::make_error_code(std::errc::connection_aborted));
        }

        if (errno == EINTR)
        {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return 0;
        }

        return std::unexpected(last_error());
    }
}

SerialLink::SerialLink(SerialLink&& other) noexcept
    : fd_ {std::exchange(other.fd_, -1)}
{
}

SerialLink& SerialLink::operator=(SerialLink&& other) noexcept
{
    if (this != &other)
    {
        close();
        fd_ = std::exchange(other.fd_, -1);
    }
    return *this;
}

SerialLink::~SerialLink()
{
    close();
}

void SerialLink::close() noexcept
{
    if (fd_ >= 0)
    {
        // Closing the descriptor also releases the flock taken in open_read_only.
        ::close(fd_);
        fd_ = -1;
    }
}

}  // namespace balance_car::bridge
