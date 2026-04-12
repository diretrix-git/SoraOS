# Coding Conventions

**Analysis Date:** 2026-04-09

## Naming Patterns

**Files:**
- C source: `module.c` - lowercase with `.c` extension
- C headers: `module.h` - lowercase with `.h` extension
- Assembly: `module.asm` - lowercase with `.asm` extension for NASM syntax

**Functions:**
- All functions use `snake_case`: `vga_init`, `pmm_alloc_frame`, `keyboard_getchar`
- Public functions: no prefix (e.g., `vga_putchar`, `scheduler_add`)
- Static/private functions: no prefix (e.g., `pmm_set_frame`, `keyboard_buffer_push`)
- Assembly global symbols: underscore prefix (e.g., `_isr0`, `_switch_context`, `_isr_handler`)

**Variables:**
- Local variables: `snake_case` (e.g., `input_index`, `keyboard_head`)
- Global/static variables: `snake_case` with module prefix (e.g., `vga_buffer`, `pmm_bitmap`, `timer_ticks`)

**Types:**
- Structs: `struct name` without typedef (e.g., `struct process`, `struct idt_entry`)
- Typedef enums: `*_t` suffix (e.g., `process_state_t`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `VGA_BLACK`, `PAGE_SIZE`, `MAX_PROCESSES`)

## Code Style

**Formatting:**
- Indentation: 4 spaces (no tabs in C files)
- Assembly indentation: 4 spaces for instructions, labels at column 0
- Braces: K&R style - opening brace on same line for functions and control structures
- Maximum line length: approximately 80-100 characters

**Linting:**
- Compiler warnings enabled: `-Wall -Wextra`
- Warning about unused parameters handled by suppressing specific warnings

## Import Organization

**Order:**
1. Standard headers: `#include <stdint.h>`
2. Project headers: `#include "module.h"`
3. Local includes for constants

**Path Aliases:**
- No path aliases; all includes use relative paths from `kernel/` directory
- Include guard pattern: `#ifndef MODULE_H` / `#define MODULE_H` / `#endif`

## Error Handling

**Patterns:**
- **Return codes for recoverable errors**: Functions return 0 for null/failure, non-zero for success
  - `pmm_alloc_frame()` returns `0` for out-of-memory
  - `process_create()` returns `0` (null pointer) for no free slots

- **Panic/halt for unrecoverable errors**: Infinite halt loop with `__asm__ volatile ("hlt")`
  - CPU exceptions halt with message: `while (1) { __asm__ volatile ("hlt"); }`
  - Page faults display diagnostics then halt

- **Inline error messages**: VGA output before halt
  ```c
  vga_print("!!! PAGE FAULT !!!\n");
  vga_print("Faulting address: 0x");
  vga_print_hex(faulting_address);
  // ... then halt
  ```

## Logging

**Framework:** Direct VGA buffer writing - no logging framework

**Patterns:**
- Debug output uses `vga_print()` and `vga_print_int()`/`vga_print_hex()`
- Initialization sequences log component status:
  ```c
  vga_init();
  vga_print("GDT loaded\n");
  vga_print("IDT loaded\n");
  ```
- Error messages prefixed with `!!!` for visibility: `!!! CPU EXCEPTION !!!`

## Comments

**When to Comment:**
- Module headers: Brief description at top of file explaining purpose
- Hardware register interactions: Comment explaining register purpose
- Assembly macros: Detailed comment blocks explaining macro behavior
- Complex calculations: Explain the formula (e.g., CHS conversion in bootloader)

**JSDoc/TSDoc:**
- Not used. Minimal inline comments for complex sections only.
- Function purpose is self-evident from naming conventions

## Function Design

**Size:** Functions are typically under 50 lines. Complex logic broken into helper functions.

**Parameters:**
- Use pointers for output parameters: `void idt_set_gate(uint8_t num, uint32_t base, ...)`
- Use structures for related data: `struct registers`, `struct process`

**Return Values:**
- Initialization functions: `void` (assume success)
- Memory allocation: return address/pointer (`uint32_t`, pointer)
- Query functions: return value (`uint32_t pmm_free_frames()`)
- State changes: `void`

## Module Design

**Exports:**
- Public API declared in header file
- Private functions declared `static` in source file
- No barrel files; each module has its own header

**File Organization:**
- One module per `.c/.h` pair (e.g., `vga.c/vga.h`, `pmm.c/pmm.h`)
- Related functionality grouped (e.g., `keyboard.c` handles keyboard buffer + scancode conversion)

---

## Assembly Conventions (NASM)

**Syntax:**
- NASM syntax (`[BITS 32]`, `[ORG 0x7C00]`)
- Labels at column 0, instructions indented 4 spaces
- Comments use `;` prefix with space after

**ISR Stubs Pattern:**
```asm
; Macro for ISRs without error codes
%macro ISR_NOERR 1
global _isr%1
_isr%1:
    cli
    push dword 0              ; Dummy error code
    push dword %1             ; Interrupt number
    jmp isr_common_stub
%endmacro

; Macro for ISRs with error codes (CPU pushes error code)
%macro ISR_ERR 1
global _isr%1
_isr%1:
    cli
    push dword %1             ; Interrupt number (error code already pushed)
    jmp isr_common_stub
%endmacro
```

**Interfacing C:**
- C functions called from assembly use underscore prefix: `extern _isr_handler`
- Symbols `global` exported for C linking: `global _switch_context`
- Calling convention: cdecl (arguments on stack, return value in EAX)
- Registers preserved: EBX, ESI, EDI, EBP, ESP

---

## Memory Patterns

**PMM (Physical Memory Manager):**
- Bitmap allocation: 1 bit per 4KB frame
- Static bitmap array: `static uint8_t pmm_bitmap[PMM_BITMAP_SIZE]`
- Return semantics: `0` = allocation failure, otherwise physical address
- First 1MB marked as used for BIOS/VGA/bootloader

**Paging:**
- Two-level hierarchy: page directory (1024 entries) → page tables (1024 entries each)
- Structures use `__attribute__((packed))` for hardware compatibility
- Inline assembly for TLB management: `invlpg`, CR3 flush

**Process Memory:**
- Fixed stack size: `#define PROCESS_STACK_SIZE 4096`
- Context saved in `struct cpu_context` with all general registers + segment registers
- Stack grows down from `stack_top`

---

*Convention analysis: 2026-04-09*