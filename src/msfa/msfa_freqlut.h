/*
 * Copyright 2012 Google Inc.
 * Licensed under the Apache License, Version 2.0. See msfa_synth.h.
 */
#ifndef MSFA_FREQLUT_H
#define MSFA_FREQLUT_H

#include <stdint.h>

namespace msfa {

// Resolve frequency signal (1.0 in Q24 format = 1 octave) to phase delta.
class Freqlut {
 public:
  static void init(double sample_rate);
  static int32_t lookup(int32_t logfreq);
};

}

#endif  // MSFA_FREQLUT_H
