#pragma once

#include <stdint.h>
#include "hidapi/hidapi/hidapi.h"

enum {
	SPI_WRITEMAX = 64 - 4,
	SPI_READMAX = 63,

	DISK_READMAX = 254,
	DISK_WRITEMAX = 255,

	//HID reportIDs
	ID_RESET = 0xf0,
	ID_UPDATEFIRMWARE = 0xf1,
	ID_SELFTEST = 0xf2,

	ID_SPI_READ = 1,
	ID_SPI_READ_STOP,
	ID_SPI_WRITE,

	ID_SPI_SRAM_READ,
	ID_SPI_SRAM_READ_STOP,
	ID_SPI_SRAM_WRITE,

	ID_READ_IO = 0x10,
	ID_DISK_READ_START,
	ID_DISK_READ,
	ID_DISK_WRITE_START,
	ID_DISK_WRITE,

	ID_FIRMWARE_READ = 0x40,
	ID_FIRMWARE_WRITE,
	ID_FIRMWARE_UPDATE,
};

class CFlash;
class CDevice
{
private:
	hid_device *handle;
	uint8_t	hidbuf[256];

public:
	char		DeviceName[256];
	int		Version;
	int		VendorID, ProductID;
	CFlash	*Flash;
	uint32_t	FlashID;
	uint32_t FlashSize, Slots;

private:

	//read flash id from device
	uint32_t ReadFlashID();

	//lookup flash size from table, returns size in bytes
	uint32_t GetFlashSize();

protected:

	//generic reading/writing functions
	bool GenericRead(int reportid, uint8_t *buf, int size, bool holdCS);
	bool GenericWrite(int reportid, uint8_t *buf, int size, bool initCS, bool holdCS);

public:
	CDevice();
	virtual ~CDevice();

	bool Open();
	void Close();

	//misc device commands
	void Reset();
	void Test();
	void UpdateFirmware();

	//spi flash commands
	bool FlashRead(uint8_t *buf, int size, bool holdCS);
	bool FlashWrite(uint8_t *buf, int size, bool initCS, bool holdCS);

	//spi sram commands
	bool SramRead(uint8_t *buf, int size, bool holdCS);
	bool SramWrite(uint8_t *buf, int size, bool initCS, bool holdCS);
};

#include "Flash.h"
