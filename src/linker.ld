/* linker.ld - Linker script for the kernel */

ENTRY(_start)

SECTIONS {
    . = 1M;                     /* Load kernel at 1 MiB, a conventional place for kernels */
    
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)           /* Put multiboot header first */
        *(.text)                /* All code sections from all files */
    }
    
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)              /* Read-only data sections */
    }
    
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)                /* Initialized data sections */
    }
    
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)               /* Common symbols */
        *(.bss)                 /* Uninitialized data sections */
    }
}
