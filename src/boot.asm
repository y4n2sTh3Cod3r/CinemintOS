; boot.asm - Multiboot-compliant bootloader

section .multiboot
align 4
    ; Multiboot header
    dd 0x1BADB002            ; Magic number
    dd 0x00000003            ; Flags: align modules on page boundaries and provide memory map
    dd -(0x1BADB002+0x00000003) ; Checksum

section .bss
align 16
stack_bottom:
    resb 16384               ; 16 KiB stack
stack_top:

section .text
global _start
extern kernel_main

_start:
    ; Set up the stack
    mov esp, stack_top
    
    ; Call the kernel main function
    call kernel_main
    
    ; If the kernel returns, hang the CPU
hang:
    cli                     ; Disable interrupts
    hlt                     ; Halt the CPU
    jmp hang                ; Just in case
