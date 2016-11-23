################
# DEFINITIONS
################

# This will be the name of the executable
TARGET:=pacman

# Directories
OBJDIR:=build
SRCDIR:=src

# ANSI color codes
# Only used for fancy output
NO_COLOR=\x1b[0m
RED:=\x1b[31m
GREEN:=\x1b[32m
YELLOW:=\x1b[33m

################
# TOOLCHAIN
################

CC:=gcc
LD:=gcc

################
# FLAGS
################

LIBUSB_PATH:=/usr/local/Cellar/libusb/1.0.20

INCLUDES:=-I$(LIBUSB_PATH)/include/libusb-1.0 -I$(SRCDIR) -Iinclude
LIBS:=-L$(LIBUSB_PATH)/lib

CFLAGS:=-std=c99 -Wall $(INCLUDES) -O2 -DNDEBUG
LDFLAGS:=$(LIBS) -lusb-1.0 -lpthread

# UNCOMMENT FOLLOWING LINE FOR SOUND TO PLAY AT BEGINNING OF PROGRAM

CFLAGS+=-DPLAY_SOUND

################
# FILES
################

CSRC:= $(addprefix $(SRCDIR)/, \
dualhost.c \
fpgacomm.c \
mcucomm.c \
usbdevice.c \
debug.c \
cmdparser.c \
barrier.c \
)
#callbacks.c

C_FILES := $(notdir $(CSRC))
C_PATHS := $(dir $(CSRC))
C_OBJS := $(addprefix $(OBJDIR)/, $(C_FILES:.c=.o))
OBJS = $(C_OBJS)

vpath %.c $(C_PATHS)

################
# BUILD RULES
################

.PHONY : all run clean
all : $(TARGET)

.PHONY : run
run : $(TARGET)
	./$(TARGET)

$(TARGET) : $(OBJS)
	@echo "$(YELLOW)Linking target: $@$(NO_COLOR)"
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "$(GREEN)==========================================$(NO_COLOR)"

$(OBJDIR)/%.o : %.c | $(OBJDIR)
	@echo "$(YELLOW)Compiling source: $@$(NO_COLOR)"
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR) :
	mkdir $@

clean :
	-rm -rf $(OBJDIR) $(TARGET) *.log mcu_out