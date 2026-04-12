# Architecture

**Analysis Date:** 2026-04-09

## Pattern Overview

**Overall:** Monolithic Kernel with Modular Subsystems

**Key Characteristics:**
- Two-stage bootloader (MBR + protected mode transition)
- 16-bit real mode to 32-bit protected mode transition
- Interrupt-driven I/O with BIOS-free operation after boot
- Physical memory management via bitmap allocation
- Round-robin preemptive multitasking

## Boot Process

### Stage 1 (MBR Bootloader)
- **Location:** `boot/stage1.asm`
- **Load Address:** `0x7C00` (standard MBR location)
- **Size:** 512 bytes (1 sector)
- **Responsibilities:**
  1. Initialize CPU state (segments, stack)
  2. Load Stage 2 from disk (LBA 1, 4 sectors)
  3. Verify Stage 2 magic number (`0xAA55`)
  4. Far jump to Stage 2 at `0x7E00`

### Stage 2 (Protected Mode Transition)
- **Location:** `boot/stage2_entry.asm`, `boot/stage2.c`
- **Load Address:** `0x7E00`
- **Responsibilities:**
  1. Enable A20 line (keyboard controller method)
  2. Load temporary GDT for protected mode
  3. Set CR0 bit 0 to enter protected mode
  4. Far jump to 32-bit code segment
  5. Set up 32-bit segment registers (DS, ES, FS, GS, SS)
  6. Set up stack at `0x90000`
  7. Load kernel from disk (LBA 5, 50 sectors) to `0x100000`
  8. Jump to kernel entry point
- **Disk Read:** Uses IDE PIO mode via ports `0x1F0-0x1F7`

### Kernel Entry
- **Location:** `kernel/kernel.c` → `kernel_main()`
- **Load Address:** `0x100000` (1 MB)
- **Initialization Sequence:**
  1. VGA text mode initialization
  2. GDT setup with flat memory model
  3. IDT setup with CPU exceptions and IRQ handlers
  4. PIC initialization and remapping
  5. Timer (PIT) at 100 Hz
  6. Keyboard driver
  7. PMM with 32 MB assumed memory
  8. Paging with 4 MB identity map
  9. Scheduler and process management
  10. Enable interrupts (`sti`)
  11. Run shell

## Memory Map

```
Address Range              Purpose
-----------------------------------------
0x00000000 - 0x00007BFF    Reserved (BIOS/IVT)
0x00007C00 - 0x00007DFF    Stage 1 MBR (512 bytes)
0x00007E00 - 0x00009FFF    Stage 2 bootloader
0x00008000 - 0x00009FFF    Stage 2 C code
0x00090000 - 0x00090FFF    Stage 2 stack (4 KB)
0x000B8000 - 0x000BFFFF    VGA text buffer (4 KB)
0x00100000 - 0x001FFFFF    Kernel (1 MB region)
0x00200000+                PMM-managed memory (30+ MB)
```

**PMM Reservations:**
- First 1 MB (256 frames of 4 KB) marked as used during PMM init
- Kernel loaded at 1 MB (linear mapping in identity-mapped region)

## CPU Mode Transition

### Real Mode (16-bit)
- **Entry:** BIOS loads MBR at `0x7C00`
- **Segments:** Flat (DS=ES=SS=0)
- **Addressing:** 20-bit (segment:offset)
- **Limitations:** 1 MB addressable memory, no protection

### Protected Mode (32-bit)
- **Entry:** Far jump in `stage2_entry.asm`
- **GDT:** 3 entries (null, kernel code, kernel data)
- **Segment Selectors:** CS=0x08, DS/ES/FS/GS/SS=0x10
- **Features Enabled:**
  - 4 GB addressable memory
  - Privilege levels (ring 0 kernel)
  - Interrupt descriptor table

### GDT Layout
```
Entry 0: Null descriptor (required by Intel)
Entry 1: Kernel Code Segment (base=0, limit=4GB, execute/read)
Entry 2: Kernel Data Segment (base=0, limit=4GB, read/write)
```
- **Flags:** Present, Ring 0, 32-bit, 4 KB granularity

## Interrupt System

### IDT (Interrupt Descriptor Table)
- **Location:** `kernel/idt.c`, `kernel/idt.h`
- **Size:** 256 entries
- **Types:**
  - CPU Exceptions (0-31): Division by zero, GPF, page fault, etc.
  - Hardware IRQs (32-47): Timer, keyboard, cascade, etc.
  - Software interrupts (48+): Reserved

### ISR/IRQ Assembly Layer
- **Location:** `kernel/isr.asm`
- **Pattern:** Stub macros push error code (or dummy), push interrupt number, jump to common handler
- **Register Saving:** All general-purpose registers + segment registers
- **C Handler Functions:**
  - `isr_handler()` for CPU exceptions (stops execution on error)
  - `irq_handler()` for hardware interrupts (routes to timer/keyboard)

### PIC (Programmable Interrupt Controller)
- **Location:** `kernel/pic.c`, `kernel/pic.h`
- **Model:** 8259A dual PIC (master + slave)
- **Remapping:** IRQ 0-7 → IDT 32-39, IRQ 8-15 → IDT 40-47
- **API:**
  - `pic_init()`: Initialize and remap IRQs
  - `pic_send_eoi(irq)`: Send End-Of-Interrupt
  - `pic_mask(irq)`: Disable specific IRQ
  - `pic_unmask(irq)`: Enable specific IRQ

## Memory Management

### PMM (Physical Memory Manager)
- **Location:** `kernel/pmm.c`, `kernel/pmm.h`
- **Algorithm:** Bitmap allocator
- **Frame Size:** 4 KB
- **Configuration:** 32 MB total memory (8192 frames)
- **Bitmap Size:** 1024 bytes (1 bit per frame)
- **API:**
  - `pmm_init(total_memory)`: Initialize bitmap, mark first 1 MB as used
  - `pmm_alloc_frame()`: Find free frame, mark used, return address
  - `pmm_free_frame(addr)`: Mark frame as free
- **Strategy:** Linear scan for first free bit

### Paging
- **Location:** `kernel/paging.c`, `kernel/paging.h`
- **Page Size:** 4 KB
- **Model:** Two-level page tables (directory + tables)
- **Identity Map:** First 4 MB mapped 1:1 virtual=physical
- **Page Directory:** 1024 entries × 4 bytes = 4 KB
- **Page Table:** 1024 entries × 4 bytes = 4 KB
- **Features:**
  - Present bit for validity
  - Write bit for permission
  - TLB invalidation via `invlpg` instruction
- **API:**
  - `paging_init()`: Create directory, identity map, enable paging
  - `map_page(virtual, physical, flags)`: Create mapping
  - `unmap_page(virtual)`: Remove mapping
  - `page_fault_handler()`: Debug crash info

## Process Management

### Process Structure
- **Location:** `kernel/process.c`, `kernel/process.h`
- **Max Processes:** 16 concurrent
- **Stack Size:** 4 KB per process
- **States:** RUNNING, READY, BLOCKED, TERMINATED
- **Context:** Saved registers (EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, EIP, EFLAGS, CS, DS)
- **API:**
  - `process_create(name, entry)`: Allocate stack, initialize context
  - `process_exit()`: Mark as terminated, free stack
  - `process_get_current()`: Get running process

### Scheduler
- **Location:** `kernel/scheduler.c`, `kernel/scheduler.h`
- **Algorithm:** Round-robin cooperative
- **Timer:** 100 Hz PIT interrupt drives context switches
- **Context Switch:** `switch.asm` saves/restores CPU state
- **API:**
  - `scheduler_init()`: Clear process queue
  - `scheduler_add(proc)`: Add to queue
  - `scheduler_tick()`: Called on timer interrupt, switch if ready
  - `scheduler_switch(proc)`: Perform context switch

### Context Switch Assembly
- **Location:** `kernel/switch.asm`
- **Function:** `_switch_context(old_context*, new_context*)`
- **Process:**
  1. Save current registers to old context
  2. Load new stack pointer from context
  3. Pop callee-saved registers (EBP, EBX, ESI, EDI)
  4. Set segment registers
  5. Push new EIP, return to it

## I/O Subsystems

### VGA Text Mode
- **Location:** `kernel/vga.c`, `kernel/vga.h`
- **Buffer:** `0xB8000` (memory-mapped)
- **Size:** 80×25 characters, 16 colors
- **Format:** 2 bytes per cell (character + attribute)
- **Features:** Scrolling, cursor tracking, color control
- **API:** `vga_print()`, `vga_putchar()`, `vga_print_hex()`, `vga_print_int()`

### Keyboard Driver
- **Location:** `kernel/keyboard.c`, `keyboard.h`
- **Port:** `0x60` (data), `0x64` (status)
- **Buffer:** Circular buffer (1024 chars)
- **IRQ:** IRQ1 (IDT entry 33)
- **Features:** Shift, caps lock, scancode to ASCII translation
- **API:** `keyboard_init()`, `keyboard_getchar()`

### Timer
- **Location:** `kernel/timer.c`, `kernel/timer.h`
- **Hardware:** PIT (Programmable Interval Timer) channel 0
- **Frequency:** Configurable (default 100 Hz)
- **Port:** `0x40-0x43`
- **Ticks:** Global counter incremented on each interrupt
- **API:** `timer_init(freq)`, `timer_get_ticks()`

## Kernel Entry Point

### Initialization Sequence
```c
// kernel/kernel.c - kernel_main()
vga_init();           // Console output
gdt_init();           // Proper GDT (not bootloader temp)
idt_init();           // Interrupt handlers
pic_init();           // IRQ remapping
timer_init(100);      // 100 Hz system timer
keyboard_init();       // PS/2 keyboard
pmm_init(32 * 1024 * 1024);  // 32 MB physical memory
paging_init();        // Enable paging
scheduler_init();     // Process scheduler
__asm__ volatile ("sti");  // Enable interrupts
// Create demo threads
shell_init();
shell_run();          // Interactive CLI
```

## Driver Integration

### Assembly ↔ C Interface
- **ISR Stubs:** `kernel/isr.asm` exports `_isr0` through `_isr31`, `_irq0` through `_irq15`
- **Context Switch:** `kernel/switch.asm` exports `_switch_context`
- **Calling Convention:** C caller saves (EAX, ECX, EDX), callee saves (EBX, ESI, EDI, EBP)
- **Register Layout:** `struct registers` in `idt.c` matches stack frame from `isr.asm`

### Header Dependencies
- All kernel modules include their own `.h` file
- Shared types in `kernel/stdint.h` (`uint8_t`, `uint32_t`, etc.)
- No standard library (freestanding build)

---

*Architecture analysis: 2026-04-09*