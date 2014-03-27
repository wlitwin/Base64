
build:
	@$(MAKE) --no-print-directory -C ./boot
	@$(MAKE) --no-print-directory -C ./kernel

postbuild:
	$(LD) -m elf_x86_64 --oformat=binary -T ./x86_64.ld ./bin/kernel.o ../obj/vm.o -o ./bin/kernel.b
	$(LD) -m elf_x86_64 -T ./x86_64.ld ./bin/kernel.o ../obj/vm.o -o ./bin/kernel_bin_dbg.o
	./bin/FancyCat 0x200000 ./bin/kernel.b
	mv image.dat ./bin/.
	cat ./bin/bootloader.b ./bin/image.dat > ../bin/kernel.bin

clean:
	@$(MAKE) --no-print-directory -C ./boot clean
	@$(MAKE) --no-print-directory -C ./kernel clean
