/*
 * Copyright 2013 Google Inc.
 * Licensed under the Apache License, Version 2.0. See msfa_synth.h.
 *
 * Reduced to what the preenfm2 drives : pitch bend and the mod wheel.
 */
#ifndef MSFA_CONTROLLERS_H
#define MSFA_CONTROLLERS_H

#include "msfa_synth.h"

namespace msfa {

class FmCore;

struct FmMod {
    int range;      // 0..99 (percent of full modulation)
    bool pitch;
    bool amp;
    bool eg;
};

struct Controllers {
    int pitchBend;      // 14 bit, 8192 = centre
    int pitchRange;     // semitones
    int pitchStep;      // 0 = continuous
    int masterTune;     // Q24 octaves

    int modwheel_cc;    // 0..127
    FmMod wheel;

    int amp_mod;
    int pitch_mod;
    int eg_mod;

    char opSwitch[7];

    FmCore *core;

    void init(FmCore *fmCore) {
        pitchBend = 8192;
        pitchRange = 2;
        pitchStep = 0;
        masterTune = 0;
        modwheel_cc = 0;
        // DX7 factory-style default : mod wheel = vibrato
        wheel.range = 50;
        wheel.pitch = true;
        wheel.amp = false;
        wheel.eg = false;
        for (int k = 0; k < 6; k++) {
            opSwitch[k] = '1';
        }
        opSwitch[6] = 0;
        core = fmCore;
        refresh();
    }

    void refresh() {
        amp_mod = 0;
        pitch_mod = 0;
        eg_mod = 0;
        int total = (modwheel_cc * wheel.range) / 100;
        if (wheel.amp) {
            amp_mod = total;
        }
        if (wheel.pitch) {
            pitch_mod = total;
        }
        if (wheel.eg) {
            eg_mod = total;
        } else {
            eg_mod = 127;
        }
    }
};

}

#endif  // MSFA_CONTROLLERS_H
