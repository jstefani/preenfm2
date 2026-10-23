/*
 * DX7SysexFile.cpp
 *
 *  Created on: 24 juil. 2015
 *      Author: xavier
 */

#include "DX7SysexFile.h"
#include "DX7Random.h"
#include "stm32f4xx_rng.h"

uint8_t dx7PackedPatch[DX7_PACKED_PATCH_SIZED];


DX7SysexFile::DX7SysexFile() {
	numberOfFilesMax = NUMBEROFDX7BANKS;
	myFiles = dx7Bank;
	const char* name = "*Random*";
	int k = 0;
	for (; name[k] != 0; k++) {
		randomBank.name[k] = name[k];
	}
	for (; k < 13; k++) {
		randomBank.name[k] = 0;
	}
	randomBank.fileType = FILE_OK;
	// Fixed seed until the bank is selected : deterministic at boot
	randomSeed = 0x2545F491;
}

const struct PFM2File* DX7SysexFile::getFile(int fileNumber) {
	if (fileNumber == 0) {
		return &randomBank;
	}
	return PreenFMFileType::getFile(fileNumber - 1);
}

void DX7SysexFile::newRandomBank() {
	randomSeed = RNG_GetRandomNumber();
	if (randomSeed == 0) {
		randomSeed = 1;
	}
}

DX7SysexFile::~DX7SysexFile() {
}

const char* DX7SysexFile::getFolderName() {
	return DX7_DIR;
}

uint8_t* DX7SysexFile::dx7LoadPatch(const struct PFM2File* bank, int patchNumber) {
    if (patchNumber > 31) {
    	return (uint8_t*)0;
    }
    if (bank == &randomBank) {
        dx7GenerateRandomPatch(randomSeed, patchNumber, dx7PackedPatch);
        return dx7PackedPatch;
    }
    const char* fullBankName = getFullName(bank->name);
    int result = load(fullBankName, patchNumber * DX7_PACKED_PATCH_SIZED + 6,  (void*)dx7PackedPatch, DX7_PACKED_PATCH_SIZED);
    if (result >0) {
    	return (uint8_t*)0;
    }
    return dx7PackedPatch;
}

bool DX7SysexFile::isCorrectFile(char *name, int size)  {
	// DX7 Dump sysex size is 4104
	if (size != 4104) {
		return false;
	}

	int pointPos = -1;
    for (int k=1; k<9 && pointPos == -1; k++) {
        if (name[k] == '.') {
            pointPos = k;
        }
    }
    if (pointPos == -1) return false;
    if (name[pointPos+1] != 's' && name[pointPos+1] != 'S') return false;
    if (name[pointPos+2] != 'y' && name[pointPos+2] != 'Y') return false;
    if (name[pointPos+3] != 'x' && name[pointPos+3] != 'X') return false;

    return true;
}


