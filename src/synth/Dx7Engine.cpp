/*
 * Copyright 2026
 *
 * DX7 engine for the preenfm2, built on the msfa engine (see src/msfa).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Dx7Engine.h"
#include "Common.h"
#include "msfa_env.h"
#include "msfa_freqlut.h"
#include "msfa_pitchenv.h"

Dx7Engine dx7Engine;

// The engine renders msfa::N samples per call : must be the preenfm2 block size
typedef char dx7BlockSizeCheck[(msfa::N == BLOCK_SIZE) ? 1 : -1];

void Dx7Engine::init(float sampleRate) {
    msfa::Env::init_sr(sampleRate);
    msfa::Freqlut::init(sampleRate);
    msfa::Lfo::init(sampleRate);
    msfa::PitchEnv::init(sampleRate);

    for (int v = 0; v < DX7_MAX_VOICES; v++) {
        voices[v].timbre = -1;
        voices[v].midiNote = 0;
        voices[v].released = false;
        voices[v].heldByPedal = false;
        voices[v].index = 0;
        voices[v].fadeBlocks = -1;
    }
    for (int t = 0; t < DX7_NUMBER_OF_TIMBRES; t++) {
        timbres[t].active = false;
        timbres[t].holdPedal = false;
        timbres[t].controllers.init(&core);
        for (int k = 0; k < DX7_UNPACKED_PATCH_SIZE; k++) {
            timbres[t].patch[k] = 0;
        }
    }
    voiceIndex = 1;
    outputScale = 0.0f;
}

// DX7 packed (128 bytes, as in a 32 voice sysex bank) to unpacked (155 bytes)
// Same routine as Hexter::patchUnpack.
void Dx7Engine::unpack(const uint8_t *packed_patch, uint8_t *unpacked_patch) {
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

void Dx7Engine::setTimbrePatch(int timbre, const uint8_t* packedPatch) {
    allSoundOff(timbre);
    Dx7TimbreState& ts = timbres[timbre];
    unpack(packedPatch, ts.patch);
    // LFO parameters start at 137 : speed, delay, pmd, amd, sync, wave
    ts.lfo.reset(&ts.patch[137]);
    ts.controllers.refresh();
    ts.active = true;
}

void Dx7Engine::deactivateTimbre(int timbre) {
    allSoundOff(timbre);
    timbres[timbre].active = false;
}

void Dx7Engine::freeVoice(int v) {
    voices[v].timbre = -1;
    voices[v].fadeBlocks = -1;
    voices[v].released = false;
    voices[v].heldByPedal = false;
}

int Dx7Engine::allocateVoice(int timbre, int maxVoices) {
    int used = 0;
    int freeVoice = -1;
    // Oldest released / oldest playing voice of this timbre
    int stealReleased = -1;
    uint32_t stealReleasedIndex = 0xffffffff;
    int stealOldest = -1;
    uint32_t stealOldestIndex = 0xffffffff;
    // Fallback : oldest voice of any timbre
    int stealAny = -1;
    uint32_t stealAnyIndex = 0xffffffff;

    for (int v = 0; v < DX7_MAX_VOICES; v++) {
        Dx7Voice& voice = voices[v];
        if (voice.timbre < 0) {
            if (freeVoice < 0) {
                freeVoice = v;
            }
            continue;
        }
        if (voice.index < stealAnyIndex) {
            stealAnyIndex = voice.index;
            stealAny = v;
        }
        if (voice.timbre != timbre) {
            continue;
        }
        used++;
        if (voice.released && voice.index < stealReleasedIndex) {
            stealReleasedIndex = voice.index;
            stealReleased = v;
        }
        if (voice.index < stealOldestIndex) {
            stealOldestIndex = voice.index;
            stealOldest = v;
        }
    }

    if (used < maxVoices && freeVoice >= 0) {
        return freeVoice;
    }
    if (stealReleased >= 0) {
        return stealReleased;
    }
    if (stealOldest >= 0) {
        return stealOldest;
    }
    return stealAny;
}

void Dx7Engine::noteOn(int timbre, uint8_t note, uint8_t velocity, int maxVoices) {
    Dx7TimbreState& ts = timbres[timbre];
    if (!ts.active || maxVoices <= 0) {
        return;
    }
    int v = allocateVoice(timbre, maxVoices);
    if (v < 0) {
        return;
    }
    Dx7Voice& voice = voices[v];
    // The audio IRQ renders every voice whose timbre is >= 0, so a free
    // voice is initialised first and made visible last. A stolen voice is
    // re-initialised in place, like the preenfm2 engine does.
    voice.midiNote = note;
    voice.released = false;
    voice.heldByPedal = false;
    voice.fadeBlocks = -1;
    voice.index = voiceIndex++;
    ts.lfo.keydown();
    voice.note.init(ts.patch, note, velocity);
    voice.timbre = timbre;
}

void Dx7Engine::noteOff(int timbre, uint8_t note) {
    Dx7TimbreState& ts = timbres[timbre];
    for (int v = 0; v < DX7_MAX_VOICES; v++) {
        Dx7Voice& voice = voices[v];
        if (voice.timbre == timbre && voice.midiNote == note && !voice.released && !voice.heldByPedal) {
            if (ts.holdPedal) {
                voice.heldByPedal = true;
            } else {
                voice.released = true;
                voice.note.keyup();
            }
        }
    }
}

void Dx7Engine::setHoldPedal(int timbre, bool hold) {
    Dx7TimbreState& ts = timbres[timbre];
    ts.holdPedal = hold;
    if (!hold) {
        for (int v = 0; v < DX7_MAX_VOICES; v++) {
            Dx7Voice& voice = voices[v];
            if (voice.timbre == timbre && voice.heldByPedal) {
                voice.heldByPedal = false;
                voice.released = true;
                voice.note.keyup();
            }
        }
    }
}

void Dx7Engine::allNoteOff(int timbre) {
    for (int v = 0; v < DX7_MAX_VOICES; v++) {
        Dx7Voice& voice = voices[v];
        if (voice.timbre == timbre && !voice.released) {
            voice.heldByPedal = false;
            voice.released = true;
            voice.note.keyup();
        }
    }
}

void Dx7Engine::allNoteOffQuick() {
    for (int v = 0; v < DX7_MAX_VOICES; v++) {
        Dx7Voice& voice = voices[v];
        if (voice.timbre >= 0 && voice.fadeBlocks < 0) {
            voice.heldByPedal = false;
            voice.released = true;
            voice.note.keyup();
            voice.fadeBlocks = DX7_FADE_BLOCKS;
        }
    }
}

void Dx7Engine::allSoundOff() {
    for (int v = 0; v < DX7_MAX_VOICES; v++) {
        freeVoice(v);
    }
}

void Dx7Engine::allSoundOff(int timbre) {
    for (int v = 0; v < DX7_MAX_VOICES; v++) {
        if (voices[v].timbre == timbre) {
            freeVoice(v);
        }
    }
}

bool Dx7Engine::isPlaying() {
    for (int v = 0; v < DX7_MAX_VOICES; v++) {
        if (voices[v].timbre >= 0) {
            return true;
        }
    }
    return false;
}

void Dx7Engine::setPitchBend(int timbre, int value14) {
    timbres[timbre].controllers.pitchBend = value14;
}

void Dx7Engine::setModWheel(int timbre, int value7) {
    timbres[timbre].controllers.modwheel_cc = value7;
    timbres[timbre].controllers.refresh();
}

int Dx7Engine::render(int timbre, float* stereoBlock) {
    Dx7TimbreState& ts = timbres[timbre];
    if (!ts.active) {
        return 0;
    }
    int playing = 0;
    // One LFO per timbre, advanced once per block
    int32_t lfoVal = ts.lfo.getsample();
    int32_t lfoDelay = ts.lfo.getdelay();

    for (int v = 0; v < DX7_MAX_VOICES; v++) {
        Dx7Voice& voice = voices[v];
        if (voice.timbre != timbre) {
            continue;
        }
        playing++;

        for (int k = 0; k < msfa::N; k++) {
            renderBuf[k] = 0;
        }
        voice.note.compute(renderBuf, lfoVal, lfoDelay, &ts.controllers);

        // Gain over the block : constant, or a linear ramp while fading out
        float g0 = outputScale;
        float g1 = outputScale;
        bool fading = voice.fadeBlocks >= 0;
        bool fadeFinished = false;
        if (fading) {
            const float inv = 1.0f / DX7_FADE_BLOCKS;
            g0 = outputScale * voice.fadeBlocks * inv;
            voice.fadeBlocks--;
            if (voice.fadeBlocks < 0) {
                // this block starts at 0 : the voice is done
                g1 = 0.0f;
                fadeFinished = true;
            } else {
                g1 = outputScale * voice.fadeBlocks * inv;
            }
        }
        float g = g0;
        float dg = (g1 - g0) * (1.0f / msfa::N);
        float* sp = stereoBlock;
        for (int k = 0; k < msfa::N; k++) {
            float s = (float)renderBuf[k] * g;
            *sp++ += s;
            *sp++ += s;
            g += dg;
        }

        if (fadeFinished) {
            freeVoice(v);
        } else if (!fading && voice.released && voice.note.carriersSilent()) {
            freeVoice(v);
        }
    }
    return playing;
}
