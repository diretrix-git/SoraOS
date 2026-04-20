; gdt_flush.asm — Load the GDT and reload segment registers
; NASM, elf32 format
;
; void gdt_flush(gdt_ptr_t *gdt_ptr);
;   [esp+4] = pointer to gdt_ptr_t (limit + base)

bits 32
section .text

global gdt_flush

gdt_flush:
    mov  eax, [esp+4]       ; load gdt_ptr_t* argument
    lgdt [eax]              ; load GDT register

    ; Far jump to reload CS with kernel code selector (0x08)
    jmp  0x08:.flush

.flush:
    ; Reload all data segment registers with kernel data selector (0x10)
    mov  ax, 0x10
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ss, ax
    ret
