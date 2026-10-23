/*
 * Copyright 2013 Google Inc.
 * Licensed under the Apache License, Version 2.0. See msfa_synth.h.
 */
#ifndef MSFA_LFO_H
#define MSFA_LFO_H

#include <stdint.h>

namespace msfa {

// Low frequency oscillator, compatible with DX7
class Lfo {
 public:
  static void init(double sample_rate);
  void reset(const uint8_t params[6]);

  // result is 0..1 in Q24
  int32_t getsample();

  // result is 0..1 in Q24
  int32_t getdelay();

  void keydown();
 private:
  static uint32_t unit_;

  uint32_t phase_;  // Q32
  uint32_t delta_;
  uint8_t waveform_;
  uint8_t randstate_;
  bool sync_;

  uint32_t delaystate_;
  uint32_t delayinc_;
  uint32_t delayinc2_;
};

}

#endif  // MSFA_LFO_H
