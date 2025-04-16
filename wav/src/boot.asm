; boot.asm - Multiboot-compliant kernel entry point
; Multiboot constants
MBALIGN     equ 1 << 0              ; Align loaded modules on page boundaries
MEMINFO     equ 1 << 1              ; Provide memory map
FLAGS       equ MBALIGN | MEMINFO   ; Multiboot flag field
MAGIC       equ 0x1BADB002          ; Multiboot magic number
CHECKSUM    equ -(MAGIC + FLAGS)    ; Checksum to prove we are multiboot

; Multiboot header
section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; Reserve stack space
section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KiB stack
stack_top:

; Kernel entry point
section .text
global _start
extern kernel_main

_start:
    ; Set up stack
    mov esp, stack_top

    ; Save multiboot info pointer (passed in ebx)
    push ebx

    ; Call the kernel's C code
    call kernel_main

    ; In case kernel_main returns, hang the CPU
.hang:
    cli                         ; Disable interrupts
    hlt                         ; Halt the CPU
    jmp .hang                   ; Just in case hlt doesn't work