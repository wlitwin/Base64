#!/bin/bash
gnome-terminal --command 'nc -l -p 4555'
qemu-system-x86_64 -s -m 512 -smp 2 -cpu core2duo -drive file=./bin/kernel.bin,format=raw,cyls=200,heads=16,secs=63 -net user -net nic,model=i82559er -soundhw hda -monitor stdio -serial tcp:127.0.0.1:4555
