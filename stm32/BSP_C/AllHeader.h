#ifndef ALLHEADER_H
#define ALLHEADER_H

// Replacement for Yahboom's AllHeader.h
// Scoped to just MPU6050 + DMP + software-I2C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "stm32f10x.h"
#include "delay.h"
#include "IOI2C.h"
#include "mpu6050.h"
#include "DMP/inv_mpu.h"
#include "DMP/inv_mpu_dmp_motion_driver.h"
#include "DMP/dmpKey.h"   
#include "DMP/dmpmap.h"


#endif