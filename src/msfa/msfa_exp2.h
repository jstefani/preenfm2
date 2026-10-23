/*
 * Copyright 2012 Google Inc.
 * Licensed under the Apache License, Version 2.0. See msfa_synth.h.
 */
#ifndef MSFA_EXP2_H
#define MSFA_EXP2_H

#include <stdint.h>

namespace msfa {

#define MSFA_EXP2_LG_N_SAMPLES 10
#define MSFA_EXP2_N_SAMPLES (1 << MSFA_EXP2_LG_N_SAMPLES)

// Precomputed (delta, value) pairs. Lives in flash.
extern const int32_t exp2tab[MSFA_EXP2_N_SAMPLES << 1];

class Exp2 {
 public:
  // Q24 in, Q24 out
  static inline int32_t lookup(int32_t x) {
    const int SHIFT = 24 - MSFA_EXP2_LG_N_SAMPLES;
    int lowbits = x & ((1 << SHIFT) - 1);
    int x_int = (x >> (SHIFT - 1)) & ((MSFA_EXP2_N_SAMPLES - 1) << 1);
    int dy = exp2tab[x_int];
    int y0 = exp2tab[x_int + 1];
    int y = y0 + (((int64_t)dy * (int64_t)lowbits) >> SHIFT);
    return y >> (6 - (x >> 24));
  }
};

}

#endif  // MSFA_EXP2_H
