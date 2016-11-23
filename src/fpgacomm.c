#include "fpgacomm.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <pthread.h>
#include "barrier.h"

#include <libusb.h>
#include "usbdevice.h"
#include "callbacks.h"

#include "debug.h"

extern barrier_t barrier;
extern long long tot_xferred;

#define MNIST_FP 			"data/mnist.bytes"

#define NOF_IMG 			70000
#define IMG_SIZE 			98

#define IMG_PR_TRANSFER 	350
#define TRANSFER_TIMEOUT 	1000

#define TEST_DURATION 		5.0f
#define DBG_OUTPUT_FP		"fpga_debug.log"

static unsigned char mnist[NOF_IMG*IMG_SIZE];

void* fpga_runloop(void* pdata_void_ptr) {
	struct USB_data* usb_data = (struct USB_data*) pdata_void_ptr;

	struct timeval start, end;
	long int ms;
	double s;

	/* ======== OPEN FILES ======== */

	FILE *fmnist, *flog;
	fmnist = fopen(MNIST_FP, "rb");
	flog = fopen(DBG_OUTPUT_FP, "w");

	/* ======== OUTPUT DBG INFO ======== */
	fprintf(flog, "========================================================\n");
	fprintf(flog, "                    USB Host --> FPGA                   \n");
	fprintf(flog, "========================================================\n\n");
	fprintf(flog, "======================= FILE I/O =======================\n");
	fprintf(flog, "** READING MNIST:\n");

	/* ======== READ IMAGES ======== */

	gettimeofday(&start, NULL);
	long long idx = 0;
	int c = 0;
	while((c = getc(fmnist)) != EOF) {
		mnist[idx++] = (unsigned char) c;
		if ((idx % (700 * IMG_SIZE)) == 0) {
			fprintf(stdout, "\rReading MNIST...                                    %2d%%", (int) ((idx / 98)  / (NOF_IMG/100)));
			fflush(stdout);
		}
	}
	gettimeofday(&end, NULL);
	ms = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
	s = ms/1e6;

	fprintf(stdout, "\rReading MNIST...                                    "_GREEN"DONE!\n"_RESET);

	/* ======== OUTPUT DBG INFO ======== */
	fprintf(flog, "   -- Read %llu bytes (%.3f MB) in %.3f seconds.\n", idx, (double)idx/1e6, s);
	fclose(fmnist);

	barrier_wait(&barrier);
	tot_xferred = 0;
	int iter = 0;
	int cnt = 0;
	gettimeofday(&start, NULL);
	while (1) {
		int xferred;
		libusb_bulk_transfer(usb_data->dev_handle, FPGA_EP_OUT, &mnist[iter * IMG_SIZE * IMG_PR_TRANSFER], IMG_SIZE*IMG_PR_TRANSFER, &xferred, TRANSFER_TIMEOUT);
		cnt++;
		tot_xferred += xferred;
		gettimeofday(&end, NULL);
		ms = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
		s = ms/1e6;
		iter++;
		iter = iter % (NOF_IMG / IMG_PR_TRANSFER);
		if (s >= TEST_DURATION) {
			break;
		}
	}

	fprintf(flog, "\n");
	fprintf(flog, "========================================================\n");
	fprintf(flog, "-- Duration:                            %.3f\n", s);
	fprintf(flog, "-- Number of transfers:                 %d\n", cnt);
	fprintf(flog, "-- Data transferred:                    %llu B\n", tot_xferred);
	fprintf(flog, "-- Avg. transfer speed:                 %.3f Mbps\n", (tot_xferred*8.0f/1e6)/s);
	fprintf(flog, "========================================================\n");


	return NULL;
}
