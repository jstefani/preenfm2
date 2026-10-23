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
#include "DX7Random.h"

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
    // The timbre block is at "voice level" : one preenfm2 voice at full
    // amplitude is about +-1 and Timbre::fxAfterBlock applies the mixer gain
    // (which already divides by the number of voices) to reach DAC level.
    // Dexed treats 2^28 as full scale; 2^27 makes a single carrier at level
    // 99 about -12 dB, in line with the native engine.
    outputScale = 1.0f / 134217728.0f;
}

void Dx7Engine::setTimbrePatch(int timbre, const uint8_t* packedPatch) {
    uint8_t unpacked[DX7_UNPACKED_PATCH_SIZE];
    dx7UnpackPatch(packedPatch, unpacked);
    setTimbrePatchUnpacked(timbre, unpacked);
}

void Dx7Engine::setTimbrePatchUnpacked(int timbre, const uint8_t* unpackedVoice) {
    allSoundOff(timbre);
    Dx7TimbreState& ts = timbres[timbre];
    for (int k = 0; k < DX7_UNPACKED_PATCH_SIZE; k++) {
        ts.patch[k] = unpackedVoice[k];
    }
    // LFO parameters start at 137 : speed, delay, pmd, amd, sync, wave
    ts.lfo.reset(&ts.patch[137]);
    ts.controllers.refresh();
    ts.active = true;
}

void Dx7Engine::setParam(int timbre, int index, uint8_t value) {
    if (index < 0 || index >= DX7_UNPACKED_PATCH_SIZE) {
        return;
    }
    Dx7TimbreState& ts = timbres[timbre];
    ts.patch[index] = value;
    if (index >= 137 && index <= 143) {
        ts.lfo.reset(&ts.patch[137]);
    }
    // Playing notes follow the edit, as on Dexed
    for (int v = 0; v < DX7_MAX_VOICES; v++) {
        Dx7Voice& voice = voices[v];
        if (voice.timbre == timbre && voice.fadeBlocks < 0) {
            voice.note.update(ts.patch, transposedNote(timbre, voice.midiNote), voice.velocity);
        }
    }
}

int Dx7Engine::transposedNote(int timbre, int midiNote) {
    // VCED 144 : transpose, 24 = C3 = none
    int note = midiNote + (int)timbres[timbre].patch[144] - 24;
    if (note < 0) {
        note = 0;
    } else if (note > 127) {
        note = 127;
    }
    return note;
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
    voice.velocity = velocity;
    voice.released = false;
    voice.heldByPedal = false;
    voice.fadeBlocks = -1;
    voice.index = voiceIndex++;
    ts.lfo.keydown();
    voice.note.init(ts.patch, transposedNote(timbre, note), velocity);
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
