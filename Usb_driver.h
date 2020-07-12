/*
 * Usb_driver.h
 *
 *  Created on: 2019
 *      Author: Simon Saadeh
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/usb.h>
#include <linux/usb/video.h>
#include <linux/completion.h>
#include "usbvideo.h"

#include <asm/atomic.h>
#include <asm/uaccess.h>

#define USB_VENDOR_ID       0x046d
#define USB_PRODUCT_ID2     0x08cc
#define USB_PRODUCT_ID      0x0994

#define DEV_MINOR       0x00
#define DEV_MINORS      0x01
#define MYLENGTH  		42666

#define MAXUSER			1
#define NB_URB			5

#define MAJOR_NUM 250

#define PANTILT_UP 1
#define PANTILT_DOWN 2
#define PANTILT_LEFT 3
#define PANTILT_RIGHT 4
#define REQUEST_TILT 0x01
#define REQUEST_STREAM 0x0B
#define GET_CUR 0x81
#define GET_MIN 0x82
#define GET_MAX 0x83
#define GET_RES 0x84

//--- IOCTL
#define MAGIC_VAL 'R'
#define IOCTL_GET _IO(MAGIC_VAL, 0x10)
#define IOCTL_SET _IO(MAGIC_VAL, 0x20)
#define IOCTL_STREAMON _IO(MAGIC_VAL, 0x30)
#define IOCTL_STREAMOFF _IO(MAGIC_VAL, 0x40)
#define IOCTL_GRAB _IO(MAGIC_VAL, 0x50)
#define IOCTL_PANTILT _IO(MAGIC_VAL, 0x60)
#define IOCTL_PANTILT_RESEST _IO(MAGIC_VAL, 0x70)
#define IOC_MAXNR 8

MODULE_AUTHOR("Usb");
MODULE_LICENSE("Dual BSD/GPL");

static struct usb_device_id usb_device_id [] = {
    {USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID)},
    {},
};

struct usb_urb_t {
	unsigned int myStatus;
	unsigned int myLength;
	unsigned int myLengthUsed;
	char *myData;
	int usercount;
	int count_urb;
	struct urb *myUrb[NB_URB];
	struct usb_device *usb_dev;
	struct usb_interface *usb_inf;
	struct semaphore semCountUser;
	struct semaphore semRead;
	struct completion *sync;

};

struct data_t { //pour IOCTL GET ET SET
	int *data[2];
	int index;
	int value;
};

MODULE_DEVICE_TABLE(usb, usb_device_id);

static int urb_Init(struct usb_interface *intf, struct usb_device *dev);
static void complete_callback(struct urb *urb);
static ssize_t driver_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static int driver_open(struct inode *inode, struct file *file);
static int driver_release(struct inode *inode, struct file *file);
static long driver_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int driver_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void driver_disconnect (struct usb_interface *intf);

static struct usb_driver udriver = {
  .name = "Usbcam Driver",
  .probe = driver_probe,
  .disconnect = driver_disconnect,
  .id_table = usb_device_id,
};

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = driver_read,
  .open = driver_open,
  .release = driver_open,
  .unlocked_ioctl = driver_ioctl,
  .release = driver_release,
};

static struct usb_class_driver class_driver = {
  .name = "Usd_driver%d",
  .fops = &fops,
  .minor_base = DEV_MINOR,
};

module_usb_driver(udriver);
