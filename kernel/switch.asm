; =============================================================================
; Context Switch — ELF32, no underscore prefix
;
; void switch_context(struct cpu_context *old_ctx, struct cpu_context *new_ctx)
;
; cpu_context layout (from process.h):
;   +0  eax
;   +4  ebx
;   +8  ecx
;   +12 edx
;   +16 esi
;   +20 edi
;   +24 ebp
;   +28 esp
;   +32 eip
;   +36 eflags   (not used in switch — eflags handled by pushf/popf if needed)
;   +40 cs
;   +44 ds
; =============================================================================

[BITS 32]
section .text

global switch_context
switch_context:
    ; On entry the stack looks like:
    ;   [esp+0]  = return address
    ;   [esp+4]  = old_ctx  (param 1)
    ;   [esp+8]  = new_ctx  (param 2)

    ; Save callee-saved registers onto the CURRENT stack.
    ; switch_context acts like a normal call: caller expects
    ; ebx, esi, edi, ebp to survive across the call.
    push ebp
    push ebx
    push esi
    push edi

    ; After the 4 pushes the stack is:
    ;   [esp+0]  = edi  (just pushed)
    ;   [esp+4]  = esi
    ;   [esp+8]  = ebx
    ;   [esp+12] = ebp
    ;   [esp+16] = return address   ← this is the EIP to restore
    ;   [esp+20] = old_ctx
    ;   [esp+24] = new_ctx

    ; -------------------------------------------------------
    ; SAVE old context
    ; -------------------------------------------------------
    mov eax, [esp+20]       ; eax = old_ctx

    ; Save general-purpose registers as they were on entry
    ; (eax itself was not modified before this point)
    ; We DON'T save eax from the register — it now holds old_ctx.
    ; That's fine: the "saved eax" value is whatever the caller had,
    ; but since we're using a callee-save convention only ebx/esi/edi/ebp
    ; actually matter for resumption. Store them from the stack copies.

    mov ecx, [esp+8]        ; saved ebx
    mov [eax+4],  ecx
    mov ecx, [esp+4]        ; saved esi
    mov [eax+16], ecx
    mov ecx, [esp+0]        ; saved edi
    mov [eax+20], ecx
    mov ecx, [esp+12]       ; saved ebp
    mov [eax+24], ecx

    ; Save the stack pointer as it will be after we return
    ; (caller's esp = current esp + 4 pushes + return addr = esp+20)
    lea ecx, [esp+20]
    mov [eax+28], ecx       ; old_ctx->esp

    ; Save the return address as the EIP to resume at
    mov ecx, [esp+16]
    mov [eax+32], ecx       ; old_ctx->eip

    ; -------------------------------------------------------
    ; LOAD new context
    ; -------------------------------------------------------
    mov eax, [esp+24]       ; eax = new_ctx

    ; Restore callee-saved registers from new context
    mov ebx, [eax+4]        ; new ebx
    mov esi, [eax+16]       ; new esi
    mov edi, [eax+20]       ; new edi
    mov ebp, [eax+24]       ; new ebp

    ; Load new esp — from here on we are on the new process's stack
    mov esp, [eax+28]

    ; Push the new EIP so that 'ret' below jumps to it
    mov eax, [eax+32]       ; new eip  (eax is scratch now)
    push eax

    ; Return — pops EIP and jumps to new process
    ret