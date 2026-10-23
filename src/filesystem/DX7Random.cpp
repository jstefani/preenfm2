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

}

// Unpacked VCED (155 bytes) to packed (128 bytes).
void dx7PackPatch(const uint8_t* u, uint8_t* p) {
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

void dx7InitVoice(uint8_t* u) {
    for (int k = 0; k < 156; k++) {
        u[k] = 0;
    }
    for (int slot = 0; slot < 6; slot++) {
        uint8_t* o = &u[slot * 21];
        o[0] = 99; o[1] = 99; o[2] = 99; o[3] = 99;      // rates
        o[4] = 99; o[5] = 99; o[6] = 99; o[7] = 0;       // levels
        o[8] = 39;                                       // break point C3
        o[16] = (slot == 5) ? 99 : 0;                    // only operator 1 is heard
        o[18] = 1;                                       // coarse x1
        o[20] = 7;                                       // detune centre
    }
    for (int i = 0; i < 4; i++) {
        u[126 + i] = 99;
        u[130 + i] = 50;
    }
    u[134] = 0;                                          // algorithm 1
    u[136] = 1;                                          // osc key sync
    u[137] = 35;                                         // LFO speed
    u[141] = 1;                                          // LFO key sync
    u[143] = 3;                                          // pitch mod sens
    u[144] = 24;                                         // transpose C3
    const char* name = "INIT VOICE";
    for (int k = 0; k < 10; k++) {
        u[145 + k] = name[k];
    }
}

// Packed to unpacked VCED. Same routine as Hexter::patchUnpack.
void dx7UnpackPatch(const uint8_t* packed_patch, uint8_t* unpacked_patch) {
    const uint8_t *pp = packed_patch;
    uint8_t *up = unpacked_patch;
    int i, j;

    for (i = 6; i > 0; i--) {
        for (j = 11; j > 0; j--) {
            *up++ = *pp++;
        }                           /* through rd */
        *up++     = (*pp) & 0x03;   /* lc */
        *up++     = (*pp++) >> 2;   /* rc */
        *up++     = (*pp) & 0x07;   /* rs */
        *(up + 6) = (*pp++) >> 3;   /* pd */
        *up++     = (*pp) & 0x03;   /* ams */
        *up++     = (*pp++) >> 2;   /* kvs */
        *up++     = *pp++;          /* ol */
        *up++     = (*pp) & 0x01;   /* m */
        *up++     = (*pp++) >> 1;   /* fc */
        *up       = *pp++;          /* ff */
        up += 2;
    }                               /* operator done */
    for (i = 9; i > 0; i--) {
        *up++ = *pp++;
    }                               /* through algorithm */
    *up++ = (*pp) & 0x07;           /* feedback */
    *up++ = (*pp++) >> 3;           /* oks */
    for (i = 4; i > 0; i--) {
        *up++ = *pp++;
    }                               /* through lamd */
    *up++ = (*pp) & 0x01;           /* lfo ks */
    *up++ = ((*pp) >> 1) & 0x07;    /* lfo wave */
    *up++ = (*pp++) >> 4;           /* lfo pms */
    for (i = 11; i > 0; i--) {
        *up++ = *pp++;
    }                               /* transpose + name */
    unpacked_patch[155] = 0;
}

static void generateTame(Rng& rng, int index, uint8_t* u) {
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
}

// Every parameter drawn over its full legal range. The only guard is that
// at least one carrier is loud and actually opens, so a note is heard.
static void generateChaos(Rng& rng, int index, uint8_t* u, bool percussive) {
    int algo = rng.range(32);
    int loudestCarrier = -1;
    int loudestLevel = -1;

    for (int slot = 0; slot < 6; slot++) {
        uint8_t* o = &u[slot * 21];
        bool carrier = msfa::FmCore::isCarrier(algo, slot);
        for (int i = 0; i < 8; i++) {
            o[i] = rng.range(100);                 // rates 1-4, levels 1-4
        }
        if (percussive) {
            // fast attack, decay to little or nothing, short release
            o[0] = rng.between(85, 99);            // R1
            o[1] = rng.between(55, 85);            // R2 : 50 ms .. 1 s, not a click
            o[2] = rng.between(50, 80);            // R3
            o[3] = rng.between(45, 99);            // R4
            o[4] = carrier ? 99 : rng.between(70, 99);   // L1
            o[6] = rng.range(carrier ? 25 : 60);   // L3 : the sustain is low
            o[7] = carrier ? 0 : rng.range(30);    // L4
        }
        o[8] = rng.range(100);                     // break point
        o[9] = rng.range(100);                     // left depth
        o[10] = rng.range(100);                    // right depth
        o[11] = rng.range(4);                      // left curve
        o[12] = rng.range(4);                      // right curve
        o[13] = rng.range(8);                      // rate scaling
        o[14] = rng.range(4);                      // amp mod sens
        o[15] = rng.range(8);                      // velocity sens
        o[16] = rng.range(100);                    // output level
        o[17] = rng.chance(15) ? 1 : 0;            // fixed frequency mode, now and then
        o[18] = rng.range(32);                     // coarse
        o[19] = rng.range(100);                    // fine
        o[20] = rng.range(15);                     // detune
        if (carrier) {
            o[7] = 0;                              // L4 : carriers release to silence, no droning voices
            if (o[16] > loudestLevel) {
                loudestLevel = o[16];
                loudestCarrier = slot;
            }
        }
    }
    // Silence guard on the loudest carrier : it must be heard at any note
    if (loudestCarrier >= 0) {
        uint8_t* o = &u[loudestCarrier * 21];
        if (o[0] < 50) {
            o[0] = rng.between(50, 99);            // R1 : the attack happens
        }
        if (o[4] < 80) {
            o[4] = rng.between(80, 99);            // L1 : and reaches something
        }
        if (o[5] < 60) {
            o[5] = rng.between(60, 99);            // L2 : and does not vanish right after
        }
        if (!percussive && o[6] < 10 && o[2] > 85) {
            o[2] = rng.between(40, 85);            // R3 : a decay, not a click
        }
        o[8] = rng.between(30, 50);                // break point near the middle
        if (o[9] > 15) {
            o[9] = rng.range(16);                  // key level scaling kept mild
        }
        if (o[10] > 15) {
            o[10] = rng.range(16);
        }
        if (o[13] > 3) {
            o[13] = rng.range(4);                  // rate scaling
        }
        if (o[15] > 4) {
            o[15] = rng.range(5);                  // velocity sens
        }
        if (o[16] < 80) {
            o[16] = rng.between(80, 99);           // output level
        }
        o[17] = 0;                                 // on the keyboard, not fixed
    }

    // Pitch EG : free within about +-2 octaves, percussive favours fast blips
    for (int i = 0; i < 4; i++) {
        u[126 + i] = percussive ? rng.between(60, 99) : rng.range(100);
        u[130 + i] = rng.between(25, 75);
    }
    u[133] = 50;                                   // ends on pitch
    if (!percussive && rng.chance(50)) {
        // half of the time : keep the pitch sane
        for (int i = 0; i < 4; i++) {
            u[130 + i] = 50;
        }
    }

    u[134] = algo;
    u[135] = rng.range(8);                         // feedback
    u[136] = rng.range(2);                         // osc key sync
    u[137] = rng.range(100);                       // LFO speed
    u[138] = rng.range(100);                       // LFO delay
    u[139] = rng.chance(50) ? rng.range(100) : rng.range(20);   // pitch mod depth
    u[140] = rng.range(100);                       // amp mod depth
    u[141] = rng.range(2);                         // LFO key sync
    u[142] = rng.range(6);                         // LFO wave
    u[143] = rng.range(8);                         // pitch mod sens
    u[144] = rng.chance(70) ? 24 : rng.between(12, 36);   // transpose

    // Name : "C01 XKQ   " / "P01 XKQ   "
    char* name = (char*)&u[145];
    name[0] = percussive ? 'P' : 'C';
    name[1] = '0' + ((index + 1) / 10);
    name[2] = '0' + ((index + 1) % 10);
    name[3] = ' ';
    for (int k = 4; k < 7; k++) {
        name[k] = 'A' + rng.range(26);
    }
    name[7] = ' ';
    name[8] = ' ';
    name[9] = ' ';
}

const char* const dx7RandomBankName[DX7_RANDOM_MODES] = { "*Random*", "*Chaos*", "*ChaosPrc*" };

void dx7GenerateRandomPatch(int mode, uint32_t seed, int index, uint8_t* packed) {
    Rng rng;
    rng.s = (seed ^ ((uint32_t)(index + 1) * 0x9E3779B9u) ^ ((uint32_t)mode * 0x85EBCA6Bu)) | 1;
    for (int k = 0; k < 4; k++) {
        rng.next();
    }

    uint8_t u[156];
    for (int k = 0; k < 156; k++) {
        u[k] = 0;
    }

    switch (mode) {
    case DX7_RANDOM_CHAOS:
        generateChaos(rng, index, u, false);
        break;
    case DX7_RANDOM_CHAOS_PERC:
        generateChaos(rng, index, u, true);
        break;
    default:
        generateTame(rng, index, u);
        break;
    }

    dx7PackPatch(u, packed);
}
