/*
 * DX7SysexFile.h
 *
 *  Created on: 24 juil. 2015
 *      Author: xavier
 */

#ifndef DX7SYSEXFILE_H_
#define DX7SYSEXFILE_H_

#include "PreenFMFileType.h"

class DX7SysexFile: public PreenFMFileType {
public:
	DX7SysexFile();
	virtual ~DX7SysexFile();

	uint8_t* dx7LoadPatch(const struct PFM2File* bank, int patchNumber);

	// The bank list starts with a virtual "Random" bank (index 0) whose 32
	// patches are generated from a seed; the banks on the key follow.
	// Hides PreenFMFileType::getFile : always call through a DX7SysexFile*.
	const struct PFM2File* getFile(int fileNumber);
	bool isRandomBank(const struct PFM2File* bank) { return bank == &randomBank; }
	// Draws a new seed : the next patches read from the random bank are new
	void newRandomBank();

protected:
	const char* getFolderName();
	bool isCorrectFile(char *name, int size);
	struct PFM2File dx7Bank[NUMBEROFDX7BANKS];
	struct PFM2File randomBank;
	uint32_t randomSeed;

};

#endif /* DX7SYSEXFILE_H_ */
