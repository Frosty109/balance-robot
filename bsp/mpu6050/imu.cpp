#include <cmath>

#include "imu.hpp"

extern "C" {
    #include "stm32f10x.h"
    #include "mpu6050.h"
    #include "DMP/inv_mpu.h"
    #include "DMP/inv_mpu_dmp_motion_driver.h"
}

Imu::Imu()
    : pitch_(0.0f), roll_(0.0f), yaw_(0.0f)
    , q0_(1.0f), q1_(0.0f), q2_(0.0f), q3_(0.0f)
    , gyro_{0, 0, 0}, accel_{0, 0, 0}
{}

const signed char Imu::GYRO_ORIENTATION[9] = {-1, 0, 0,
                                           0,-1, 0,
                                           0, 0, 1};

bool Imu::init()
{
    IIC_MPU6050_Init();

    MPU6050_initialize();

    uint8_t temp[1] {0};
    i2cRead(DEV_ADDR, WHO_AM_I_REG, 1, temp);
    if (temp[0] != WHO_AM_I_VAL) return false;

    if (mpu_init()) return false;

    if(mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL))        return false;
    if(mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL))     return false;
    if(mpu_set_sample_rate(DEFAULT_MPU_HZ))                    return false;
    if(dmp_load_motion_driver_firmware())                      return false;
    if(dmp_set_orientation(
        inv_orientation_matrix_to_scalar(GYRO_ORIENTATION)))   return false;
    if(dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT |
        DMP_FEATURE_TAP | DMP_FEATURE_ANDROID_ORIENT |
        DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO |
        DMP_FEATURE_GYRO_CAL))                                 return false;
    if(dmp_set_fifo_rate(DEFAULT_MPU_HZ))                      return false;
    run_self_test();
    if(mpu_set_dmp_state(1))                                   return false;

    return true;

}

bool Imu::read()
{
    short gyro[3] {};
    short accel[3] {};
    long quat[4] {};

    unsigned long sensor_timestamp {};
    unsigned char more {};
    short sensors {};

    const int result = dmp_read_fifo(
            gyro,
            accel,
            quat,
            &sensor_timestamp,
            &sensors,
            &more
        );

    constexpr short REQUIRED_SENSORS = INV_WXYZ_QUAT | INV_XYZ_GYRO | INV_XYZ_ACCEL;

    if (result != 0 || (sensors & REQUIRED_SENSORS) != REQUIRED_SENSORS)
    {
        return false;
    }

    const float q0 = quat[0] / q30;
    const float q1 = quat[1] / q30;
    const float q2 = quat[2] / q30;
    const float q3 = quat[3] / q30;

    const float roll  =
        asin(-2 * q1 * q3 + 2 * q0 * q2) * 57.3f;

    const float pitch =
        atan2(2 * q2 * q3 + 2 * q0 * q1,
             -2 * q1 * q1 - 2 * q2 * q2 + 1) * 57.3f;

    const float yaw =
        atan2(2 * (q1 * q2 + q0 * q3),
              q0*q0 + q1*q1 - q2*q2 - q3*q3) * 57.3f;

    q0_ = q0;
    q1_ = q1;
    q2_ = q2;
    q3_ = q3;

    roll_ = roll;
    pitch_ = pitch;
    yaw_ = yaw;

    gyro_[0] = gyro[0];
    gyro_[1] = gyro[1];
    gyro_[2] = gyro[2];

    accel_[0] = accel[0];
    accel_[1] = accel[1];
    accel_[2] = accel[2];
    return true;
}

float Imu::getPitch() const { return pitch_; }
float Imu::getRoll()  const { return roll_;  }
float Imu::getYaw()   const { return yaw_;   }
short Imu::getGyroX() const { return gyro_[0]; }
short Imu::getGyroZ() const { return gyro_[2]; }

float Imu::getAccelZ() const { return accel_[2]; }

unsigned short Imu::inv_row_2_scale(const signed char* row)
{
    if      (row[0] > 0) return 0;
    else if (row[0] < 0) return 4;
    else if (row[1] > 0) return 1;
    else if (row[1] < 0) return 5;
    else if (row[2] > 0) return 2;
    else if (row[2] < 0) return 6;
    else                 return 7;
}

unsigned short Imu::inv_orientation_matrix_to_scalar(const signed char* mtx)
{
    unsigned short scalar { inv_row_2_scale(mtx) };
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;
    return scalar;
}

void Imu::run_self_test()
{

    long gyro[3] {};
    long accel[3] {};
    const int result { mpu_run_self_test(gyro, accel) } ;

    if (result == 0x7) {
        float sens {};
        unsigned short accel_sens {};

        mpu_get_gyro_sens(&sens);

        gyro[0] = (long)(gyro[0] * sens);
        gyro[1] = (long)(gyro[1] * sens);
        gyro[2] = (long)(gyro[2] * sens);

        dmp_set_gyro_bias(gyro);
        mpu_get_accel_sens(&accel_sens);

        accel[0] *= accel_sens;
        accel[1] *= accel_sens;
        accel[2] *= accel_sens;

        dmp_set_accel_bias(accel);
    }
}
