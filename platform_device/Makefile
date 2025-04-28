# See example Makefile from scull project
# Comment/uncomment the following line to disable/enable debugging
DEBUG = y

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g
else
  DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)

all: modules

ifneq ($(KERNELRELEASE),)
# call from kernel build system
obj-m := platform_dev.o
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.cmd *.symvers *.order *.mod