#include "mcucomm.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <pthread.h>
#include "barrier.h"

#include <libusb.h>
#include "usbdevice.h"
#include "callbacks.h"

#include "debug.h"

#define TEST_DURATION			15.0f
#define DBG_OUTPUT_FP 			"mcu_debug.log"
#define MCU_OUT_FP				"mcu_out"
#define TRAIN_LBL_FP			"data/train-labels-idx1-ubyte"
#define TEST_LBL_FP				"data/t10k-labels-idx1-ubyte"


#define TRANSFER_TIMEOUT 		1000
#define RECV_BUF_SIZE 			4096*4

extern barrier_t barrier;
extern long long tot_xferred, tot_recvd;

static unsigned char start_msg[64] = "run";
static unsigned char stop_msg[64] = "stop";
static unsigned char usb_recv_buffer[RECV_BUF_SIZE];

static unsigned char mnist_lbl[70000];

// static void recv_async(libusb_device_handle* dev_handle, unsigned char* buffer, int buflen);
// static void send_async(libusb_device_handle* dev_handle, unsigned char* buffer, int buflen);

void* mcu_runloop(void* pdata_void_ptr) {
	struct USB_data* usb_data = (struct USB_data*) pdata_void_ptr;

	struct timeval start, end;
	long int ms;
	double s;

	FILE *flog, *fout, *flbl1, *flbl2;
	flog = fopen(DBG_OUTPUT_FP, "w");
	fout = fopen(MCU_OUT_FP, "w");

	/* ======== OUTPUT DBG INFO ======== */
	fprintf(flog, "========================================================\n");
	fprintf(flog, "                      MCU --> USB Host                  \n");
	fprintf(flog, "========================================================\n\n");
	fprintf(flog, "======================= FILE I/O =======================\n");
	fprintf(flog, "** READING MNIST LABELS:\n");
	
	gettimeofday(&start, NULL);
	flbl1 = fopen(TRAIN_LBL_FP, "rb");
	flbl2 = fopen(TEST_LBL_FP, "rb");
	fseek(flbl1, 8, SEEK_SET);
	fseek(flbl2, 8, SEEK_SET);
	int img = 0;
	int c = 0;
	while ((c = fgetc(flbl1)) != EOF) {
		mnist_lbl[img++] = (unsigned char) c;
	}
	fprintf(flog, "  -- Read %d bytes from training label set.\n", img);
	while ((c = fgetc(flbl2)) != EOF) {
		mnist_lbl[img++] = (unsigned char) c;
	}
	fprintf(flog, "  -- Read %d bytes from MNIST label set.\n", img);
	gettimeofday(&end, NULL);
	ms = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
	s = ms/1e6;
	fprintf(flog, "  ** MNIST label set read in %.3f seconds.\n", s);
	fclose(flbl1);
	fclose(flbl2);

	barrier_wait(&barrier);
	/* Both threads ready, start the show */
	tot_recvd = 0;
	int xferred;
	libusb_bulk_transfer(usb_data->dev_handle, MCU_EP_OUT, start_msg, 64, &xferred, TRANSFER_TIMEOUT);
	fprintf(flog, "** Sent start message to MCU\n");

	gettimeofday(&start, NULL);

	while (1) {
		int recvd;
		libusb_bulk_transfer(usb_data->dev_handle, MCU_EP_IN, usb_recv_buffer, RECV_BUF_SIZE, &recvd, TRANSFER_TIMEOUT);
		fwrite(usb_recv_buffer, sizeof(unsigned char), recvd, fout);
		tot_recvd += recvd;
		gettimeofday(&end, NULL);
		ms = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
		s = ms/1e6;
		fprintf(stdout, "\rSent: \x1b[32m%.2f\x1b[0m MB | Recv: \x1b[32m%.2f\x1b[0m MB | Elapsed: \x1b[32m%.3f\x1b[0m s | Up: \x1b[32m%.3f\x1b[0m Mbps | Down: \x1b[32m%.3f\x1b[0m Mbps", ((double)tot_xferred/1e6), ((double)tot_recvd/1e6),  s, (double)((tot_xferred*8.0f/1e6)/s), (double)(tot_recvd*8.0f/1e6)/s);
		fflush(stdout);

		if (s >= TEST_DURATION) {
			libusb_bulk_transfer(usb_data->dev_handle, MCU_EP_OUT, stop_msg, 64, &xferred, TRANSFER_TIMEOUT);
			fprintf(flog, "** Sent stop message to MCU\n");
			break;
		}
	}

	fprintf(flog, "\n");
	fprintf(flog, "========================================================\n");
	fprintf(flog, "-- Duration:                            %.3f\n", s);
	fprintf(flog, "-- Data received:                       %llu B\n", tot_recvd);
	fprintf(flog, "-- Avg. transfer speed:                 %.3f Mbps\n", (tot_recvd*8/1e6)/s);
	fprintf(flog, "========================================================\n");







	fclose(flog);

	fprintf(stdout, "\n");
	fprintf(stdout, ">>>");
	fflush(stdout);

	return NULL;
}

