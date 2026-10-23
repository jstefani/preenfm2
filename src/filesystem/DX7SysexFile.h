/*
 * DX7SysexFile.h
 *
 *  Created on: 24 juil. 2015
 *      Author: xavier
 */

#ifndef DX7SYSEXFILE_H_
#define DX7SYSEXFILE_H_

#include "PreenFMFileType.h"
#include "DX7Random.h"

class DX7SysexFile: public PreenFMFileType {
public:
	DX7SysexFile();
	virtual ~DX7SysexFile();

	uint8_t* dx7LoadPatch(const struct PFM2File* bank, int patchNumber);

	// The bank list starts with DX7_RANDOM_MODES virtual banks whose 32
	// patches are generated from a seed; the banks on the key follow.
	// Hides PreenFMFileType::getFile : always call through a DX7SysexFile*.
	const struct PFM2File* getFile(int fileNumber);
	// Index of the virtual bank (a Dx7RandomMode), or -1 for a file
	int randomBankIndex(const struct PFM2File* bank);
	// Draws a new seed : the next patches read from that bank are new
	void newRandomBank(int randomBank);

protected:
	const char* getFolderName();
	bool isCorrectFile(char *name, int size);
	struct PFM2File dx7Bank[NUMBEROFDX7BANKS];
	struct PFM2File randomBank[DX7_RANDOM_MODES];
	uint32_t randomSeed[DX7_RANDOM_MODES];

};

#endif /* DX7SYSEXFILE_H_ */
