include ../defs.mk

all:
ifeq ($(PLATFORM), x86_64)
	@$(MAKE) --no-print-directory -C x86_all/
else
	$(error Don't know how to build for platform: $(PLATFORM))
endif

clean:
ifeq ($(PLATFORM), x86_64)
	@$(MAKE) --no-print-directory -C x86_all/ clean
else
	$(error Don't know how to clean for platform: $(PLATFORM))
endif
	@echo Cleaning Bootloader	
	@/bin/rm -rf ./bin/*
