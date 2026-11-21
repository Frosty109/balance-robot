#include "balance_car_bridge/range_mapping.hpp"
#include "balance_car_bridge/serial_link.hpp"

#include <balance_car_core/health_telemetry.hpp>
#include <balance_car_core/line_buffer.hpp>
#include <balance_car_interfaces/msg/range_reading.hpp>
#include <builtin_interfaces/msg/time.hpp>
#include <rclcpp/rclcpp.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace balance_car::bridge
{

namespace
{

constexpr std::chrono::milliseconds poll_period {10};
constexpr std::size_t max_line_length {256};

// SerialLink has no empty state, so a port that will not open stops the node before it starts.
SerialLink open_link(const std::string& port)
{
    auto link = SerialLink::open_read_only(port);
    if (!link)
    {
        throw std::runtime_error {"cannot open " + port + ": " + link.error().message()};
    }
    return std::move(*link);
}

}  // namespace

// Read-only bridge: car telemetry in over serial, /car/range out; it never writes to the car.
class RangeBridgeNode : public rclcpp::Node
{
public:
    RangeBridgeNode()
        : rclcpp::Node {"range_bridge"},
        port_ {declare_parameter<std::string>("port", "/dev/ttyUSB0")},
        frame_id_ {declare_parameter<std::string>("frame_id", "ultrasonic_link")},
        link_ {open_link(port_)},
        publisher_ {create_publisher<balance_car_interfaces::msg::RangeReading>(
            "/car/range", rclcpp::SensorDataQoS())},
        timer_ {create_wall_timer(poll_period, [this] { poll_serial(); })}
    {
        RCLCPP_INFO(get_logger(), "reading %s read-only; publishing /car/range in frame %s",
                    port_.c_str(), frame_id_.c_str());
    }

private:
    void poll_serial()
    {
        // One receipt stamp per timer tick. Every line in this batch arrived within the last 10 ms.
        const builtin_interfaces::msg::Time stamp = now();
        while (true)
        {
            const auto count = link_.read_some(read_buffer_);
            if (!count)
            {
                const std::string detail = count.error().message();
                const std::string reason = "Serial read failed on " + port_ + ": " + detail;
                // Propagates out of spin() to main, which logs it and exits with status 1.
                throw std::runtime_error {reason};
            }
            if (*count == 0)
            {
                return;
            }
            for (const char byte : std::span {read_buffer_}.first(*count))
            {
                handle(lines_.feed(byte), stamp);
            }
        }
    }

    void handle(const core::LineEvent& event, const builtin_interfaces::msg::Time& stamp)
    {
        if (event.kind == core::LineEventKind::too_long)
        {
            RCLCPP_WARN(get_logger(), "discarded a serial line longer than %zu characters",
                        max_line_length);
            return;
        }
        if (event.kind != core::LineEventKind::line)
        {
            return;
        }

        const std::string_view line = event.line;
        // printf's %.*s takes the length as an int.
        const int line_length = static_cast<int>(line.size());
        const auto message = range_message(line, stamp, frame_id_);
        if (message)
        {
            RCLCPP_DEBUG(get_logger(), "%.*s", line_length, line.data());
            publisher_->publish(*message);
        }
        else if (message.error() != core::ParseError::wrong_line_type)
        {
            RCLCPP_WARN(get_logger(), "bad RANGE line: %.*s", line_length, line.data());
        }
        else if (core::parse_health(line))
        {
            // Health rows arrive at 10 Hz, so they stay out of the default log level.
            RCLCPP_DEBUG(get_logger(), "%.*s", line_length, line.data());
        }
        else
        {
            RCLCPP_INFO(get_logger(), "%.*s", line_length, line.data());
        }
    }

    std::string port_;
    std::string frame_id_;
    SerialLink link_;
    core::LineBuffer<max_line_length> lines_;
    std::array<char, 512> read_buffer_ {};
    rclcpp::Publisher<balance_car_interfaces::msg::RangeReading>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace balance_car::bridge

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    int status = 0;
    try
    {
        rclcpp::spin(std::make_shared<balance_car::bridge::RangeBridgeNode>());
    }
    catch (const std::exception& error)
    {
        RCLCPP_FATAL(rclcpp::get_logger("range_bridge"), "%s", error.what());
        status = 1;
    }
    rclcpp::shutdown();
    return status;
}
