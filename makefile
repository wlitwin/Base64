include defs.mk

all: create_bin

.PHONY: vm
vm:
	@$(MAKE) --no-print-directory -C vm/

clean:
	@$(MAKE) --no-print-directory -C ./$(PLATFORM) clean
	@$(MAKE) --no-print-directory -C ./vm clean
	@/bin/rm -f ./obj/vm.o
	@/bin/rm -f ./bin/kernel.bin

build_platform:
	@$(MAKE) --no-print-directory -C ./$(PLATFORM) build

create_bin: build_platform vm
	@$(MAKE) --no-print-directory -C ./$(PLATFORM) postbuild

usb: clean create_bin
	sudo dd if=bin/kernel.bin of=/dev/sdb ; sync

objdump:
	objdump -D ./x86_64/bin/kernel_dbg.o | less

x64_qemu: export CFLAGS += -DQEMU
x64_qemu: create_bin
	gnome-terminal --command 'nc -l -p 4555'
	qemu-system-x86_64 -s -m 2560 -smp 2 -cpu core2duo -drive file=./bin/kernel.bin,format=raw,cyls=200,heads=16,secs=63 -net user -net nic,model=i82559er -soundhw hda -monitor stdio -serial tcp:127.0.0.1:4555
