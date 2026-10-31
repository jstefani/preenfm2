/*
 * DX7 engine for the preenfm2, built on the msfa engine (see src/msfa).
 *
 * A timbre can run either the native preenfm2 FM engine or this one.
 * Loading a DX7 sysex patch (with "DX7 Engine" set to Native in the
 * settings) switches the timbre to this engine; loading a preenfm2 patch
 * switches it back. The timbre's per-block effect, gain and pan stage
 * still apply to the output.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef DX7ENGINE_H_
#define DX7ENGINE_H_

#include <stdint.h>
#include "msfa_dx7note.h"
#include "msfa_lfo.h"
#include "msfa_controllers.h"
#include "msfa_fm_core.h"

// Shared by all timbres. Each voice is a full 6 operator DX7 voice, which
// costs more CPU than a preenfm2 voice, so the pool is smaller.
#define DX7_MAX_VOICES 8
#define DX7_NUMBER_OF_TIMBRES 4
#define DX7_PACKED_PATCH_SIZE 128
#define DX7_UNPACKED_PATCH_SIZE 156

struct Dx7Voice {
    msfa::Dx7Note note;
    int8_t timbre;          // -1 = free
    uint8_t midiNote;
    uint8_t velocity;
    bool released;
    bool heldByPedal;
    uint32_t index;         // allocation order, lowest = oldest
};

struct Dx7TimbreState {
    uint8_t patch[DX7_UNPACKED_PATCH_SIZE];
    msfa::Lfo lfo;
    msfa::Controllers controllers;
    bool active;
    bool holdPedal;
};

class Dx7Engine {
public:
    void init(float sampleRate);

    // Patch management (called from the main loop, never from the audio IRQ)
    void setTimbrePatch(int timbre, const uint8_t* packedPatch);
    void setTimbrePatchUnpacked(int timbre, const uint8_t* unpackedVoice);
    // Live edit of one byte of the timbre's voice; playing notes follow
    void setParam(int timbre, int index, uint8_t value);
    void deactivateTimbre(int timbre);
    bool isTimbreActive(int timbre) { return timbres[timbre].active; }

    // Notes
    void noteOn(int timbre, uint8_t note, uint8_t velocity, int maxVoices);
    void noteOff(int timbre, uint8_t note);
    void setHoldPedal(int timbre, bool hold);
    void allNoteOff(int timbre);
    void allSoundOff();
    void allSoundOff(int timbre);
    bool isPlaying();

    // Controllers
    void setPitchBend(int timbre, int value14);
    void setModWheel(int timbre, int value7);

    // Audio. Adds the timbre's voices into its stereo float block, at the
    // same level as the preenfm2 voices (the timbre gain stage follows).
    void setOutputScale(float scale) { outputScale = scale; }
    int render(int timbre, float* stereoBlock);

private:
    int transposedNote(int timbre, int midiNote);
    int allocateVoice(int timbre, int maxVoices);
    void freeVoice(int v);

    msfa::FmCore core;
    Dx7Voice voices[DX7_MAX_VOICES];
    Dx7TimbreState timbres[DX7_NUMBER_OF_TIMBRES];
    uint32_t voiceIndex;
    float outputScale;
    int32_t renderBuf[msfa::N];
};

// Single instance, lives in main RAM (CCM is full)
extern Dx7Engine dx7Engine;

#endif /* DX7ENGINE_H_ */
