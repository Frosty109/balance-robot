/* Linker script for STM32F103RCT6
 * Flash: 256KB at 0x08000000
 * SRAM:   48KB at 0x20000000
 *
 * Based on ST standard peripheral library GCC linker script template.
 */

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);   /* top of stack = end of RAM */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 48K
}

SECTIONS
{
  /* Interrupt vector table — must be first in flash */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* Code and read-only data */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  /* Load address of .data in flash — startup copies it to RAM */
  _sidata = LOADADDR(.data);

  /* Initialised variables: stored in flash, copied to RAM on startup */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT>FLASH

  /* Zero-initialised variables: cleared to 0 by startup */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } >RAM

  /* Verify heap + stack fit in RAM */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + 0x400;   /* 1KB minimum heap  */
    . = . + 0x400;   /* 1KB minimum stack */
    . = ALIGN(8);
  } >RAM
}
