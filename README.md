# MyOS - Hobby Operating System

A 32-bit x86 hobby operating system with a two-stage bootloader, written for a university final project.

## Features Demonstrated

1. **Two-Stage Bootloader** - MBR (512 bytes) + Stage 2 bootloader
2. **Interrupts** - IDT, PIC (8259), IRQ handlers for timer and keyboard
3. **Memory Management** - Physical memory manager (bitmap), paging
4. **CPU Scheduling** - Round-robin scheduler with context switching
5. **Multithreading** - Kernel threads with demo processes

## Project Structure

```
OSollama/
├── boot/
│   ├── stage1.asm       # MBR bootloader (512 bytes)
│   ├── stage2_entry.asm # Stage 2 assembly entry
│   └── stage2.c         # Stage 2 C code (protected mode switch)
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

### 1. Install MSYS2

Download from https://www.msys2.org and install.

### 2. Install Required Packages

Open **MSYS2 UCRT64** shell and run:

```bash
pacman -S mingw-w64-ucrt-x86_64-nasm
pacman -S mingw-w64-ucrt-x86_64-make
pacman -S mingw-w64-ucrt-x86_64-qemu
pacman -S mingw-w64-ucrt-x86_64-gcc
pacman -S mingw-w64-ucrt-x86_64-binutils
```

### 3. Install i686-elf Cross Compiler

Download prebuilt toolchain from:
https://github.com/nicovankooij/i686-elf-toolchain/releases

Or build from source following the OSDev wiki.

Extract and add to PATH in your MSYS2 shell:

```bash
export PATH="/path/to/i686-elf/bin:$PATH"
```

### 4. Verify Installation

```bash
i686-elf-gcc --version
nasm --version
qemu-system-i386 --version
make --version
```

## Building

```bash
make
```

This creates `myos.img` - a bootable disk image.

## Running

```bash
make run
```

Or manually:

```bash
qemu-system-i386 -drive format=raw,file=myos.img -m 32M -vga std
```

## Debugging

```bash
make debug
```

This runs QEMU with debug logging enabled.

Press **Ctrl+Alt+2** in QEMU to open the monitor console for inspecting registers and memory.

## Shell Commands

Once booted, the shell accepts these commands:

- `help` - Show available commands
- `clear` - Clear screen
- `echo [text]` - Print text
- `meminfo` - Show memory information
- `ps` - Show process list
- `timer` - Show timer ticks

## Memory Map

```
0x00000000 - 0x000FFFFF  : First 1MB (reserved)
0x00007C00               : Stage 1 MBR load address
0x00007E00               : Stage 2 load address
0x00100000 (1MB)         : Kernel load address
0x000B8000               : VGA text buffer
0x00200000+              : Available for kernel heap/stacks
```

## Key I/O Ports

```
0x60        : PS/2 Keyboard data
0x20, 0x21  : PIC1 command/data
0xA0, 0xA1  : PIC2 command/data
0x40-0x43   : PIT (timer) ports
```

## Troubleshooting

### Triple Fault
- Check GDT/IDT setup
- Verify stack pointer is valid
- Ensure segment registers are loaded correctly

### Page Fault at 0x00000000
- Null pointer dereference
- Check memory accesses before paging is enabled

### General Protection Fault
- Wrong segment selector
- Invalid GDT entry

## License

This is a hobby/educational project. Feel free to use and modify.
