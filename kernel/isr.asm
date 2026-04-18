; =============================================================================
; ISR (Interrupt Service Routine) Stubs
; ELF32 version — no underscore prefixes
; =============================================================================

[BITS 32]
section .text

extern isr_handler
extern irq_handler

; Macro: ISR without error code (CPU doesn't push one)
%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push dword 0        ; dummy error code
    push dword %1       ; interrupt number
    jmp isr_common_stub
%endmacro

; Macro: ISR with error code (CPU already pushed it)
%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push dword %1       ; interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; Macro: Hardware IRQ stub
%macro IRQ 2
global irq%1
irq%1:
    cli
    push dword 0        ; dummy error code
    push dword %2       ; interrupt number (32 + IRQ line)
    jmp irq_common_stub
%endmacro

; =============================================================================
; CPU Exceptions 0–31
; =============================================================================
ISR_NOERR 0    ; Division by Zero
ISR_NOERR 1    ; Debug
ISR_NOERR 2    ; Non Maskable Interrupt
ISR_NOERR 3    ; Breakpoint
ISR_NOERR 4    ; Overflow
ISR_NOERR 5    ; Bound Range Exceeded
ISR_NOERR 6    ; Invalid Opcode
ISR_NOERR 7    ; Device Not Available
ISR_ERR   8    ; Double Fault          (CPU pushes error code)
ISR_NOERR 9    ; Coprocessor Segment Overrun
ISR_ERR   10   ; Invalid TSS           (CPU pushes error code)
ISR_ERR   11   ; Segment Not Present   (CPU pushes error code)
ISR_ERR   12   ; Stack Fault           (CPU pushes error code)
ISR_ERR   13   ; General Protection    (CPU pushes error code)
ISR_ERR   14   ; Page Fault            (CPU pushes error code)
ISR_NOERR 15   ; Reserved
ISR_NOERR 16   ; x87 FPU Error
ISR_ERR   17   ; Alignment Check       (CPU pushes error code)
ISR_NOERR 18   ; Machine Check
ISR_NOERR 19   ; SIMD FPU Exception
ISR_NOERR 20   ; Virtualization Exception
ISR_NOERR 21   ; Control Protection Exception
ISR_NOERR 22   ; Reserved
ISR_NOERR 23   ; Reserved
ISR_NOERR 24   ; Reserved
ISR_NOERR 25   ; Reserved
ISR_NOERR 26   ; Reserved
ISR_NOERR 27   ; Reserved
ISR_NOERR 28   ; Reserved
ISR_NOERR 29   ; Reserved
ISR_NOERR 30   ; Security Exception
ISR_NOERR 31   ; Reserved

; =============================================================================
; Hardware IRQs 0–15  (remapped to IDT vectors 32–47)
; =============================================================================
IRQ  0, 32     ; Timer
IRQ  1, 33     ; Keyboard
IRQ  2, 34     ; Cascade (PIC2)
IRQ  3, 35     ; COM2
IRQ  4, 36     ; COM1
IRQ  5, 37     ; LPT2
IRQ  6, 38     ; Floppy
IRQ  7, 39     ; LPT1 / Spurious
IRQ  8, 40     ; CMOS RTC
IRQ  9, 41     ; Free / ACPI
IRQ 10, 42     ; Free
IRQ 11, 43     ; Free
IRQ 12, 44     ; PS/2 Mouse
IRQ 13, 45     ; FPU Coprocessor
IRQ 14, 46     ; Primary ATA
IRQ 15, 47     ; Secondary ATA

; =============================================================================
; Common ISR stub — saves full CPU state, calls C isr_handler
; =============================================================================
global isr_common_stub
isr_common_stub:
    pusha               ; push eax ecx edx ebx esp ebp esi edi

    push ds
    push es
    push fs
    push gs

    mov ax, 0x10        ; kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; pointer to saved-registers struct
    call isr_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds

    popa
    add esp, 8          ; discard int_no + err_code
    sti
    iret

; =============================================================================
; Common IRQ stub — saves full CPU state, calls C irq_handler
; =============================================================================
global irq_common_stub
irq_common_stub:
    pusha

    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds

    popa
    add esp, 8
    sti
    iret