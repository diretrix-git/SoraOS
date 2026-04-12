; =============================================================================
; Context Switch Function
; Saves current process context and loads next process context
; =============================================================================

[BITS 32]
section .text

global _switch_context
_switch_context:
    ; Input: 
    ;   [esp+4] = old context pointer (struct cpu_context*)
    ;   [esp+8] = new context pointer (struct cpu_context*)
    
    ; cpu_context structure layout (from process.h):
    ;   0:  eax
    ;   4:  ebx
    ;   8:  ecx
    ;   12: edx
    ;   16: esi
    ;   20: edi
    ;   24: ebp
    ;   28: esp
    ;   32: eip
    ;   36: eflags
    ;   40: cs
    ;   44: ds
    
    push ebp
    push ebx
    push esi
    push edi
    
    ; After pushes, stack layout:
    ; [esp] = edi
    ; [esp+4] = esi
    ; [esp+8] = ebx
    ; [esp+12] = ebp (saved)
    ; [esp+16] = return address
    ; [esp+20] = old context pointer (first param)
    ; [esp+24] = new context pointer (second param)
    
    ; Save current registers to old context
    mov ebp, [esp+20]         ; old context pointer
    
    mov [ebp+0], eax          ; save eax
    mov [ebp+4], ebx          ; save ebx (from stack)
    mov [ebp+8], ecx          ; save ecx
    mov [ebp+12], edx         ; save edx
    mov [ebp+16], esi         ; save esi (from stack)
    mov [ebp+20], edi         ; save edi (from stack)
    
    ; Save ebp - it's currently on the stack at [esp+12] (pushed at line with push ebp)
    mov eax, [esp+12]         ; get the pushed ebp value
    mov [ebp+24], eax         ; save ebp to context
    
    ; Get new context pointer
    mov ebx, [esp+24]         ; new context pointer (second parameter)
    
    ; Load new registers
    mov eax, [ebx+0]          ; new eax
    mov ecx, [ebx+8]          ; new ecx
    mov edx, [ebx+12]         ; new edx
    
    ; Load new stack pointer for the target process
    mov esp, [ebx+28]         ; new esp
    
    ; Restore saved registers for new process
    pop edi                   ; restore edi from new stack
    pop esi                   ; restore esi from new stack
    pop ebx                   ; restore ebx from new stack
    pop ebp                   ; restore ebp from new stack
    
    ; Now we're running on the new process's stack
    ; Load segment registers
    mov ds, [ebx+44]          ; new ds
    
    ; Push new eip and return to it
    mov eax, [ebx+32]         ; new eip
    push eax
    ret
