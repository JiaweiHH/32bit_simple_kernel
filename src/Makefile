#!Makefile

C_SOURCES = $(shell find . -name "*.c")
C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))
S_SOURCES = $(shell find . -name "*.s")
S_OBJECTS = $(patsubst %.s, %.o, $(S_SOURCES))

CC = gcc
LD = ld
ASM = nasm

# -m32: 生成 32bit 代码, -ggdb && -gstabs+: 添加相关调试信息
# -nostdinc: 不包含 C 标准库里面的头文件, -fno-builtin: 要求 gcc 不主动使用内建函数
# -fno-stack-protector: 不使用栈保护等检测
C_FLAGS = -c -Wall -m32 -ggdb -gstabs+ -nostdinc -fno-pic -fno-builtin -fno-stack-protector -I include
# -T scripts/kernel.ld 使用自己的链接脚本
# -m elf_i386 生成 i386 平台下的 ELF 格式的可执行文件
LD_FLAGS = -T scripts/kernel.ld -m elf_i386 -nostdlib
ASM_FLAGS = -f elf -g -F stabs

all: $(S_OBJECTS) $(C_OBJECTS) link update_image

.c.o:
	@echo 编译代码文件 $< ...
	$(CC) $(C_FLAGS) $< -o $@

.s.o:
	@echo 编译汇编文件 $< ...
	$(ASM) $(ASM_FLAGS) $<

link:
	@echo 链接内核文件...
	$(LD) $(LD_FLAGS) $(S_OBJECTS) $(C_OBJECTS) -o l_core

.PHONY:clean
clean:
	$(RM) $(S_OBJECTS) $(C_OBJECTS) l_core

.PHONY: update_image
update_image:
	sudo mount floppy.img ../mnt
	sudo cp l_core ../mnt/l_core
	sleep 1
	sudo umount ../mnt

.PHONY: mount_image
mount_image:
	sudo mount floppy.img ../mnt

.PHONY: umount_image
umount_image:
	sudo umount ../mnt

.PHONY: qemu
qemu:
	qemu-system-i386 -fda floppy.img -boot a

.PHONY: bochs
bochs:
	bochs -f tools/bochsrc.txt

.PHONY: debug
debug:
	qemu-system-i386 -S -s -fda floppy.img -boot a &
	sleep 1
	cgdb -x tools/gdbinit