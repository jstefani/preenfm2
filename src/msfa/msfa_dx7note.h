/*
 * Copyright 2016-2017 Pascal Gauthier.
 * Copyright 2012 Google Inc.
 * Licensed under the Apache License, Version 2.0. See msfa_synth.h.
 */
#ifndef MSFA_DX7NOTE_H
#define MSFA_DX7NOTE_H

// This is the logic to put together a note from the MIDI description
// and run the low-level modules.

#include "msfa_env.h"
#include "msfa_pitchenv.h"
#include "msfa_fm_core.h"
#include "msfa_controllers.h"

namespace msfa {

class Dx7Note {
public:
    Dx7Note();
    void init(const uint8_t patch[156], int midinote, int velocity);

    // Note: this _adds_ to the buffer.
    void compute(int32_t *buf, int32_t lfo_val, int32_t lfo_delay,
                 const Controllers *ctrls);

    void keyup();

    void update(const uint8_t patch[156], int midinote, int velocity);

    void transferState(Dx7Note& src);
    void transferSignal(Dx7Note &src);
    void oscSync();

    // true when every carrier of the note is below the audible gain
    // threshold used by FmCore (the note no longer produces sound)
    bool carriersSilent() const;

private:
    Env env_[6];
    FmOpParams params_[6];
    PitchEnv pitchenv_;
    int32_t basepitch_[6];
    int32_t fb_buf_[2];
    int32_t fb_shift_;
    int32_t ampmodsens_[6];
    int32_t opMode[6];

    int ampmoddepth_;
    int algorithm_;
    int pitchmoddepth_;
    int pitchmodsens_;
};

}

#endif  // MSFA_DX7NOTE_H
