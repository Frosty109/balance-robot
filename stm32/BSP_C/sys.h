#ifndef SYS_H
#define SYS_H

#include "stm32f10x.h"

// SWJ_CFG selector values for JTAG_Set(), written into AFIO_MAPR[26:24].
// Disabling JTAG frees PA15/PB3/PB4 as GPIO while keeping SWD for the ST-Link.
#define JTAG_SWD_DISABLE   0x02   // both JTAG and SWD off
#define SWD_ENABLE         0x01   // JTAG off, SWD on  (debug-friendly default)
#define JTAG_SWD_ENABLE    0x00   // full JTAG + SWD (reset state)

#ifdef __cplusplus
extern "C" {
#endif

void JTAG_Set(uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif
