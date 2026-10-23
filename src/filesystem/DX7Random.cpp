/*
 * Copyright 2026
 *
 * Random DX7 patch generator for the virtual "Random" DX7 bank.
 * See DX7Random.h.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "DX7Random.h"
#include "msfa_fm_core.h"

namespace {

struct Rng {
    uint32_t s;
    uint32_t next() {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return s;
    }
    int range(int n) { return (int)(next() % (uint32_t)n); }           // 0 .. n-1
    int between(int lo, int hi) { return lo + range(hi - lo + 1); }    // lo .. hi
    bool chance(int percent) { return range(100) < percent; }
};

// Each random patch gets a character that shapes the carrier envelopes
enum Character {
    PERCUSSIVE = 0,
    KEYS,
    PAD,
    LEAD,
    NUMBER_OF_CHARACTERS
};
const char* const characterName[NUMBER_OF_CHARACTERS] = { "PRC", "KEY", "PAD", "LED" };

// Frequency coarse values (0 = x0.5, 1 = x1 ...), weighted by repetition
const uint8_t carrierCoarse[] = { 1, 1, 1, 1, 1, 1, 2, 2, 0, 0, 3, 4 };
const uint8_t modulatorCoarse[] = { 1, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 0 };

// Unpacked VCED (155 bytes) to packed (128 bytes). Inverse of the unpack in
// Hexter / Dx7Engine.
void pack(const uint8_t* u, uint8_t* p) {
    for (int o = 0; o < 6; o++) {
        const uint8_t* s = u + o * 21;
        uint8_t* d = p + o * 17;
        for (int i = 0; i < 11; i++) {
            d[i] = s[i];                          // rates, levels, break point, depths
        }
        d[11] = (s[12] << 2) | (s[11] & 3);       // right curve, left curve
        d[12] = (s[20] << 3) | (s[13] & 7);       // detune, rate scaling
        d[13] = (s[15] << 2) | (s[14] & 3);       // velocity sens, amp mod sens
        d[14] = s[16];                            // output level
        d[15] = (s[18] << 1) | (s[17] & 1);       // coarse, mode
        d[16] = s[19];                            // fine
    }
    for (int i = 0; i < 9; i++) {
        p[102 + i] = u[126 + i];                  // pitch EG + algorithm
    }
    p[111] = (u[136] << 3) | (u[135] & 7);        // osc sync, feedback
    for (int i = 0; i < 4; i++) {
        p[112 + i] = u[137 + i];                  // LFO speed, delay, pmd, amd
    }
    p[116] = (u[143] << 4) | (u[142] << 1) | (u[141] & 1);   // pms, wave, sync
    p[117] = u[144];                              // transpose
    for (int i = 0; i < 10; i++) {
        p[118 + i] = u[145 + i];                  // name
    }
}

}

void dx7GenerateRandomPatch(uint32_t seed, int index, uint8_t* packed) {
    Rng rng;
    rng.s = (seed ^ ((uint32_t)(index + 1) * 0x9E3779B9u)) | 1;
    for (int k = 0; k < 4; k++) {
        rng.next();
    }

    uint8_t u[156];
    for (int k = 0; k < 156; k++) {
        u[k] = 0;
    }

    int algo = rng.range(32);
    int character = rng.range(NUMBER_OF_CHARACTERS);

    // VCED operator slots : slot 0 holds DX7 operator 6. FmCore::isCarrier
    // uses the same slot order.
    for (int slot = 0; slot < 6; slot++) {
        uint8_t* o = &u[slot * 21];
        bool carrier = msfa::FmCore::isCarrier(algo, slot);
        int r1, r2, r3, r4, l1, l2, l3, l4;

        if (carrier) {
            switch (character) {
            case PERCUSSIVE:
                r1 = 99;                 r2 = rng.between(60, 90); r3 = rng.between(50, 80); r4 = rng.between(40, 70);
                l1 = 99;                 l2 = rng.between(70, 95); l3 = rng.between(0, 35);  l4 = 0;
                break;
            case KEYS:
                r1 = rng.between(75, 99); r2 = rng.between(40, 70); r3 = rng.between(30, 60); r4 = rng.between(50, 75);
                l1 = 99;                  l2 = rng.between(85, 99); l3 = rng.between(50, 85); l4 = 0;
                break;
            case PAD:
                r1 = rng.between(25, 60); r2 = rng.between(30, 60); r3 = rng.between(20, 50); r4 = rng.between(20, 45);
                l1 = 99;                  l2 = rng.between(90, 99); l3 = rng.between(85, 99); l4 = 0;
                break;
            default: // LEAD
                r1 = rng.between(80, 99); r2 = rng.between(50, 80); r3 = rng.between(40, 70); r4 = rng.between(55, 85);
                l1 = 99;                  l2 = rng.between(90, 99); l3 = rng.between(80, 99); l4 = 0;
                break;
            }
            o[8] = rng.between(30, 50);            // key scaling break point
            o[9] = rng.range(15);                  // left depth
            o[10] = rng.range(15);                 // right depth
            o[13] = rng.range(3);                  // rate scaling
            o[14] = 0;                             // amp mod sens
            o[15] = rng.between(2, 5);             // velocity sens
            o[16] = rng.between(90, 99);           // output level
            o[18] = carrierCoarse[rng.range(sizeof(carrierCoarse))];
            o[19] = rng.chance(20) ? rng.range(8) : 0;
            o[20] = 7 + rng.between(-1, 1);
        } else {
            r1 = rng.between(60, 99); r2 = rng.between(30, 90); r3 = rng.between(30, 70); r4 = rng.between(40, 99);
            l1 = 99;                  l2 = rng.between(60, 99); l3 = rng.range(100);      l4 = rng.chance(50) ? 0 : rng.range(60);
            o[8] = rng.between(25, 55);
            o[9] = rng.range(30);
            o[10] = rng.range(30);
            o[13] = rng.range(4);
            o[14] = rng.chance(20) ? rng.between(1, 3) : 0;
            o[15] = rng.range(8);
            o[16] = rng.chance(20) ? rng.between(20, 50) : rng.between(50, 92);
            o[18] = modulatorCoarse[rng.range(sizeof(modulatorCoarse))];
            o[19] = rng.chance(20) ? rng.range(100) : (rng.chance(30) ? rng.range(10) : 0);
            o[20] = 7 + rng.between(-2, 2);
        }
        o[0] = r1; o[1] = r2; o[2] = r3; o[3] = r4;
        o[4] = l1; o[5] = l2; o[6] = l3; o[7] = l4;
        o[11] = rng.range(4);                      // left curve
        o[12] = rng.range(4);                      // right curve
        o[17] = 0;                                 // ratio mode
    }

    // Pitch EG : flat, with an occasional short attack blip
    for (int i = 0; i < 4; i++) {
        u[126 + i] = 99;
        u[130 + i] = 50;
    }
    if (rng.chance(15)) {
        u[130] = 50 + rng.between(-15, 15);
        u[126] = rng.between(60, 99);
    }

    u[134] = algo;
    u[135] = rng.chance(60) ? rng.range(8) : 0;   // feedback
    u[136] = 1;                                   // osc key sync
    bool vibrato = rng.chance(40);
    u[137] = rng.between(20, 60);                 // LFO speed
    u[138] = vibrato ? rng.range(60) : 0;         // LFO delay
    u[139] = vibrato ? rng.between(5, 25) : 0;    // pitch mod depth
    u[140] = rng.chance(20) ? rng.between(5, 25) : 0;   // amp mod depth
    u[141] = rng.range(2);                        // LFO key sync
    u[142] = rng.range(6);                        // LFO wave
    u[143] = rng.between(3, 5);                   // pitch mod sens
    u[144] = 24;                                  // transpose : C3

    // Name : "R01 PAD   "
    char* name = (char*)&u[145];
    name[0] = 'R';
    name[1] = '0' + ((index + 1) / 10);
    name[2] = '0' + ((index + 1) % 10);
    name[3] = ' ';
    name[4] = characterName[character][0];
    name[5] = characterName[character][1];
    name[6] = characterName[character][2];
    name[7] = ' ';
    name[8] = ' ';
    name[9] = ' ';

    pack(u, packed);
}
