#
# Makefile for DWC_otg Highspeed USB controller driver
#
TARGET := cas-kprobe-ext
KDIR = $(LINUX_DIR)
PWD	:= $(shell pwd)

EXTRA_CFLAGS    += -fsigned-char 

obj-m	:= $(TARGET).o

#$(TARGET)-objs	:= jprobe_example.o 
$(TARGET)-objs	:= kretprobe_example.o vm.o show.o elf.o command.o tty.o disasm.o sim.o fwatch.o
#$(TARGET)-objs	:= kprobe_example.o jprobe_example.o kretprobe_example.o



ifndef JPWD
export JPWD = $(shell ../getKernelPath.pl `pwd -L`)
endif

KERNEL_SRC_DIR := $(JPWD)/projects/bcm589x/build-output/extracted-kernel/linux-2.6.32.9
KERNEL_OBJ_DIR := $(JPWD)/projects/bcm589x/build-output/secuapp/BCM95892/linux

all: 
	make ARCH=arm -C $(KERNEL_SRC_DIR) CROSS_COMPILE=arm-brcm-linux-gnueabi- OBJWRAP= KBUILD_EXTMOD=$(PWD) KBUILD_OUTPUT=$(KERNEL_OBJ_DIR)  modules

release:
	make ARCH=arm -C $(KERNEL_SRC_DIR) CROSS_COMPILE=arm-brcm-linux-gnueabi- OBJWRAP= KBUILD_EXTMOD=$(PWD) KBUILD_OUTPUT=$(KERNEL_OBJ_DIR)  modules
	rm -rf output unsigned
	cp -rf output-template output
	test -r $(TARGET).ko && cp -f $(TARGET).ko output/RootFS-For-Update/usr/lib/modules/
	tar cvzf output/update.tgz --remove-file -Coutput RootFS-For-Update
	test -r output/castles.patch && capLinuxDriver TR220 debug 0000 output/castles.patch unsigned && CAPSCon TR220 unsigned/output.mci


install: install-mod

.PHONY: clean
clean:
	make ARCH=arm -C $(KERNEL_SRC_DIR) CROSS_COMPILE=arm-brcm-linux-gnueabi- OBJWRAP= KBUILD_EXTMOD=$(PWD) KBUILD_OUTPUT=$(KERNEL_OBJ_DIR)  clean
	find . -name *.o -name *.o.cmd | rm -f cadrv.{tgz,ko,o}
