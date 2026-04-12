# MyOS Makefile
# Builds a two-stage bootloader and kernel for x86 32-bit

# Toolchain
CC = gcc.exe
LD = ld.exe
NASM = C:/msys64/ucrt64/bin/nasm.exe
OBJCOPY = objcopy.exe

# Directories
BOOT_DIR = boot
KERNEL_DIR = kernel
BUILD_DIR = build

# Output files
OS_IMAGE = myos.img

# Compiler flags
CFLAGS = -ffreestanding -fno-stack-protector -nostdlib -nodefaultlibs \
         -m32 -O2 -Wall -Wextra -I$(KERNEL_DIR)
LDFLAGS = -m32 -T linker.ld -nostdlib

# NASM flags
NASM_FLAGS = -f bin

# Source files
STAGE1_SRC = $(BOOT_DIR)/stage1.asm
STAGE2_ENTRY_SRC = $(BOOT_DIR)/stage2_entry.asm
STAGE2_SRC = $(BOOT_DIR)/stage2.c

KERNEL_SRCS = $(wildcard $(KERNEL_DIR)/*.c)
KERNEL_ASM_SRCS = $(wildcard $(KERNEL_DIR)/*.asm)

# Object files
STAGE2_ENTRY_OBJ = $(BUILD_DIR)/stage2_entry.o
STAGE2_OBJ = $(BUILD_DIR)/stage2.o
KERNEL_OBJS = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))
KERNEL_ASM_OBJS = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/%.o,$(KERNEL_ASM_SRCS))

# All object files for kernel
ALL_KERNEL_OBJS = $(KERNEL_OBJS) $(KERNEL_ASM_OBJS)

.PHONY: all clean run debug

all: $(BUILD_DIR) $(OS_IMAGE)

$(BUILD_DIR):
	mkdir "$(BUILD_DIR)" 2>nul || mkdir -p "$(BUILD_DIR)"

# =============================================================================
# Stage 1 Bootloader (MBR)
# =============================================================================
$(BUILD_DIR)/stage1.bin: $(STAGE1_SRC)
	$(NASM) $(NASM_FLAGS) $< -o $@

# =============================================================================
# Stage 2 Bootloader
# =============================================================================
$(BUILD_DIR)/stage2_entry.bin: $(STAGE2_ENTRY_SRC)
	$(NASM) -f bin $< -o $@

$(BUILD_DIR)/stage2.o: $(STAGE2_SRC)
	$(CC) $(CFLAGS) -c $< -o $@ -fno-pic -fno-pie

$(BUILD_DIR)/stage2_c.bin: $(BUILD_DIR)/stage2.o
	$(OBJCOPY) -O binary -j .text $< $@

$(BUILD_DIR)/stage2.bin: $(BUILD_DIR)/stage2_entry.bin $(BUILD_DIR)/stage2_c.bin
	cat "$(BUILD_DIR)/stage2_entry.bin" "$(BUILD_DIR)/stage2_c.bin" > "$@"

# =============================================================================
# Kernel
# =============================================================================
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c
	$(CC) $(CFLAGS) -m32 -c $< -o $@

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.asm
	$(NASM) -f win32 $< -o $@

$(BUILD_DIR)/kernel.bin: $(ALL_KERNEL_OBJS)
	$(CC) -B C:/msys64/ucrt64/bin/ -fno-use-linker-plugin $(LDFLAGS) $^ -o $(BUILD_DIR)/kernel.exe -Wl,--subsystem=native,-e,_kernel_main
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel.exe $@

# =============================================================================
# Create disk image
# =============================================================================
$(OS_IMAGE): $(BUILD_DIR)/stage1.bin $(BUILD_DIR)/stage2.bin $(BUILD_DIR)/kernel.bin
	# Create a 10MB disk image
	dd if=/dev/zero of=$(OS_IMAGE) bs=1M count=10 2>/dev/null
	
	# Write stage 1 (MBR) at sector 0
	dd if=$(BUILD_DIR)/stage1.bin of=$(OS_IMAGE) conv=notrunc bs=512 seek=0 2>/dev/null
	
	# Write stage 2 at sector 1 (LBA 1)
	dd if=$(BUILD_DIR)/stage2.bin of=$(OS_IMAGE) conv=notrunc bs=512 seek=1 2>/dev/null
	
	# Write kernel at sector 5 (LBA 5)
	dd if=$(BUILD_DIR)/kernel.bin of=$(OS_IMAGE) conv=notrunc bs=512 seek=5 2>/dev/null
	
	echo "OS image created: $(OS_IMAGE)"

# =============================================================================
# Run in QEMU
# =============================================================================
run: $(OS_IMAGE)
	qemu-system-i386 -drive format=raw,file=$(OS_IMAGE) -m 32M -vga std

debug: $(OS_IMAGE)
	qemu-system-i386 -drive format=raw,file=$(OS_IMAGE) -m 32M -vga std \
		-d int,cpu_reset -no-reboot -no-shutdown

clean:
	-rmdir /s /q $(BUILD_DIR) 2>nul || echo "Build directory cleaned"
	-del /f $(OS_IMAGE) 2>nul || echo "Image deleted"
