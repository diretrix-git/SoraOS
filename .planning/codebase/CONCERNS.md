# Codebase Concerns

**Analysis Date:** 2026-04-09

## Tech Debt

### No User Mode Support (Ring 3)
- **Issue:** All code runs at ring 0 (highest privilege). No separation between kernel and user processes.
- **Files:** `kernel/gdt.c`, `kernel/idt.c`
- **Impact:** Any process can access any memory, execute privileged instructions, and crash the entire system. No security boundary exists between processes.
- **Fix approach:** 
  1. Add ring 3 code/data segment descriptors to GDT (`gdt.c`)
  2. Create user mode switching mechanism with proper TSS setup
  3. Separate page tables per process with user/supervisor bit
  4. Implement privilege level transitions via call gates or sysenter/sysexit

### Fixed Process Table Limit
- **Issue:** Hard-coded `MAX_PROCESSES` limit of 16 processes.
- **Files:** `kernel/process.h` (line 6)
- **Impact:** Cannot create more than 16 processes. Silent failure returns `0` (null) from `process_create()`.
- **Fix approach:** Use dynamic allocation or linked list-based process table. Return proper error status instead of null.

### Global Static Process State
- **Issue:** All process state is in static global arrays without synchronization.
- **Files:** `kernel/process.c` (lines 4-6), `kernel/scheduler.c` (lines 4-7)
- **Impact:** Not thread-safe. Cannot scale to SMP (multi-core). State corruption risk if interrupts modify process state during context switches.
- **Fix approach:** Use per-CPU data structures or implement proper locking primitives.

### Static PMM Bitmap
- **Issue:** Physical memory bitmap is statically allocated at compile time (8KB for 32MB).
- **Files:** `kernel/pmm.c` (line 8)
- **Impact:** Memory size is hard-coded to 32MB. Cannot run on systems with different memory configurations.
- **Fix approach:** Dynamically allocate bitmap based on actual memory size from BIOS memory map.

---

## Known Bugs

### Context Switch Assembly Race Condition
- **Issue:** The context switch assembly doesn't properly save/restore EAX, ECX, EDX. The comments claim to save them but the implementation has inconsistencies.
- **Files:** `kernel/switch.asm` (lines 45-47)
- **Impact:** Registers may be corrupted during context switch, causing unpredictable behavior in multi-threaded execution.
- **Workaround:** Currently only affects demo threads that don't rely heavily on register preservation.

### Page Fault Handler Hangs System
- **Issue:** Page fault handler enters infinite `hlt` loop without recovery or meaningful diagnostics.
- **Files:** `kernel/paging.c` (lines 98-100)
- **Impact:** Any page fault (including null pointer dereferences) causes permanent hang requiring reboot.
- **Fix approach:** Implement proper page fault recovery with process termination or system panic message.

### Exception Handler Infinite Hang
- **Issue:** All CPU exceptions cause infinite `hlt` loop without stack trace or debug info.
- **Files:** `kernel/idt.c` (lines 167-169)
- **Impact:** Debugging crashes requires external hardware debugger. No post-mortem analysis possible.
- **Fix approach:** Add register dump, stack trace, and optional crash log to VGA buffer.

---

## Security Considerations

### No Memory Protection Between Processes
- **Risk:** Any process can read/write any memory location, including kernel code/data.
- **Files:** `kernel/paging.c`, `kernel/process.c`
- **Current mitigation:** None - flat memory model with identity mapping.
- **Recommendations:**
  1. Implement separate page directories per process
  2. Mark kernel pages with supervisor-only bit
  3. Implement user/kernel address space separation (higher-half kernel or split address space)

### No Input Validation in Shell
- **Risk:** Shell buffer overflow if input exceeds `SHELL_BUFFER_SIZE` (256 bytes).
- **Files:** `kernel/shell.c` (lines 48-50)
- **Current mitigation:** Bounds check exists but truncates silently.
- **Recommendations:** Add overflow warning, consider input length enforcement at keyboard handler level.

### No Address Space Layout Randomization (ASLR)
- **Risk:** Predictable memory layout makes exploitation easier.
- **Files:** `linker.ld` (fixed load addresses)
- **Current mitigation:** None - all addresses are deterministic.
- **Recommendations:** Add randomization to kernel load address and process stack locations.

### Interrupt Handlers All Ring 0
- **Risk:** All IDT gates configured for ring 0 only.
- **Files:** `kernel/idt.c` (lines 89-137)
- **Current mitigation:** None - no user mode exists.
- **Recommendations:** Add ring 3 interrupt gates for user mode exceptions and system calls.

---

## Performance Bottlenecks

### Linear PMM Frame Search
- **Issue:** Memory allocation scans bitmap linearly from start every time.
- **Files:** `kernel/pmm.c` (lines 24-35)
- **Impact:** O(n) allocation time where n = number of frames. Degrades as memory fills up.
- **Improvement path:** Keep track of last allocated frame hint, use buddy allocator, or maintain free frame linked list.

### No TLB Optimization on Page Operations
- **Issue:** Every page mapping change calls `invlpg` individually.
- **Files:** `kernel/paging.c` (lines 72, 82)
- **Impact:** Single page flush is expensive; multiple operations are slow.
- **Improvement path:** Batch page table updates when possible, use `cr3` reload for full TLB flush when updating many pages.

### VGA Scrolling Memcpy Loop
- **Issue:** Screen scrolling implemented with simple loop copy in C.
- **Files:** `kernel/vga.c` (lines 55-60)
- **Impact:** Slow for large scroll amounts. No DMA or hardware acceleration.
- **Improvement path:** Use `rep movsw` assembly instruction for faster block copy, or implement hardware scrolling if supported.

### Context Switch Overhead
- **Issue:** Full register save/restore on every timer tick (100Hz = 100 switches/second).
- **Files:** `kernel/switch.asm`, `kernel/scheduler.c`
- **Impact:** CPU spends 100Hz minimum doing context switch, plus ISR overhead.
- **Improvement path:** Implement lazy FPU save, reduce timer frequency, or use smarter scheduling (only switch on yield/IO).

---

## Fragile Areas

### Boot Process Chain of Dependencies
- **Files:** `kernel/kernel.c` (lines 40-83)
- **Why fragile:** Initialization order is critical. GDT must load before IDT, IDT before enabling interrupts, PMM before paging. Wrong order causes triple fault.
- **Safe modification:** Add boot stage validation checks between each init step.
- **Test coverage:** None - boot chain has no fallback or recovery.

### Context Switch Assembly Correctness
- **Files:** `kernel/switch.asm`
- **Why fragile:** Assembly must match exactly with `struct cpu_context` layout. Any mismatch corrupts process state silently.
- **Test coverage:** None - context switch has no self-test or verification.

### Stack Pointer Assumptions
- **Files:** `kernel/process.c` (lines 23, 32)
- **Why fragile:** Stack allocated as single 4KB page grows down. No guard page to detect overflow. Stack collisions corrupt adjacent memory silently.
- **Safe modification:** Add red zones (poison pattern) at stack boundaries for overflow detection.

### Keyboard Buffer Lockless Ring Buffer
- **Files:** `kernel/keyboard.c` (lines 34-48)
- **Why fragile:** Producer (ISR) and consumer (main loop) share buffer without synchronization. Race conditions possible on `head`/`tail` modification.
- **Test coverage:** None - could cause dropped or duplicated keystrokes under high interrupt rate.

---

## Scaling Limits

### Physical Memory Hardcoded to 32MB
- **Current capacity:** 32 MB (`pmm.c` line 3)
- **Limit:** Cannot use more memory. Wastes available RAM on larger systems.
- **Scaling path:** Parse BIOS memory map at boot, dynamically size PMM bitmap.

### Process Count Limited to 16
- **Current capacity:** 16 concurrent processes (`process.h` line 6)
- **Limit:** Cannot create more threads/processes.
- **Scaling path:** Make process table dynamic or linked list-based.

### Single Processor Only
- **Current capacity:** 1 CPU core
- **Limit:** No SMP/Multi-core support. GDT, IDT, process state all assume single CPU.
- **Scaling path:** 
  1. Add per-CPU GDT/TSS
  2. Implement atomic operations and spinlocks
  3. Add APIC support for inter-processor interrupts
  4. Separate per-CPU scheduler state

### Stack Size Fixed at 4KB
- **Current capacity:** 4096 bytes per process stack (`process.h` line 7)
- **Limit:** Deep recursion or large stack allocations cause silent overflow.
- **Scaling path:** Make stack size configurable per process, add guard pages.

---

## Dependencies at Risk

### Cross Compiler Dependency
- **Issue:** Requires `i686-elf-gcc` cross compiler (README lines 60-71).
- **Risk:** Build breaks if toolchain not available. Cross compiler setup is non-trivial.
- **Impact:** Cannot build OS without specific toolchain.
- **Migration plan:** Document Docker container with pre-built toolchain, or add build scripts for toolchain compilation.

### QEMU-Specific Configuration
- **Issue:** Run command uses QEMU-specific flags (README line 99).
- **Risk:** Porting to other emulators or real hardware requires different boot approach.
- **Impact:** May not run on bare metal without hardware-specific drivers (e.g., actual PIT timing, keyboard controller).

---

## Technical Debt

### Incomplete ISR17 (Alignment Check)
- **Issue:** Comment says "has error code" but uses `ISR_NOERR` macro.
- **Files:** `kernel/isr.asm` (line 60)
- **Impact:** Incorrect error code handling for alignment check exception.
- **Fix:** Should use `ISR_ERR 17` to properly handle error code.

### Duplicate GDT Initialization
- **Issue:** GDT is initialized in both `boot/stage2_entry.asm` and `kernel/gdt.c`.
- **Files:** `boot/stage2_entry.asm` (lines 94-102), `kernel/gdt.c` (lines 28-65)
- **Impact:** Wastes time, potential confusion about which GDT is active. Overwrites stage 2 GDT with same values.
- **Fix:** Move GDT init entirely to kernel, or add comment explaining why both exist (early boot vs. kernel GDT).

### No Heap Allocator
- **Issue:** PMM provides frame allocation but no kmalloc/kfree for smaller allocations.
- **Files:** `kernel/pmm.c`
- **Impact:** All allocations must be page-sized. Cannot dynamically allocate smaller objects efficiently.
- **Fix:** Implement slab allocator or linked-list heap on top of PMM.

### Shell Command "reboot" Not Implemented
- **Issue:** Listed in help but shows "not implemented".
- **Files:** `kernel/shell.c` (line 68)
- **Impact:** Poor user experience, incomplete feature.
- **Fix:** Implement via CPU reset (0xCF9 port) or keyboard controller reset, or remove from help list.

### No Process Cleanup
- **Issue:** `process_exit()` frees stack but process slot remains in `TERMINATED` state forever.
- **Files:** `kernel/process.c` (lines 58-69)
- **Impact:** Process slots leak. After 16 process creations, no new processes can be created even if old ones terminated.
- **Fix:** Implement process reaping in scheduler to recycle terminated process slots.

---

## Missing Critical Features

### No System Call Interface
- **Issue:** No syscall mechanism. All code runs in kernel mode.
- **Files:** Entire kernel
- **Impact:** Cannot run untrusted code. No separation between kernel and applications.

### No File System
- **Issue:** No persistent storage support.
- **Files:** None - feature doesn't exist.
- **Impact:** Cannot save state, load programs, or persist data across reboots.
- **Blocks:** User applications, configuration persistence, program loading.

### No Virtual Memory Per Process
- **Issue:** All processes share same address space (identity mapped).
- **Files:** `kernel/paging.c`
- **Impact:** No memory isolation. Process crash corrupts entire system. Limited to physical memory size.
- **Blocks:** Process isolation, security, memory overcommit.

### No Inter-Process Communication (IPC)
- **Issue:** No pipes, signals, shared memory, or message passing.
- **Files:** None - feature doesn't exist.
- **Impact:** Processes cannot coordinate or exchange data.
- **Blocks:** Multi-process applications, device drivers, service architecture.

---

## Test Coverage Gaps

### No Automated Tests
- **What's not tested:** Entire codebase
- **Files:** N/A
- **Risk:** Any change could break critical functionality. Manual testing required for every change.
- **Priority:** High - Add basic boot test, context switch validation, memory allocator stress test.

### Boot Process Untested
- **What's not tested:** Stage 1 → Stage 2 → Kernel transition
- **Files:** `boot/stage1.asm`, `boot/stage2_entry.asm`, `boot/stage2.c`
- **Risk:** Boot failure on different hardware/configurations. No validation of GDT, A20, protected mode switch.
- **Priority:** Medium - Add boot signature validation, memory integrity checks.

### Context Switch Untested
- **What's not tested:** Save/restore of all registers, stack switching, segment loading
- **Files:** `kernel/switch.asm`
- **Risk:** Silent corruption of process state. Incorrect assembly could cause subtle bugs.
- **Priority:** High - Add self-test that validates register preservation across switch.

### Scheduler Edge Cases Untested
- **What's not tested:** Empty queue, single process, all processes blocked, max processes reached
- **Files:** `kernel/scheduler.c`
- **Risk:** Scheduler could hang or crash in edge cases.
- **Priority:** Medium - Add scheduler state invariant checks.

---

## Known Limitations (from README)

### Limited Shell Commands
- **Constraint:** Only 6 commands implemented (help, clear, echo, meminfo, ps, timer).
- **Impact:** Limited interactive capability. Cannot manage processes, view files, or configure system.
- **Workaround:** Add more commands as needed; shell architecture supports extension.

### Specific Memory Map Required
- **Constraint:** Hard-coded memory layout:
  ```
  0x00000000 - 0x000FFFFF  : First 1MB (reserved)
  0x00007C00               : Stage 1 MBR load address
  0x00007E00               : Stage 2 load address
  0x00100000 (1MB)         : Kernel load address
  0x000B8000               : VGA text buffer
  0x00200000+              : Available for kernel heap/stacks
  ```
- **Impact:** Won't boot on systems with non-standard memory maps or addresses in use.
- **Workaround:** Currently none - requires system with compatible memory layout (QEMU with 32MB RAM).

### No Persistence
- **Constraint:** All state lost on reboot.
- **Impact:** Configuration, logs, user data, loaded programs all disappear.
- **Workaround:** None - requires file system implementation.

---

*Concerns audit: 2026-04-09*