/*
 * Copyright 2012 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MSFA_SYNTH_H
#define MSFA_SYNTH_H

#include <stdint.h>

namespace msfa {

// Render block size. Must match the preenfm2 BLOCK_SIZE (32).
static const int LG_N = 5;
static const int N = 1 << LG_N;

template<typename T>
inline T msfaMin(const T& a, const T& b) { return a < b ? a : b; }

template<typename T>
inline T msfaMax(const T& a, const T& b) { return a > b ? a : b; }

}

#endif  // MSFA_SYNTH_H
