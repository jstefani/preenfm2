/*
 * Copyright 2012 Google Inc.
 * Licensed under the Apache License, Version 2.0. See msfa_synth.h.
 */
#include "msfa_synth.h"
#include "msfa_sin.h"
#include "msfa_fm_op_kernel.h"

namespace msfa {

void FmOpKernel::compute(int32_t *output, const int32_t *input,
                         int32_t phase0, int32_t freq,
                         int32_t gain1, int32_t gain2, bool add) {
  int32_t dgain = (gain2 - gain1 + (N >> 1)) >> LG_N;
  int32_t gain = gain1;
  int32_t phase = phase0;
  if (add) {
    for (int i = 0; i < N; i++) {
      gain += dgain;
      int32_t y = Sin::lookup(phase + input[i]);
      int32_t y1 = ((int64_t)y * (int64_t)gain) >> 24;
      output[i] += y1;
      phase += freq;
    }
  } else {
    for (int i = 0; i < N; i++) {
      gain += dgain;
      int32_t y = Sin::lookup(phase + input[i]);
      int32_t y1 = ((int64_t)y * (int64_t)gain) >> 24;
      output[i] = y1;
      phase += freq;
    }
  }
}

void FmOpKernel::compute_pure(int32_t *output, int32_t phase0, int32_t freq,
                              int32_t gain1, int32_t gain2, bool add) {
  int32_t dgain = (gain2 - gain1 + (N >> 1)) >> LG_N;
  int32_t gain = gain1;
  int32_t phase = phase0;
  if (add) {
    for (int i = 0; i < N; i++) {
      gain += dgain;
      int32_t y = Sin::lookup(phase);
      int32_t y1 = ((int64_t)y * (int64_t)gain) >> 24;
      output[i] += y1;
      phase += freq;
    }
  } else {
    for (int i = 0; i < N; i++) {
      gain += dgain;
      int32_t y = Sin::lookup(phase);
      int32_t y1 = ((int64_t)y * (int64_t)gain) >> 24;
      output[i] = y1;
      phase += freq;
    }
  }
}

void FmOpKernel::compute_fb(int32_t *output, int32_t phase0, int32_t freq,
                            int32_t gain1, int32_t gain2,
                            int32_t *fb_buf, int fb_shift, bool add) {
  int32_t dgain = (gain2 - gain1 + (N >> 1)) >> LG_N;
  int32_t gain = gain1;
  int32_t phase = phase0;
  int32_t y0 = fb_buf[0];
  int32_t y = fb_buf[1];
  if (add) {
    for (int i = 0; i < N; i++) {
      gain += dgain;
      int32_t scaled_fb = (y0 + y) >> (fb_shift + 1);
      y0 = y;
      y = Sin::lookup(phase + scaled_fb);
      y = ((int64_t)y * (int64_t)gain) >> 24;
      output[i] += y;
      phase += freq;
    }
  } else {
    for (int i = 0; i < N; i++) {
      gain += dgain;
      int32_t scaled_fb = (y0 + y) >> (fb_shift + 1);
      y0 = y;
      y = Sin::lookup(phase + scaled_fb);
      y = ((int64_t)y * (int64_t)gain) >> 24;
      output[i] = y;
      phase += freq;
    }
  }
  fb_buf[0] = y0;
  fb_buf[1] = y;
}

}
