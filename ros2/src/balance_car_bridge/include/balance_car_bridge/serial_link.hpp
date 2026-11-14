#pragma once

#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <system_error>

namespace balance_car::bridge
{

// Exclusive, nonblocking owner of the car's serial port
class SerialLink
{
public:
    // Pseudo-terminals in tests have no modem lines, so they need leave.
    enum class ControlLines
    {
        set_run_levels,
        leave
    };

    // Read-only by construction: nothing in this class can send bytes to the car.
    [[nodiscard]] static std::expected<SerialLink, std::error_code>
    open_read_only(const std::string& path, ControlLines lines = ControlLines::set_run_levels);

    SerialLink(SerialLink&& other) noexcept;
    SerialLink& operator=(SerialLink&& other) noexcept;
    SerialLink(const SerialLink&) = delete;
    SerialLink& operator=(const SerialLink&) = delete;
    ~SerialLink();

    // Bytes read, or 0 when nothing is waiting; an error if the port failed or hung up.
    [[nodiscard]] std::expected<std::size_t, std::error_code>
    read_some(std::span<char> buffer) noexcept;

    [[nodiscard]] int native_handle() const noexcept { return fd_; }

private:
    explicit SerialLink(int fd) noexcept : fd_ {fd} {}
    void close() noexcept;

    int fd_ {-1};
};

}  // namespace balance_car::bridge
