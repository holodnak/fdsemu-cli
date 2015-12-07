#include <stdio.h>
#include "Flash.h"

CFlash::CFlash(CDevice *d)
{
	dev = d;
}


CFlash::~CFlash()
{
}

bool CFlash::Read(uint32_t addr, uint8_t *buf, int size)
{
	uint8_t cmd[4] = { CMD_READDATA, 0, 0, 0 };

	cmd[1] = addr >> 16;
	cmd[2] = addr >> 8;
	cmd[3] = addr;
	if (!dev->FlashWrite(cmd, 4, 1, 1))
		return false;
	for (; size>0; size -= SPI_READMAX) {
		if (!dev->FlashRead(buf, size>SPI_READMAX ? SPI_READMAX : size, size>SPI_READMAX))
			return false;
		buf += SPI_READMAX;
	}
	return true;
}

bool CFlash::Write(uint32_t addr, uint8_t *buf, int size)
{
	uint32_t wrote, pageWriteSize;
	bool ok = false;
/*	do {
		if (blockErase(addr) == 0) {
			printf("spi_WriteFlash: blockErase failed\n");
			break;
		}
		if (!unWriteProtect())
		{
			printf("Write protected.\n"); break;
		}
		for (wrote = 0; wrote<size; wrote += pageWriteSize) {
			pageWriteSize = PAGESIZE - (addr & (PAGESIZE - 1));   //bytes left in page
			if (pageWriteSize>size - wrote)
				pageWriteSize = size - wrote;
			if (pageProgram(addr + wrote, buf + wrote, pageWriteSize) == 0) {
				printf("spi_WriteFlash: pageErase failed\n");
				break;
			}
			//				if (!pageWrite(addr + wrote, buf + wrote, pageWriteSize))
			//					break;
			if ((addr + wrote) % 0x800 == 0)
				printf(".");
		}
		printf("\n");
		ok = (wrote == size);
	} while (0);*/
	return ok;
}
