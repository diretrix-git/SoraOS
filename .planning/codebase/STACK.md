# Technology Stack

**Analysis Date:** 2026-04-09

## Languages

**Primary:**
- C (freestanding) - Kernel implementation, bootloader Stage 2 C code
- Assembly (NASM syntax) - Bootloader, low-level CPU operations, context switching

**Secondary:**
- None - Bare-metal OS with no higher-level languages

## Runtime

**Environment:**
- 32-bit x86 protected mode
- No runtime library (freestanding)
- No standard library dependencies

**Package Manager:**
- None - Direct compilation with GCC toolchain
- Package manifest: None

## Frameworks

**Core:**
- Bare-metal - Direct hardware access, no OS framework
- BIOS services (Stage 1 only) - Disk I/O, text output in real mode

**Testing:**
- None - No automated test infrastructure detected

**Build/Dev:**
- Make - Build automation via `Makefile`
- QEMU - x86 emulation for testing via `qemu-system-i386`

## Key Dependencies

**Critical:**
- GCC (mingw-w64-ucrt-x86_64-gcc) - Cross-compilation targeting i686
- NASM (mingw-w64-ucrt-x86_64-nasm) - Assembler for boot code and low-level routines
- Binutils (mingw-w64-ucrt-x86_64-binutils) - Linker and object manipulation

**Infrastructure:**
- QEMU (mingw-w64-ucrt-x86_64-qemu) - Emulation and testing
- Make (mingw-w64-ucrt-x86_64-make) - Build orchestration
- MSYS2/UCRT64 - Windows development environment

## Configuration

**Environment:**
- Windows with MSYS2 UCRT64 shell
- Toolchain: GCC for 32-bit (`-m32` flag)
- Cross-compiler recommended: `i686-elf-gcc` (README mentions external toolchain)

**Build:**
- `Makefile` - Main build configuration
- `linker.ld` - Linker script for kernel memory layout
- Kernel entry point: `kernel_main` at 0x100000 (1MB)

**Compiler Flags:**
- `-ffreestanding` - No standard library linkage
- `-fno-stack-protector` - Disable stack protection
- `-nostdlib` - No standard library
- `-nodefaultlibs` - No default libraries
- `-m32` - 32-bit code generation
- `-O2` - Optimization level 2
- `-Wall -Wextra` - Warning flags

## Platform Requirements

**Development:**
- Windows host with MSYS2 UCRT64 environment
- NASM assembler for assembly files
- GCC cross-compiler (i686-elf-gcc recommended)
- QEMU for testing

**Production:**
- Target: x86 (32-bit) protected mode
- Boot: BIOS bootable disk image (raw format)
- Memory: Minimum 32MB RAM configured in QEMU
- CPU: i386 compatible

---

*Stack analysis: 2026-04-09*