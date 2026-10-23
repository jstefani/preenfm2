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

const char* DX7SysexFile::dx7LoadPatchName(const struct PFM2File* bank, int patchNumber) {
    int randomIndex = randomBankIndex(bank);
    if (randomIndex >= 0) {
        dx7GenerateRandomPatch(randomIndex, randomSeed[randomIndex], patchNumber, dx7PackedPatch);
        for (int k = 0; k < 10; k++) {
            patchName[k] = dx7PackedPatch[118 + k];
        }
    } else {
        load(getFullName(bank->name), 6 + patchNumber * DX7_PACKED_PATCH_SIZED + 118, (void*)patchName, 10);
    }
    patchName[10] = 0;
    return patchName;
}

// DX7 bulk dump checksum : two's complement of the sum of the 4096 data bytes, 7 bits
void DX7SysexFile::updateChecksum(const char* fullBankName) {
    uint32_t sum = 0;
    for (int b = 0; b < 32; b++) {
        load(fullBankName, 6 + b * DX7_PACKED_PATCH_SIZED, (void*)dx7PackedPatch, DX7_PACKED_PATCH_SIZED);
        for (int k = 0; k < DX7_PACKED_PATCH_SIZED; k++) {
            sum += dx7PackedPatch[k];
        }
    }
    uint8_t checksum = (uint8_t)((128 - (sum & 0x7f)) & 0x7f);
    save(fullBankName, 6 + 32 * DX7_PACKED_PATCH_SIZED, (void*)&checksum, 1);
}

void DX7SysexFile::dx7SavePatch(const struct PFM2File* bank, int patchNumber, const uint8_t* unpackedVoice) {
    if (patchNumber > 31 || randomBankIndex(bank) >= 0 || bank->fileType != FILE_OK) {
        return;
    }
    dx7PackPatch(unpackedVoice, dx7PackedPatch);
    const char* fullBankName = getFullName(bank->name);
    save(fullBankName, 6 + patchNumber * DX7_PACKED_PATCH_SIZED, (void*)dx7PackedPatch, DX7_PACKED_PATCH_SIZED);
    updateChecksum(fullBankName);
}

const struct PFM2File* DX7SysexFile::addEmptyBank(const char* newBankName) {
    if (!isInitialized) {
        initFiles();
    }
    int k;
    for (k = 0; k < NUMBEROFDX7BANKS && myFiles[k].fileType != FILE_EMPTY; k++);
    if (k == NUMBEROFDX7BANKS) {
        return 0;
    }
    myFiles[k].fileType = FILE_OK;
    for (int n = 0; n < 12; n++) {
        myFiles[k].name[n] = newBankName[n];
    }
    myFiles[k].name[12] = 0;
    isInitialized = false;
    return &myFiles[k];
}

void DX7SysexFile::create(const char* name) {
    const struct PFM2File* newBank = addEmptyBank(name);
    if (newBank == 0) {
        return;
    }
    const char* fullBankName = getFullName(name);
    uint8_t header[6] = { 0xf0, 0x43, 0x00, 0x09, 0x20, 0x00 };
    save(fullBankName, 0, (void*)header, 6);

    uint8_t unpacked[DX7_UNPACKED_PATCH_SIZED + 1];
    dx7InitVoice(unpacked);
    dx7PackPatch(unpacked, dx7PackedPatch);
    uint32_t sum = 0;
    for (int k = 0; k < DX7_PACKED_PATCH_SIZED; k++) {
        sum += dx7PackedPatch[k];
    }
    for (int b = 0; b < 32; b++) {
        save(fullBankName, 6 + b * DX7_PACKED_PATCH_SIZED, (void*)dx7PackedPatch, DX7_PACKED_PATCH_SIZED);
    }
    uint8_t tail[2] = { (uint8_t)((128 - ((sum * 32) & 0x7f)) & 0x7f), 0xf7 };
    save(fullBankName, 6 + 32 * DX7_PACKED_PATCH_SIZED, (void*)tail, 2);
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


