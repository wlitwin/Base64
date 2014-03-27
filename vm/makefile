include ../defs.mk

CC=gcc
override CFLAGS := -m64 \
	   -Wwrite-strings \
	   -mcmodel=large \
	   -ffreestanding \
	   -nostdlib \
	   -mno-red-zone \
	   -mno-3dnow \
	   -mno-mmx \
	   -mno-sse \
	   -mno-sse2 \
	   -mno-sse3 \
	   -Wall \
	   -Wextra \
	   -Wformat \
	   -pedantic \
	   -fno-builtin \
	   -march=native \
	   -std=c99 \
	   -Isrc/ \
	   $(CFLAGS)

VM_OBJECTS=$(shell find src/ -name '*.[csS]' | sed -e 's/^\(.*\.[csS]\)$$/obj\/\1.o/g')

LD=ld
LD_FLAGS=-m elf_x86_64 -r
LD_FLAGS_DBG=-m elf_x86_64 -r

DBG=

all: vm

clean:
	@echo Cleaning VM
	@/bin/rm -rf ./obj/*

./obj/%.c.o : %.c
	@echo " - Compiling" $^
	@mkdir -p $(shell dirname $@)
	@$(CC) $(DBG) $(CFLAGS) $(QEMU) -c $^ -o $@

.PHONY: vm debug
debug: DBG += -g
debug: vm

vm: $(VM_OBJECTS)
	@echo
	@echo ----------------------------
	@echo "     Creating VM Image"
	@echo ----------------------------
	$(LD) $(LD_FLAGS) -T vm.ld $(VM_OBJECTS) -o ../obj/vm.o
	@echo ---------------------------
	@echo "  Creating VM Debug Image"
	@echo ---------------------------
	@$(LD) $(LD_FLAGS_DBG) -T vm.ld $(VM_OBJECTS) -o ../obj/vm_dbg.o
