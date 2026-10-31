/*
 * Edit rows for a timbre running the DX7 engine : which byte of the
 * unpacked DX7 voice (VCED, 155 bytes) each encoder of each row edits,
 * its range and how it is shown.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef DX7PARAMS_H_
#define DX7PARAMS_H_

#include <stdint.h>

#define DX7_INDEX_NONE   -1
#define DX7_INDEX_VOICES -2    // preenfm2 number of voices, not part of the DX7 voice

struct Dx7EncoderDef {
    const char* name;                 // 4 characters shown above the encoder
    int16_t index;                    // VCED byte (operator rows : offset in the 21 byte block) or DX7_INDEX_*
    uint8_t maxValue;                 // inclusive, minimum is 0
    const char* const* valueNames;    // 0 : shown as a number
    int8_t displayOffset;             // added to the number shown (detune -7, transpose -24, algo +1)
};

struct Dx7RowDef {
    const char* name;
    Dx7EncoderDef encoders[4];
};

// Global rows first, then 6 views per operator
enum {
    DX7_ROW_ENGINE = 0,
    DX7_ROW_PEG_RATES,
    DX7_ROW_PEG_LEVELS,
    DX7_ROW_LFO1,
    DX7_ROW_LFO2,
    DX7_GLOBAL_ROWS
};
#define DX7_OPERATOR_VIEWS 6
#define DX7_ROWS (DX7_GLOBAL_ROWS + 6 * DX7_OPERATOR_VIEWS)

const Dx7RowDef* dx7RowDef(int row);
bool dx7RowIsOperator(int row);
int dx7RowOperator(int row);               // 0..5 : DX7 operator 1..6
int dx7RowView(int row);                   // 0..DX7_OPERATOR_VIEWS-1
int dx7OperatorRow(int op, int view);
// Absolute VCED byte index edited by (row, encoder), or DX7_INDEX_*
int dx7EncoderIndex(int row, int encoder);

#endif /* DX7PARAMS_H_ */
