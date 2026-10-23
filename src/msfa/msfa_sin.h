/*
 * Copyright 2012 Google Inc.
 * Licensed under the Apache License, Version 2.0. See msfa_synth.h.
 */
#ifndef MSFA_SIN_H
#define MSFA_SIN_H

#include <stdint.h>

namespace msfa {

#define MSFA_SIN_LG_N_SAMPLES 10
#define MSFA_SIN_N_SAMPLES (1 << MSFA_SIN_LG_N_SAMPLES)

// Precomputed, "SIN_DELTA" layout : (delta, value) pairs. Lives in flash.
extern const int32_t sintab[MSFA_SIN_N_SAMPLES << 1];

class Sin {
 public:
  static inline int32_t lookup(int32_t phase) {
    const int SHIFT = 24 - MSFA_SIN_LG_N_SAMPLES;
    int lowbits = phase & ((1 << SHIFT) - 1);
    int phase_int = (phase >> (SHIFT - 1)) & ((MSFA_SIN_N_SAMPLES - 1) << 1);
    int dy = sintab[phase_int];
    int y0 = sintab[phase_int + 1];
    return y0 + (((int64_t)dy * (int64_t)lowbits) >> SHIFT);
  }
};

}

#endif  // MSFA_SIN_H
