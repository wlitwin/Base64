CC=gcc
override CFLAGS := -m64 \
	   -g \
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

AS=as
AS_FLAGS=--64 -am

LD=ld
LD_FLAGS=-m elf_x86_64
LD_FLAGS_DBG=-m elf_x86_64

# Filled in by debug rule
DBG=

KERNEL_OBJECTS=$(shell find src/ -name '*.[csS]' | sed -e 's/^\(.*\.[csS]\)$$/obj\/\1.o/g')
PRE_KERNEL = obj/src/prekernel.s.o

all: kernel 

clean:
	@echo Cleaning Kernel
	@/bin/rm -rf ./obj/*

# TODO Separate the runtime compilation from the kernels compilation
./obj/%.c.o : %.c
	@echo " - Compiling" $^
	@mkdir -p $(shell dirname $@)
	@$(CC) $(DBG) $(CFLAGS) $(QEMU) -c $^ -o $@

./obj/%.S.o : %.S
	@echo " - Assembling" $^
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) $(QEMU) -c $^ -o $@

./obj/%.s.o : %.s
	@echo " - Assembling" $^
	@mkdir -p $(shell dirname $@)
	@$(AS) $(AS_FLAGS) $^ -o $@

.PHONY: kernel debug
debug: DBG += -g

debug: kernel

print_args:
	@echo
	@echo Compiler: $(CC)
	@echo CFLAGS: $(CFLAGS)
	@echo
	@echo Assembler: $(AS)
	@echo AFLAGS: $(AS_FLAGS)
	@echo
	@echo Linker: $(LD)
	@echo LFLAGS: $(LD_FLAGS)
	@echo

kernel: print_args $(KERNEL_OBJECTS)
	@echo
	@echo ----------------------------
	@echo Creating Kernel Binary Image
	@echo ----------------------------
	@$(LD) $(LD_FLAGS) -T ./kernel.ld $(PRE_KERNEL) $(filter-out $(PRE_KERNEL),$(KERNEL_OBJECTS)) -o ../bin/kernel.o
	@echo ----------------------------
	@echo Creating Kernel Debug Image
	@echo ----------------------------
	@$(LD) $(LD_FLAGS_DBG) -T ./kernel.ld $(PRE_KERNEL) $(filter-out $(PRE_KERNEL),$(KERNEL_OBJECTS)) -o ../bin/kernel_dbg.o
