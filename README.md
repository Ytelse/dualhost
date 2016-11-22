# dualhost
Simplified USB host program connecting to both FPGA and MCU

## Compiling
In order to compile this program the Makefile needs to be edited with the proper location of libusb-1.0. If this is installed on MacOS with homebrew, no changes are necessary. Edit LIBUSB_PATH to point to the root directory of the most current installed version of libusb. Example: `LIBUSB_PATH:=/.../libusb/<version>`

## Running
Can be executed either by `make run` or `./pacman`. Takes no command-line arguments. Type 'help' for a list of commands.

## Input
The input to the program is the 3 files found in `data/`. The file named `bit_mnist_images` is a downsampled version of the MNIST dataset (training and testing set combined) without headers. Each image is thresholded and downsampled to 1 bit per pixel, and 8 pixels are packed into 1 byte. Thus the image size is reduced from 784 bytes per image to 98 bytes per image. The other two files are the label sets from MNIST (`train-labels-idx1-ubyte` and `t10k-labels-idx1-ubyte`), used for validating the output from PACMAN.

## Output
The program outputs 3 files, `fpga_debug.log`, `mcu_debug.log`, and `mcu_out`. The first two is debug output from the threads handling communication with the FPGA and MCU respectively, and the last file is the raw output from the MCU.