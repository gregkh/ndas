ndas_block-y	:= ndas_block_main.o procfs.o ops.o	\
			ctrldev.o block26.o ndasuser.o 	\
			lpxproto.o serial.o des.o crc.o	\
			xbuf.o xhash.o dpc.o		\
			des_key.o nddev.o registrar.o	\
			nhix.o conn.o scrc32.o		\
			lsp.o lsp_hash.o lsp_util.o	\
			lockmgmt.o sdev.o gen_uuid.o	\
			ndpnp.o udev.o raid.o

ndas_sal-y	:=	sal.o sal_net.o sal_mem.o sal_sync.o	\
			sal_time.o sal_thread.o sal_debug.o sal_libc.o

ndas_core-y	:=	ndas_core_main.o ndasuser.o nddev.o udev.o	\
			ndpnp.o registrar.o scrc32.o xhash.o conn.o	\
			dpc.o lsp.o lpxproto.o nhix.o lsp_util.o	\
			lsp_hash.o lockmgmt.o gen_uuid.o xbuf.o		\
			serial.o des.o des_key.o crc.o raid.o sdev.o

obj-m	:= ndas_block.o ndas_sal.o ndas_core.o


KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)

clean:
	rm -f *.o *~ .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
