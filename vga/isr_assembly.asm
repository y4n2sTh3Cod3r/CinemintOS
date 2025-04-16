; isr_assembly.asm - Assembly interrupt service routines for timer

section .text
global isr_timer_wrapper     ; Make the ISR handler visible to C code
global load_idt              ; Make IDT loader visible to C code
extern isr_timer_handler     ; Reference to the C handler function
extern idtp                  ; Reference to IDT pointer structure

; ISR for Timer (IRQ0)
isr_timer_wrapper:
    pusha                    ; Push all registers
    call isr_timer_handler   ; Call our C handler
    popa                     ; Pop all registers
    iret                     ; Return from interrupt

; Load IDT function
load_idt:
    lidt [idtp]              ; Load the IDT pointer
    ret                      ; Return to caller