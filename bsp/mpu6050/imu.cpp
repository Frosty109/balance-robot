#include <cmath>

#include "imu.hpp"

extern "C" {
    #include "stm32f10x.h"
    #include "mpu6050.h"
    #include "DMP/inv_mpu.h"
    #include "DMP/inv_mpu_dmp_motion_driver.h"
}

Imu::Imu() {}

const signed char Imu::GYRO_ORIENTATION[9] = {-1, 0, 0,
                                           0,-1, 0,
                                           0, 0, 1};

bool Imu::init()
{
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

void Imu::read()
{
    unsigned long sensor_timestamp {};
    unsigned char more {};
    long quat[4];
    short sensors {};

    dmp_read_fifo(gyro_, accel_, quat, &sensor_timestamp, &sensors, &more);

    if (sensors & INV_WXYZ_QUAT) 
    {
        q0_ = quat[0] / q30;
        q1_ = quat[1] / q30;
        q2_ = quat[2] / q30;
        q3_ = quat[3] / q30;

        roll_  = asin(-2 * q1_ * q3_ + 2 * q0_ * q2_) * 57.3f;
        pitch_ = atan2(2 * q2_ * q3_ + 2 * q0_ * q1_,
                       -2 * q1_ * q1_ - 2 * q2_ * q2_ + 1) * 57.3f;
        yaw_   = atan2(2 * (q1_ * q2_ + q0_ * q3_),
                       q0_*q0_ + q1_*q1_ - q2_*q2_ - q3_*q3_) * 57.3f;
    }
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