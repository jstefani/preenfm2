/*
 * Copyright 2012 Google Inc.
 * Licensed under the Apache License, Version 2.0. See msfa_synth.h.
 */
#include <math.h>
#include "msfa_freqlut.h"

namespace msfa {

#define FREQ_LG_N_SAMPLES 10
#define FREQ_N_SAMPLES (1 << FREQ_LG_N_SAMPLES)
#define FREQ_SAMPLE_SHIFT (24 - FREQ_LG_N_SAMPLES)
#define MAX_LOGFREQ_INT 20

// Depends on the sample rate, so computed at boot (4 KB of RAM).
static int32_t lut[FREQ_N_SAMPLES + 1];

void Freqlut::init(double sample_rate) {
  double y = (1LL << (24 + MAX_LOGFREQ_INT)) / sample_rate;
  double inc = pow(2, 1.0 / FREQ_N_SAMPLES);
  for (int i = 0; i < FREQ_N_SAMPLES + 1; i++) {
    lut[i] = (int32_t)floor(y + 0.5);
    y *= inc;
  }
}

// Note: if logfreq is more than 20.0, the results will be inaccurate. However,
// that will be many times the Nyquist rate.
int32_t Freqlut::lookup(int32_t logfreq) {
  int ix = (logfreq & 0xffffff) >> FREQ_SAMPLE_SHIFT;
  int32_t y0 = lut[ix];
  int32_t y1 = lut[ix + 1];
  int lowbits = logfreq & ((1 << FREQ_SAMPLE_SHIFT) - 1);
  int32_t y = y0 + ((((int64_t)(y1 - y0) * (int64_t)lowbits)) >> FREQ_SAMPLE_SHIFT);
  int hibits = logfreq >> 24;
  return y >> (MAX_LOGFREQ_INT - hibits);
}

}
