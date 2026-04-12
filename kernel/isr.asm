; =============================================================================
; ISR (Interrupt Service Routine) Stubs
; These are low-level assembly stubs that save CPU state and call C handlers
; =============================================================================

[BITS 32]
section .text

extern _isr_handler
extern _irq_handler

; Macro to create ISR stub without error code
%macro ISR_NOERR 1
global _isr%1
_isr%1:
    cli
    push dword 0              ; Dummy error code
    push dword %1             ; Interrupt number
    jmp isr_common_stub
%endmacro

; Macro to create ISR stub with error code
%macro ISR_ERR 1
global _isr%1
_isr%1:
    cli
    push dword %1             ; Interrupt number (error code already pushed by CPU)
    jmp isr_common_stub
%endmacro

; Macro to create IRQ stub
%macro IRQ 2
global _irq%1
_irq%1:
    cli
    push dword 0              ; Dummy error code
    push dword %2             ; Interrupt number (32 + IRQ)
    jmp irq_common_stub
%endmacro

; =============================================================================
; CPU Exceptions (0-31)
; =============================================================================
ISR_NOERR 0    ; Division by Zero
ISR_NOERR 1    ; Debug
ISR_NOERR 2    ; Non Maskable Interrupt
ISR_NOERR 3    ; Breakpoint
ISR_NOERR 4    ; Overflow
ISR_NOERR 5    ; Bound Range Exceeded
ISR_NOERR 6    ; Invalid Opcode
ISR_NOERR 7    ; Device Not Available
ISR_ERR   8    ; Double Fault (has error code)
ISR_NOERR 9    ; Coprocessor Segment Overrun
ISR_ERR   10   ; Invalid TSS (has error code)
ISR_ERR   11   ; Segment Not Present (has error code)
ISR_ERR   12   ; Stack Fault (has error code)
ISR_ERR   13   ; General Protection Fault (has error code)
ISR_ERR   14   ; Page Fault (has error code)
ISR_NOERR 15   ; Reserved
ISR_NOERR 16   ; x87 FPU Error
ISR_NOERR 17   ; Alignment Check (has error code, but we treat as noerr)
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
; Hardware IRQs (32-47, remapped from IRQ0-IRQ15)
; =============================================================================
IRQ 0, 32      ; Timer
IRQ 1, 33      ; Keyboard
IRQ 2, 34      ; Cascade (PIC2)
IRQ 3, 35      ; COM2
IRQ 4, 36      ; COM1
IRQ 5, 37      ; LPT2
IRQ 6, 38      ; Floppy Disk
IRQ 7, 39      ; LPT1 / Spurious
IRQ 8, 40      ; CMOS RTC
IRQ 9, 41      ; Free / ACPI
IRQ 10, 42     ; Free
IRQ 11, 43     ; Free
IRQ 12, 44     ; PS/2 Mouse
IRQ 13, 45     ; FPU / Coprocessor
IRQ 14, 46     ; Primary ATA
IRQ 15, 47     ; Secondary ATA

; =============================================================================
; Common ISR handler - saves all registers and calls C handler
; =============================================================================
global isr_common_stub
isr_common_stub:
    pusha                   ; Push all general purpose registers
    
    push ds
    push es
    push fs
    push gs
    
    mov ax, 0x10            ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp                ; Push pointer to registers struct
    call _isr_handler       ; Call C handler
    add esp, 4
    
    pop gs
    pop fs
    pop es
    pop ds
    popa
    
    add esp, 8              ; Remove error code and int_no
    sti
    iret

; =============================================================================
; Common IRQ handler - saves all registers and calls C handler
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
    call _irq_handler
    add esp, 4
    
    pop gs
    pop fs
    pop es
    pop ds
    popa
    
    add esp, 8
    sti
    iret
