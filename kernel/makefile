include ../defs.mk

all:
ifeq ($(PLATFORM), x86_64)
	@$(MAKE) --no-print-directory -C x86_64/
	# Bleh we need to run FancyCat on the output binary...
	# Need to rethink how we should go about this dependency
	# between the kernel and the bootloader
	@../boot/x86_all/bin/FancyCat 0x200000 ./bin/kernel.b
	@mv image.dat ./bin/kernel.b
else
	$(error Don't know how to build for platform: $(PLATFORM))
endif

clean:
ifeq ($(PLATFORM), x86_64)
	@$(MAKE) --no-print-directory -C x86_64/ clean
else
	$(error Don't know how to clean for platform: $(PLATFORM))
endif
	@/bin/rm -rf ./bin/*
