ndas_block-y	:= ndas_block_main.o procfs.o ops.o ctrldev.o

obj-m	:= ndas_block.o

LINUX=1

#ccflags-y += -Iinc/

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)

clean:
	rm -f *.o *~ .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
