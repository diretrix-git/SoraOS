; context_switch.asm — Save/restore ESP for cooperative/preemptive context switch
; NASM, elf32 format
;
; void context_switch(pcb_t* old_pcb, pcb_t* new_pcb);
;
; pcb_t layout (must match scheduler.h):
;   offset 0:  pid        (uint32_t)
;   offset 4:  state      (uint32_t / enum)
;   offset 8:  priority   (uint8_t, padded to 4 bytes by compiler)
;   offset 12: esp        (uint32_t)  <-- we save/restore here
;   offset 16: stack_base (uint32_t)
;   offset 20: next       (pointer)

bits 32
section .text

global context_switch

context_switch:
    ; [esp+4] = old_pcb
    ; [esp+8] = new_pcb

    mov eax, [esp+4]        ; eax = old_pcb
    mov [eax+12], esp       ; old_pcb->esp = current ESP (saves return address too)

    mov eax, [esp+8]        ; eax = new_pcb
    mov esp, [eax+12]       ; ESP = new_pcb->esp

    ret                     ; pops saved EIP, resumes new process
