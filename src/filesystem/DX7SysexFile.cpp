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
	for (int b = 0; b < DX7_RANDOM_MODES; b++) {
		const char* name = dx7RandomBankName[b];
		int k = 0;
		for (; name[k] != 0 && k < 12; k++) {
			randomBank[b].name[k] = name[k];
		}
		for (; k < 13; k++) {
			randomBank[b].name[k] = 0;
		}
		randomBank[b].fileType = FILE_OK;
		// Fixed seed until the bank is selected : deterministic at boot
		randomSeed[b] = 0x2545F491 + b;
	}
}

const struct PFM2File* DX7SysexFile::getFile(int fileNumber) {
	if (fileNumber >= 0 && fileNumber < DX7_RANDOM_MODES) {
		return &randomBank[fileNumber];
	}
	return PreenFMFileType::getFile(fileNumber - DX7_RANDOM_MODES);
}

int DX7SysexFile::randomBankIndex(const struct PFM2File* bank) {
	for (int b = 0; b < DX7_RANDOM_MODES; b++) {
		if (bank == &randomBank[b]) {
			return b;
		}
	}
	return -1;
}

void DX7SysexFile::newRandomBank(int b) {
	randomSeed[b] = RNG_GetRandomNumber();
	if (randomSeed[b] == 0) {
		randomSeed[b] = 1;
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
    int randomIndex = randomBankIndex(bank);
    if (randomIndex >= 0) {
        dx7GenerateRandomPatch(randomIndex, randomSeed[randomIndex], patchNumber, dx7PackedPatch);
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


