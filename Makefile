CC = gcc
AS = nasm
LD = ld
GRUB_MKRESCUE = grub-mkrescue
GRUB_MKSTANDALONE = grub-mkstandalone

CFLAGS = -m32 -std=gnu11 -ffreestanding -fno-pie -fno-stack-protector -fno-builtin -Wall -Wextra -O2
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib
ASFLAGS = -f elf32

KERNEL = kernel.bin
ISO = system.iso
ISO_DIR = iso
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub
EFI_DIR = $(ISO_DIR)/EFI/BOOT
OBJS = boot.o kernel.o system.o memory.o fs.o ata.o idt.o pit.o keyboard.o

.PHONY: all clean iso dirs efi

all: $(ISO)

$(ISO): $(KERNEL) $(GRUB_DIR)/grub.cfg $(EFI_DIR)/BOOTX64.EFI
	mkdir -p $(BOOT_DIR)
	cp $(KERNEL) $(BOOT_DIR)/kernel.bin
	$(GRUB_MKRESCUE) -o $(ISO) $(ISO_DIR)

$(EFI_DIR)/BOOTX64.EFI: $(GRUB_DIR)/grub.cfg | dirs
	mkdir -p $(EFI_DIR)
	$(GRUB_MKSTANDALONE) -O x86_64-efi --modules="multiboot all_video" -o $@ "boot/grub/grub.cfg=$(GRUB_DIR)/grub.cfg"

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJS)

boot.o: boot.asm
	$(AS) $(ASFLAGS) boot.asm -o boot.o

kernel.o: ./kernel.c
	$(CC) $(CFLAGS) -c ./kernel.c -o kernel.o

system.o: ./system.c ./system.h ./fs.h ./keyboard.h
	$(CC) $(CFLAGS) -c ./system.c -o system.o

memory.o: ./memory.c
	$(CC) $(CFLAGS) -c ./memory.c -o memory.o

fs.o: ./fs.c ./fs.h
	$(CC) $(CFLAGS) -c ./fs.c -o fs.o

ata.o: ./ata.c ./ata.h
	$(CC) $(CFLAGS) -c ./ata.c -o ata.o

idt.o: ./idt.c ./idt.h ./keyboard.h
	$(CC) $(CFLAGS) -c ./idt.c -o idt.o

pit.o: ./pit.c ./pit.h
	$(CC) $(CFLAGS) -c ./pit.c -o pit.o

keyboard.o: ./keyboard.c ./keyboard.h
	$(CC) $(CFLAGS) -c ./keyboard.c -o keyboard.o

$(GRUB_DIR)/grub.cfg: | dirs
	@test -f $(GRUB_DIR)/grub.cfg

dirs:
	mkdir -p $(GRUB_DIR) $(EFI_DIR)

iso: $(ISO)

clean:
	rm -f $(OBJS) $(KERNEL) $(ISO)
	rm -f $(ISO_DIR)/boot/kernel.bin
