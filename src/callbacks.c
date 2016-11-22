#include "callbacks.h"

#include <stdbool.h>

extern bool mcu_pending_receive, mcu_pending_write, fpga_pending_receive, fpga_pending_write;

void LIBUSB_CALL mcu_data_received_callback(struct libusb_transfer* transfer) {
	libusb_free_transfer(transfer);
	mcu_pending_receive = false;
}

void LIBUSB_CALL mcu_data_sent_callback(struct libusb_transfer* transfer) {
	libusb_free_transfer(transfer);
	mcu_pending_write = false;
}

void LIBUSB_CALL fpga_data_received_callback(struct libusb_transfer* transfer) {
	libusb_free_transfer(transfer);
	fpga_pending_receive = false;
}

void LIBUSB_CALL fpga_data_sent_callback(struct libusb_transfer* transfer) {
	libusb_free_transfer(transfer);
	fpga_pending_write = false;
}