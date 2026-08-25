# STM32F103RCT6 firmware target. Configured only during a cross build:
#   cmake -B build-fw -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
# Produces firmware.elf + .hex + .bin and prints flash/RAM usage.

set(ROOT ${CMAKE_SOURCE_DIR})

# Whole FWLib is vendored; -Wl,--gc-sections drops unused drivers at link time,
# so globbing all 23 costs compile time only, not flash.
file(GLOB FWLIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/FWLib/src/*.c)

add_executable(firmware.elf
    # shared application + STM32 HAL (same src/ .cpp the sim compiles)
    ${ROOT}/src/pid/pid_control.cpp
    ${ROOT}/src/app/app_control.cpp
    ${ROOT}/hal/stm32/stm32_sensor_hal.cpp
    ${ROOT}/hal/stm32/stm32_motor_hal.cpp
    ${ROOT}/hal/stm32/stm32_monotonic_clock.cpp
    # BSP C++ classes
    ${ROOT}/bsp/motor/motor.cpp
    ${ROOT}/bsp/encoder/encoder.cpp
    ${ROOT}/bsp/mpu6050/imu.cpp
    ${ROOT}/bsp/battery/battery.cpp
    ${ROOT}/bsp/usart/usart.cpp
    ${ROOT}/bsp/clock/tim2_millisecond_timer.cpp
    # entry point + interrupt vector handlers
    USER/main.cpp
    USER/stm32f10x_it.c
    # owned C helpers + vendored IMU/DMP/I2C/delay C drivers
    BSP_C/sys.c
    BSP_C/delay.c
    BSP_C/IOI2C.c
    BSP_C/mpu6050.c
    BSP_C/DMP/inv_mpu.c
    BSP_C/DMP/inv_mpu_dmp_motion_driver.c
    # CMSIS system + startup assembly.
    # core_cm3.c is deliberately NOT compiled: it's a CMSIS-1.x relic whose out-of-line
    # GCC __STREXB/__STREXH inline asm is rejected by modern binutils (strex needs distinct
    # registers). Nothing references those exclusive-access intrinsics, and the core helpers
    # that ARE used are static-inline in core_cm3.h — so the .c is pure dead weight here.
    CMSIS/system_stm32f10x.c
    CMSIS/startup_stm32f10x_hd_gcc.s

    ${FWLIB_SRC}
)

target_compile_definitions(firmware.elf PRIVATE
    STM32F10X_HD            # high-density part — selects the right register map
    USE_STDPERIPH_DRIVER    # makes stm32f10x.h pull in stm32f10x_conf.h (the FWLib)
)

target_include_directories(firmware.elf PRIVATE
    ${ROOT}                                    # src/, hal/, bsp/ headers
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS
    ${CMAKE_CURRENT_SOURCE_DIR}/FWLib/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP_C
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP_C/DMP
    ${CMAKE_CURRENT_SOURCE_DIR}/USER
)

# Bare-metal C++: no exception unwinding or RTTI runtime (smaller image, no
# surprise libstdc++ pulls). Guarded to C++ only — these flags are invalid for C.
target_compile_options(firmware.elf PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>
)

target_link_options(firmware.elf PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/STM32F103RCT6_gcc.ld   # memory map: flash@0x08000000, RAM@0x20000000
    --specs=nano.specs      # newlib-nano: tiny libc for embedded
    --specs=nosys.specs     # stub out syscalls (no OS) — _write etc. become weak stubs
    -Wl,--gc-sections       # drop unused sections (pairs with -ffunction/-fdata-sections)
    -Wl,-Map=firmware.map   # linker map — useful for seeing what landed where
)

# elf -> hex + bin, then print flash/RAM usage after every build
add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:firmware.elf> firmware.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:firmware.elf> firmware.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:firmware.elf>
    COMMENT "Generating firmware.hex + firmware.bin, printing size"
)

add_custom_target(flash
    COMMAND openocd -f ${CMAKE_SOURCE_DIR}/stm32/openocd.cfg
        -c "program $<TARGET_FILE:firmware.elf> verify reset exit"
    DEPENDS firmware.elf
    USES_TERMINAL
    COMMENT "Flashing firmware via OpenOCD / ST-Link")
