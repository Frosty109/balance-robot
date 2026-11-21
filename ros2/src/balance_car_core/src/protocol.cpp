#include "balance_car_core/protocol.hpp"

#include <format>

namespace balance_car::core
{

std::uint8_t crc8(std::string_view text)
{
    constexpr std::uint8_t polynomial {0x07};
    constexpr std::uint8_t top_bit {0x80};

    std::uint8_t crc {0};
    for (const char ch : text)
    {
        crc ^= static_cast<std::uint8_t>(ch);
        for (int bit = 0; bit < 8; ++bit)
        {
            const bool top_bit_set = (crc & top_bit) != 0;
            crc = static_cast<std::uint8_t>(crc << 1);
            if (top_bit_set)
            {
                crc ^= polynomial;
            }
        }
    }
    return crc;
}

std::string encode_motion(std::uint8_t sequence,
                          std::int32_t forward_mm_s,
                          std::int32_t yaw_tenths_dps)
{
    const std::string body = std::format("M,{},{},{}", sequence, forward_mm_s, yaw_tenths_dps);
    return std::format("${}*{:02X}\n", body, crc8(body));
}

}  // namespace balance_car::core