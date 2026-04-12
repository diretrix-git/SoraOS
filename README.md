# os - Hobby Operating System

A 32-bit x86 hobby operating system with a two-stage bootloader, protected mode kernel, interrupt handling, paging, and round-robin scheduling.

> Repository renamed from `OSollama` to `os`.

## Features Demonstrated

1. **Two-Stage Bootloader** - MBR + protected-mode stage 2 loader
2. **Interrupts** - IDT, PIC (8259), timer and keyboard IRQ handling
3. **Memory Management** - Physical memory manager and paging support
4. **CPU Scheduling** - Simple round-robin scheduler with context switching
5. **Shell** - Basic CLI with process and memory commands

## Project Structure

```
os/
├── boot/
│   ├── stage1.asm       # MBR bootloader (512 bytes)
│   ├── stage2_entry.asm # Stage 2 assembly entry
│   └── stage2.c         # Stage 2 C code for protected mode setup
├── kernel/
│   ├── kernel.c         # Kernel main entry
│   ├── vga.c/h          # VGA text mode driver
│   ├── gdt.c/h          # Global Descriptor Table
│   ├── idt.c/h          # Interrupt Descriptor Table
│   ├── isr.asm          # ISR/IRQ assembly stubs
│   ├── pic.c/h          # PIC (8259) driver
│   ├── timer.c/h        # PIT timer driver
│   ├── keyboard.c/h     # PS/2 keyboard driver
│   ├── pmm.c/h          # Physical memory manager
│   ├── paging.c/h       # Paging implementation
│   ├── process.c/h      # Process management
│   ├── scheduler.c/h    # Round-robin scheduler
│   ├── switch.asm       # Context switch assembly
│   ├── shell.c/h        # Simple CLI shell
│   └── stdint.h         # Standard integer types
├── linker.ld            # Linker script
├── Makefile
└── README.md
```

## Prerequisites (Windows)

### 1. Install MSYS2 UCRT64

Download MSYS2 from https://www.msys2.org and install it.

### 2. Install Required Packages

Open the **MSYS2 UCRT64** shell and run:

```bash
pacman -S mingw-w64-ucrt-x86_64-nasm mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-binutils mingw-w64-ucrt-x86_64-qemu
```

### 3. Verify Installation

```bash
gcc.exe --version
nasm --version
qemu-system-i386 --version
make --version
```

## Build

```bash
make
```

This creates a bootable `myos.img` disk image.

## Run

```bash
make run
```

Or manually:

```bash
qemu-system-i386 -drive format=raw,file=myos.img -m 32M -vga std
```

## Debug

```bash
make debug
```

## Notes

- `.claude/`, `.planning/`, and `-p/` are kept out of git and removed from the repository.
- `build/` and `myos.img` are generated artifacts and are not tracked.
- The project name and repo have been updated to `os`.

## Shell Commands

- `help` - Show available commands
- `clear` - Clear screen
- `echo [text]` - Print text
- `meminfo` - Show memory information
- `ps` - Show process list
- `timer` - Show timer ticks

## License

This is a hobby/educational project. Feel free to use and modify.
