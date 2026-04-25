#pragma once 

#include <cstdint>

class Imu
{
public:
    Imu();
    bool init();
    void read();

    float getPitch() const;
    float getRoll()  const;
    float getYaw()   const;
    short getGyroX() const;
    short getGyroZ() const;

private:
    float pitch_, roll_, yaw_;
    float q0_, q1_, q2_, q3_;
    short gyro_[3];
    short accel_[3];

    static constexpr float      q30 {1073741824.0f};
    static constexpr int        DEFAULT_MPU_HZ {200};
    static constexpr uint8_t    DEV_ADDR {0x68};
    static constexpr uint8_t    WHO_AM_I_REG {0x75};
    static constexpr uint8_t    WHO_AM_I_VAL {0x68};

    static const signed char GYRO_ORIENTATION[9];
    
    static unsigned short inv_row_2_scale(const signed char* row);
    static unsigned short inv_orientation_matrix_to_scalar (const signed char* mtx);
    static void run_self_test();
};