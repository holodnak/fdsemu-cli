#include <stdio.h>
#include <string.h>
#include "version.h"
#include "Device.h"
#include "Flash.h"

CDevice dev;

enum {
	SLOTSIZE = 65536,
};

enum {
	ACTION_NONE = 0,
	ACTION_USAGE,
	ACTION_CONVERT,
	ACTION_LIST,
};

int action = ACTION_NONE;
int verbose = 0;

void usage()
{
	char usagestr[] =
		"   -f file.fds [1..n]            write image to flash slot [1..n]\n"
		"   -s file.fds [1..n]            save image from flash slot [1..n]\n"
		"   -l                            list images stored on flash\n"
		"   -v                            more verbose output\n"
		"";

	printf(usagestr);
}

//list disks stored in flash
bool fds_list()
{
	uint8_t buf[256];
	uint32_t slot;
	int side = 0, empty = 0;

	//loop thru all possible disk sides stored on flash
	for (slot = 1; slot < dev.Slots; slot++) {

		//read header from flash
		if (dev.Flash->Read(slot * SLOTSIZE, buf, 256) == false)
			return(false);

		//empty slot
		if (buf[0] == 0xFF) {          //empty
			empty++;
		}

		//filename is here
		else if (buf[0] != 0) {
			printf(" . %s\n", buf);
		}
	}
	printf("\nEmpty slots: %d\n", empty);
	return(true);
}

bool fds_list_verbose()
{
	uint8_t buf[256];
	uint32_t slot;
	int side = 0;

	//loop thru all possible disk sides stored on flash
	for (slot = 1; slot < dev.Slots; slot++) {

		//read header from flash
		if (dev.Flash->Read(slot * SLOTSIZE, buf, 256) == false)
			return(false);

		if (buf[0] == 0xFF) {          //empty
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
	return(true);
}

int main(int argc, char *argv[])
{
	int i;
	bool success;

	printf("fdsemu-cli v%d.%d by James Holodnak, portions based on code by loopy\n", VERSION / 100, VERSION % 100);

	if (argc < 2) {
		action = ACTION_USAGE;
		usage();
	}

	if (dev.Open() == false) {
		printf("Error opening device.\n");
		return(2);
	}

	printf(" . Device opened: %s (%04X:%04X) firmware build %d, %dMB flash\n", dev.DeviceName, dev.VendorID, dev.ProductID, dev.Version, dev.FlashSize / 0x100000);

	for (i = 1; i < argc; i++) {

		//use more verbose output
		if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
			verbose = 1;
		}

		//get program usage
		if (strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			action = ACTION_USAGE;
		}

		//list disk images
		else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0) {
			action = ACTION_LIST;
		}

	}

	switch (action) {

	default:
		success = false;
		break;

	case ACTION_NONE:
		printf("\nNo operation specified.\n");
		success = true;
		break;
	case ACTION_LIST:
		printf("\nListing disks stored in flash:\n");
		if (verbose) {
			success = fds_list_verbose();
		}
		else {
			success = fds_list();
		}
		break;

	}

	dev.Close();

	if (success) {
		printf("Operation completed successfully.\n");
	}
	else {
		printf("Operation failed.\n");
	}
	return(0);
}
