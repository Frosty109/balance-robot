#pragma once

#include <cstdint>

enum class OperatorCommand { None, Arm, Disarm };

constexpr OperatorCommand decodeOperatorByte(std::uint8_t byte, bool receive_error)
{
    if (receive_error) { return OperatorCommand::Disarm; }
    if (byte == 'a')   { return OperatorCommand::Arm; }
    return OperatorCommand::Disarm;             // any other byte disarms
}

constexpr OperatorCommand resolveOperatorCommands(bool arm, bool disarm)
{
    if (disarm) { return OperatorCommand::Disarm; }
    if (arm)    { return OperatorCommand::Arm; }
    return OperatorCommand::None;
}