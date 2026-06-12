#include "sys.h"

// Remap the debug port. `mode` (one of the *_DISABLE/*_ENABLE macros) lands in
// the SWJ_CFG field, AFIO_MAPR bits [26:24]. The macro values are pre-shifted by
// one, so a left shift of 25 (not 24) places them correctly: SWD_ENABLE (0x01)
// -> 0b010 = JTAG off / SWD on; JTAG_SWD_DISABLE (0x02) -> 0b100 = both off.
void JTAG_Set(uint8_t mode)
{
    uint32_t temp = (uint32_t)mode << 25;
    RCC->APB2ENR |= 1 << 0;     // AFIO clock must be on before writing AFIO->MAPR
    AFIO->MAPR &= 0xF8FFFFFF;    // clear SWJ_CFG[26:24]
    AFIO->MAPR |= temp;
}
