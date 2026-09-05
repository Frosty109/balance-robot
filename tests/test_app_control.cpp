#include "src/app/app_control.hpp"
#include "src/pid/pid_control.hpp"
#include "hal/interface/i_sensor_hal.hpp"
#include "hal/interface/i_motor_hal.hpp"
#include <gtest/gtest.h>
#include <string>

class MockMonotonicClock : public IMonotonicClock
{
public:
    std::uint32_t now_ms {0};

    std::uint32_t nowMs() const override
    {
        return now_ms;
    }

    void advance(std::uint32_t elapsed_ms)
    {
        now_ms += elapsed_ms;
    }
};

class MockSensorHal : public ISensorHal
{
public:
    float angle     {0.0f};
    float battery   {12.0f};
    float gyro      {0.0f};
    float gyro_z    {0.0f};
    int   enc_l     {0};
    int   enc_r     {0};
    int   poll_calls {0};
    int   encoder_left_reads {0};
    int   encoder_right_reads {0};
    bool  fresh     {true};

    // Optional: when set, poll() consumes poll_duration_ms of mock time.
    MockMonotonicClock* clock {nullptr};
    std::uint32_t poll_duration_ms {0};

    float getAngle()        override { return angle; }
    float getBattery()      override { return battery; }
    float getGyroBalance()  override { return gyro; }
    float getGyroTurn()     override { return gyro_z; }
    float getAccelZ()       override { return 0.0f; }
    int   getEncoderLeft() override
    {
        ++encoder_left_reads;
        return enc_l;
    }
    int   getEncoderRight() override
    {
        ++encoder_right_reads;
        return enc_r;
    }
    bool  poll() override
    {
        ++poll_calls;
        if (clock != nullptr)
        {
            clock->advance(poll_duration_ms);
        }

        return fresh;
    }
};

class MockMotorHal : public IMotorHal
{
public:
    int last_left   {0};
    int last_right  {0};
    int set_calls   {0};

    void setMotorPWM(int left, int right) override
    {
        ++set_calls;
        last_left   = left;
        last_right  = right;
    }
};

TEST(AppControlTest, SafetyCutoffOnHighAngle)
{
    MockSensorHal sensor;
    MockMotorHal  motor;
    MockMonotonicClock clock;
    AppControl    app(sensor,
                     motor,
                     clock,
                     BalancePD(200.0f, 0.8f, 0.0f),
                     VelocityPI(1.2f, 0.05f, 200.0f),
                     TurnPD(5.0f, 0.1f));

    sensor.angle    = 45.0f;
    sensor.battery  = 12.0f;

    app.update();

    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, SafetyCutoffOnLowBattery)
{
    MockSensorHal sensor;
    MockMotorHal  motor;
    MockMonotonicClock clock;
    AppControl    app(sensor,
                     motor,
                     clock,
                     BalancePD(200.0f, 0.8f, 0.0f),
                     VelocityPI(1.2f, 0.05f, 200.0f),
                     TurnPD(5.0f, 0.1f));

    sensor.angle    = 0.0f;
    sensor.battery  = 9.0f;

    app.update();

    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, NormalPathDrivesMotors)
{
    MockSensorHal sensor;
    MockMotorHal  motor;
    MockMonotonicClock clock;
    AppControl    app(sensor,
                     motor,
                     clock,
                     BalancePD(200.0f, 0.8f, 0.0f),
                     VelocityPI(1.2f, 0.05f, 200.0f),
                     TurnPD(5.0f, 0.1f));

    sensor.angle    = 5.0f;
    sensor.battery  = 12.0f;

    app.update();

    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(motor.last_right, 10);
}

TEST(AppControlTest, FreshSamplePerformsOneControlUpdate)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;
    AppControl app(
        sensor,
        motor,
        clock,
        BalancePD(200.0f, 0.0f, 0.0f),
        VelocityPI(0.0f, 0.0f, 200.0f),
        TurnPD(0.0f, 0.0f));

    sensor.fresh = true;
    sensor.angle = 5.0f;

    app.update();

    EXPECT_EQ(sensor.poll_calls, 1);
    EXPECT_EQ(sensor.encoder_left_reads, 1);
    EXPECT_EQ(sensor.encoder_right_reads, 1);
    EXPECT_EQ(motor.set_calls, 1);

    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(motor.last_right, 10);
}

TEST(AppControlTest, NotFreshSampleDoesNotUpdateControl)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(
        sensor,
        motor,
        clock,
        BalancePD(200.0f, 0.0f, 0.0f),
        VelocityPI(0.0f, 0.0f, 200.0f),
        TurnPD(0.0f, 0.0f));

    sensor.fresh = true;
    sensor.angle = 5.0f;
    app.update();

    ASSERT_EQ(motor.last_left, 10);
    ASSERT_EQ(motor.last_right, 10);
    ASSERT_EQ(motor.set_calls, 1);
    ASSERT_EQ(sensor.encoder_left_reads, 1);
    ASSERT_EQ(sensor.encoder_right_reads, 1);

    sensor.fresh = false;
    sensor.angle = 20.0f;
    sensor.enc_l = 100;
    sensor.enc_r = 100;

    app.update();

    EXPECT_EQ(sensor.poll_calls, 2);

    EXPECT_EQ(sensor.encoder_left_reads, 1);
    EXPECT_EQ(sensor.encoder_right_reads, 1);

    EXPECT_EQ(motor.set_calls, 1);
    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(motor.last_right, 10);
}

TEST(AppControlTest, NotFreshSampleDoesNotAdvanceVelocityState)
{
    MockSensorHal tested_sensor;
    MockMotorHal tested_motor;
    MockMonotonicClock tested_clock;

    MockSensorHal reference_sensor;
    MockMotorHal reference_motor;
    MockMonotonicClock reference_clock;

    AppControl tested_app(
        tested_sensor,
        tested_motor,
        tested_clock,
        BalancePD(0.0f, 0.0f, 0.0f),
        VelocityPI(100.0f, 100.0f, 10000.0f),
        TurnPD(0.0f, 0.0f));

    AppControl reference_app(
        reference_sensor,
        reference_motor,
        reference_clock,
        BalancePD(0.0f, 0.0f, 0.0f),
        VelocityPI(100.0f, 100.0f, 10000.0f),
        TurnPD(0.0f, 0.0f));

    tested_sensor.enc_l = 100;
    tested_sensor.enc_r = 100;
    reference_sensor.enc_l = 100;
    reference_sensor.enc_r = 100;

    tested_app.update();
    reference_app.update();

    tested_sensor.fresh = false;
    tested_sensor.enc_l = 1000;
    tested_sensor.enc_r = 1000;
    tested_app.update();

    tested_sensor.fresh = true;
    tested_sensor.enc_l = 0;
    tested_sensor.enc_r = 0;
    reference_sensor.enc_l = 0;
    reference_sensor.enc_r = 0;

    tested_app.update();
    reference_app.update();

    EXPECT_EQ(tested_sensor.poll_calls, 3);
    EXPECT_EQ(tested_sensor.encoder_left_reads, 2);
    EXPECT_EQ(tested_sensor.encoder_right_reads, 2);
    EXPECT_EQ(tested_motor.set_calls, 2);

    EXPECT_EQ(tested_motor.last_left, reference_motor.last_left);
    EXPECT_EQ(tested_motor.last_right, reference_motor.last_right);
}

TEST(AppControlTest, TelemetryDecimationCountsFreshSamples)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(
        sensor,
        motor,
        clock,
        BalancePD(0.0f, 0.0f, 0.0f),
        VelocityPI(0.0f, 0.0f, 200.0f),
        TurnPD(0.0f, 0.0f));

    testing::internal::CaptureStdout();

    for (int i = 0; i < 19; ++i)
    {
        app.update();
    }

    sensor.fresh = false;

    for (int i = 0; i < 10; ++i)
    {
        app.update();
    }

    const std::string before_twentieth = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(before_twentieth.empty());

    sensor.fresh = true;

    testing::internal::CaptureStdout();
    app.update();

    const std::string twentieth_output = testing::internal::GetCapturedStdout();

    EXPECT_NE(twentieth_output.find("angle="), std::string::npos);

    EXPECT_EQ(sensor.poll_calls, 30);
    EXPECT_EQ(sensor.encoder_left_reads, 20);
    EXPECT_EQ(sensor.encoder_right_reads, 20);
    EXPECT_EQ(motor.set_calls, 20);
}

TEST(AppControlTest, StaleDeadlineIsWrapSafe)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   TurnPD(0.0f, 0.0f));

    clock.now_ms = 0xFFFFFFF0u;    // 16 ms before rollover
    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.update();                  // accepted; last_fresh_ms_ = 0xFFFFFFF0
    ASSERT_EQ(motor.last_left, 10);
    const int writes_after_fresh = motor.set_calls;

    sensor.fresh = false;
    clock.advance(24);             // now = 0x00000008, wrapped
    app.update();
    EXPECT_EQ(motor.set_calls, writes_after_fresh);   // 24 < 25, no shutdown

    clock.advance(1);              // elapsed is now exactly 25
    app.update();
    EXPECT_EQ(motor.set_calls, writes_after_fresh + 1);
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, RejectedPollBelowDeadlineRetainsPWM)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   TurnPD(0.0f, 0.0f));

    sensor.clock = &clock;
    sensor.poll_duration_ms = 5;
    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.update();                          // last_fresh_ms_ = 5
    ASSERT_EQ(motor.last_left, 10);
    ASSERT_EQ(motor.set_calls, 1);

    sensor.fresh = false;
    for (int i = 0; i < 4; ++i)
    {
        app.update();                      // elapsed 5, 10, 15, 20
    }

    EXPECT_EQ(sensor.poll_calls, 5);
    EXPECT_EQ(motor.set_calls, 1);
    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(motor.last_right, 10);
}

TEST(AppControlTest, ShutdownAtExactDeadline)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   TurnPD(0.0f, 0.0f));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.update();                  // last_fresh_ms_ = 0
    ASSERT_EQ(motor.set_calls, 1);

    sensor.fresh = false;
    clock.advance(25);             // elapsed exactly STALE_TIMEOUT_MS
    app.update();

    EXPECT_EQ(motor.set_calls, 2);
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, StaleResetsVelocityState)
{
    MockSensorHal tested_sensor;
    MockMotorHal tested_motor;
    MockMonotonicClock tested_clock;

    MockSensorHal reference_sensor;
    MockMotorHal reference_motor;
    MockMonotonicClock reference_clock;

    AppControl tested_app(
        tested_sensor,
        tested_motor,
        tested_clock,
        BalancePD(0.0f, 0.0f, 0.0f),
        VelocityPI(100.0f, 100.0f, 10000.0f),
        TurnPD(0.0f, 0.0f));

    AppControl reference_app(
        reference_sensor,
        reference_motor,
        reference_clock,
        BalancePD(0.0f, 0.0f, 0.0f),
        VelocityPI(100.0f, 100.0f, 10000.0f),
        TurnPD(0.0f, 0.0f));

    tested_sensor.enc_l = 100;
    tested_sensor.enc_r = 100;
    reference_sensor.enc_l = 100;
    reference_sensor.enc_r = 100;

    for (int i = 0; i < 2; ++i)
    {
        tested_app.update();
        reference_app.update();
    }

    // Both apps must be in lockstep before they diverge.
    ASSERT_EQ(tested_motor.last_left, reference_motor.last_left);
    ASSERT_EQ(tested_motor.set_calls, 2);

    // Tested clears its integral by going stale; reference clears it explicitly.
    tested_sensor.fresh = false;
    tested_clock.advance(25);
    tested_app.update();
    reference_app.reset();

    tested_sensor.fresh = true;
    tested_sensor.enc_l = 0;
    tested_sensor.enc_r = 0;
    reference_sensor.enc_l = 0;
    reference_sensor.enc_r = 0;

    for (int i = 0; i < 3; ++i)
    {
        tested_app.update();       // two gated by recovery, third drives
    }
    reference_app.update();

    // Gated recovery samples must not run the control path at all.
    EXPECT_EQ(tested_sensor.encoder_left_reads, 3);
    EXPECT_EQ(reference_sensor.encoder_left_reads, 3);
    EXPECT_EQ(tested_motor.set_calls, 4);      // 2 drive, 1 stale zero, 1 recovered
    EXPECT_EQ(reference_motor.set_calls, 3);

    EXPECT_EQ(tested_motor.last_left, reference_motor.last_left);
    EXPECT_EQ(tested_motor.last_right, reference_motor.last_right);
}

TEST(AppControlTest, RecoveryRequiresThreeConsecutiveFresh)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   TurnPD(0.0f, 0.0f));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.update();
    ASSERT_EQ(motor.last_left, 10);

    sensor.fresh = false;
    clock.advance(25);
    app.update();
    ASSERT_EQ(motor.set_calls, 2);
    ASSERT_EQ(motor.last_left, 0);

    sensor.fresh = true;
    app.update();
    app.update();

    EXPECT_EQ(motor.set_calls, 2);             // two fresh is not enough
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);

    app.update();                              // third clears stale

    EXPECT_EQ(motor.set_calls, 3);
    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(motor.last_right, 10);
}

TEST(AppControlTest, RecoveryCounterResetsOnRejectedPoll)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   TurnPD(0.0f, 0.0f));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.update();

    sensor.fresh = false;
    clock.advance(25);
    app.update();
    ASSERT_EQ(motor.set_calls, 2);
    ASSERT_EQ(motor.last_left, 0);

    sensor.fresh = true;
    app.update();
    app.update();                              // count 2

    sensor.fresh = false;
    app.update();                              // count back to 0

    sensor.fresh = true;
    app.update();                              // count 1, not 3

    EXPECT_EQ(motor.set_calls, 2);
    EXPECT_EQ(motor.last_left, 0);

    app.update();                              // count 2
    EXPECT_EQ(motor.set_calls, 2);

    app.update();                              // count 3, recovers
    EXPECT_EQ(motor.set_calls, 3);
    EXPECT_EQ(motor.last_left, 10);
}

TEST(AppControlTest, RecoveryRejectedWhenAngleUnsafe)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   TurnPD(0.0f, 0.0f));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.update();
    ASSERT_EQ(sensor.encoder_left_reads, 1);

    sensor.fresh = false;
    clock.advance(25);
    app.update();
    ASSERT_EQ(motor.set_calls, 2);
    ASSERT_EQ(motor.last_left, 0);

    // 30 deg is outside the +/-10 recovery band but inside the 40 deg fault
    // threshold, so only the recovery gate can reject these.
    sensor.angle = 30.0f;
    sensor.fresh = true;
    for (int i = 0; i < 3; ++i)
    {
        app.update();
    }

    EXPECT_EQ(motor.set_calls, 2);
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
    EXPECT_EQ(sensor.encoder_left_reads, 1);
}

TEST(AppControlTest, RecoveryCounterResetsOnUnsafeAngle)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   TurnPD(0.0f, 0.0f));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.update();

    sensor.fresh = false;
    clock.advance(25);
    app.update();
    ASSERT_EQ(motor.set_calls, 2);
    ASSERT_EQ(motor.last_left, 0);

    sensor.fresh = true;
    app.update();
    app.update();                              // count 2

    sensor.angle = 30.0f;
    app.update();                              // fresh, but out of band

    // An out-of-band sample resets the counter the same way a rejected poll
    // does: a sample the app declined to act on is not evidence of recovery.
    sensor.angle = 5.0f;
    app.update();                              // count 1, not 3
    EXPECT_EQ(motor.set_calls, 2);
    EXPECT_EQ(motor.last_left, 0);

    app.update();                              // count 2
    EXPECT_EQ(motor.set_calls, 2);

    app.update();                              // count 3, recovers
    EXPECT_EQ(motor.set_calls, 3);
    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(sensor.encoder_left_reads, 2);   // no gated sample touched the PI
}

TEST(AppControlTest, MaxPollDurationIsRecorded)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   TurnPD(0.0f, 0.0f));

    sensor.clock = &clock;
    sensor.angle = 5.0f;
    sensor.fresh = true;

    testing::internal::CaptureStdout();

    sensor.poll_duration_ms = 30;
    app.update();                              // the one slow poll

    sensor.poll_duration_ms = 1;
    for (int i = 0; i < 19; ++i)
    {
        app.update();                          // 20th fires telemetry
    }

    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("poll=30\n"), std::string::npos);
    EXPECT_EQ(output.find("poll=1\n"), std::string::npos);   // max, not last
}

TEST(AppControlTest, StaleEntryPrintsOnce)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   TurnPD(0.0f, 0.0f));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.update();
    ASSERT_EQ(motor.set_calls, 1);

    sensor.fresh = false;
    clock.advance(25);

    testing::internal::CaptureStdout();

    for (int i = 0; i < 200; ++i)
    {
        app.update();
    }

    const std::string output = testing::internal::GetCapturedStdout();

    std::string::size_type stale_lines = 0;
    for (std::string::size_type pos = output.find("STALE");
         pos != std::string::npos;
         pos = output.find("STALE", pos + 1))
    {
        ++stale_lines;
    }

    EXPECT_EQ(stale_lines, 1u);                // report is one-shot
    EXPECT_EQ(motor.set_calls, 201);                 // shutdown re-asserts
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}


