#pragma once

#include <cstdint>

enum class OperatorCommand { None, Arm, Disarm };

constexpr OperatorCommand decodeOperatorByte(std::uint8_t byte, bool receive_error)
{
    return !receive_error && byte == 'a'
         ? OperatorCommand::Arm
         : OperatorCommand::Disarm;
}

constexpr OperatorCommand resolveOperatorCommands(bool arm, bool disarm)
{
    if (disarm) { return OperatorCommand::Disarm; }
    if (arm)    { return OperatorCommand::Arm; }
    return OperatorCommand::None;
}