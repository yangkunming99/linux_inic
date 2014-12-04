
ifneq ($(KERNELRELEASE),)
	obj-m := inic_8195a.o
	inic_8195a-objs := osdep/linux/sdio_intf.o hal/rtl8195a/sdio/sdio_ops.o \
				core/rtw_xmit.o core/rtw_recv.o core/rtw_io.o core/rtw_cmd.o \
				osdep/linux/sdio_ops_linux.o osdep/linux/ioctl_linux.o \
				osdep/osdep_service.o
else
	BASEDIR := /lib/modules/$(shell uname -r)
	KERNELDIR ?= $(BASEDIR)/build
	PWD :=$(shell pwd)
	DRIVERDIR := $(shell find $(BASEDIR)/kernel/drivers/net -name realtek -type d)
	ifeq ($(DRIVERDIR),)
		DRIVERDIR := $(BASEDIR)/kernel/drivers/net
	endif
	RTKDIR := $(subst $(BASEDIR)/,,$(DRIVERDIR))

.PHONY: all
all: clean modules

.PHONY:modules
modules:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules

.PHONY:clean
clean:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) clean

.PHONY:install
install:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) INSTALL_MOD_DIR=$(RTKDIR) modules_install

endif

