#include "src/app/motion_link.hpp"
#include "src/app/byte_ring.hpp"
#include <gtest/gtest.h>

namespace
{
// Feeds clean bytes and returns the event from the last one.
LinkEvent feedAll(FrameParser& parser, const char* text)
{
    LinkEvent last {};
    for (; *text != '\0'; ++text)
    {
        last = parser.feed(static_cast<std::uint8_t>(*text), false);
    }
    return last;
}

// Stage 10 starting limits from plan §2: ±100 mm/s, ±30 deg/s, 200 mm/s², 60 deg/s², 250 ms.
const MotionGovernor::Limits LIMITS {100.0f, 30.0f, 200.0f, 60.0f, 250};

// Sends a fresh command and steps the governor every 10 ms; returns the last output.
MotionTarget holdCommand(MotionGovernor& governor, std::uint8_t& seq, MotionTarget request,
                         std::uint32_t from_ms, std::uint32_t to_ms)
{
    MotionTarget out {};
    for (std::uint32_t t = from_ms; t <= to_ms; t += 10)
    {
        governor.onCommand(++seq, request, t);
        out = governor.step(true, t);
    }
    return out;
}

}

TEST(MotionLinkTest, Crc8CheckValue)
{
    EXPECT_EQ(crc8("123456789", 9), 0xF4);
}

TEST(MotionLinkTest, ValidFrameYieldsMotion)
{
    FrameParser parser;
    const LinkEvent event = feedAll(parser, "$M,17,120,-150*4D\n");
    EXPECT_EQ(event.kind, LinkEvent::Kind::Motion);
    EXPECT_EQ(event.seq, 17);
    EXPECT_FLOAT_EQ(event.target.forward_mm_s, 120.0f);
    EXPECT_FLOAT_EQ(event.target.yaw_rate_dps, -15.0f);   // wire unit is 0.1 deg/s
}

TEST(MotionLinkTest, BadCrcIsFrameError)
{
    FrameParser parser;
    EXPECT_EQ(feedAll(parser, "$M,17,120,-150*4E\n").kind, LinkEvent::Kind::FrameError);
}

TEST(MotionLinkTest, MissingStarOrShortCrcIsFrameError)
{
    FrameParser parser;
    EXPECT_EQ(feedAll(parser, "$M,17,120,-150\n").kind, LinkEvent::Kind::FrameError);
    EXPECT_EQ(feedAll(parser, "$M,17,120,-150*4\n").kind, LinkEvent::Kind::FrameError);
}

TEST(MotionLinkTest, OverlongFrameIsFrameError)
{
    FrameParser parser;
    parser.feed('$', false);
    LinkEvent last {};
    for (int i = 0; i < 33; ++i)
    {
        last = parser.feed('0', false);
    }
    EXPECT_EQ(last.kind, LinkEvent::Kind::FrameError);
}

TEST(MotionLinkTest, SpaceInsideFrameDisarms)
{
    FrameParser parser;
    feedAll(parser, "$M,17");
    EXPECT_EQ(parser.feed(' ', false).kind, LinkEvent::Kind::Disarm);
}

TEST(MotionLinkTest, LowercaseAInsideFrameDisarms)
{
    FrameParser parser;
    feedAll(parser, "$M,17");
    EXPECT_EQ(parser.feed('a', false).kind, LinkEvent::Kind::Disarm);
}

TEST(MotionLinkTest, ReceiveErrorInsideFrameDisarms)
{
    FrameParser parser;
    feedAll(parser, "$M,1");
    EXPECT_EQ(parser.feed('7', true).kind, LinkEvent::Kind::Disarm);
}

TEST(MotionLinkTest, DollarRestartsFrame)
{
    FrameParser parser;
    const LinkEvent event = feedAll(parser, "$M,17,12$M,17,120,-150*4D\n");
    EXPECT_EQ(event.kind, LinkEvent::Kind::Motion);
    EXPECT_EQ(event.seq, 17);
}

TEST(MotionLinkTest, CrLfTolerated)
{
    FrameParser parser;
    EXPECT_EQ(feedAll(parser, "$M,17,120,-150*4D\r\n").kind, LinkEvent::Kind::Motion);
}

TEST(MotionLinkTest, IdleArmAndDisarmMatchOperatorRules)
{
    FrameParser parser;
    EXPECT_EQ(parser.feed('a', false).kind, LinkEvent::Kind::Arm);
    EXPECT_EQ(parser.feed('a', true).kind, LinkEvent::Kind::Disarm);
    EXPECT_EQ(parser.feed('d', false).kind, LinkEvent::Kind::Disarm);
    EXPECT_EQ(parser.feed(0x00, false).kind, LinkEvent::Kind::Disarm);
    EXPECT_EQ(parser.feed(0xFF, false).kind, LinkEvent::Kind::Disarm);
}

TEST(MotionLinkTest, SeqOutOfRangeIsFrameError)
{
    FrameParser parser;
    EXPECT_EQ(feedAll(parser, "$M,256,0,0*00\n").kind, LinkEvent::Kind::FrameError);
}

TEST(MotionLinkTest, DisarmCausesAreReported)
{
    FrameParser parser;

    LinkEvent event = parser.feed(' ', false);
    EXPECT_EQ(event.cause, LinkEvent::Cause::Key);
    EXPECT_EQ(event.byte, ' ');

    event = parser.feed('d', false);
    EXPECT_EQ(event.cause, LinkEvent::Cause::Byte);
    EXPECT_EQ(event.byte, 'd');

    event = parser.feed('x', true);
    EXPECT_EQ(event.cause, LinkEvent::Cause::RxError);
    EXPECT_EQ(event.byte, 'x');

    feedAll(parser, "$M,1");
    event = parser.feed('7', true);
    EXPECT_EQ(event.cause, LinkEvent::Cause::RxError);   // errored byte mid-frame is rxerr, not frame
    EXPECT_EQ(event.byte, '7');

    feedAll(parser, "$M,1");
    event = parser.feed(' ', false);
    EXPECT_EQ(event.cause, LinkEvent::Cause::Frame);     // space mid-frame is frame, not key
    EXPECT_EQ(event.byte, ' ');
}

TEST(MotionGovernorTest, DisarmedOutputsZeroAndResets)
{
    MotionGovernor governor {LIMITS};
    std::uint8_t seq = 0;
    EXPECT_NEAR(holdCommand(governor, seq, {100.0f, 0.0f}, 0, 500).forward_mm_s, 100.0f, 1e-3f);

    EXPECT_FLOAT_EQ(governor.step(false, 510).forward_mm_s, 0.0f);
    EXPECT_FLOAT_EQ(governor.step(true, 520).forward_mm_s, 0.0f);   // the old command is gone
    EXPECT_TRUE(governor.onCommand(1, {100.0f, 0.0f}, 530));         // so is the sequence memory
}

TEST(MotionGovernorTest, CommandBeforeArmIsForgotten)
{
    MotionGovernor governor {LIMITS};
    governor.onCommand(1, {100.0f, 0.0f}, 0);
    governor.step(false, 0);
    EXPECT_FLOAT_EQ(governor.step(true, 10).forward_mm_s, 0.0f);
    EXPECT_FLOAT_EQ(governor.step(true, 20).forward_mm_s, 0.0f);
}

TEST(MotionGovernorTest, ClampsSymmetrically)
{
    MotionGovernor forward {LIMITS};
    std::uint8_t seq = 0;
    MotionTarget out = holdCommand(forward, seq, {1000.0f, 1000.0f}, 0, 1000);
    EXPECT_NEAR(out.forward_mm_s, 100.0f, 1e-3f);
    EXPECT_NEAR(out.yaw_rate_dps, 30.0f, 1e-3f);

    MotionGovernor reverse {LIMITS};
    seq = 0;
    out = holdCommand(reverse, seq, {-1000.0f, -1000.0f}, 0, 1000);
    EXPECT_NEAR(out.forward_mm_s, -100.0f, 1e-3f);
    EXPECT_NEAR(out.yaw_rate_dps, -30.0f, 1e-3f);
}

TEST(MotionGovernorTest, SlewLimitsRise)
{
    MotionGovernor governor {LIMITS};
    std::uint8_t seq = 0;
    // 100 ms at 200 mm/s² is 20 mm/s; the first step at t=0 has dt 0.
    EXPECT_NEAR(holdCommand(governor, seq, {100.0f, 0.0f}, 0, 100).forward_mm_s, 20.0f, 1e-3f);
}

TEST(MotionGovernorTest, TimeoutReturnsTargetToZeroWithSlew)
{
    MotionGovernor governor {LIMITS};
    std::uint8_t seq = 0;
    holdCommand(governor, seq, {100.0f, 0.0f}, 0, 500);   // last command at 500 ms

    MotionTarget out {};
    for (std::uint32_t t = 510; t <= 740; t += 10)
    {
        out = governor.step(true, t);
    }
    EXPECT_NEAR(out.forward_mm_s, 100.0f, 1e-3f);           // age 240 ms: still alive

    EXPECT_NEAR(governor.step(true, 750).forward_mm_s, 98.0f, 1e-3f);   // age 250 ms: lost
    EXPECT_NEAR(governor.step(true, 760).forward_mm_s, 96.0f, 1e-3f);
}

TEST(MotionGovernorTest, FirstStepAfterResetHasZeroDt)
{
    MotionGovernor governor {LIMITS};
    governor.onCommand(1, {100.0f, 0.0f}, 1000);
    EXPECT_FLOAT_EQ(governor.step(true, 1000).forward_mm_s, 0.0f);
    EXPECT_NEAR(governor.step(true, 1010).forward_mm_s, 2.0f, 1e-3f);
}

TEST(MotionGovernorTest, LongGapDtIsCapped)
{
    MotionGovernor governor {LIMITS};
    governor.onCommand(1, {100.0f, 0.0f}, 0);
    governor.step(true, 0);
    governor.onCommand(2, {100.0f, 0.0f}, 5000);
    // A 5 s gap is capped at 50 ms: 50 ms at 200 mm/s² is 10 mm/s.
    EXPECT_NEAR(governor.step(true, 5000).forward_mm_s, 10.0f, 1e-3f);
}

TEST(MotionGovernorTest, DuplicateSeqRejected)
{
    MotionGovernor governor {LIMITS};
    EXPECT_TRUE(governor.onCommand(5, {}, 0));
    EXPECT_FALSE(governor.onCommand(5, {}, 10));

    governor.reset();
    EXPECT_TRUE(governor.onCommand(5, {}, 20));             // first after reset always accepted
}

TEST(MotionGovernorTest, OlderSeqRejected)
{
    MotionGovernor governor {LIMITS};
    EXPECT_TRUE(governor.onCommand(5, {}, 0));
    EXPECT_FALSE(governor.onCommand(4, {}, 10));
    EXPECT_FALSE(governor.onCommand(133, {}, 20));          // 128 ahead counts as behind
}

TEST(MotionGovernorTest, SeqWrapAccepted)
{
    MotionGovernor governor {LIMITS};
    EXPECT_TRUE(governor.onCommand(255, {}, 0));
    EXPECT_TRUE(governor.onCommand(0, {}, 10));
}

TEST(MotionGovernorTest, LinkAliveFollowsTimeout)
{
    MotionGovernor governor {LIMITS};
    EXPECT_FALSE(governor.linkAlive(0));                    // no command yet
    governor.onCommand(1, {}, 1000);
    EXPECT_TRUE(governor.linkAlive(1249));
    EXPECT_FALSE(governor.linkAlive(1250));
}

TEST(MotionLinkTest, ToMoveXScale)
{
    EXPECT_NEAR(toMoveX(100.0f), 6.28f, 0.01f);
}

TEST(ByteRingTest, PushPopOrderAndFull)
{
    ByteRing<64> ring;
    for (std::uint16_t i = 0; i < 63; ++i)
    {
        EXPECT_TRUE(ring.push(i));
    }
    EXPECT_FALSE(ring.push(63));                            // one slot stays empty to mark full

    std::uint16_t entry = 0;
    for (std::uint16_t i = 0; i < 63; ++i)
    {
        ASSERT_TRUE(ring.pop(entry));
        EXPECT_EQ(entry, i);
    }
    EXPECT_FALSE(ring.pop(entry));
}
