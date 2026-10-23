/*
 * Copyright 2026
 *
 * Random DX7 patch generator for the virtual "Random" DX7 bank.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef DX7RANDOM_H_
#define DX7RANDOM_H_

#include <stdint.h>

enum Dx7RandomMode {
    DX7_RANDOM_TAME = 0,     // musical ranges, carriers/modulators told apart
    DX7_RANDOM_CHAOS,        // every parameter free, only guarded against silence
    DX7_RANDOM_CHAOS_PERC,   // chaos with fast, percussive envelopes
    DX7_RANDOM_MODES
};

extern const char* const dx7RandomBankName[DX7_RANDOM_MODES];

// Generates patch 'index' (0..31) of the virtual random bank 'mode' identified
// by 'seed' into a 128 byte packed DX7 voice, the format of a 32 voice sysex
// bank. Deterministic : the same seed and index always give the same patch,
// so browsing a bank back and forth is stable. A new seed = a new bank.
void dx7GenerateRandomPatch(int mode, uint32_t seed, int index, uint8_t* packed);

#endif /* DX7RANDOM_H_ */
