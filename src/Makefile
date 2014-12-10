

EXTRA_CFLAGS += -I$(src)/include

########################## IC ############################
CONFIG_RTL8195A = y

######################### Interface ###########################
CONFIG_USB_HCI = n
CONFIG_SDIO_HCI = y

###############################################################
export TopDIR ?= $(shell pwd)

########### COMMON  #################################
ifeq ($(CONFIG_SDIO_HCI), y)
HCI_NAME = sdio
endif

ifeq ($(CONFIG_USB_HCI), y)
HCI_NAME = usb
endif

_OS_INTFS_FILES :=	os_dep/osdep_service.o \
			os_dep/linux/os_intfs.o \
			os_dep/linux/$(HCI_NAME)_intf.o \
			os_dep/linux/$(HCI_NAME)_ops_linux.o \
			os_dep/linux/ioctl_linux.o \
			os_dep/linux/xmit_linux.o 

ifeq ($(CONFIG_SDIO_HCI), y)
_OS_INTFS_FILES += os_dep/linux/$(HCI_NAME)_ops_linux.o
endif

_HAL_INTFS_FILES :=	hal/hal_intf.o 


obj-m := inic_8195a.o
inic_8195a-objs := osdep/linux/sdio_intf.o hal/rtl8195a/sdio/sdio_ops.o \
			core/rtw_xmit.o core/rtw_recv.o core/rtw_io.o core/rtw_cmd.o \
			osdep/linux/sdio_ops_linux.o osdep/linux/ioctl_linux.o \
			osdep/osdep_service.o osdep/linux/os_intfs.o \
			hal/hal_intf.o hal/rtl8195a/rtl8195a_hal_ops.o \
			hal/rtl8195a/sdio/rtl8195as_xmit.o osdep/linux/xmit_linux.o

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


