################################################################################
#  This product is covered by one or more of the following patents:
#  US6,570,884, US6,115,776, and US6,327,625.
################################################################################

KFLAG := 2$(shell uname -r | sed -ne 's/^2\.[4]\..*/4/p')x

all: clean modules install

modules:
ifeq ($(KFLAG),24x)
	$(MAKE) -C src/ -f Makefile_linux24x modules
else
	$(MAKE) -C src/ modules
endif

clean:
ifeq ($(KFLAG),24x)
	$(MAKE) -C src/ -f Makefile_linux24x clean
else
	$(MAKE) -C src/ clean
endif

install:
ifeq ($(KFLAG),24x)
	$(MAKE) -C src/ -f Makefile_linux24x install
else
	$(MAKE) -C src/ install
endif



