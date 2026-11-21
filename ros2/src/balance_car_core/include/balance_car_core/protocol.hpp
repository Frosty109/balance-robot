// Encodes the $M motion frames that the STM32 parses.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace balance_car::core
{

// CRC-8/SMBUS: poly 0x07, init 0, no reflection; check("123456789") == 0xF4.
[[nodiscard]] std::uint8_t crc8(std::string_view text);

// One frame, "$M,<seq>,<forward_mm_s>,<yaw_tenths_dps>*HH\n"; the caller clamps the values.
[[nodiscard]] std::string encode_motion(std::uint8_t sequence,
                                        std::int32_t forward_mm_s,
                                        std::int32_t yaw_tenths_dps);

}  // namespace balance_car::core