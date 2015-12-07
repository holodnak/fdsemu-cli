#pragma once
#include "Device.h"

enum {
//	PAGESIZE = 256,
	CMD_READID = 0x9F,
	CMD_READSTATUS = 0x05,
	CMD_WRITEENABLE = 0x06,

	CMD_READDATA = 0x03,
	CMD_WRITESTATUS = 0x01,
	CMD_PAGEWRITE = 0x0a,
	CMD_PAGEERASE = 0xdb,
	CMD_PAGEPROGRAM = 0x02,
	CMD_BLOCKERASE = 0xd8,
	CMD_BLOCKERASE64 = CMD_BLOCKERASE,
	CMD_BLOCKERASE32 = 0x52,
	CMD_SECTORERASE = 0x20,
};

class CFlash
{
protected:
	CDevice *dev;
public:
	CFlash(CDevice *d);
	virtual ~CFlash();

	virtual bool Read(uint32_t addr, uint8_t *buf, int size);
	virtual bool Write(uint32_t addr, uint8_t *buf, int size);
};