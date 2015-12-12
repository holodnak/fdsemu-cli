#pragma once

#include <stdint.h>

class CDiskImage
{
private:

protected:

	//disk data in different formats
	uint8_t *fds, *bin, *raw03;

	//number of disk sides
	int sides;

public:
	CDiskImage();
	virtual ~CDiskImage();

	//load disk image from file
	bool Load(char *filename);

	//save disk image to file
	bool Save(char *filename);

	//returns pointer to data in fwNES format
	uint8_t *GetFDS();

	//returns pointer to data in bin format
	uint8_t *GetBin();

	//returns pointer to data in raw03 format
	uint8_t *GetRaw();
};
