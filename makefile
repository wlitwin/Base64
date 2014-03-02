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

	#./mkterm.sh

x64_qemu: export CFLAGS += -DQEMU
x64_qemu: create_bin
	gnome-terminal --command 'nc -l -p 4555'
	qemu-system-x86_64 -s -m 512 -smp 2 -cpu core2duo -drive file=./bin/kernel.bin,format=raw,cyls=200,heads=16,secs=63 -net user -net nic,model=i82559er -soundhw hda -monitor stdio -serial tcp:127.0.0.1:4555
