#include "usbdevice.h"

#include <stdio.h>
#include <string.h>

#include "debug.h"

static int getDeviceName(libusb_device_handle* dev_handle, char* stringBuffer, int bufferLength);
static int connect_mcu(libusb_context* context, libusb_device_handle** dev_handle, int interface);
static int connect_fpga(libusb_context* context, libusb_device_handle** dev_handle, int interface);

int connect(int device, libusb_context* context, libusb_device_handle** dev_handle, int interface) {
	
	if (device == MCU) {
		if (connect_mcu(context, dev_handle, interface)) {
			return 1;
		}
	} else {
		if (connect_fpga(context, dev_handle, interface)) {
			return 1;
		}
	}

	char nameBuffer[256];
	int nameLength = getDeviceName(*dev_handle, nameBuffer, 256);
	UNUSED(nameLength);
	printf("Connected to "_CYAN"%s\n"_RESET, nameBuffer);

	return 0;
}

static int connect_mcu(libusb_context* context, libusb_device_handle** dev_handle, int interface) {
	libusb_device** device_list = NULL;
	libusb_device* efm_dev = NULL;

	ssize_t count = 0;
	count = libusb_get_device_list(context, &device_list);

	for (size_t i = 0; i < count; i++) {
		libusb_device* device = device_list[i];
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(device, &desc);

		if (desc.idVendor == MCU_VID && desc.idProduct == MCU_PID) {
			efm_dev = device;
		}
	}

	if (!efm_dev) {
		return 1;
	}

	if(libusb_open(efm_dev, dev_handle)) {
		libusb_free_device_list(device_list, 1);
		return 1;
	}

	libusb_free_device_list(device_list, 1);

	if(libusb_claim_interface(*dev_handle, interface)) {
		return 1;
	}

	return 0;
}

static int connect_fpga(libusb_context* context, libusb_device_handle** dev_handle, int interface) {
	libusb_device** device_list = NULL;
	libusb_device* efm_dev = NULL;

	ssize_t count = 0;
	count = libusb_get_device_list(context, &device_list);

	for (size_t i = 0; i < count; i++) {
		libusb_device* device = device_list[i];
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(device, &desc);

		if (desc.idVendor == FPGA_VID && desc.idProduct == FPGA_PID) {
			efm_dev = device;
		}
	}

	if (!efm_dev) {
		return 1;
	}

	if(libusb_open(efm_dev, dev_handle)) {
		libusb_free_device_list(device_list, 1);
		return 1;
	}

	libusb_free_device_list(device_list, 1);

	if(libusb_claim_interface(*dev_handle, interface)) {
		return 1;
	}

	return 0;
}

static int getDeviceName(libusb_device_handle* dev_handle, char* stringBuffer, int bufferLength) {
		int rc = 0, mlength = 0, plength = 0;
		unsigned char manufacturer[100], product[100];
		memset(manufacturer, 0, 100);
		memset(product, 0, 100);

		libusb_device* dev = libusb_get_device(dev_handle);
		struct libusb_device_descriptor desc;
		rc = libusb_get_device_descriptor(dev, &desc);
		if (rc) {
			return -1;
		}
		mlength = libusb_get_string_descriptor_ascii(dev_handle, desc.iManufacturer, manufacturer, 100);
		plength = libusb_get_string_descriptor_ascii(dev_handle, desc.iProduct, product, 100);
		memcpy(stringBuffer, manufacturer, mlength);
		memset(stringBuffer+mlength, ' ', 1);
		memcpy(stringBuffer+mlength+1, product, plength+1);
		return mlength+plength+1;
}