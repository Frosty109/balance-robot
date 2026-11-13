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
    // Read-only by constructionthe car.
    [[nodiscard]] static std::expected<SerialLink, std::error_code>
    open_read_only(const std::string& path);

    SerialLink(SerialLink&& other) noexcept;
    SerialLink& operator=(SerialLink&& other) noexcept;
    SerialLink(const SerialLink&) = delete;
    SerialLink& operator=(const SerialLink&) = delete;
    ~SerialLink();

    // Bytes read, or 0 when nothing is waiting
    [[nodiscard]] std::expected<std::size_t, std::error_code>
    read_some(std::span<char> buffer) noexcept;

    [[nodiscard]] int native_handle() const noexcept { return fd_; }

private:
    explicit SerialLink(int fd) noexcept : fd_ {fd} {}
    void close() noexcept;

    int fd_ {-1};
};

}  // namespace balance_car::bridge
