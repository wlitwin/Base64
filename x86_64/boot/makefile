CC=gcc
CFLAGS=-Wall -Werror -Wextra -pedantic -std=gnu99

AS=as
AS_FLAGS=--32 -n32

LD=ld
LD_FLAGS=-m elf_i386 --gc-sections --oformat=binary
LD_FLAGS_DBG=-m elf_i386 --gc-sections

OUTPUT_DIR=./bin
OBJ_DIR=./obj

# Filled in by debug rule
DBG=

all: bootloader fancycat

clean:
	@echo Cleaning Boot
	@/bin/rm -rf ./obj/*

fancycat: ./bin/FancyCat

bootloader:
	@echo Creating Bootloader
	@$(AS) $(AS_FLAGS) ./src/bootloader.s -o ./obj/bootloader.o
	@$(LD) $(LD_FLAGS) ./obj/bootloader.o -Tbootloader.ld -o ../bin/bootloader.b

./bin/FancyCat:
	@echo Creating FancyCat
	@$(CC) $(CFLAGS) ./src/FancyCat.c -o ../bin/FancyCat
