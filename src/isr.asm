; isr.asm — ISR stubs for vectors 0-47
; NASM elf32
;
; Stack layout when common_isr_stub is reached:
;   [CPU pushed]  ss, useresp, eflags, cs, eip
;   [stub pushed] err_code (real or dummy 0)
;   [stub pushed] int_no   (vector number)
;
; common_isr_stub then pushes all GP regs + ds,
; then passes ESP (pointer to that frame) as the argument
; to common_isr_handler(registers_t*).

bits 32
section .text

extern common_isr_handler

global isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7
global isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15
global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
global isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39
global isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47

; ── Exceptions WITHOUT CPU error code ────────────────────────────────────────
; Push dummy 0 first, then vector number, then jump to common stub.
%macro ISR_NOERR 1
isr%1:
    push dword 0        ; dummy error code
    push dword %1       ; vector number
    jmp  common_isr_stub
%endmacro

; ── Exceptions WITH CPU error code ───────────────────────────────────────────
; CPU already pushed error code. Push only vector number.
%macro ISR_ERR 1
isr%1:
    push dword %1       ; vector number (error code already on stack)
    jmp  common_isr_stub
%endmacro

; CPU exceptions — which ones push an error code:
; 8 (DF), 10 (TS), 11 (NP), 12 (SS), 13 (GP), 14 (PF), 17 (AC), 21 (CP), 29 (VC), 30 (SX)
ISR_NOERR  0    ; Division Error
ISR_NOERR  1    ; Debug
ISR_NOERR  2    ; NMI
ISR_NOERR  3    ; Breakpoint
ISR_NOERR  4    ; Overflow
ISR_NOERR  5    ; Bound Range
ISR_NOERR  6    ; Invalid Opcode
ISR_NOERR  7    ; Device Not Available
ISR_ERR    8    ; Double Fault          (has error code)
ISR_NOERR  9    ; Coprocessor Overrun
ISR_ERR   10    ; Invalid TSS           (has error code)
ISR_ERR   11    ; Segment Not Present   (has error code)
ISR_ERR   12    ; Stack Fault           (has error code)
ISR_ERR   13    ; General Protection    (has error code)
ISR_ERR   14    ; Page Fault            (has error code)
ISR_NOERR 15    ; Reserved
ISR_NOERR 16    ; x87 FP Exception
ISR_ERR   17    ; Alignment Check       (has error code)
ISR_NOERR 18    ; Machine Check
ISR_NOERR 19    ; SIMD FP Exception
ISR_NOERR 20    ; Virtualization
ISR_ERR   21    ; Control Protection    (has error code)
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR   29    ; VMM Communication     (has error code)
ISR_ERR   30    ; Security Exception    (has error code)
ISR_NOERR 31

; Hardware IRQs — none push error codes
ISR_NOERR 32    ; IRQ0  — PIT Timer
ISR_NOERR 33    ; IRQ1  — PS/2 Keyboard
ISR_NOERR 34    ; IRQ2  — Cascade
ISR_NOERR 35    ; IRQ3  — COM2
ISR_NOERR 36    ; IRQ4  — COM1
ISR_NOERR 37    ; IRQ5
ISR_NOERR 38    ; IRQ6  — Floppy
ISR_NOERR 39    ; IRQ7  — LPT1 / spurious
ISR_NOERR 40    ; IRQ8  — RTC
ISR_NOERR 41    ; IRQ9
ISR_NOERR 42    ; IRQ10
ISR_NOERR 43    ; IRQ11
ISR_NOERR 44    ; IRQ12 — PS/2 Mouse
ISR_NOERR 45    ; IRQ13 — FPU
ISR_NOERR 46    ; IRQ14 — Primary ATA
ISR_NOERR 47    ; IRQ15 — Secondary ATA / spurious

; ── Common stub ──────────────────────────────────────────────────────────────
; On entry the stack (from top = low address) looks like:
;   int_no   ← ESP points here
;   err_code
;   eip
;   cs
;   eflags
;   (useresp, ss — only if privilege change, not in ring-0 kernel)
;
; We build a registers_t struct on the stack, then pass a pointer to it.

common_isr_stub:
    pusha               ; push eax,ecx,edx,ebx,esp,ebp,esi,edi (8 dwords)

    mov  ax, ds
    push eax            ; save ds

    ; Switch all data segments to kernel data selector 0x10
    mov  ax, 0x10
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax

    ; Pass pointer to the register frame as the argument
    push esp            ; arg: registers_t*
    call common_isr_handler
    add  esp, 4         ; pop the argument

    ; Restore data segments
    pop  eax
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax

    popa                ; restore GP registers
    add  esp, 8         ; discard int_no and err_code
    iret
