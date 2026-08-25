#include "src/app/app_control.hpp"
#include "src/pid/pid_control.hpp"
#include "hal/interface/i_sensor_hal.hpp"
#include "hal/interface/i_motor_hal.hpp"
#include <gtest/gtest.h>
#include <string>

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
