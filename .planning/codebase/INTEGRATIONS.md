# External Integrations

**Analysis Date:** 2026-04-09

## Hardware Integration

**VGA Text Mode (0xB8000):**
- Memory-mapped text buffer at physical address 0xB8000
- 80x25 character display (text mode)
- Implementation: `kernel/vga.c`, `kernel/vga.h`
- Character output via direct memory writes
- Supports colors via 16-color palette

**PIC 8259 (Programmable Interrupt Controller):**
- Two cascaded 8259 PICs (master/slave)
- Master PIC: ports 0x20 (command), 0x21 (data)
- Slave PIC: ports 0xA0 (command), 0xA1 (data)
- Implementation: `kernel/pic.c`, `kernel/pic.h`
- IRQ remapping: IRQ0-7 → IDT 32-39, IRQ8-15 → IDT 40-47
- Functions: `pic_init()`, `pic_send_eoi()`, `pic_mask()`, `pic_unmask()`

**PIT Timer (Programmable Interval Timer - 8253/8254):**
- Channel 0 on port 0x40 for timer interrupts
- Control register on port 0x43
- Base frequency: 1,193,180 Hz
- Configurable frequency via divisor (initialized at 100Hz)
- Implementation: `kernel/timer.c`, `kernel/timer.h`
- Timer tick counter maintained in kernel

**PS/2 Keyboard Controller:**
- Data port: 0x60
- Status/command port: 0x64
- Implementation: `kernel/keyboard.c`, `kernel/keyboard.h`
- US QWERTY scancode translation
- Circular buffer for key events (256 entries)
- Shift/Caps lock state tracking
- A20 line enable via keyboard controller (boot stage)

**IDE Disk Controller (Stage 2):**
- Primary IDE ports: 0x1F0-0x1F7
- PIO mode sector reading for kernel bootstrap
- Implementation: `boot/stage2.c` (IDE read functions)
- Used by bootloader to load kernel from disk

## Boot Chain Integration

**Stage 1 Bootloader (MBR - 512 bytes):**
- Location: `boot/stage1.asm`
- Load address: 0x7C00 (BIOS loads here)
- Responsibility: Load Stage 2 from LBA 1
- BIOS interrupt: `int 0x13` for disk read
- Output: BIOS teletype `int 0x10` for debug messages

**Stage 2 Bootloader:**
- Assembly entry: `boot/stage2_entry.asm` (load address 0x7E00)
- C implementation: `boot/stage2.c`
- Responsibilities:
  - Enable A20 line via keyboard controller
  - Load 32-bit GDT (flat memory model)
  - Switch CPU to protected mode (set CR0 bit 0)
  - Load kernel from disk (LBA 5)
  - Jump to kernel entry at 0x100000

**Boot Layout:**
- LBA 0 (sector 0): Stage 1 MBR (512 bytes)
- LBA 1-4 (sectors 1-4): Stage 2 bootloader (4 sectors = 2048 bytes)
- LBA 5+ (sector 5+): Kernel binary (loaded at 0x100000)

**Memory Map:**
- 0x00007C00: Stage 1 MBR load address
- 0x00007E00: Stage 2 load address
- 0x00090000: Stage 2 stack
- 0x000B8000: VGA text buffer
- 0x00100000: Kernel load address (1MB)
- 0x00200000+: Available for kernel heap/stacks

## CPU Integration

**GDT (Global Descriptor Table):**
- Implementation: `kernel/gdt.c`, `kernel/gdt.h`
- Flat memory model: base=0, limit=4GB
- Segments:
  - Null selector (0x00)
  - Kernel code segment (0x08): ring 0, execute-only
  - Kernel data segment (0x10): ring 0, read/write

**IDT (Interrupt Descriptor Table):**
- Implementation: `kernel/idt.c`, `kernel/idt.h`
- ISR assembly stubs: `kernel/isr.asm`
- IRQ handlers for timer (IRQ0), keyboard (IRQ1)
- Exception handlers for CPU faults (e.g., page fault)

**Context Switching:**
- Assembly implementation: `kernel/switch.asm`
- Saves/restores all general-purpose registers (EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP)
- Saves instruction pointer (EIP) and flags (EFLAGS)

## Data Storage

**Memory Management:**
- Physical Memory Manager (PMM): `kernel/pmm.c`, `kernel/pmm.h`
  - Bitmap-based frame allocator
  - Frame size: 4KB (4096 bytes)
  - Total managed: 32MB (configurable)
  - First 1MB marked as used (BIOS/VGA/bootloader)

- Paging: `kernel/paging.c`, `kernel/paging.h`
  - Two-level page table structure
  - Page directory + page tables
  - Page size: 4KB
  - Flags: present, writable, user-accessible

**No Persistent Storage:**
- No disk filesystem implementation
- No file I/O beyond bootloader loading kernel

## Process Management

**Process Management:**
- Implementation: `kernel/process.c`, `kernel/process.h`
- Process struct: PID, state, CPU context, stack
- Maximum processes: 16
- Stack size per process: 4096 bytes
- States: RUNNING, READY, BLOCKED, TERMINATED

**Scheduler:**
- Implementation: `kernel/scheduler.c`, `kernel/scheduler.h`
- Algorithm: Round-robin preemptive multitasking
- Timer-driven: Scheduler tick called from timer interrupt
- Integration: `scheduler_tick()` called by timer handler

**Shell:**
- Implementation: `kernel/shell.c`, `kernel/shell.h`
- Simple command-line interface
- Commands: help, clear, echo, meminfo, ps, timer

## Interrupt Handling

**IRQ Routing:**
- IRQ0 (Timer) → Timer handler → `pic_send_eoi(0)` + scheduler tick
- IRQ1 (Keyboard) → Keyboard handler → Scancode decode + buffer push + `pic_send_eoi(1)`

**Interrupt Entry Points:**
- Assembly ISR stubs in `kernel/isr.asm`
- C handlers registered in IDT
- End-of-Interrupt sent to PIC after handling

## Configuration Files

**Required env vars:**
- PATH: Must include i686-elf toolchain bin directory

**Build configuration:**
- `Makefile`: Compiles all sources, creates disk image
- `linker.ld`: Kernel memory layout (entry at 0x100000)
- Sections: .text, .rodata, .data, .bss

**No secrets management:**
- Bare-metal OS, no environment files or credential storage

## Debugging Integration

**QEMU Debug Mode:**
- Command: `make debug`
- Flags: `-d int,cpu_reset -no-reboot -no-shutdown`
- Enables interrupt logging and CPU reset logging
- QEMU monitor (Ctrl+Alt+2) for register/memory inspection

## Webhooks & Callbacks

**Incoming:**
- None (no network stack)

**Outgoing:**
- None (no network stack)

---

*Integration audit: 2026-04-09*