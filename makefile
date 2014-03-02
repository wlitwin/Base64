include defs.mk

all: create_bin

.PHONY: boot
boot:
	@$(MAKE) --no-print-directory -C boot/

.PHONY: kernel
kernel:
	@$(MAKE) --no-print-directory -C kernel/

clean:
	@$(MAKE) --no-print-directory -C boot/ clean
	@$(MAKE) --no-print-directory -C kernel/ clean
	@/bin/rm -f ./bin/kernel.bin

create_bin: boot kernel
	@echo Creating Boot Image
	@cat ./boot/bin/bootloader.b ./kernel/bin/kernel.b > ./bin/kernel.bin

x64_qemu: export CFLAGS += -DQEMU
x64_qemu: create_bin
	./mkterm.sh

