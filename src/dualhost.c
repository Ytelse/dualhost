#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <pthread.h>
#include "barrier.h"

#include <libusb.h>
#include "usbdevice.h"
#include "callbacks.h"

#include "debug.h"
#include "cmdparser.h"

#include "mcucomm.h"
#include "fpgacomm.h"

volatile long long tot_xferred;
volatile long long tot_recvd;

volatile bool _kill, _keepalive;
barrier_t barrier;

int main(int argc, char **argv) {
	int rc;
	UNUSED(rc);
	UNUSED(argc);
	UNUSED(argv);

	print_startup_msg();

	/* ========== INITIALIZE LIBUSB ========== */

	fprintf(stdout, "Initializing libusb...                              ");
	libusb_context* context = NULL;
	if (libusb_init(&context)) {
		fprintf(stdout, _RED"FAILED!\n"_RESET);
		return EXIT_FAILURE;
	}
	fprintf(stdout, _GREEN"DONE!\n"_RESET);
	libusb_set_debug(context, 0);

	/* ========== DECLARE & INITIALIZE ======== */

	int cmd;

	bool connected = false;
	_keepalive = false;

	pthread_t fpga_thread, mcu_thread;
	struct USB_data mcu_data, fpga_data;
	// struct USB_data fpga_data;

	libusb_device_handle *mcu, *fpga;

	_mainloop: {
		cmd = cmdloop();

		if (cmd == HELP) {
			print_help_string();
			goto _mainloop;
		} else if (cmd == CONNECT && !connected) {
			if (connect(MCU, context, &mcu, 0)) {
				fprintf(stdout, _RED"ERROR: Failed to connect to MCU!\n"_RESET);
				return EXIT_FAILURE;
			}

			if (connect(FPGA, context, &fpga, 1)) {
				fprintf(stdout, _RED"ERROR: Failed to connect to FPGA!\n"_RESET);
				return EXIT_FAILURE;
			}
			connected = true;
			goto _mainloop;
		} else if (cmd == START && !_keepalive && connected) {
			barrier_init(&barrier, 3);
			/* Initialize thread data */
			_keepalive = true;
			mcu_data.context = fpga_data.context = context;
			mcu_data.dev_handle = mcu;
			mcu_data.interface = 0;
			fpga_data.dev_handle = fpga;
			fpga_data.interface = 1;
			/* Start threads */
			fprintf(stdout, "Creating FPGA thread...                             ");
			if (pthread_create(&fpga_thread, NULL, fpga_runloop, (void*)&fpga_data)) {
				fprintf(stdout, _RED"FAILED!\n"_RESET);
				return EXIT_FAILURE;
			}
			fprintf(stdout, _GREEN"DONE!\n"_RESET);
			fprintf(stdout, "Creating MCU thread...                              ");
			if (pthread_create(&mcu_thread, NULL, mcu_runloop, (void*)&mcu_data)) {
				fprintf(stdout, _RED"FAILED!\n"_RESET);
				return EXIT_FAILURE;
			}
			fprintf(stdout, _GREEN"DONE!\n"_RESET);
			barrier_wait(&barrier);
			fprintf(stdout, "Syncing threads...                                  "_GREEN"DONE!\n"_RESET);
			fflush(stdout);
			goto _mainloop;
		} else if (cmd == STOP && _keepalive && connected) {
			_keepalive = false;
			fprintf(stdout, "Joining threads...                                  ");
			pthread_join(fpga_thread, NULL);
			pthread_join(mcu_thread, NULL);
			fprintf(stdout, _GREEN"DONE!\n"_RESET);
			goto _mainloop;
		} else if (cmd == QUIT) {
			goto _exit;
		} else if (cmd == START && !connected) {
			fprintf(stdout, _MAGENTA"No connected devices!\n"_RESET);
			goto _mainloop;
		} else if (cmd == STOP && !connected) {
			fprintf(stdout, _MAGENTA"Nothing to stop!\n"_RESET);
			goto _mainloop;
		} else {
			goto _mainloop;
		}
	}

	_exit: ;

	if (connected) {
		libusb_release_interface(mcu, 0);
		libusb_release_interface(fpga, 1);
		libusb_close(mcu);
		libusb_close(fpga);
	}

	libusb_exit(context);
}