#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "version.h"
#include "Device.h"
#include "Flash.h"
#include "System.h"

CDevice dev;

enum {
	SLOTSIZE = 65536,
};

enum {
	ACTION_NONE = 0,
	ACTION_CONVERT,
	ACTION_LIST,
	ACTION_READFLASH,
	ACTION_WRITEFLASH,
	ACTION_UPDATEFIRMWARE,
	ACTION_UPDATELOADER,
};

enum {
	DEFAULT_LEAD_IN = 28300,      //#bits (~25620 min)
	GAP = 976 / 8 - 1,                //(~750 min)
	MIN_GAP_SIZE = 0x300,         //bits
	FDSSIZE = 65500,              //size of .fds disk side, excluding header
	FLASHHEADERSIZE = 0x100,
};

//don't include gap end
uint16_t calc_crc(uint8_t *buf, int size) {
	uint32_t crc = 0x8000;
	int i;
	while (size--) {
		crc |= (*buf++) << 16;
		for (i = 0; i<8; i++) {
			if (crc & 1) crc ^= 0x10810;
			crc >>= 1;
		}
	}
	return crc;
}

void copy_block(uint8_t *dst, uint8_t *src, int size) {
	dst[0] = 0x80;
	memcpy(dst + 1, src, size);
	uint32_t crc = calc_crc(dst + 1, size + 2);
	dst[size + 1] = crc;
	dst[size + 2] = crc >> 8;
}

//Adds GAP + GAP end (0x80) + CRCs to .FDS image
//Returns size (0=error)
int fds_to_bin(uint8_t *dst, uint8_t *src, int dstSize) {
	int i = 0, o = 0;

	//check *NINTENDO-HVC* header
	if (src[0] != 0x01 || src[1] != 0x2a || src[2] != 0x4e) {
		printf("Not an FDS file.\n");
		return 0;
	}
	memset(dst, 0, dstSize);

	//block type 1
	copy_block(dst + o, src + i, 0x38);
	i += 0x38;
	o += 0x38 + 3 + GAP;

	//block type 2
	copy_block(dst + o, src + i, 2);
	i += 2;
	o += 2 + 3 + GAP;

	//block type 3+4...
	while (src[i] == 3) {
		int size = (src[i + 13] | (src[i + 14] << 8)) + 1;
		if (o + 16 + 3 + GAP + size + 3 > dstSize) {    //end + block3 + crc + gap + end + block4 + crc
			printf("Out of space (%d bytes short), adjust GAP size?\n", (o + 16 + 3 + GAP + size + 3) - dstSize);
			return 0;
		}
		copy_block(dst + o, src + i, 16);
		i += 16;
		o += 16 + 3 + GAP;

		copy_block(dst + o, src + i, size);
		i += size;
		o += size + 3 + GAP;
	}
	return o;
}

int action = ACTION_NONE;
int verbose = 0;

//allocate buffer and read whole file
bool loadfile(char *filename, uint8_t **buf, int *filesize)
{
	FILE *fp;
	int size;
	bool result = false;

	//check if the pointers are ok
	if (buf == 0 || filesize == 0) {
		return(false);
	}

	//open file
	if ((fp = fopen(filename, "rb")) == 0) {
		return(false);
	}

	//get file size
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//allocate buffer
	*buf = new uint8_t[size];

	//read in file
	*filesize = fread(*buf, 1, size, fp);

	//close file and return
	fclose(fp);
	return(true);
}

static void usage(char *argv0)
{
	char usagestr[] =
		"  Flash operations:\n"
		"    -f file.fds [1..n]            write image to flash slot [1..n]\n"
		"    -s file.fds [1..n]            save image from flash slot [1..n]\n"
		"    -l                            list images stored on flash\n"
		"\n"
		"  Disk operations:\n"
		"    -R file.fds                   read disk to fwNES format disk image\n"
		"    -r file.raw [file.bin]        read disk to raw file (and optional bin file)\n"
		"    -w file.fds                   write disk from fwNES format disk image\n"
		"\n"
		"  Other operations:\n"
		"    -U firmware.bin               update firmware from firmware image\n"
		"    -L loader.fds                 update loader from fwNES loader image\n"
		"\n"
		"  Options:\n"
		"    -v                            more verbose output\n"
		"";

	printf("\n  usage: %s <arguments>\n\n%s", "fdsemu-cli", usagestr);
}

bool firmware_update(char *filename)
{
	uint8_t *firmware;
	uint8_t *buf;
	uint32_t *buf32;
	uint32_t chksum;
	int filesize, i;

	//try to load the firmware image
	if (loadfile(filename, &firmware, &filesize) == false) {
		printf("Error loading firmware file %s'\n", filename);
		return(false);
	}

	//create new buffer to hold 32kb of data and clear it
	buf = new uint8_t[0x8000];
	memset(buf, 0, 0x8000);

	//copy firmware loaded to the new buffer
	memcpy(buf, firmware, filesize);
	buf32 = (uint32_t*)buf;

	//free firmware data
	delete[] firmware;

	//insert firmware identifier
	buf32[(0x8000 - 8) / 4] = 0xDEADBEEF;

	//calculate the simple xor checksum
	chksum = 0;
	for (i = 0; i < (0x8000 - 4); i += 4) {
		chksum ^= buf32[i / 4];
	}

	printf("firmware is %d bytes, checksum is $%08X\n", filesize, chksum);

	//insert checksum into the image
	buf32[(0x8000 - 4) / 4] = chksum;

	printf("uploading new firmware");
	if (!dev.Flash->Write(buf, 0x8000, 0x8000)) {
		printf("Write failed.\n");
		return false;
	}
	delete[] buf;

	printf("waiting for device to reboot\n");

	dev.UpdateFirmware();
	sleep_ms(5000);

	if (!dev.Open()) {
		printf("Open failed.\n");
		return false;
	}
	printf("Updated to build %d\n", dev.Version);

	return(true);
}

uint32_t chksum_calc(uint8_t *buf, int size)
{
	uint32_t ret = 0;
	uint32_t *data = (uint32_t*)buf;
	int i;

	for (i = 0; i < size / 4; i++) {
		ret ^= buf[i];
	}
	return(ret);
}


bool write_flash(char *filename, int slot)
{
	enum { FILENAMELENGTH = 240, };   //number of characters including null

	uint8_t *inbuf = 0;
	uint8_t *outbuf = 0;
	int filesize;

	if (!loadfile(filename, &inbuf, &filesize))
	{
		printf("Can't read %s\n", filename); return false;
	}

	outbuf = new uint8_t[SLOTSIZE];

	int pos = 0, side = 0;
	if (inbuf[0] == 'F')
		pos = 16;      //skip fwNES header

	filesize -= (filesize - pos) % FDSSIZE;  //truncate down to whole disks

	while (pos<filesize && inbuf[pos] == 0x01) {
		printf("Side %d\n", side + 1);
		if (fds_to_bin(outbuf + FLASHHEADERSIZE, inbuf + pos, SLOTSIZE - FLASHHEADERSIZE)) {
			memset(outbuf, 0, FLASHHEADERSIZE);
			uint32_t chksum = chksum_calc(outbuf + FLASHHEADERSIZE, SLOTSIZE - FLASHHEADERSIZE);
			outbuf[240] = (uint8_t)(chksum >> 0);
			outbuf[241] = (uint8_t)(chksum >> 8);
			outbuf[242] = (uint8_t)(chksum >> 16);
			outbuf[243] = (uint8_t)(chksum >> 24);
			outbuf[244] = DEFAULT_LEAD_IN & 0xff;
			outbuf[245] = DEFAULT_LEAD_IN / 256;

			if (side == 0) {
				//strip path from filename
				char *shortName = strrchr(filename, '/');      // ...dir/file.fds
#ifdef _WIN32
				if (!shortName)
					shortName = strrchr(filename, '\\');        // ...dir\file.fds
				if (!shortName)
					shortName = strchr(filename, ':');         // C:file.fds
#endif
				if (!shortName)
					shortName = filename;
				else
					shortName++;
				//                utf8_to_utf16((uint16_t*)outbuf, shortName, FILENAMELENGTH*2);
				//                ((uint16_t*)outbuf)[FILENAMELENGTH-1]=0;
				strncpy((char*)outbuf, shortName, 240);
			}
			dev.Flash->Write(outbuf, (slot + side)*SLOTSIZE, SLOTSIZE);
		}
		pos += FDSSIZE;
		side++;
	}
	delete[] inbuf;
	delete[] outbuf;
	printf("\n");
	return true;
}

//list disks storedin flash
bool fds_list()
{
	uint8_t buf[256];
	uint32_t slot;
	int side = 0, empty = 0;

	//loop thru all possible disk sides stored on flash
	for (slot = 1; slot < dev.Slots; slot++) {

		//read header from flash
		if (dev.Flash->Read(buf, slot * SLOTSIZE, 256) == false)
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
		if (dev.Flash->Read(buf, slot * SLOTSIZE, 256) == false)
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
	char *param = 0;
	char *param2 = 0;

	printf("fdsemu-cli v%d.%d by James Holodnak, portions based on code by loopy\n", VERSION / 100, VERSION % 100);

	if (argc < 2) {
		usage(argv[0]);
		return(1);
	}

	//parse command line
	for (i = 1; i < argc; i++) {

		//use more verbose output
		if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
			verbose = 1;
		}

		//get program usage
		if (strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			usage(argv[0]);
			return(1);
		}

		//list disk images
		else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0) {
			action = ACTION_LIST;
		}

		//write disk image to flash
		else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--write-flash") == 0) {
			if ((i + 1) >= argc) {
				printf("\nPlease specify a filename.\n");
				return(1);
			}
			action = ACTION_WRITEFLASH;
			param = argv[++i];
			if (argv[i][0] != '-') {
				param2 = argv[++i];
			}
		}

		//update firmware
		else if (strcmp(argv[i], "-U") == 0 || strcmp(argv[i], "--update-firmware") == 0) {
			if ((i + 1) >= argc) {
				printf("\nPlease specify a filename for the firmware to update with.\n");
				return(1);
			}
			action = ACTION_UPDATEFIRMWARE;
			param = argv[++i];
		}

	}

	if (dev.Open() == false) {
		printf("Error opening device.\n");
		return(2);
	}

	printf(" . Device opened: %s, %dMB flash (firmware build %d, flashID %06X)\n", dev.DeviceName, dev.FlashSize / 0x100000, dev.Version, dev.FlashID);

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

	case ACTION_UPDATEFIRMWARE:
		success = firmware_update(param);
		break;

	case ACTION_WRITEFLASH:
		int slot = atoi(param2);
		success = write_flash(param,slot);
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
