#ifndef __USBDEVICE_H_
#define __USBDEVICE_H_

#include "libusb.h"

/* USB DEVICE DESCRIPTOR FPGA */	
#define FPGA_VID 		0x4ac3
#define FPGA_PID 		0xa200
#define FPGA_EP_IN		0x81
#define FPGA_EP_OUT		0x01

/* USB DEVICE DESCRIPTOR MCU */
#define MCU_VID			0x10c4
#define MCU_PID			0x0007
#define MCU_EP_IN		0x81
#define MCU_EP_OUT		0x01

enum {MCU, FPGA};

struct USB_data {
	libusb_context* context;
	libusb_device_handle* dev_handle;
	int interface;
};

/**
 * Function for connecting to a YTELSE USB device.
 */

int connect(int device, libusb_context* context, libusb_device_handle** dev_handle, int interface);

#endif