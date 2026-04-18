# MyOS Makefile — WSL/Linux (clean version)

# Toolchain
CC      = i686-linux-gnu-gcc
LD      = i686-linux-gnu-ld
NASM    = nasm
OBJCOPY = i686-linux-gnu-objcopy
QEMU    = qemu-system-i386

# Directories
BOOT_DIR   = boot
KERNEL_DIR = kernel
BUILD_DIR  = build

# Output
OS_IMAGE = myos.img

# Flags
CFLAGS = -ffreestanding -fno-stack-protector -nostdlib -nodefaultlibs \
         -m32 -O2 -Wall -Wextra -std=c99 -I$(KERNEL_DIR)

# =========================
# Sources
# =========================

# Stage 1 + 2
STAGE1_SRC       = $(BOOT_DIR)/stage1.asm
STAGE2_ENTRY_SRC = $(BOOT_DIR)/stage2_entry.asm
STAGE2_SRC       = $(BOOT_DIR)/stage2.c
STAGE2_LD        = ./stage2.ld

# Kernel
KERNEL_C_SRCS   = $(wildcard $(KERNEL_DIR)/*.c)
KERNEL_ASM_SRCS = $(KERNEL_DIR)/isr.asm $(KERNEL_DIR)/switch.asm

# Objects
KERNEL_C_OBJS   = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(KERNEL_C_SRCS))
KERNEL_ASM_OBJS = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/%.o,$(KERNEL_ASM_SRCS))
ALL_KERNEL_OBJS = $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS)

# =========================
# Targets
# =========================

.PHONY: all clean run debug

all: $(BUILD_DIR) $(OS_IMAGE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# =========================
# Stage 1 (MBR)
# =========================
$(BUILD_DIR)/stage1.bin: $(STAGE1_SRC)
	$(NASM) -f bin $< -o $@

# =========================
# Stage 2
# =========================
$(BUILD_DIR)/stage2_entry.bin: $(STAGE2_ENTRY_SRC)
	$(NASM) -f bin $< -o $@

$(BUILD_DIR)/stage2.o: $(STAGE2_SRC)
	$(CC) $(CFLAGS) -fno-pic -fno-pie -c $< -o $@

$(BUILD_DIR)/stage2.elf: $(BUILD_DIR)/stage2.o $(STAGE2_LD)
	$(LD) -m elf_i386 -T $(STAGE2_LD) -o $@ $<

$(BUILD_DIR)/stage2_c.bin: $(BUILD_DIR)/stage2.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/stage2.bin: $(BUILD_DIR)/stage2_entry.bin $(BUILD_DIR)/stage2_c.bin
	cat $^ > $@

# =========================
# Kernel
# =========================

# C → object
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c
	$(CC) $(CFLAGS) -fno-pic -fno-pie -c $< -o $@

# ASM → object (ELF32!)
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.asm
	$(NASM) -f elf32 $< -o $@

# Link kernel
$(BUILD_DIR)/kernel.elf: $(ALL_KERNEL_OBJS) linker.ld
	$(LD) -m elf_i386 -T linker.ld -o $@ $(ALL_KERNEL_OBJS)

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $< $@

# =========================
# Disk image
# =========================
$(OS_IMAGE): $(BUILD_DIR)/stage1.bin $(BUILD_DIR)/stage2.bin $(BUILD_DIR)/kernel.bin
	dd if=/dev/zero of=$(OS_IMAGE) bs=1M count=10 2>/dev/null
	dd if=$(BUILD_DIR)/stage1.bin  of=$(OS_IMAGE) conv=notrunc bs=512 seek=0  2>/dev/null
	dd if=$(BUILD_DIR)/stage2.bin  of=$(OS_IMAGE) conv=notrunc bs=512 seek=1  2>/dev/null
	dd if=$(BUILD_DIR)/kernel.bin  of=$(OS_IMAGE) conv=notrunc bs=512 seek=11 2>/dev/null
	@echo "Image built: $(OS_IMAGE)"

# =========================
# Run
# =========================
run: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) -m 32M \
	        -serial stdio -monitor none -nographic -device i8042

debug: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) -m 32M \
	        -serial stdio -monitor none -nographic -device i8042 \
	        -d int,cpu_reset -no-reboot -no-shutdown

clean:
	rm -rf $(BUILD_DIR) $(OS_IMAGE)