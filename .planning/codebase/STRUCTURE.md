# Codebase Structure

**Analysis Date:** 2026-04-09

## Directory Layout

```
OSollama/
├── boot/                    # Bootloader code
│   ├── stage1.asm           # MBR (512 bytes, loaded at 0x7C00)
│   ├── stage2_entry.asm     # Stage 2 assembly entry (real → protected mode)
│   └── stage2.c             # Stage 2 C code (kernel loader)
├── kernel/                  # Kernel source files
│   ├── kernel.c             # Main kernel entry point
│   ├── gdt.c / gdt.h        # Global Descriptor Table
│   ├── idt.c / idt.h        # Interrupt Descriptor Table
│   ├── isr.asm              # Interrupt service routine stubs
│   ├── pic.c / pic.h        # PIC (8259) driver
│   ├── timer.c / timer.h    # PIT timer driver
│   ├── keyboard.c / keyboard.h # PS/2 keyboard driver
│   ├── pmm.c / pmm.h        # Physical memory manager
│   ├── paging.c / paging.h  # Paging implementation
│   ├── process.c / process.h # Process management
│   ├── scheduler.c / scheduler.h # Round-robin scheduler
│   ├── switch.asm           # Context switch assembly
│   ├── shell.c / shell.h    # Interactive CLI shell
│   ├── vga.c / vga.h        # VGA text mode driver
│   └── stdint.h             # Standard integer types
├── build/                   # Build artifacts (object files, binaries)
├── linker.ld                # Kernel linker script
├── Makefile                 # Build configuration
├── myos.img                 # Output disk image
└── README.md                # Project documentation
```

## Directory Purposes

### `boot/`
- **Purpose:** Bootloader implementation (two-stage)
- **Contains:** Assembly and C files for boot process
- **Key Files:**
  - `stage1.asm`: MBR bootloader (must be exactly 512 bytes)
  - `stage2_entry.asm`: Protected mode transition assembly
  - `stage2.c`: Kernel loading and handoff

### `kernel/`
- **Purpose:** Core kernel implementation
- **Contains:** C source/header files and assembly modules
- **Key Files:**
  - `kernel.c`: Entry point and initialization sequence
  - `isr.asm`: Interrupt stubs for IDT
  - `switch.asm`: Context switching for scheduler
  - All other `.c/.h` pairs are kernel subsystems

### `build/`
- **Purpose:** Build output directory (generated)
- **Contains:** Object files (`.o`), binary files (`.bin`)
- **Generated:** Created by Makefile during build
- **Key Artifacts:**
  - `stage1.bin`: Stage 1 bootloader binary
  - `stage2.bin`: Combined Stage 2 binary
  - `kernel.bin`: Raw kernel binary

## Key File Locations

### Boot Files
| File | Purpose | Size/Notes |
|------|---------|------------|
| `boot/stage1.asm` | MBR bootloader | 512 bytes, loads at 0x7C00 |
| `boot/stage2_entry.asm` | Protected mode entry | Assembly stub for mode switch |
| `boot/stage2.c` | Kernel loader | Loads kernel to 1 MB |

### Core Kernel Files
| File | Purpose | Lines |
|------|---------|-------|
| `kernel/kernel.c` | Main entry, initialization | 88 |
| `kernel/stdint.h` | Type definitions | Small header |

### Interrupt System
| File | Purpose | Key Exports |
|------|---------|-------------|
| `kernel/idt.c` | IDT setup | `idt_init()` |
| `kernel/idt.h` | IDT structures | `struct idt_entry`, `struct idt_ptr` |
| `kernel/isr.asm` | ISR stubs | `_isr0` to `_isr31`, `_irq0` to `_irq15` |
| `kernel/pic.c` | PIC driver | `pic_init()`, `pic_send_eoi()` |

### Memory Management
| File | Purpose | Key Functions |
|------|---------|---------------|
| `kernel/pmm.c` | Physical allocator | `pmm_alloc_frame()`, `pmm_free_frame()` |
| `kernel/paging.c` | Virtual memory | `paging_init()`, `map_page()` |

### Scheduling
| File | Purpose | Key Functions |
|------|---------|---------------|
| `kernel/process.c` | Process struct | `process_create()`, `process_exit()` |
| `kernel/scheduler.c` | Round-robin dispatch | `scheduler_tick()`, `scheduler_add()` |
| `kernel/switch.asm` | Context assembly | `_switch_context()` |

### I/O Drivers
| File | Purpose | Key Functions |
|------|---------|---------------|
| `kernel/vga.c` | Console output | `vga_print()`, `vga_putchar()` |
| `kernel/keyboard.c` | Input handling | `keyboard_getchar()` |
| `kernel/timer.c` | System timer | `timer_get_ticks()` |

### Shell
| File | Purpose | Commands |
|------|---------|----------|
| `kernel/shell.c` | Interactive CLI | help, clear, echo, meminfo, ps, timer |

### Build Configuration
| File | Purpose |
|------|---------|
| `Makefile` | Build rules, toolchain setup |
| `linker.ld` | Linker script for kernel (loads at 0x100000) |

## Naming Conventions

### Assembly Files
- **Bootloader:** `stage1.asm`, `stage2_entry.asm` (snake_case)
- **Kernel:** `isr.asm`, `switch.asm` (descriptive function)
- **Functions:** Prefixed with underscore for C linkage (`_isr_handler`, `_switch_context`)

### C Files
- **Files:** `snake_case.c` / `snake_case.h`
- **Functions:** `subsystem_action()` pattern (e.g., `vga_print()`, `pmm_alloc_frame()`)
- **Types:** `snake_case_t` for typedefs (e.g., `process_state_t`)
- **Structs:** `snake_case` (e.g., `struct process`, `struct cpu_context`)

### Header Guards
- **Pattern:** `#ifndef SUBSYSTEM_H` / `#define SUBSYSTEM_H` / `#endif`
- **Examples:** `VGA_H`, `IDT_H`, `PMM_H`

### Macros & Constants
- **Uppercase snake_case:** `MAX_PROCESSES`, `PMM_FRAME_SIZE`, `VGA_WIDTH`
- **Register flags:** `IDT_FLAG_PRESENT`, `GDT_ACCESS_CODE`
- **Port addresses:** `PIC1_COMMAND`, `PIC2_DATA`

## Where to Add New Code

### New Kernel Subsystem
1. Create `kernel/subsystem.c` and `kernel/subsystem.h`
2. Add function declarations to `.h` file
3. Include `stdint.h` for types
4. Add to Makefile source list:
   ```makefile
   KERNEL_SRCS = $(wildcard $(KERNEL_DIR)/*.c)
   ```
5. Include in `kernel/kernel.c`:
   ```c
   #include "subsystem.h"
   ```
6. Call `subsystem_init()` in `kernel_main()`

### New Shell Command
1. Add command function in `kernel/shell.c`:
   ```c
   static void cmd_newcommand(void) {
       // implementation
   }
   ```
2. Add to `shell_execute()` switch:
   ```c
   } else if (shell_strcmp(cmd, "newcommand") == 0) {
       cmd_newcommand();
   }
   ```
3. Update `cmd_help()` documentation

### New Hardware Driver
1. Create `kernel/driver.c` and `kernel/driver.h`
2. Implement `driver_init()` for setup
3. If interrupt-driven:
   - Add IRQ handler in `kernel/isr.asm` (use `IRQ` macro)
   - Register in `kernel/idt.c` with `idt_set_gate()`
   - Call `pic_unmask(irq)` in init function
4. Add to kernel initialization sequence

### New Process Type
1. Define entry function:
   ```c
   void my_thread_entry(void) {
       while (1) {
           // work
           __asm__ volatile ("hlt");
       }
   }
   ```
2. Create process in `kernel_main()`:
   ```c
   struct process* p = process_create("MyThread", my_thread_entry);
   if (p) scheduler_add(p);
   ```

### New System Call (Future)
1. Add ISR handler for software interrupt (e.g., `int 0x80`)
2. Create syscall table in new `kernel/syscall.c`
3. Add user-space wrapper in library

## Assembly/C Integration

### ISR Handler Pattern
**Assembly (`isr.asm`):**
```asm
global _isr14
_isr14:
    cli
    push dword 0          ; error code (for page fault, CPU pushes it)
    push dword 14         ; interrupt number
    jmp isr_common_stub

isr_common_stub:
    pusha                 ; save general registers
    push ds/es/fs/gs      ; save segment registers
    mov ax, 0x10          ; load kernel data segment
    mov ds, ax
    push esp              ; pass registers struct pointer
    call _isr_handler     ; call C function
    ; ... restore and iret
```

**C Handler (`idt.c`):**
```c
void isr_handler(struct registers* r) {
    // r->int_no, r->err_code, r->eip, etc.
}
```

### Context Switch Pattern
**C Declaration (`scheduler.c`):**
```c
extern void switch_context(struct cpu_context* old, struct cpu_context* new);
```

**Assembly Implementation (`switch.asm`):**
```asm
global _switch_context
_switch_context:
    ; [esp+4] = old context pointer
    ; [esp+8] = new context pointer
    ; Save current regs to old, load new, return to new EIP
```

### Importing Assembly Symbols in C
```c
// In isr.asm:
global _isr_handler
extern _isr_handler

// In idt.c:
extern void _isr0();
void isr_handler(struct registers* r);
```

### Exporting C Functions to Assembly
```asm
; In isr.asm:
extern _isr_handler    ; C function name with underscore prefix

; Call it:
call _isr_handler
```

## Build Artifacts

### Stage 1 Bootloader
- **Source:** `boot/stage1.asm`
- **Output:** `build/stage1.bin`
- **Command:** `nasm -f boot/stage1.asm -o build/stage1.bin`
- **Size:** 512 bytes (1 sector)

### Stage 2 Bootloader
- **Sources:** `boot/stage2_entry.asm` + `boot/stage2.c`
- **Process:**
  1. Assemble entry: `nasm -f bin boot/stage2_entry.asm -o build/stage2_entry.bin`
  2. Compile C: `gcc -m32 -c boot/stage2.c -o build/stage2.o -fno-pic`
  3. Extract text: `objcopy -O binary -j .text build/stage2.o build/stage2_c.bin`
  4. Concatenate: `copy /b stage2_entry.bin + stage2_c.bin stage2.bin`
- **Output:** `build/stage2.bin`
- **Load Address:** Sector 1 (LBA 1)

### Kernel
- **Sources:** All `.c` files in `kernel/`, all `.asm` files in `kernel/`
- **Process:**
  1. Compile C files: `gcc -m32 -c kernel.c -o build/kernel.o`
  2. Assemble ASM: `nasm -f win32 kernel/isr.asm -o build/isr.o`
  3. Link: `ld -m i386pe -e _kernel_main *.o -o kernel.exe --subsystem native`
  4. Convert: `objcopy -O binary kernel.exe kernel.bin`
- **Output:** `build/kernel.bin`
- **Load Address:** Sector 5 (LBA 5), runtime at 0x100000

### Disk Image
- **Process:**
  1. Create 10 MB blank image: `dd if=/dev/zero of=myos.img bs=1M count=10`
  2. Write Stage 1 at sector 0
  3. Write Stage 2 at sector 1
  4. Write Kernel at sector 5
- **Output:** `myos.img`

## Special Directories

### None
- No generated/committed special directories
- `build/` is cleaned by `make clean`
- No dependency directories (deps)

---

*Structure analysis: 2026-04-09*