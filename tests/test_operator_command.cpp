#include "src/app/operator_command.hpp"
#include <gtest/gtest.h>

TEST(OperatorCommandTest, CleanArmByteDecodesToArm)
{
    EXPECT_EQ(decodeOperatorByte('a', false), OperatorCommand::Arm);
}

TEST(OperatorCommandTest, ErroredArmByteDecodesToDisarm)
{
    // The failsafe rule: an errored 'a' must never reach the arm flag.
    EXPECT_EQ(decodeOperatorByte('a', true), OperatorCommand::Disarm);
}

TEST(OperatorCommandTest, EveryOtherByteDecodesToDisarm)
{
    EXPECT_EQ(decodeOperatorByte('d', false), OperatorCommand::Disarm);
    EXPECT_EQ(decodeOperatorByte('A', false), OperatorCommand::Disarm);
    EXPECT_EQ(decodeOperatorByte(0x00, false), OperatorCommand::Disarm);
    EXPECT_EQ(decodeOperatorByte(0xFF, false), OperatorCommand::Disarm);
}

TEST(OperatorCommandTest, NoFlagsResolveToNone)
{
    EXPECT_EQ(resolveOperatorCommands(false, false), OperatorCommand::None);
}

TEST(OperatorCommandTest, ArmOnlyResolvesToArm)
{
    EXPECT_EQ(resolveOperatorCommands(true, false), OperatorCommand::Arm);
}

TEST(OperatorCommandTest, DisarmOnlyResolvesToDisarm)
{
    EXPECT_EQ(resolveOperatorCommands(false, true), OperatorCommand::Disarm);
}

TEST(OperatorCommandTest, DisarmBeatsArmInOneSnapshot)
{
    EXPECT_EQ(resolveOperatorCommands(true, true), OperatorCommand::Disarm);
}