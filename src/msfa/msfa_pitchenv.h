/*
 * Copyright 2013 Google Inc.
 * Licensed under the Apache License, Version 2.0. See msfa_synth.h.
 */
#ifndef MSFA_PITCHENV_H
#define MSFA_PITCHENV_H

#include <stdint.h>

namespace msfa {

// Computation of the DX7 pitch envelope
class PitchEnv {
 public:
  static void init(double sample_rate);
  // The rates and levels arrays are calibrated to match the Dx7 parameters
  // (ie, value 0..99).
  void set(const int rates[4], const int levels[4]);
  // Result is in Q24/octave
  int32_t getsample();
  void keydown(bool down);
  void getPosition(char *step);
 private:
  static int unit_;
  int rates_[4];
  int levels_[4];
  int32_t level_;
  int targetlevel_;
  bool rising_;
  int ix_;
  int inc_;
  bool down_;

  void advance(int newix);
};

}

#endif  // MSFA_PITCHENV_H
