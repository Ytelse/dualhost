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

#define TEST_DURATION			5.0f
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
		// if (img < 100) {
		// 	fprintf(stdout, "c = %d, mnist_lbl[%d] = %d\n", c, img-1,  mnist_lbl[img-1]);
		// }
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

		for (int i = 0; i < recvd; i++) {
			putc(usb_recv_buffer[i], fout);
		}

		tot_recvd += recvd;
		gettimeofday(&end, NULL);
		ms = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
		s = ms/1e6;
		fprintf(stdout, "\rSent: "_GREEN"%.2f"_RESET" MB | Recv: "_GREEN"%.2f"_RESET" MB | Elapsed: "_GREEN"%.3f"_RESET" s | Up: "_GREEN"%.3f"_RESET" Mbps | Down: "_GREEN"%.3f"_RESET" Mbps", ((double)tot_xferred/1e6), ((double)tot_recvd/1e6),  s, (double)((tot_xferred*8.0f/1e6)/s), (double)(tot_recvd*8.0f/1e6)/s);
		fflush(stdout);

		if (s >= TEST_DURATION) {
			libusb_bulk_transfer(usb_data->dev_handle, MCU_EP_OUT, stop_msg, 64, &xferred, TRANSFER_TIMEOUT);
			fprintf(flog, "** Sent stop message to MCU\n");
			break;
		}
	}

	fclose(fout);

	fprintf(flog, "\n");
	fprintf(flog, "========================================================\n");
	fprintf(flog, "-- Duration:                            %.3f\n", s);
	fprintf(flog, "-- Data received:                       %llu B\n", tot_recvd);
	fprintf(flog, "-- Avg. transfer speed:                 %.3f Mbps\n", (tot_recvd*8/1e6)/s);
	fprintf(flog, "========================================================\n\n");

	int errors, read;
	errors = read = 0;
	fprintf(stdout, "\nVerifying data...                                   ");
	fflush(stdout);
	FILE* fin = fopen(MCU_OUT_FP, "rb");
	c = 0;
	while ((c = getc(fin)) != EOF) {
		unsigned char top = (c & 0xf0) >> 4;
		unsigned char bottom = c & 0x0f;
		if (top != mnist_lbl[read % 70000]) {
			fprintf(flog, "ERROR: Pos %d top -- Exp: %d, act: %d\n", read, (int) mnist_lbl[read % 70000], top);
			errors++;
		}
		read++;
		if (bottom != mnist_lbl[read % 70000]) {
			fprintf(flog, "ERROR: Pos %d bot -- Exp: %d, act: %d\n", read, (int) mnist_lbl[read % 70000], bottom);
			errors++;
		}
		read++;

		if (read >= 0x8000) {
			break;
		}
	}
	fprintf(stdout, _GREEN"DONE!\n"_RESET);

	fprintf(flog, "\n");
	fprintf(flog, "========================================================\n");
	fprintf(flog, "-- Verified:                            %d\n", read);
	fprintf(flog, "-- Correct:                             %d\n", read - errors);
	fprintf(flog, "-- Incorrect:                           %d\n", errors);
	fprintf(flog, "-- Hit rate:                            %.3f%%\n", (((double)read - (double)errors)/(double)read) * 100.0f);
	fprintf(flog, "========================================================\n\n");

	fclose(flog);

	fprintf(stdout, ">>>");
	fflush(stdout);

	return NULL;
}

