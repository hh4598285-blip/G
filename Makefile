CC = gcc
AS = nasm
LD = ld
GRUB_MKRESCUE = grub-mkrescue

CFLAGS = -m32 -std=gnu11 -ffreestanding -fno-pie -fno-stack-protector -fno-builtin -Wall -Wextra -O2
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib
ASFLAGS = -f elf32

KERNEL = kernel.bin
ISO = system.iso
ISO_DIR = iso
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub

.PHONY: all clean iso dirs

all: $(ISO)

$(ISO): $(KERNEL) $(GRUB_DIR)/grub.cfg
	mkdir -p $(BOOT_DIR)
	cp $(KERNEL) $(BOOT_DIR)/kernel.bin
	$(GRUB_MKRESCUE) -o $(ISO) $(ISO_DIR)

$(KERNEL): boot.o kernel.o linker.ld
	$(LD) $(LDFLAGS) -o $(KERNEL) boot.o kernel.o

boot.o: boot.asm
	$(AS) $(ASFLAGS) boot.asm -o boot.o

kernel.o: ./kernel.c
	$(CC) $(CFLAGS) -c ./kernel.c -o kernel.o

$(GRUB_DIR)/grub.cfg: | dirs
	@test -f $(GRUB_DIR)/grub.cfg

dirs:
	mkdir -p $(GRUB_DIR)

iso: $(ISO)

clean:
	rm -f boot.o kernel.o $(KERNEL) $(ISO)
	rm -f $(ISO_DIR)/boot/kernel.bin
