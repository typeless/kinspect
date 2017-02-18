TARGET := kinpect

PWD := $(shell pwd)

EXTRA_CFLAGS    += -fsigned-char

obj-m	:= $(TARGET).o

$(TARGET)-objs	:= kretprobe_example.o vm.o show.o elf.o command.o tty.o disasm.o sim.o fwatch.o

all: 
	make ARCH=arm -C $(KERNEL_SRC_DIR) CROSS_COMPILE=arm-linux-gnueabi- OBJWRAP= KBUILD_EXTMOD=$(PWD) KBUILD_OUTPUT=$(KERNEL_OBJ_DIR)  modules

.PHONY: clean
clean:
	make ARCH=arm -C $(KERNEL_SRC_DIR) CROSS_COMPILE=arm-linux-gnueabi- OBJWRAP= KBUILD_EXTMOD=$(PWD) KBUILD_OUTPUT=$(KERNEL_OBJ_DIR)  clean
	find . -name *.o -name *.o.cmd | rm -f cadrv.{tgz,ko,o}
