/*
 * Makefile
 *
 *  Created on: 2019
 *      Author: Simon Saadeh
 */

# Variables pointing to different paths
KERNEL_DIR    	:= /lib/modules/$(shell uname -r)/build
PWD        	:= $(shell pwd)

obj-m        := Usb_driver.o

all: usbCam_driver

# We build our module in this section
usbCam_driver:
	@echo "Building the driver: Logitech Orbit driver"
	@make -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

# It's always a good thing to offer a way to cleanup our development directory
clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c Module.symvers modules.order
	-rm -rf .tmp_versions
