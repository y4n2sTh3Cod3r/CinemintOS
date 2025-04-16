/* Linker script for the kernel */
ENTRY(_start)

SECTIONS {
    /* Start at 1MB (conventional for kernels) */
    . = 1M;

    /* Multiboot header first for GRUB/bootloader */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }

    /* Read-only data */
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }

    /* End marker for the kernel */
    end = .;
}