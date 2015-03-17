

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

_OS_INTFS_FILES :=	osdep/osdep_service.o \
			osdep/linux/os_intfs.o \
			osdep/linux/$(HCI_NAME)_intf.o \
			osdep/linux/$(HCI_NAME)_ops_linux.o \
			osdep/linux/ioctl_linux.o \
			osdep/linux/xmit_linux.o \
			osdep/linux/recv_linux.o

_HAL_INTFS_FILES :=	hal/hal_intf.o 

########### HAL_RTL8195A #################################
ifeq ($(CONFIG_RTL8195A), y)

RTL871X = rtl8195a
ifeq ($(CONFIG_SDIO_HCI), y)
MODULE_NAME = 8195as
endif
ifeq ($(CONFIG_USB_HCI), y)
MODULE_NAME = 8195au
endif

_HAL_INTFS_FILES +=	hal/$(RTL871X)/$(RTL871X)_hal_init.o \

_HAL_INTFS_FILES +=	 \
			hal/$(RTL871X)/$(HCI_NAME)/$(HCI_NAME)_halinit.o \
			hal/$(RTL871X)/$(HCI_NAME)/rtl$(MODULE_NAME)_xmit.o \
			hal/$(RTL871X)/$(HCI_NAME)/rtl$(MODULE_NAME)_recv.o

_HAL_INTFS_FILES += hal/$(RTL871X)/$(HCI_NAME)/$(HCI_NAME)_ops.o
endif

#######################################################


########### AUTO_CFG  #################################	

ifeq ($(CONFIG_RTL8195A),y) 
$(shell cp $(TopDIR)/autoconf_rtl8195a_$(HCI_NAME)_linux.h $(TopDIR)/include/autoconf.h)
endif

rtk_core :=	core/rtw_cmd.o \
		core/rtw_io.o \
		core/rtw_recv.o \
		core/rtw_xmit.o \
		core/rtw_debug.o

$(MODULE_NAME)-y += $(rtk_core)
$(MODULE_NAME)-y += $(_OS_INTFS_FILES)
$(MODULE_NAME)-y += $(_HAL_INTFS_FILES)

obj-m := $(MODULE_NAME).o

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
