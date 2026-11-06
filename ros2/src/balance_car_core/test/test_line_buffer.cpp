#include "balance_car_core/line_buffer.hpp"
#include "balance_car_core/telemetry.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

using namespace balance_car::core;

namespace
{

struct Collected
{
    std::vector<std::string> lines;
    std::size_t overflows {0};
};

template <std::size_t Capacity>
void collect(LineBuffer<Capacity>& buffer, std::string_view bytes, Collected& out)
{
    for (const char byte : bytes)
    {
        const auto event = buffer.feed(byte);
        if (event.kind == LineEventKind::line)
        {
            // Copy now: the next byte may overwrite the borrowed view.
            out.lines.emplace_back(event.line);
        }
        else if (event.kind == LineEventKind::too_long)
        {
            ++out.overflows;
        }
    }
}

}  // namespace

TEST(LineBufferTest, BuffersUntilNewline)
{
    LineBuffer<> buffer;
    Collected out;
    collect(buffer, "BO", out);
    EXPECT_TRUE(out.lines.empty());
    collect(buffer, "OT", out);
    EXPECT_TRUE(out.lines.empty());
    collect(buffer, "\n", out);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"BOOT"}));
    EXPECT_EQ(out.overflows, 0u);
}

TEST(LineBufferTest, EmitsSeveralLinesFromOneChunk)
{
    LineBuffer<> buffer;
    Collected out;
    collect(buffer, "BOOT\nIMU OK\nMOTOR OK\n", out);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"BOOT", "IMU OK", "MOTOR OK"}));
    EXPECT_EQ(out.overflows, 0u);
}

TEST(LineBufferTest, HandlesCrLfSplitAcrossChunks)
{
    LineBuffer<> buffer;
    Collected out;
    collect(buffer, "BOOT\r", out);
    EXPECT_TRUE(out.lines.empty());
    collect(buffer, "\nIMU OK\r\n", out);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"BOOT", "IMU OK"}));
    EXPECT_EQ(out.overflows, 0u);
}

TEST(LineBufferTest, EmitsEmptyLines)
{
    LineBuffer<> buffer;
    Collected out;
    collect(buffer, "\n\r\n", out);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"", ""}));
    EXPECT_EQ(out.overflows, 0u);
}

TEST(LineBufferTest, AcceptsExactlyCapacityWithLf)
{
    LineBuffer<4> buffer;
    Collected out;
    collect(buffer, "ABCD\n", out);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"ABCD"}));
    EXPECT_EQ(out.overflows, 0u);
}

TEST(LineBufferTest, AcceptsExactlyCapacityWithCrLf)
{
    LineBuffer<4> buffer;
    Collected out;
    collect(buffer, "ABCD\r", out);
    EXPECT_TRUE(out.lines.empty());
    EXPECT_EQ(out.overflows, 0u);
    collect(buffer, "\n", out);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"ABCD"}));
    EXPECT_EQ(out.overflows, 0u);
}

TEST(LineBufferTest, ReportsOverflowOncePerLine)
{
    LineBuffer<4> buffer;
    Collected out;
    collect(buffer, "ABCDE", out);
    EXPECT_EQ(out.overflows, 1u);
    EXPECT_TRUE(out.lines.empty());
    collect(buffer, "FGHIJKLMNOP\n", out);
    EXPECT_EQ(out.overflows, 1u);
    EXPECT_TRUE(out.lines.empty());
}

TEST(LineBufferTest, DiscardsSuffixOfOverlongLine)
{
    LineBuffer<4> buffer;
    Collected out;
    collect(buffer, "ABCDEBOOT\nOK\n", out);
    EXPECT_EQ(out.overflows, 1u);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"OK"}));
}

TEST(LineBufferTest, ResynchronisesAcrossChunks)
{
    LineBuffer<4> buffer;
    Collected out;
    collect(buffer, "ABCDE", out);
    collect(buffer, "BOOT", out);
    EXPECT_TRUE(out.lines.empty());
    collect(buffer, "\r\nHI\n", out);
    EXPECT_EQ(out.overflows, 1u);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"HI"}));
}

TEST(LineBufferTest, RejectsExtraPayloadAfterCapacityAndCr)
{
    LineBuffer<4> buffer;
    Collected out;
    collect(buffer, "ABCD\rX\nOK\n", out);
    EXPECT_EQ(out.overflows, 1u);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"OK"}));
}

TEST(LineBufferTest, PreservesEmbeddedCarriageReturn)
{
    LineBuffer<4> buffer;
    Collected out;
    collect(buffer, "a\rb\n", out);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"a\rb"}));
    EXPECT_EQ(out.overflows, 0u);
}

TEST(LineBufferTest, ResetDiscardsPartialLine)
{
    LineBuffer<4> buffer;
    Collected out;
    collect(buffer, "BO", out);
    buffer.reset();
    collect(buffer, "OK\n", out);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"OK"}));
    EXPECT_EQ(out.overflows, 0u);
}

TEST(LineBufferTest, ResetClearsOverflow)
{
    LineBuffer<4> buffer;
    Collected out;
    collect(buffer, "ABCDE", out);
    buffer.reset();
    collect(buffer, "OK\n", out);
    EXPECT_EQ(out.overflows, 1u);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"OK"}));
}

TEST(LineBufferTest, ResetDiscardsPendingCr)
{
    LineBuffer<4> buffer;
    Collected out;
    collect(buffer, "ABCD\r", out);
    buffer.reset();
    collect(buffer, "OK\n", out);
    EXPECT_EQ(out.overflows, 0u);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"OK"}));
}

TEST(LineBufferTest, WorksAtCapacityOne)
{
    LineBuffer<1> buffer;
    Collected out;
    collect(buffer, "A\r\nBB\nC\n", out);
    EXPECT_EQ(out.overflows, 1u);
    EXPECT_EQ(out.lines, (std::vector<std::string> {"A", "C"}));
}

TEST(LineBufferTest, FeedsCompletedRangeLineToParser)
{
    LineBuffer<> buffer;
    Collected out;
    collect(buffer, "RANGE v=1 seq=685 t=70334 ", out);
    collect(buffer, "mm=1540 st=valid age=0\r", out);
    EXPECT_TRUE(out.lines.empty());
    collect(buffer, "\n", out);
    ASSERT_EQ(out.lines.size(), 1u);
    EXPECT_EQ(out.overflows, 0u);

    const auto reading = parse_range(out.lines.front());
    ASSERT_TRUE(reading.has_value());
    EXPECT_EQ(reading->sequence, 685u);
    EXPECT_EQ(reading->completed_ms, 70334u);
    EXPECT_EQ(reading->distance_mm, 1540u);
    EXPECT_EQ(reading->status, RangeStatus::valid);
    EXPECT_EQ(reading->reported_age_ms, 0u);
}