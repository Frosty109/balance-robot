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
    float angle         {0.0f};
    float battery       {12.0f};
    float gyro          {0.0f};
    float yaw_rate_dps  {0.0f};
    int   enc_l         {0};
    int   enc_r         {0};
    int   poll_calls    {0};
    int   encoder_left_reads {0};
    int   encoder_right_reads {0};
    bool  fresh     {true};

    bool accumulate_encoders {false}; // Opt in timer model to test backlog
    int  counts_per_poll_l {0};
    int  counts_per_poll_r {0};
    int  pending_l {0};
    int  pending_r {0};

    // Optional: when set, poll() consumes poll_duration_ms of mock time.
    MockMonotonicClock* clock {nullptr};
    std::uint32_t poll_duration_ms {0};

    float getAngle()        override { return angle; }
    float getBattery()      override { return battery; }
    float getGyroBalance()  override { return gyro; }
    float getYawRateDps()   override { return yaw_rate_dps; }
    float getAccelZ()       override { return 0.0f; }
    int   getEncoderLeft()  override
    {
        ++encoder_left_reads;
        if (accumulate_encoders)
        {
            const int count = pending_l;
            pending_l = 0;
            return count;
        }
        return enc_l;
    }
    int   getEncoderRight() override
    {
        ++encoder_right_reads;
        if (accumulate_encoders)
        {
            const int count = pending_r;
            pending_r = 0;
            return count;
        }
        return enc_r;
    }
    bool  poll() override
    {
        ++poll_calls;
        if (clock != nullptr)
        {
            clock->advance(poll_duration_ms);
        }

        if (accumulate_encoders)
        {
            pending_l += counts_per_poll_l;
            pending_r += counts_per_poll_r;
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

static int countOccurrences(const std::string& text, const std::string& needle)
{
    int count {0};
    for (std::string::size_type pos = text.find(needle);
         pos != std::string::npos;
         pos = text.find(needle, pos + needle.size()))
    {
        ++count;
    }
    return count;
}


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
                     YawRateP(0.0f, 30.0f, 100));

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
                     YawRateP(0.0f, 30.0f, 100));

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
                     YawRateP(0.0f, 30.0f, 100));

    sensor.angle    = 5.0f;
    sensor.battery  = 12.0f;

    app.requestArm();
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
        YawRateP(0.0f, 30.0f, 100));

    sensor.fresh = true;
    sensor.angle = 5.0f;

    app.requestArm();
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
        YawRateP(0.0f, 30.0f, 100));

    sensor.fresh = true;
    sensor.angle = 5.0f;
    app.requestArm();
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
        YawRateP(0.0f, 30.0f, 100));

    AppControl reference_app(
        reference_sensor,
        reference_motor,
        reference_clock,
        BalancePD(0.0f, 0.0f, 0.0f),
        VelocityPI(100.0f, 100.0f, 10000.0f),
        YawRateP(0.0f, 30.0f, 100));

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
        YawRateP(0.0f, 30.0f, 100));

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

TEST(AppControlTest, TelemetrySumsEncoderCountsAcrossTheWindow)
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
        YawRateP(0.0f, 30.0f, 100));
        
    // 3 and -5 are chosen so the four plausible mistakes fail differently
    sensor.enc_l = 3;
    sensor.enc_r = -5;

    app.requestArm();
    testing::internal::CaptureStdout();
    for (int i = 0; i < 20; ++i) { app.update(); }
    const std::string first = testing::internal::GetCapturedStdout();

    EXPECT_NE(first.find("enc_l=60"), std::string::npos);
    EXPECT_NE(first.find("enc_r=-100"), std::string::npos);

    // Rejected polls must neither print nor accumulate.
    sensor.fresh = false;
    testing::internal::CaptureStdout();
    for (int i = 0; i < 10; ++i) { app.update(); }
    EXPECT_TRUE(testing::internal::GetCapturedStdout().empty());

    sensor.fresh = true;
    sensor.enc_l = 1;
    sensor.enc_r = 1;

    testing::internal::CaptureStdout();
    for (int i = 0; i < 20; ++i) { app.update(); }
    const std::string second = testing::internal::GetCapturedStdout();

    EXPECT_NE(second.find("enc_l=20"), std::string::npos);
    EXPECT_NE(second.find("enc_r=20"), std::string::npos);
}


TEST(AppControlTest, StaleDeadlineIsWrapSafe)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    clock.now_ms = 0xFFFFFFF0u;    // 16 ms before rollover
    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.requestArm();
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
                   YawRateP(0.0f, 30.0f, 100));

    sensor.clock = &clock;
    sensor.poll_duration_ms = 5;
    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.requestArm();
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
                   YawRateP(0.0f, 30.0f, 100));

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
        YawRateP(0.0f, 30.0f, 100));

    AppControl reference_app(
        reference_sensor,
        reference_motor,
        reference_clock,
        BalancePD(0.0f, 0.0f, 0.0f),
        VelocityPI(100.0f, 100.0f, 10000.0f),
        YawRateP(0.0f, 30.0f, 100));

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

    // Every fresh sample drains the encoders (includes gated recovery samples)
    EXPECT_EQ(tested_sensor.encoder_left_reads, 5);
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
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.requestArm();
    app.update();
    ASSERT_EQ(motor.last_left, 10);

    sensor.fresh = false;
    clock.advance(25);
    app.update();
    ASSERT_EQ(motor.last_left, 0);

    testing::internal::CaptureStdout();
    sensor.fresh = true;
    app.requestArm();
    app.update();                              // count 1
    app.requestArm();
    app.update();                              // count 2
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "ARM REJECTED reason=recovering"), 2);
    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);

    testing::internal::CaptureStdout();
    app.requestArm();
    app.update();                              // count 3: recovers and arms
    const std::string armed_out = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(armed_out, "ARMED angle="), 1);
    EXPECT_TRUE(app.armed());
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
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.requestArm();
    app.update();
    ASSERT_EQ(motor.last_left, 10);

    sensor.fresh = false;
    clock.advance(25);
    app.update();
    ASSERT_EQ(motor.last_left, 0);

    sensor.fresh = true;
    app.update();                              // count 1
    app.update();                              // count 2

    sensor.fresh = false;
    app.update();                              // rejected poll: count back to 0

    testing::internal::CaptureStdout();
    sensor.fresh = true;
    app.requestArm();
    app.update();                              // count 1, not 3
    app.requestArm();
    app.update();                              // count 2
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "ARM REJECTED reason=recovering"), 2);
    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);

    testing::internal::CaptureStdout();
    app.requestArm();
    app.update();                              // count 3: recovers and arms
    const std::string armed_out = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(armed_out, "ARMED angle="), 1);
    EXPECT_TRUE(app.armed());
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
                   YawRateP(0.0f, 30.0f, 100));

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
    EXPECT_EQ(sensor.encoder_left_reads, 4);
}

TEST(AppControlTest, RecoveryCounterResetsOnUnsafeAngle)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.requestArm();
    app.update();
    ASSERT_EQ(motor.last_left, 10);

    sensor.fresh = false;
    clock.advance(25);
    app.update();
    ASSERT_EQ(motor.last_left, 0);

    sensor.fresh = true;
    app.update();                              // count 1
    app.update();                              // count 2

    sensor.angle = 30.0f;
    app.update();                              // fresh, out of band: count back to 0

    testing::internal::CaptureStdout();
    sensor.angle = 5.0f;
    app.requestArm();
    app.update();                              // count 1, not 3
    app.requestArm();
    app.update();                              // count 2
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "ARM REJECTED reason=recovering"), 2);
    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(sensor.encoder_left_reads, 6);   // every fresh sample drains

    testing::internal::CaptureStdout();
    app.requestArm();
    app.update();                              // count 3: recovers and arms
    const std::string armed_out = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(armed_out, "ARMED angle="), 1);
    EXPECT_TRUE(app.armed());
    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(sensor.encoder_left_reads, 7);
}

TEST(AppControlTest, RecoveryCounterResetsOnAngleFault)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.requestArm();
    app.update();
    ASSERT_EQ(motor.last_left, 10);

    sensor.fresh = false;
    clock.advance(25);
    app.update();
    ASSERT_EQ(motor.last_left, 0);

    sensor.fresh = true;
    app.update();                              // count 1
    app.update();                              // count 2

    sensor.angle = 45.0f;
    app.update();                              // faulted: count back to 0

    testing::internal::CaptureStdout();
    sensor.angle = 5.0f;
    app.requestArm();
    app.update();                              // RECOVERED, count 1, not 3
    app.requestArm();
    app.update();                              // count 2
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "RECOVERED"), 1);
    EXPECT_EQ(countOccurrences(output, "ARM REJECTED reason=recovering"), 2);
    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);

    testing::internal::CaptureStdout();
    app.requestArm();
    app.update();                              // count 3: recovers and arms
    const std::string armed_out = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(armed_out, "ARMED angle="), 1);
    EXPECT_TRUE(app.armed());
    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(sensor.encoder_left_reads, 7);   // every fresh sample drains
}

TEST(AppControlTest, MaxPollDurationIsRecorded)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

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
                   YawRateP(0.0f, 30.0f, 100));

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

TEST(AppControlTest, AngleFaultBacklogIsDiscardedOnRecovery) 
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(0.0f, 0.0f, 0.0f),
                   VelocityPI(100.0f, 0.0f, 100000.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.accumulate_encoders = true;
    sensor.angle = 5.0f;
    sensor.fresh = true;

    app.update();

    ASSERT_EQ(motor.set_calls, 1);
    ASSERT_EQ(motor.last_left, 0);

    sensor.angle = 45.0f;
    sensor.counts_per_poll_l = 100;
    sensor.counts_per_poll_r = 100;

    for (int i = 0; i < 50; ++i)
    {
        app.update();            // faulted; the timer keeps counting
    }

    ASSERT_EQ(motor.last_left, 0); // Fault held the motors stopped

    sensor.angle = 5.0f;
    sensor.counts_per_poll_l = 0;
    sensor.counts_per_poll_r = 0;

    app.update();                    // RECOVERED, drives this sample

    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, StaleBacklogIsDiscardedOnRecovery)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(0.0f, 0.0f, 0.0f),
                   VelocityPI(100.0f, 0.0f, 100000.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.accumulate_encoders = true;
    sensor.angle = 5.0f;
    sensor.fresh = true;

    app.update();
    ASSERT_EQ(motor.set_calls, 1);
    ASSERT_EQ(motor.last_left, 0);

    sensor.fresh = false;
    sensor.counts_per_poll_l = 100;
    sensor.counts_per_poll_r = 100;

    clock.advance(25);
    app.update();                              // STALE at the exact deadline

    for (int i = 0; i < 49; ++i)
    {
        app.update();                          // 50 non-fresh polls in total
    }

    ASSERT_EQ(motor.last_left, 0);

    sensor.fresh = true;
    sensor.counts_per_poll_l = 0;
    sensor.counts_per_poll_r = 0;

    for (int i = 0; i < 3; ++i)
    {
        app.update();                          // two gated by recovery, third drives
    }

    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, ArmRejectedOutsideClearBand)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 20.0f;                      // inside the 40° fault, outside the 10° clear band

    app.requestArm();
    app.update();

    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, ArmRequestIsNotLatched)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 20.0f;

    app.requestArm();
    app.update();                              // rejected, and the request is cleared

    sensor.angle = 5.0f;

    for (int i = 0; i < 10; ++i)
    {
        app.update();                          // safe angle, but no new request
    }

    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, ArmAcceptedOnFreshInBandSample)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;

    app.requestArm();
    app.update();                              // B1.4: the arming sample drives

    EXPECT_TRUE(app.armed());
    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(motor.last_right, 10);
}

TEST(AppControlTest, NoRestartAfterAngleRecovery)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    app.requestArm();
    app.update();

    EXPECT_TRUE(app.armed());
    EXPECT_EQ(motor.last_left, 10);            // armed and driving before the fault

    sensor.angle = 45.0f;
    app.update();                              // FAULT, disarms

    sensor.angle = 5.0f;

    for (int i = 0; i < 50; ++i)
    {
        app.update();                          // back in band, no new request
    }

    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, NoRestartAfterStaleRecovery)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    app.requestArm();
    app.update();

    EXPECT_TRUE(app.armed());
    EXPECT_EQ(motor.last_left, 10);

    sensor.fresh = false;
    clock.advance(25);

    testing::internal::CaptureStdout();
    app.update();                              // STALE at the exact deadline, disarms
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "DISARMED reason=stale"), 1);

    sensor.fresh = true;

    for (int i = 0; i < 50; ++i)
    {
        app.update();                          // stale recovery completes, no new request
    }

    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, DisarmedTelemetryReportsState)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;                       // upright, but never armed

    testing::internal::CaptureStdout();
    for (int i = 0; i < 20; ++i) 
    { 
        app.update(); 
    }
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "state=DISARMED"), 1);
    EXPECT_EQ(countOccurrences(output, "bal="), 0);    // no armed-format line
    EXPECT_EQ(motor.last_left, 0);
}

TEST(AppControlTest, RearmStartsAFullTelemetryWindow)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    sensor.enc_l = 3; // first-window counts, distinct from the second
    sensor.enc_r = 3;

    app.requestArm();
    for (int i = 0; i < 10; ++i) 
    { 
        app.update(); // half an armed window
    }     

    app.requestDisarm();
    for (int i = 0; i < 5; ++i) 
    { 
        app.update(); // disarmed samples
    }      

    sensor.enc_l = 1;
    sensor.enc_r = 1;

    testing::internal::CaptureStdout();
    app.requestArm();
    for (int i = 0; i < 19; ++i) 
    { 
        app.update(); // re-armed, one short of a window
    }     
    const std::string partial = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(partial, "bal="), 0);

    testing::internal::CaptureStdout();
    app.update();                                      // 20th driving sample since re-arm
    const std::string full = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(full, "bal="), 1);
    EXPECT_NE(full.find("enc_l=20"), std::string::npos);
    EXPECT_NE(full.find("enc_r=20"), std::string::npos);
}

TEST(AppControlTest, TransitionsPrintOnce)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    app.requestArm();
    app.update();

    sensor.angle = 45.0f;

    testing::internal::CaptureStdout();
    for (int i = 0; i < 200; ++i) 
    { 
        app.update(); // held in the fault
    }    
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "DISARMED reason=angle"), 1);
    EXPECT_EQ(countOccurrences(output, "DISARMED reason="), 1);  // no other disarm reason either
    EXPECT_EQ(countOccurrences(output, "FAULT"), 1);
    EXPECT_EQ(motor.last_left, 0);
}

TEST(AppControlTest, ArmRejectedOnAngleRecoverySample)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 45.0f;
    app.update();                              // FAULT

    sensor.angle = 5.0f;                       // inside the clear band

    testing::internal::CaptureStdout();
    app.requestArm();
    app.update();                              // clears the fault; the request is refused
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "RECOVERED"), 1);
    EXPECT_EQ(countOccurrences(output, "ARM REJECTED reason=recovering"), 1);
    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);

    for (int i = 0; i < 10; ++i)
    {
        app.update();                          // safe and upright, no new request
    }

    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
}

TEST(AppControlTest, ArmRejectedWhileStaleRecovering)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.requestArm();
    app.update();                              // ARMED
    ASSERT_EQ(motor.last_left, 10);

    sensor.fresh = false;
    clock.advance(25);
    app.update();                              // STALE
    ASSERT_EQ(motor.last_left, 0);

    sensor.angle = 45.0f;
    sensor.fresh = true;
    app.update();                              // FAULT during recovery

    sensor.angle = 5.0f;

    testing::internal::CaptureStdout();
    app.requestArm();
    app.update();                              // RECOVERED, count 1: rejected
    app.requestArm();
    app.update();                              // count 2: rejected
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "RECOVERED"), 1);
    EXPECT_EQ(countOccurrences(output, "ARM REJECTED reason=recovering"), 2);
    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);

    testing::internal::CaptureStdout();
    app.requestArm();
    app.update();                              // count 3: recovers and arms
    const std::string armed_out = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(armed_out, "ARMED angle="), 1);
    EXPECT_TRUE(app.armed());
    EXPECT_EQ(motor.last_left, 10);
}

TEST(AppControlTest, BootsDisarmedAndCommandsZero)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    motor.last_left = 999;                     // whatever the outputs held before main ran
    motor.last_right = 999;

    testing::internal::CaptureStdout();
    app.requestDisarm();                       // main's boot step, before any update
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.set_calls, 1);
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
    EXPECT_EQ(countOccurrences(output, "DISARMED reason=operator"), 0);   // it was never armed

    sensor.angle = 5.0f;
    app.update();                              // upright, fresh, no request

    EXPECT_EQ(motor.last_left, 0);
}

TEST(AppControlTest, DisarmCommandsZeroImmediately)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    app.requestArm();
    app.update();

    EXPECT_TRUE(app.armed());
    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(motor.set_calls, 1);

    testing::internal::CaptureStdout();
    app.requestDisarm();                       // no update() follows
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.set_calls, 2);             // the disarm call itself wrote the motors
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
    EXPECT_EQ(countOccurrences(output, "DISARMED reason=operator"), 1);
}

TEST(AppControlTest, DisarmResetsVelocityState)
{
    MockSensorHal tested_sensor;
    MockMotorHal tested_motor;
    MockMonotonicClock tested_clock;

    MockSensorHal reference_sensor;
    MockMotorHal reference_motor;
    MockMonotonicClock reference_clock;

    AppControl tested_app(tested_sensor, tested_motor, tested_clock,
                          BalancePD(0.0f, 0.0f, 0.0f),
                          VelocityPI(100.0f, 100.0f, 10000.0f),
                          YawRateP(0.0f, 30.0f, 100));

    AppControl reference_app(reference_sensor, reference_motor, reference_clock,
                             BalancePD(0.0f, 0.0f, 0.0f),
                             VelocityPI(100.0f, 100.0f, 10000.0f),
                             YawRateP(0.0f, 30.0f, 100));

    tested_sensor.enc_l = 100;
    tested_sensor.enc_r = 100;
    reference_sensor.enc_l = 100;
    reference_sensor.enc_r = 100;

    tested_app.requestArm();
    reference_app.requestArm();

    for (int i = 0; i < 2; ++i)
    {
        tested_app.update();
        reference_app.update();
    }

    EXPECT_TRUE(tested_app.armed());
    ASSERT_EQ(tested_motor.last_left, reference_motor.last_left);

    // Tested clears its state by disarming and re-arming; reference clears it explicitly.
    tested_app.requestDisarm();
    tested_app.requestArm();
    reference_app.reset();

    tested_sensor.enc_l = 50;
    tested_sensor.enc_r = 50;
    reference_sensor.enc_l = 50;
    reference_sensor.enc_r = 50;

    tested_app.update();
    reference_app.update();

    EXPECT_TRUE(tested_app.armed());
    EXPECT_NE(reference_motor.last_left, 0);   // a real command, not a disarmed zero
    EXPECT_EQ(tested_motor.last_left, reference_motor.last_left);
    EXPECT_EQ(tested_motor.last_right, reference_motor.last_right);
}

TEST(AppControlTest, RepeatedArmRequestIsNoOp)
{
    MockSensorHal tested_sensor;
    MockMotorHal tested_motor;
    MockMonotonicClock tested_clock;

    MockSensorHal reference_sensor;
    MockMotorHal reference_motor;
    MockMonotonicClock reference_clock;

    AppControl tested_app(tested_sensor, tested_motor, tested_clock,
                          BalancePD(0.0f, 0.0f, 0.0f),
                          VelocityPI(100.0f, 100.0f, 10000.0f),
                          YawRateP(0.0f, 30.0f, 100));

    AppControl reference_app(reference_sensor, reference_motor, reference_clock,
                             BalancePD(0.0f, 0.0f, 0.0f),
                             VelocityPI(100.0f, 100.0f, 10000.0f),
                             YawRateP(0.0f, 30.0f, 100));

    tested_sensor.enc_l = 100;
    tested_sensor.enc_r = 100;
    reference_sensor.enc_l = 100;
    reference_sensor.enc_r = 100;

    tested_app.requestArm();
    reference_app.requestArm();

    for (int i = 0; i < 2; ++i)
    {
        tested_app.update();
        reference_app.update();
    }

    tested_app.requestArm();                   // key repeat while already armed

    testing::internal::CaptureStdout();
    for (int i = 0; i < 18; ++i) { tested_app.update(); }
    const std::string tested_output = testing::internal::GetCapturedStdout();

    testing::internal::CaptureStdout();
    for (int i = 0; i < 18; ++i) { reference_app.update(); }
    const std::string reference_output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(tested_app.armed());
    EXPECT_EQ(countOccurrences("\n" + tested_output, "\nARMED angle="), 0);
    EXPECT_EQ(countOccurrences(tested_output, "bal="), 1);     // 2 + 18 = one full window
    EXPECT_EQ(tested_output, reference_output);
    EXPECT_EQ(tested_motor.last_left, reference_motor.last_left);
}

TEST(AppControlTest, ArmingSampleSeesOnlyItsOwnCounts)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(0.0f, 0.0f, 0.0f),
                   VelocityPI(100.0f, 0.0f, 100000.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.accumulate_encoders = true;
    sensor.counts_per_poll_l = 100;
    sensor.counts_per_poll_r = 100;
    sensor.angle = 5.0f;

    for (int i = 0; i < 50; ++i)
    {
        app.update();                          // disarmed; wheels turning (e.g. pushed by hand)
    }

    EXPECT_EQ(sensor.encoder_left_reads, 50);  // disarmed samples still drain

    app.requestArm();
    app.update();                              // arming sample: one sample's 100 counts

    EXPECT_TRUE(app.armed());
    EXPECT_GT(motor.last_left, 0);
    EXPECT_LT(motor.last_left, 100);           // ~32 for one sample; ~1632 with a 5000 backlog
}

TEST(AppControlTest, ArmRejectedWhileFaulted)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 45.0f;
    sensor.fresh = true;
    app.update();                              // FAULT

    testing::internal::CaptureStdout();
    sensor.angle = 20.0f;                      // > 10 deg hysteresis limit while faulted
    app.requestArm();
    app.update();
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "ARM REJECTED reason=angle"), 1);
    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, AngleFaultDisarms)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.requestArm();
    app.update();
    ASSERT_TRUE(app.armed());
    ASSERT_EQ(motor.last_left, 10);

    testing::internal::CaptureStdout();
    sensor.angle = 45.0f;
    app.update();                              // faulted: disarms
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "DISARMED reason=angle"), 1);
    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, StaleDisarms)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(0.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    sensor.fresh = true;
    app.requestArm();
    app.update();
    ASSERT_TRUE(app.armed());
    ASSERT_EQ(motor.last_left, 10);

    testing::internal::CaptureStdout();
    sensor.fresh = false;
    clock.advance(25);
    app.update();                              // deadline: disarms
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countOccurrences(output, "DISARMED reason=stale"), 1);
    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, DefaultZeroTargetPreservesStraightMixAtZeroYaw)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(2.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    app.requestArm();
    app.update();

    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(motor.last_right, 10);
}

TEST(AppControlTest, PositiveTurnEffortCreatesOppositeWheelDifferential)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(2.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    app.requestArm();
    app.update(0.0f, 10.0f);                   // turn = +20: left/CCW

    EXPECT_EQ(motor.last_left, -10);           // right wheel forward of left
    EXPECT_EQ(motor.last_right, 30);
}

TEST(AppControlTest, TurnDoesNotAlterPreClampCommonSum)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(2.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    sensor.yaw_rate_dps = 5.0f;                // spinning CCW at zero target: turn = -10
    app.requestArm();
    app.update();

    EXPECT_EQ(motor.last_left + motor.last_right, 20);
    EXPECT_EQ(motor.last_right - motor.last_left, -20);
}

TEST(AppControlTest, DisarmedTargetCannotDrive)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(10.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    app.update(0.0f, 30.0f);                   // never armed

    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, AngleFaultBeatsTurnTarget)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(10.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    app.requestArm();
    app.update(0.0f, 30.0f);
    ASSERT_TRUE(app.armed());

    sensor.angle = 45.0f;
    app.update(0.0f, 30.0f);

    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, StaleFaultBeatsTurnTarget)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(10.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    app.requestArm();
    app.update(0.0f, 30.0f);
    ASSERT_TRUE(app.armed());

    sensor.fresh = false;
    clock.advance(25);
    app.update(0.0f, 30.0f);                   // deadline: disarms

    EXPECT_FALSE(app.armed());
    EXPECT_EQ(motor.last_left, 0);
    EXPECT_EQ(motor.last_right, 0);
}

TEST(AppControlTest, TargetAndEffortLimitsReachAppMix)
{
    MockSensorHal sensor;
    MockMotorHal motor;
    MockMonotonicClock clock;

    AppControl app(sensor, motor, clock,
                   BalancePD(200.0f, 0.0f, 0.0f),
                   VelocityPI(0.0f, 0.0f, 200.0f),
                   YawRateP(10.0f, 30.0f, 100));

    sensor.angle = 5.0f;
    app.requestArm();

    app.update(0.0f, 50.0f);                   // target clamps to 30, effort clamps to 100
    EXPECT_EQ(motor.last_left, -90);
    EXPECT_EQ(motor.last_right, 110);

    app.update(0.0f, -50.0f);
    EXPECT_EQ(motor.last_left, 110);
    EXPECT_EQ(motor.last_right, -90);
}














