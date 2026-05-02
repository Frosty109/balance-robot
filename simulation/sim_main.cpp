#include <iostream>
#include <thread>
#include <chrono>

#include "physics.hpp"
#include "../hal/sim/sim_sensor_hal.hpp"
#include "../hal/sim/sim_motor_hal.hpp"
#include "../src/app/app_control.hpp"

static constexpr float DT   { 0.005f }; // 200Hz - which matches STM32
static constexpr int STEPS  { 10000 }; // 50s

int main()
{
    Physics      physics;
    SimSensorHal sensor_hal(physics);
    SimMotorHal  motor_hal(physics);

    AppControl app(sensor_hal, motor_hal,
                    BalancePD(200.0f, 0.8f, 0.0f),
                    VelocityPI(1.2f, 0.05f, 200.0f),
                    TurnPD(5.0f, 0.1f));

    for (int i {0}; i < STEPS; ++i)
    {
        physics.update(DT);
        app.update();

        std::cout << "/*"
                  << physics.getPitch() << ","
                  << physics.getPitchRate() << ","
                  << physics.getVelocity()
                  << "*/\n";
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    return 0;
}