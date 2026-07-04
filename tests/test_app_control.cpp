#include "src/app/app_control.hpp"
#include "src/pid/pid_control.hpp"
#include "hal/interface/i_sensor_hal.hpp"
#include "hal/interface/i_motor_hal.hpp"
#include <gtest/gtest.h>

class MockSensorHal : public ISensorHal
{
public:
    float angle     {0.0f};
    float battery   {12.0f};
    float gyro      {0.0f};
    float gyro_z    {0.0f};
    int   enc_l     {0};
    int   enc_r     {0};

    float getAngle()        override { return angle; }
    float getBattery()      override { return battery; }
    float getGyroBalance()  override { return gyro; }
    float getGyroTurn()     override { return gyro_z; }
    float getAccelZ()       override { return 0.0f; }
    int   getEncoderLeft()  override { return enc_l; }
    int   getEncoderRight() override { return enc_r; }
    void  poll()            override {}
};

class MockMotorHal : public IMotorHal
{
public:
    int last_left   {0};
    int last_right  {0};

    void setMotorPWM(int left, int right) override
    {
        last_left   = left;
        last_right  = right;
    }
};

TEST(AppControlTest, SafetyCutoffOnHighAngle)
{
    MockSensorHal sensor;
    MockMotorHal  motor;
    AppControl    app(sensor, motor,
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
    AppControl    app(sensor, motor, 
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
    AppControl    app(sensor, motor, 
                    BalancePD(200.0f, 0.8f, 0.0f),
                    VelocityPI(1.2f, 0.05f, 200.0f),
                    TurnPD(5.0f, 0.1f));

    sensor.angle    = 5.0f;
    sensor.battery  = 12.0f;

    app.update();

    EXPECT_EQ(motor.last_left, 10);
    EXPECT_EQ(motor.last_right, 10);
}