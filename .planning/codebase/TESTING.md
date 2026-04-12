# Testing Patterns

**Analysis Date:** 2026-04-09

## Test Framework

**Runner:**
- No automated test framework
- Manual testing only through QEMU runtime verification

**Assertion Library:**
- None - visual verification via VGA output

**Run Commands:**
```bash
make                  # Build myos.img
make run              # Boot in QEMU
make debug            # Boot in QEMU with debug flags
make clean            # Remove build artifacts
```

## Test File Organization

**Location:**
- No test files present in project
- No test directory

**Testing Approach:**
- Build verification: successful `make` producing `myos.img`
- Runtime verification: QEMU boot testing

## Test Structure

**Build-time Verification:**
- Compilation succeeds with `-Wall -Wextra` enabled
- Linking produces valid kernel binary
- Disk image creation completes without errors

**Boot-time Verification:**
```c
// Visual confirmation in kernel_main()
vga_init();
vga_print("Booting MyOS...\n");
vga_print("GDT loaded\n");
vga_print("IDT loaded\n");
vga_print("PIC initialized\n");
vga_print("Timer started (100Hz)\n");
vga_print("Keyboard ready\n");
vga_print("PMM ready: ... frames free\n");
vga_print("Paging enabled\n");
vga_print("Scheduler ready\n");
vga_print("Interrupts enabled\n");
```

**Interactive Testing:**
- Shell commands accessible at `myos>` prompt
- Commands available for verification:
  - `help` - list commands
  - `meminfo` - verify PMM allocation counts
  - `ps` - verify process/scheduler state
  - `timer` - verify timer tick counter
  - `echo <text>` - verify shell input/output

## Mocking

**Framework:** None

**Hardware Abstraction:**
- Direct hardware access via inline assembly
- No mocking layer for hardware I/O
- Port I/O functions inline:
  ```c
  static inline void outb(uint16_t port, uint8_t value) {
      __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
  }
  static inline uint8_t inb(uint16_t port) {
      uint8_t result;
      __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
      return result;
  }
  ```

## Fixtures and Factories

**Test Data:**
- Not applicable - no unit tests
- Runtime system uses static allocations only

**Location:**
- N/A

## Coverage

**Requirements:**
- No coverage requirements
- No coverage tooling

**Manual Verification Areas:**
- Boot sequence: GDT → IDT → PIC → Timer → Keyboard → PMM → Paging
- Interrupt handling: timer IRQ, keyboard IRQ, CPU exceptions
- Memory management: allocation/deallocation, page mapping
- Process scheduling: context switch on timer tick
- Shell: input parsing, command execution

## Test Types

**Unit Tests:**
- Not implemented
- Individual functions tested through runtime behavior only

**Integration Tests:**
- Boot sequence acts as integration test
- Component initialization chain verifies integration
- Shell commands verify end-to-end functionality

**E2E Tests:**
- QEMU boot test is the primary E2E test
- Manual interaction through shell

## Common Patterns

**Exception Testing:**
- CPU exceptions halt with message:
  ```c
  void isr_handler(struct registers* r) {
      vga_print("\n!!! CPU EXCEPTION !!!\n");
      vga_print("Exception: ");
      vga_print(exception_messages[r->int_no]);
      vga_print("\nError code: 0x");
      vga_print_hex(r->err_code);
      vga_print("\nEIP: 0x");
      vga_print_hex(r->eip);
      vga_print("\n");
      while (1) {
          __asm__ volatile ("hlt");
      }
  }
  ```

**Page Fault Testing:**
- Page fault handler provides detailed diagnostics:
  ```c
  void page_fault_handler(uint32_t error_code, uint32_t faulting_address) {
      vga_print("\n!!! PAGE FAULT !!!\n");
      vga_print("Faulting address: 0x");
      vga_print_hex(faulting_address);
      vga_print("\nError code: 0x");
      vga_print_hex(error_code);
      // Decode error bits
      while (1) { __asm__ volatile ("hlt"); }
  }
  ```

## Debug Mode

**QEMU Debug Launch:**
```bash
make debug
# Equivalent to:
qemu-system-i386 -drive format=raw,file=myos.img -m 32M -vga std \
    -d int,cpu_reset -no-reboot -no-shutdown
```

**Debug Flags:**
- `-d int,cpu_reset` - Log interrupts and CPU resets
- `-no-reboot` - Halt on triple fault instead of rebooting
- `-no-shutdown` - Don't exit on shutdown

**QEMU Monitor:**
- QEMU launched without `-nographic` for interactive debugging
- Monitor commands available (Ctrl+Alt+2 to access):
  - `info registers` - View CPU state
  - `x/10x $esp` - Examine memory
  - `xp/10x <addr>` - Examine physical memory

## Limitations

**Current Gaps:**
- **No unit tests** - Individual functions not tested in isolation
- **No automated test runner** - Manual QEMU boot required
- **No regression testing** - Changes verified manually
- **No CI/CD integration** - Build/run commands local only
- **No memory leak detection** - PMM not verified for correctness
- **Undefined behavior detection** - No sanitizers configured

**Manual Testing Requirements:**
- Visual inspection of boot messages
- Shell command verification (`help`, `meminfo`, `ps`, `timer`, `echo`)
- Thread switching observation (Thread1/Thread2 output alternation)
- Keyboard input handling verification

**Testing Recommendations:**
- Add QEMU automation scripts for boot verification
- Create integration tests for core subsystems (GDT, IDT, PMM, paging)
- Add memory allocation stress tests
- Implement syscall/interrupt stress tests
- Consider adding simple kernel panic on assertion failures

---

## Makefile Build Process

**Build Steps:**
```makefile
# Stage 1: Compile bootloader assembly
nasm -f bin boot/stage1.asm -o build/stage1.bin

# Stage 2: Compile and link bootloader C entry + C code
nasm -f bin boot/stage2_entry.asm -o build/stage2_entry.bin
gcc -ffreestanding -m32 -c boot/stage2.c -o build/stage2.o
objcopy -O binary build/stage2.o build/stage2_c.bin
# Combine entry + C code
copy /b build\stage2_entry.bin + build\stage2_c.bin build\stage2.bin

# Kernel: Compile all .c and .asm files
gcc -ffreestanding -m32 -c kernel/*.c -o build/*.o
nasm -f win32 kernel/*.asm -o build/*.o

# Link kernel
ld -m i386pe -e _kernel_main build/*.o -o build/kernel.exe
objcopy -O binary build/kernel.exe build/kernel.bin

# Create disk image
dd if=/dev/zero of=myos.img bs=1M count=10
dd if=build/stage1.bin of=myos.img conv=notrunc bs=512 seek=0
dd if=build/stage2.bin of=myos.img conv=notrunc bs=512 seek=1
dd if=build/kernel.bin of=myos.img conv=notrunc bs=512 seek=5
```

**Verification Points:**
1. All `.c` files compile without errors
2. All `.asm` files assemble without errors
3. Linking succeeds
4. `myos.img` created (10MB disk image)
5. QEMU boots to shell prompt

---

*Testing analysis: 2026-04-09*