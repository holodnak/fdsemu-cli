
UNAME := $(shell uname)

CC       ?= gcc
CXX      ?= g++
CFLAGS   ?= -I hidapi/hidapi -Wall -g -c

TARGET    = fdsemu-cli
CPPOBJS   = Device.o DiskImage.o DiskSide.o Flash.o FlashUtil.o Sram.o System.o main.o
ifeq ($(UNAME),Darwin)
 COBJS    = hidapi/mac/hid.o
 LIBS     = -framework IOKit -framework CoreFoundation -liconv
endif
ifeq ($(UNAME),Linux)
 COBJS    = hidapi/linux/hid.o
 LIBS      = `pkg-config libusb-1.0 --libs` -l pthread
 INCLUDES ?= `pkg-config libusb-1.0 --cflags`
endif
OBJS      = $(COBJS) $(CPPOBJS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -Wall -g $^ $(LIBS) -o $(TARGET)

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
