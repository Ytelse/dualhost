#ifndef __CALLBACKS_H_
#define __CALLBACKS_H_

#include <libusb.h>

void LIBUSB_CALL mcu_data_received_callback(struct libusb_transfer* transfer);
void LIBUSB_CALL mcu_data_sent_callback(struct libusb_transfer* transfer);
void LIBUSB_CALL fpga_data_received_callback(struct libusb_transfer* transfer);
void LIBUSB_CALL fpga_data_sent_callback(struct libusb_transfer* transfer);

#endif