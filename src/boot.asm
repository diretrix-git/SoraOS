; boot.asm — GRUB Multiboot entry point
; Assembled with: nasm -f elf32 boot.asm -o boot.o

bits 32

MULTIBOOT_MAGIC            equ 0x1BADB002
MULTIBOOT_FLAGS            equ 0x0
MULTIBOOT_CHECKSUM         equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)
MULTIBOOT_BOOTLOADER_MAGIC equ 0x2BADB002

STACK_SIZE equ 0x4000          ; 16 KB initial stack

; ── .multiboot section ────────────────────────────────────────────────────
; Placed at 0x100000 by linker.ld — contains header + entry code.
; ENTRY(kernel_start) points here so QEMU jumps straight to our code.
section .multiboot
align 4

; 12-byte Multiboot header (magic, flags, checksum)
dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS
dd MULTIBOOT_CHECKSUM

; ── kernel_start ──────────────────────────────────────────────────────────
global kernel_start
extern kernel_main

kernel_start:
    ; ── 1. Validate Multiboot magic in EAX ──────────────────────────────
    cmp eax, MULTIBOOT_BOOTLOADER_MAGIC
    jne .bad_magic

    ; ── 2. Set up stack ──────────────────────────────────────────────────
    mov esp, stack_top

    ; ── 3. Guaranteed VGA test — write 'OK' directly to 0xB8000 ─────────
    ; This bypasses ALL C code. If you see green 'OK' the CPU is running.
    mov edi, 0xB8000
    mov word [edi],      0x2F4F   ; 'O' white-on-green  (attr=0x2F)
    mov word [edi + 2],  0x2F4B   ; 'K' white-on-green
    mov word [edi + 4],  0x2F20   ; ' '
    mov word [edi + 6],  0x2F42   ; 'B'
    mov word [edi + 8],  0x2F4F   ; 'O'
    mov word [edi + 10], 0x2F4F   ; 'O'
    mov word [edi + 12], 0x2F54   ; 'T'

    ; ── 4. Call kernel_main(multiboot_info_t* mb) ────────────────────────
    push ebx                      ; EBX = Multiboot info pointer (arg 1)
    call kernel_main

    ; ── 5. kernel_main returned — halt ───────────────────────────────────
.halt:
    cli
    hlt
    jmp .halt

.bad_magic:
    ; Write 'BAD' in red to VGA so we know magic check failed
    mov edi, 0xB8000
    mov word [edi],     0x4C42    ; 'B' white-on-red
    mov word [edi + 2], 0x4C41   ; 'A'
    mov word [edi + 4], 0x4C44   ; 'D'
    jmp .halt

; ── .bss section — stack storage ──────────────────────────────────────────
section .bss
align 16
stack_bottom:
    resb STACK_SIZE
stack_top:
