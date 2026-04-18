/* Linker script for MyOS kernel */

ENTRY(kernel_entry)

SECTIONS
{
    /* Kernel loads at 1MB */
    . = 0x100000;
    
    /* Code section */
    .text :
    {
        *(.text.startup)
        *(.text*)
    }
    
    /* Read-only data */
    .rodata :
    {
        *(.rodata*)
    }
    
    /* Initialized data */
    .data :
    {
        *(.data*)
    }
    
    /* Uninitialized data (BSS) */
    .bss : {
        _bss_start = .;
        *(COMMON)
        *(.bss*)
        _bss_end = .;
    }
    
    /* Discard unwanted sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
    }
}
