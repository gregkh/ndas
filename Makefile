ndas_block-y	:= ndas_block_main.o procfs.o ops.o	\
			ctrldev.o block26.o ndasuser.o 	\
			lpxproto.o serial.o des.o crc.o	\
			xbuf.o des_key.o nddev.o registrar.o

ndas_sal-y	:=	sal.o sal_net.o sal_mem.o sal_sync.o


obj-m	:= ndas_block.o ndas_sal.o

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
