/*
 * Usb_driver.c
 *
 *  Created on: 2019
 *      Author: Simon Saadeh
 */

#include "Usb_driver.h"

// Event when the device is opened
int driver_open(struct inode *inode, struct file *file) {
  struct usb_interface *intf;
  struct usb_urb_t *usb_driver = NULL;
  int subminor;
  int retval = 0;

  printk(KERN_WARNING "ELE784 -> Open\n");

  subminor = iminor(inode);
  intf = usb_find_interface(&udriver, subminor);
  if (!intf) {
    printk(KERN_WARNING "ELE784 -> Open: Ne peux ouvrir le peripherique\n");
    return -ENODEV;
  }
 
  //on s'assure qu'il y a seulement 1 user.
  down(&usb_driver->semCountUser);
  if(usb_driver->usercount==MAXUSER){
	return -ENOTTY;
  }
  else
	usb_driver->usercount++; //on incrémente le nombre de user.
  up(&usb_driver->semCountUser);

 // save our object in the file's private structure /
  file->private_data = intf;
  
  return retval;
}

int driver_release(struct inode *inode, struct file *file){
  struct usb_interface *interface = file->private_data;
  struct usb_urb_t *usb_driver = usb_get_intfdata(interface);
  down(&usb_driver->semCountUser);
  usb_driver->usercount--; //on décrémente le nombre de user.
  up(&usb_driver->semCountUser);

  return 0;
}

// Probing USB device
int driver_probe(struct usb_interface *interface, const struct usb_device_id *id) {
  int retval;  
  struct usb_device *dev= interface_to_usbdev(interface); 
  struct usb_urb_t *usb_driver = NULL;

  if (interface->altsetting->desc.bInterfaceClass == CC_VIDEO)
  {
    if (interface->altsetting->desc.bInterfaceSubClass == SC_VIDEOCONTROL)
      return 0;
    if (interface->altsetting->desc.bInterfaceSubClass == SC_VIDEOSTREAMING)
    {
	  /* allocate memory for our device state and initialize it */
      usb_driver = (struct usb_urb_t *)kmalloc(sizeof(struct usb_urb_t), GFP_KERNEL);
      if(usb_driver == 0)
      {
        printk(KERN_WARNING "Out of Memory\n");
	retval = -ENOMEM;
        return retval;
      }
	  usb_set_intfdata(interface, usb_driver);
	  retval = usb_register_dev(interface, &class_driver);
	  if (retval < 0)
      	  {
           /* error: something happened with driver */
            usb_set_intfdata(interface, 0);
          }

	  usb_driver->usb_dev = usb_get_dev(dev);
	  usb_driver->usb_inf = interface;
	  usb_set_interface(dev, 1, 4);
	  //init notre struct	
	  usb_driver->myData = kmalloc(MYLENGTH * sizeof(char), GFP_KERNEL);
	  sema_init(&usb_driver->semCountUser, 1);
	  sema_init(&usb_driver->semRead, 1);
	  usb_driver->myStatus = 0;
	  usb_driver->myLength = MYLENGTH;
	  usb_driver->myLengthUsed = 0;
	  usb_driver->usercount = 0;
	  usb_driver->count_urb = 0;	

    }
    else
      retval = -ENODEV;
  }
  else
    retval = -ENODEV;

  return retval;

}

void driver_disconnect (struct usb_interface *intf) {
  struct usb_urb_t *usb_driver = usb_get_intfdata(intf);

  printk(KERN_INFO "ELE784 -> Disconnect\n");
  
  kfree(usb_driver->myData);
  usb_deregister_dev(intf, &class_driver);
}



ssize_t driver_read(struct file *file, char __user *buffer, size_t count, loff_t *f_pos) {
  int i = 0;
  struct usb_interface *interface = file->private_data;
  struct usb_urb_t *usb_driver = usb_get_intfdata(interface);

  wait_for_completion(usb_driver->sync);
  printk(KERN_WARNING "ELE784 -> READ - wait_for_completion \n");

  count = copy_to_user(buffer, usb_driver->myData, usb_driver->myLengthUsed);
  printk(KERN_WARNING "ELE784 -> READ - copy to user \n");

  if (count < 0)
    count = -EFAULT;
  
  for (i = 0; i < NB_URB; i++) {
    usb_kill_urb(usb_driver->myUrb[i]);
    usb_free_coherent(usb_driver->usb_dev, usb_driver->myUrb[i]->transfer_buffer_length, usb_driver->myUrb[i]->transfer_buffer,
        usb_driver->myUrb[i]->transfer_dma);
    usb_free_urb(usb_driver->myUrb[i]);
  }
  
  return count;
}


long driver_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  unsigned char DirectionHaut[4]= {0x00, 0x00, 0x80, 0xFF};
  unsigned char DirectionBas[4]= {0x00, 0x00, 0x80, 0x00};
  unsigned char DirectionGauche[4]= {0x80, 0x00, 0x00, 0x00};
  unsigned char DirectionDroite[4]= {0x80, 0xFF, 0x00, 0x00};
  unsigned int Direction;
  unsigned int GetFunction;
  int error = 0;
  int retval = 0;
  char reset;
  struct usb_interface *interface = file->private_data;
  struct usb_urb_t *usb_driver = usb_get_intfdata(interface);
  struct usb_device *udev = usb_driver->usb_dev;

  struct data_t *data_sg = 0;
  data_sg->value = 0; 
  data_sg->index = 0;  		
  
  if(_IOC_TYPE(cmd) != MAGIC_VAL)
    return -ENOTTY;
  if(_IOC_NR(cmd) > MAGIC_VAL)
    return -ENOTTY;

  else if(_IOC_DIR(cmd) & _IOC_WRITE)
    error =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
  if(error)
    return -EFAULT;

	switch(cmd){
		case IOCTL_GET:
					retval = __get_user(GetFunction, (unsigned int __user *)arg);
					data_sg = (struct data_t *)arg;
					switch(GetFunction){
						case GET_CUR:
							usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), GET_CUR, (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE), data_sg->value, data_sg->index, data_sg->data, 2, 0);
							break;
						case GET_MIN:
							usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), GET_CUR, (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE), data_sg->value, data_sg->index, data_sg->data, 2, 0);
							break;
						case GET_MAX:
							usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), GET_CUR, (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE), data_sg->value, data_sg->index, data_sg->data, 2, 0);
							break;
						case GET_RES:
							usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), GET_CUR, (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE), data_sg->value, data_sg->index, data_sg->data, 2, 0);
							break;
					}
					
					printk("ELE784 -> ioctl : %i\n\r", IOCTL_GET);
			break;
		case IOCTL_SET:
					usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), SET_CUR, (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE), data_sg->value, data_sg->index, data_sg->data, 2, 0);
					printk("ELE784 -> ioctl : %i\n\r", IOCTL_SET);
			break;
		case IOCTL_STREAMON:
					usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), REQUEST_STREAM, (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE), 0x0004, 0x0001, 0, 2, 0);
					printk("ELE784 -> ioctl : %i\n\r", IOCTL_STREAMON);
			break;
		case IOCTL_STREAMOFF:
					usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), REQUEST_STREAM, (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE), 0x0000, 0x0001, 0, 0, 0);
					printk("ELE784 -> ioctl : %i\n\r", IOCTL_STREAMOFF);
			break;
		case IOCTL_GRAB:
					urb_Init(interface, udev);
					printk("ELE784 -> ioctl : %i\n\r", IOCTL_GRAB);
			break;
		case IOCTL_PANTILT:
			printk("ELE784 -> ioctl : %i\n\r", IOCTL_PANTILT);
         retval = __get_user(Direction, (unsigned int __user *)arg);
			switch(Direction){
				case PANTILT_UP:
					usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), REQUEST_TILT, (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE), 0x0100, 0x0900, &DirectionHaut, 4, 0);
					break;
				case PANTILT_DOWN:
					usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), REQUEST_TILT, (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE), 0x0100, 0x0900, &DirectionBas, 4, 0);
					break;
				case PANTILT_LEFT:
					usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), REQUEST_TILT, (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE), 0x0100, 0x0900, &DirectionGauche, 4, 0);
					break;
				case PANTILT_RIGHT:
					usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), REQUEST_TILT, (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE), 0x0100, 0x0900, &DirectionDroite, 4, 0);
					break;
			}
		case IOCTL_PANTILT_RESEST:
					reset = 0x03;
					usb_control_msg(udev, usb_sndctrlpipe(udev, udev->ep0.desc.bEndpointAddress), REQUEST_TILT, (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE), 0x0200, 0x0900, &reset, 1, 0);
					printk("ELE784 -> ioctl : %i\n\r", IOCTL_PANTILT_RESEST);
			break;

	}
	return 0;
}

static void complete_callback(struct urb *urb){
	int ret;
	int i;	
	unsigned char * data;
	unsigned int len;
	unsigned int maxlen;
	unsigned int nbytes;
	void * mem;
	struct usb_urb_t *usb_driver = urb->context;

	if(urb->status == 0){
		
		for (i = 0; i < urb->number_of_packets; ++i) {
			if( usb_driver->myStatus == 1){
				continue;
			}
			if (urb->iso_frame_desc[i].status < 0) {
				continue;
			}
			
			data = urb->transfer_buffer + urb->iso_frame_desc[i].offset;
			if(data[1] & (1 << 6)){
				continue;
			}
			len = urb->iso_frame_desc[i].actual_length;
			if (len < 2 || data[0] < 2 || data[0] > len){
				continue;
			}
		
			len -= data[0];
			maxlen = usb_driver->myLength - usb_driver->myLengthUsed ;
			mem = usb_driver->myData + usb_driver->myLengthUsed;
			nbytes = min(len, maxlen);
			memcpy(mem, data + data[0], nbytes);
			usb_driver->myLengthUsed += nbytes;
	
			if (len > maxlen) {				
				 usb_driver->myStatus = 1; // DONE
			}
	
			/* Mark the buffer as done if the EOF marker is set. */
			if ((data[1] & (1 << 1)) && (usb_driver->myLengthUsed != 0)) {						
				 usb_driver->myStatus = 1; // DONE
			}					
		}
	
		if (!( usb_driver->myStatus == 1)){				
			if ((ret = usb_submit_urb(urb, GFP_ATOMIC)) < 0) {
				//printk(KERN_WARNING "");
			}
		}else{
			///////////////////////////////////////////////////////////////////////
			//  Synchronisation
			///////////////////////////////////////////////////////////////////////
		   usb_driver->count_urb += 1;
		   usb_driver->myStatus = 0;
		   if (usb_driver->count_urb == NB_URB) {
			 complete(usb_driver->sync);
			 usb_driver->count_urb = 0;
		   }
		}			
	}else{
		printk(KERN_WARNING "ERREUR: NA PAS EXÉCUTÉ LA FONCTION CALLBACK");
	}
}

int urb_Init(struct usb_interface *intf, struct usb_device *dev) {
  int nbPackets;
  int myPacketSize;
  int size;
  int i;
  int ret;
  int j;

  struct usb_endpoint_descriptor *endpointDesc;
  struct usb_urb_t *usb_driver = usb_get_intfdata(intf);

  endpointDesc = &intf->cur_altsetting->endpoint[0].desc;

  nbPackets = 40;  // The number of isochronous packets this urb should contain
  myPacketSize = le16_to_cpu(endpointDesc->wMaxPacketSize);
  size = myPacketSize * nbPackets;

  for (i = 0; i < NB_URB; i++) {
    usb_free_urb(usb_driver->myUrb[i]);
    usb_driver->myUrb[i] = usb_alloc_urb(nbPackets, GFP_KERNEL);
    if (usb_driver->myUrb[i] == NULL) {
      //printk(KERN_WARNING "");
      return -ENOMEM;
    }

    usb_driver->myUrb[i]->transfer_buffer = usb_alloc_coherent(dev, size, GFP_KERNEL, &usb_driver->myUrb[i]->transfer_dma);

    if (usb_driver->myUrb[i]->transfer_buffer == NULL) {
      //printk(KERN_WARNING "");
      usb_free_urb(usb_driver->myUrb[i]);
      return -ENOMEM;
    }

    usb_driver->myUrb[i]->dev = dev;
    usb_driver->myUrb[i]->context = usb_driver;
    usb_driver->myUrb[i]->pipe = usb_rcvisocpipe(dev, endpointDesc->bEndpointAddress);
    usb_driver->myUrb[i]->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
    usb_driver->myUrb[i]->interval = endpointDesc->bInterval;
    usb_driver->myUrb[i]->complete = complete_callback;
    usb_driver->myUrb[i]->number_of_packets = nbPackets;
    usb_driver->myUrb[i]->transfer_buffer_length = size;

    for (j = 0; j < nbPackets; ++j) {
      usb_driver->myUrb[i]->iso_frame_desc[j].offset = j * myPacketSize;
      usb_driver->myUrb[i]->iso_frame_desc[j].length = myPacketSize;
    }
  }

  for (i = 0; i < NB_URB; i++) {
    if ((ret = usb_submit_urb(usb_driver->myUrb[i], GFP_KERNEL)) < 0) {
      //printk(KERN_WARNING "");
      return ret;
    }
  }

  return 0;
}
