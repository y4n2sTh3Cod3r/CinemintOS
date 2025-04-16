; boot.asm - Multiboot-compliant bootloader for 32-bit mode

section .multiboot
align 4
    ; Multiboot header
    dd 0x1BADB002                ; Magic number
    dd 0x00000003                ; Flags: align modules on page boundaries and provide memory map
    dd -(0x1BADB002+0x00000003)  ; Checksum

section .data
align 4
mboot_info_ptr:
    dd 0                         ; Store multiboot info pointer here

section .bss
align 16
stack_bottom:
    resb 16384                   ; 16 KiB stack
stack_top:

section .text
global _start
global mboot_info_ptr            ; Export memory info to C++
extern main

_start:
    ; Save multiboot info pointer passed in ebx by GRUB
    mov [mboot_info_ptr], ebx
    
    ; Set up the stack
    mov esp, stack_top
    
    ; Make sure interrupts are disabled
    cli
    
    ; Load GDT (Global Descriptor Table) for 32-bit mode
    lgdt [gdt_descriptor]
    
    ; Update segment registers
    jmp 0x08:reload_segments     ; Far jump to code segment
    
reload_segments:
    mov ax, 0x10                 ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Enable A20 line (might already be enabled by bootloader)
    in al, 0x92
    or al, 2
    out 0x92, al
    
    ; Call the kernel main function
    push mboot_info_ptr          ; Pass multiboot info pointer to kernel
    call main
    
    ; If the kernel returns, hang the CPU
hang:
    cli                          ; Disable interrupts
    hlt                          ; Halt the CPU
    jmp hang                     ; Just in case

; GDT (Global Descriptor Table)
align 8
gdt:
    ; Null descriptor
    dq 0

    ; Code segment descriptor
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0            ; Base (bits 0-15)
    db 0            ; Base (bits 16-23)
    db 10011010b    ; Access byte: Present, Ring 0, Code Segment, Executable, Direction 0, Readable
    db 11001111b    ; Flags + Limit (bits 16-19): Granularity 1, 32-bit, Limit (bits 16-19)
    db 0            ; Base (bits 24-31)

    ; Data segment descriptor
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0            ; Base (bits 0-15)
    db 0            ; Base (bits 16-23)
    db 10010010b    ; Access byte: Present, Ring 0, Data Segment, Not Executable, Direction 0, Writable
    db 11001111b    ; Flags + Limit (bits 16-19): Granularity 1, 32-bit, Limit (bits 16-19)
    db 0            ; Base (bits 24-31)

gdt_descriptor:
    dw gdt_descriptor - gdt - 1  ; GDT size (minus 1)
    dd gdt                       ; GDT address