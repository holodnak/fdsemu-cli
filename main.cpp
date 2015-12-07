#include <stdio.h>
#include "version.h"
#include "Device.h"
#include "Flash.h"

CDevice dev;

enum {
	SLOTSIZE = 65536,
};

void usage()
{
	char usagestr[] =
		"   -f file.fds [1..n]            write image to flash slot [1..n]\n"
		"   -s file.fds [1..n]            save image from flash slot [1..n]\n"
		"   -l                            list images stored on flash\n"
		"";

	printf(usagestr);
}

bool FDS_list() {
	uint8_t buf[256];
	int side = 0;
	printf("\n");
	for (uint32_t slot = 1; slot<dev.Slots; slot++) {
		if (!dev.Flash->Read((slot)*SLOTSIZE, buf, 256))
			return false;

		if (buf[0] == 0xff) {          //empty
			printf("%d:\n", slot);
			side = 0;
		}
		else if (buf[0] != 0) {      //filename present
			printf("%d: %s\n", slot, buf);
			side = 1;
		}
		else if (!side) {          //first side is missing
			printf("%d: ?\n", slot);
		}
		else {                    //next side
			printf("%d:    Side %d\n", slot, ++side);
		}
	}
	return true;
}


int main(int argc, char *argv[])
{
	printf("fdsemu-cli v%d.%d by James Holodnak, portions based on code by loopy\n\n", VERSION / 100, VERSION % 100);

	if (argc < 2) {
		usage();
		return(1);
	}

	if (dev.Open() == false) {
		printf("Error opening device.\n");
		return(2);
	}

	printf("Device opened: %s (%04X:%04X) firmware build %d, %dMB flash\n", dev.DeviceName, dev.VendorID, dev.ProductID, dev.Version, dev.FlashSize / 0x100000);

	switch (argv[1][1]) {
	case 'l':
		FDS_list();
		break;
	}

	dev.Close();

	return(0);
}