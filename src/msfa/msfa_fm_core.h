/*
 * Copyright 2012 Google Inc.
 * Licensed under the Apache License, Version 2.0. See msfa_synth.h.
 */
#ifndef MSFA_FM_CORE_H
#define MSFA_FM_CORE_H

#include "msfa_synth.h"
#include "msfa_fm_op_kernel.h"

namespace msfa {

enum FmOperatorFlags {
    OUT_BUS_ONE = 1 << 0,
    OUT_BUS_TWO = 1 << 1,
    OUT_BUS_ADD = 1 << 2,
    IN_BUS_ONE = 1 << 4,
    IN_BUS_TWO = 1 << 5,
    FB_IN = 1 << 6,
    FB_OUT = 1 << 7
};

struct FmAlgorithm {
    int ops[6];
};

class FmCore {
public:
    void render(int32_t *output, FmOpParams *params, int algorithm, int32_t *fb_buf, int feedback_shift);
    // true if operator 'op' of 'algorithm' writes to the output bus (a carrier)
    static bool isCarrier(int algorithm, int op) {
        return (algorithms[algorithm & 31].ops[op] & 7) == OUT_BUS_ADD;
    }
protected:
    int32_t buf_[2][N] __attribute__ ((aligned (16)));
    const static FmAlgorithm algorithms[32];
};

}

#endif  // MSFA_FM_CORE_H
