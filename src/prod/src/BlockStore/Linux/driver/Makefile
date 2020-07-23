kernelver = $(shell uname -r)
obj-m += sfblkdevice.o
sfblkdevice-objs := src/sfblkdev_main.o src/sfblkdev_ctrl.o src/sfblkdev.o src/sfblkdev_util.o
EXTRA_CFLAGS = -I$(PWD)/inc/ 

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
