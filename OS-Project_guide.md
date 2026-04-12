# OS Project — Complete AI Prompt Guide

> Give this document to an AI from top to bottom. Each section is a self-contained prompt. Work through them in order. Do not skip phases.

---

## Context (Read This First — Always Include This in Every Prompt)

```
I am building a hobby OS for a university final project on Windows. Here is my full stack:
- Stage 1 Bootloader: NASM assembly (512 bytes, MBR)
- Stage 2 Bootloader: C (enters protected mode, loads kernel)
- Kernel: C (interrupts, paging, scheduling, deadlock)
- Shell: C (VGA text mode, keyboard input, CLI commands)
- Target architecture: x86 32-bit (NOT 64-bit)
- Toolchain: i686-elf-gcc cross compiler, NASM, GNU Make, QEMU on Windows (MSYS2)
- I am NOT using GRUB. I am writing my own two-stage bootloader.

My project must demonstrate:
1. Booting / Bootloader (two stage)
2. Interrupts (IDT, PIC, IRQ handlers)
3. Memory management (paging)
4. CPU scheduling (round robin, context switching)
5. Multithreading (kernel threads)

Always write code for 32-bit x86 bare metal. Never include libc headers. Never assume an OS exists.
```

---

## PHASE 0 — Windows Toolchain Setup

### Prompt 0.1 — MSYS2 and Base Packages

```
I am on Windows and need to set up MSYS2 to build a hobby OS.

After I install MSYS2 from https://www.msys2.org, give me the exact terminal commands (for the MSYS2 UCRT64 shell) to install:
- nasm
- make
- qemu (full system emulation)
- git
- base development tools (gcc, binutils for host)

Then tell me how to verify each tool installed correctly.
```

---

### Prompt 0.2 — i686-elf Cross Compiler

```
I need to build or install a prebuilt i686-elf-gcc cross compiler on Windows using MSYS2.

Option A: Give me commands to download a prebuilt i686-elf toolchain and add it to my PATH in MSYS2.
Option B: If no reliable prebuilt exists, give me exact step-by-step commands to build binutils and gcc from source for the i686-elf target inside MSYS2.

I need:
- i686-elf-gcc
- i686-elf-ld
- i686-elf-as
- i686-elf-objcopy

After setup, give me a test command to confirm the cross compiler works (compile a tiny C file with no libc).
```

---

### Prompt 0.3 — Project Folder Structure

```
Create the full folder and file structure for my OS project. Use this layout:

myos/
  boot/
    stage1.asm       # MBR bootloader (NASM)
    stage2.c         # Second stage bootloader (C)
    stage2_entry.asm # Assembly entry for stage2
  kernel/
    kernel.c         # Kernel main
    kernel.h
    gdt.c / gdt.h    # Global Descriptor Table
    idt.c / idt.h    # Interrupt Descriptor Table
    isr.asm          # Low-level interrupt stubs
    pic.c / pic.h    # PIC (8259) driver
    timer.c / timer.h
    keyboard.c / keyboard.h
    pmm.c / pmm.h    # Physical memory manager
    paging.c / paging.h
    kheap.c / kheap.h
    process.c / process.h
    scheduler.c / scheduler.h
    shell.c / shell.h
    vga.c / vga.h
  linker.ld           # Linker script
  Makefile

Give me:
1. The commands to create all folders and blank files
2. The complete Makefile that compiles stage1 with NASM, cross-compiles all C files with i686-elf-gcc, links with the linker script, and produces a bootable disk image
3. The complete linker.ld file
4. How to run the OS with QEMU after building
```

---

## PHASE 1 — Stage 1 Bootloader

### Prompt 1.1

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write boot/stage1.asm — the Stage 1 MBR bootloader in NASM x86 assembly.

It must:
1. Be exactly 512 bytes (padded with zeros, ending with 0x55AA boot signature)
2. Set up segment registers (CS, DS, ES, SS) and a stack
3. Print "Stage 1 OK" using BIOS interrupt int 0x10
4. Use BIOS interrupt int 0x13 (CHS mode) to read 4 sectors from disk starting at LBA 1 into memory at 0x0000:0x7E00 (right after the MBR)
5. Jump to 0x7E00 to hand off to Stage 2

Include full comments explaining every instruction. Explain what CHS mode means and what values to use.
```

---

## PHASE 2 — Stage 2 Bootloader

### Prompt 2.1 — Assembly Entry

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write boot/stage2_entry.asm in NASM. This is the assembly entry point for Stage 2, which Stage 1 jumps to at 0x7E00.

It must:
1. Still be in 16-bit real mode at entry
2. Call a C function named stage2_main (which I will write next)
3. After stage2_main returns, enter an infinite halt loop

This file will be linked with stage2.c. The C function stage2_main will handle the rest.
```

---

### Prompt 2.2 — Protected Mode Switch and Kernel Load

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write boot/stage2.c — the Stage 2 bootloader in C. This runs in 16-bit real mode at first, transitions to 32-bit protected mode, then loads and jumps to the kernel.

It must:
1. Define and load a minimal GDT (Global Descriptor Table) with three entries: null, code segment, data segment. Use inline assembly to load it with lgdt.
2. Enable the A20 line (use the BIOS int 0x15, ax=0x2401 method)
3. Switch the CPU from 16-bit real mode to 32-bit protected mode by:
   - Setting PE bit in CR0
   - Performing a far jump to flush the pipeline
4. After entering protected mode, set up all segment registers to the data segment
5. Set up a stack at 0x90000
6. Read the kernel from disk into memory at 0x100000 using a simple loop. For now, assume the kernel starts at LBA 5 and is 50 sectors long. Use a basic read loop (you may use a placeholder comment for the actual disk read since we are in protected mode and cannot use BIOS anymore — explain this limitation)
7. Jump to 0x100000 (kernel entry point)

Explain the real mode to protected mode transition in comments. Explain why we cannot use BIOS interrupts after entering protected mode.
```

---

## PHASE 3 — VGA Driver (Do This Early — You Need It for Debugging)

### Prompt 3.1

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/vga.c and kernel/vga.h — a VGA text mode driver.

The VGA text buffer is at physical address 0xB8000. It is 80 columns x 25 rows. Each character is 2 bytes: one for the ASCII character, one for the color attribute.

Implement:
- vga_init() — clears the screen
- vga_putchar(char c) — writes a character, handles newline and scrolling
- vga_print(const char* str) — prints a string
- vga_print_hex(uint32_t value) — prints a number in hex (for debugging)
- vga_print_int(int value) — prints a decimal integer
- vga_set_color(uint8_t fg, uint8_t bg) — sets text color

Use an enum or defines for the 16 VGA colors. Do not use any libc functions. Implement scrolling manually by copying rows up when the cursor reaches the bottom.
```

---

## PHASE 4 — GDT

### Prompt 4.1

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/gdt.c and kernel/gdt.h — the Global Descriptor Table setup for the kernel.

The kernel needs its own GDT (separate from the Stage 2 one).

Implement:
- A GDT with 3 entries: null descriptor, kernel code segment (ring 0), kernel data segment (ring 0)
- gdt_init() function that sets up and loads the GDT using lgdt inline assembly
- A gdt_entry_t struct with all fields properly defined
- A gdt_ptr_t struct for the lgdt instruction

After loading the GDT, reload all segment registers using a far jump for CS and mov for the rest. Show this in assembly (can be inline asm in C).

Explain what each GDT field does (base, limit, access byte, granularity byte) with comments.
```

---

## PHASE 5 — IDT and Interrupts

### Prompt 5.1 — IDT Setup

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/idt.c and kernel/idt.h — the Interrupt Descriptor Table.

Implement:
- idt_entry_t struct (8 bytes per entry)
- idt_ptr_t struct for lidt
- idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) — sets one IDT entry
- idt_init() — zeroes all 256 entries, sets up gates, loads with lidt

The IDT needs 256 entries. Leave the handlers blank for now — I will fill them in the next step.

Explain the difference between interrupt gates and trap gates.
```

---

### Prompt 5.2 — ISR Stubs

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/isr.asm — low-level interrupt service routine stubs in NASM.

I need:
1. ISR stubs for CPU exceptions 0–31 (some push a dummy error code, some push a real one — list which ones)
2. IRQ stubs for hardware interrupts IRQ0–IRQ15 (mapped to IDT entries 32–47 after PIC remapping)
3. A common_stub that:
   - Pushes all registers (pusha)
   - Pushes segment registers (ds)
   - Calls a C handler function named isr_handler or irq_handler
   - Restores everything and returns with iret

Also write the C side: kernel/idt.c additions — an isr_handler(registers_t* regs) function that prints which exception occurred using my VGA print functions, and an irq_handler(registers_t* regs) dispatcher.

Define the registers_t struct that matches exactly what the assembly pushes onto the stack.
```

---

### Prompt 5.3 — PIC Driver

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/pic.c and kernel/pic.h — the 8259 PIC (Programmable Interrupt Controller) driver.

I need:
1. pic_init() — remaps the PIC so IRQ0-7 map to IDT 32-39 and IRQ8-15 map to IDT 40-47 (avoids conflict with CPU exceptions 0-31). Use the standard ICW1-ICW4 initialization sequence.
2. pic_send_eoi(uint8_t irq) — sends End Of Interrupt signal
3. pic_mask(uint8_t irq) and pic_unmask(uint8_t irq) — enable/disable individual IRQs

Explain what the PIC is, why we remap it, and what EOI means.
Use outb/inb port I/O helper functions (show those too).
```

---

## PHASE 6 — Timer (PIT)

### Prompt 6.1

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/timer.c and kernel/timer.h — a PIT (Programmable Interval Timer) driver.

The PIT (Intel 8253/8254) is on I/O port 0x40-0x43.

Implement:
- timer_init(uint32_t frequency) — programs the PIT to fire IRQ0 at the given frequency (e.g., 100 Hz)
- A global tick counter (uint32_t timer_ticks)
- timer_handler() — the IRQ0 handler, increments tick counter, sends EOI, and calls scheduler_tick() (stub for now)
- timer_get_ticks() — returns current tick count
- Register the timer handler with the IDT for IRQ0

Explain the PIT frequency formula: divisor = 1193180 / frequency
```

---

## PHASE 7 — Keyboard Driver

### Prompt 7.1

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/keyboard.c and kernel/keyboard.h — a PS/2 keyboard driver.

The keyboard sends scan codes via IRQ1. Data port is 0x60.

Implement:
- keyboard_init() — registers IRQ1 handler
- keyboard_handler() — reads scan code from port 0x60, translates to ASCII using a scancode map, sends EOI
- A keyboard buffer (circular buffer, 256 bytes) that stores keypresses
- keyboard_getchar() — returns next character from buffer (blocking — spins until a key is available)
- Handle: letters (upper and lower case via shift), numbers, space, enter, backspace

Provide the full US QWERTY scancode-to-ASCII lookup table for scan codes 0x00–0x58.
```

---

## PHASE 8 — Physical Memory Manager

### Prompt 8.1

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/pmm.c and kernel/pmm.h — a physical memory manager using a bitmap allocator.

Assumptions:
- Total RAM: 32 MB (for simplicity)
- Page size: 4096 bytes (4 KB)
- Bitmap: one bit per page frame, stored in a static array

Implement:
- pmm_init(uint32_t total_memory) — initializes bitmap, marks first 1MB as used (reserved for BIOS/bootloader), marks kernel pages as used
- pmm_alloc_frame() — finds first free bit in bitmap, marks it used, returns physical address
- pmm_free_frame(uint32_t addr) — marks a page as free
- pmm_used_frames() and pmm_free_frames() — statistics

Explain the bitmap technique: how you use each bit to represent one 4KB frame, how to find a free bit efficiently using bitwise ops.
```

---

## PHASE 9 — Paging

### Prompt 9.1

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/paging.c and kernel/paging.h — x86 32-bit paging implementation.

x86 paging uses a two-level structure: Page Directory (1024 entries) → Page Tables (1024 entries each) → 4KB pages.

Implement:
- paging_init() — creates and loads a page directory that identity-maps the first 4MB (maps virtual address = physical address), enables paging by setting CR0 bit 31
- page_directory_t and page_table_t structs with correct bit fields (present, rw, user, etc.)
- map_page(uint32_t virtual, uint32_t physical, uint32_t flags) — maps a virtual address to physical
- unmap_page(uint32_t virtual) — unmaps a page
- A page fault handler (ISR 14) that prints the faulting address (from CR2) and halts

Explain:
- What identity mapping means and why we do it initially
- The CR3 register
- The page directory and page table entry format (bit layout)
- What a page fault is and when it occurs
```

---

## PHASE 10 — Process and Scheduler

### Prompt 10.1 — Process Structure

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/process.c and kernel/process.h — process/thread management.

Define a process_t struct that contains:
- pid (process ID)
- state (RUNNING, READY, BLOCKED, TERMINATED)
- A saved CPU context struct (all registers: eax, ebx, ecx, edx, esp, ebp, esi, edi, eip, eflags, cs, ds)
- Stack pointer and allocated stack (4KB per process)
- A name string (char[32])

Implement:
- process_create(const char* name, void (*entry)()) — allocates a stack using pmm_alloc_frame(), sets up an initial stack frame so the process starts at entry(), returns a process_t*
- process_exit() — marks current process as TERMINATED

Explain how the initial stack frame must be set up so that when the scheduler does its first context switch, the process starts executing at entry().
```

---

### Prompt 10.2 — Round Robin Scheduler

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/scheduler.c and kernel/scheduler.h — a round robin CPU scheduler.

Use a fixed-size process table (max 16 processes).

Implement:
- scheduler_init() — initializes the process table
- scheduler_add(process_t* proc) — adds a process to the table
- scheduler_tick() — called by the timer IRQ every tick. Picks the next READY process in round robin order.
- Context switch: save current process registers, load next process registers, update current_process pointer
- scheduler_get_current() — returns the currently running process

The context switch must be done carefully:
1. Show the C side (saving/restoring process_t->context)
2. Show the assembly side — write a switch_context(context_t* old, context_t* new) function in NASM that saves all registers to old and loads from new, then returns into the new process

Explain: why the context switch must be done in assembly, what the stack looks like during a switch, and why we disable interrupts during a switch.
```

---

### Prompt 10.3 — Deadlock Prevention

```
[INCLUDE CONTEXT BLOCK FROM TOP]

I need to demonstrate deadlock management from my OS course. Implement a simple mutex system with deadlock prevention.

Implement in kernel/process.c or a new mutex.c:

1. A mutex_t struct with: locked flag, owner pid, name
2. mutex_init(mutex_t* m, const char* name)
3. mutex_lock(mutex_t* m) — if already locked by another process, block the current process (set state = BLOCKED) and call scheduler_tick(). Use resource ordering to prevent deadlock: assign each mutex a numeric ID, processes must always acquire mutexes in ascending ID order.
4. mutex_unlock(mutex_t* m) — releases mutex, unblocks any waiting process
5. A brief demonstration: create 2 processes and 2 mutexes, show that with resource ordering they cannot deadlock

Explain: what deadlock is, the four Coffman conditions, and how resource ordering (prevention) breaks the circular wait condition.
```

---

## PHASE 11 — Kernel Main

### Prompt 11.1

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/kernel.c — the main kernel entry point.

void kernel_main() must initialize all subsystems in this exact order:
1. vga_init() — clear screen, print "Booting MyOS..."
2. gdt_init() — print "GDT loaded"
3. idt_init() — print "IDT loaded"
4. pic_init() — print "PIC initialized"
5. timer_init(100) — 100Hz timer, print "Timer started"
6. keyboard_init() — print "Keyboard ready"
7. pmm_init(32 * 1024 * 1024) — 32MB RAM, print "PMM ready, X frames free"
8. paging_init() — print "Paging enabled"
9. scheduler_init() — print "Scheduler ready"
10. Enable interrupts with sti
11. Create 2 demo kernel threads that print their name every N ticks
12. Start the shell: shell_init() then shell_run()

The kernel should never return. End with an infinite loop if shell exits.
```

---

## PHASE 12 — Shell

### Prompt 12.1

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Write kernel/shell.c and kernel/shell.h — a simple command-line shell.

Implement:
- shell_init() — prints a welcome banner with OS name and version
- shell_run() — infinite loop: prints "> " prompt, reads a line using keyboard_getchar(), calls shell_execute()
- shell_execute(char* input) — parses the command and dispatches

Commands to implement:
1. help — lists all commands
2. clear — clears the screen using vga_init()
3. echo [text] — prints the text back
4. meminfo — prints used/free frames from PMM
5. ps — prints the process table (pid, name, state) from scheduler
6. timer — prints current tick count

For input reading: implement a readline() that reads characters into a buffer, handles backspace (erase last char from VGA too), and returns on Enter key.

Make the shell look polished with a colored prompt and a welcome message.
```

---

## PHASE 13 — Testing and Debugging

### Prompt 13.1

```
[INCLUDE CONTEXT BLOCK FROM TOP]

Give me a complete testing checklist and QEMU commands for debugging my OS.

Include:
1. The exact QEMU command to run my OS from a disk image (e.g. myos.img)
2. QEMU flags for: enabling debug logging (-d int,cpu_reset), using a serial console, limiting RAM to 32MB
3. How to use QEMU's monitor (Ctrl+Alt+2) to inspect registers and memory
4. A checklist of things to test at each phase:
   - After bootloader: did we enter protected mode? (check CS=0x08)
   - After GDT/IDT: trigger a divide by zero, confirm exception handler fires
   - After timer: confirm tick counter increments (print it in shell)
   - After paging: access an unmapped address, confirm page fault handler runs
   - After scheduler: confirm two threads alternate in output
5. Common crash patterns and what they mean:
   - Triple fault → likely GDT/IDT/stack issue
   - Page fault at 0x00000000 → null pointer dereference
   - General Protection Fault → segment violation, usually wrong GDT
```

---

## Quick Reference — Key Memory Map

```
0x00000000 - 0x000FFFFF  : First 1MB (BIOS, VGA, reserved)
0x00007C00               : Stage 1 MBR loads here
0x00007E00               : Stage 2 loads here
0x00100000 (1MB mark)    : Kernel loads here
0x000B8000               : VGA text buffer
0x00090000               : Stage 2 stack
0x00200000+              : Available for kernel heap / process stacks
```

---

## Quick Reference — Key I/O Ports

```
0x60        : PS/2 Keyboard data
0x20, 0x21  : PIC1 command / data
0xA0, 0xA1  : PIC2 command / data
0x40-0x43   : PIT (timer) ports
0x3F8       : COM1 serial (for QEMU debug output)
```

---

_End of guide. Work through each prompt in order. Always include the Context Block at the top of every new AI conversation._
