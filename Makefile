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
OBJS = boot.o kernel.o system.o memory.o fs.o idt.o pit.o keyboard.o

.PHONY: all clean iso dirs

all: $(ISO)

$(ISO): $(KERNEL) $(GRUB_DIR)/grub.cfg
	mkdir -p $(BOOT_DIR)
	cp $(KERNEL) $(BOOT_DIR)/kernel.bin
	$(GRUB_MKRESCUE) -o $(ISO) $(ISO_DIR)

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJS)

boot.o: boot.asm
	$(AS) $(ASFLAGS) boot.asm -o boot.o

kernel.o: ./kernel.c
	$(CC) $(CFLAGS) -c ./kernel.c -o kernel.o

system.o: ./system.c ./system.h
	$(CC) $(CFLAGS) -c ./system.c -o system.o

memory.o: ./memory.c
	$(CC) $(CFLAGS) -c ./memory.c -o memory.o

fs.o: ./fs.c
	$(CC) $(CFLAGS) -c ./fs.c -o fs.o

idt.o: ./idt.c ./idt.h ./keyboard.h
	$(CC) $(CFLAGS) -c ./idt.c -o idt.o

pit.o: ./pit.c ./pit.h
	$(CC) $(CFLAGS) -c ./pit.c -o pit.o

keyboard.o: ./keyboard.c ./keyboard.h
	$(CC) $(CFLAGS) -c ./keyboard.c -o keyboard.o

$(GRUB_DIR)/grub.cfg: | dirs
	@test -f $(GRUB_DIR)/grub.cfg

dirs:
	mkdir -p $(GRUB_DIR)

iso: $(ISO)

clean:
	rm -f $(OBJS) $(KERNEL) $(ISO)
	rm -f $(ISO_DIR)/boot/kernel.bin
