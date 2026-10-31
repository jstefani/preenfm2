/*
 * Edit rows for a timbre running the DX7 engine. See Dx7Params.h.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Dx7Params.h"

static const char* const onOffNames[] = { "Off ", "On  " };
static const char* const modeNames[] = { "Rati", "Fixd" };
static const char* const curveNames[] = { "-Lin", "-Exp", "+Exp", "+Lin" };
static const char* const waveNames[] = { "Tri ", "SawD", "SawU", "Squa", "Sine", "S&H " };

static const Dx7RowDef globalRows[DX7_GLOBAL_ROWS] = {
    { "Engine", {
        { "Algo", 134, 31, 0, 1 },
        { "Fdbk", 135, 7, 0, 0 },
        { "Sync", 136, 1, onOffNames, 0 },
        { "Voic", DX7_INDEX_VOICES, 0, 0, 0 } } },
    { "PitchEG Rate", {
        { "R1  ", 126, 99, 0, 0 },
        { "R2  ", 127, 99, 0, 0 },
        { "R3  ", 128, 99, 0, 0 },
        { "R4  ", 129, 99, 0, 0 } } },
    { "PitchEG Lvl", {
        { "L1  ", 130, 99, 0, 0 },
        { "L2  ", 131, 99, 0, 0 },
        { "L3  ", 132, 99, 0, 0 },
        { "L4  ", 133, 99, 0, 0 } } },
    { "LFO", {
        { "Sped", 137, 99, 0, 0 },
        { "Dely", 138, 99, 0, 0 },
        { "PMD ", 139, 99, 0, 0 },
        { "AMD ", 140, 99, 0, 0 } } },
    { "LFO 2", {
        { "Sync", 141, 1, onOffNames, 0 },
        { "Wave", 142, 5, waveNames, 0 },
        { "PMS ", 143, 7, 0, 0 },
        { "Trsp", 144, 48, 0, -24 } } },
};

// index = offset inside the operator's 21 byte block
static const Dx7RowDef operatorRows[DX7_OPERATOR_VIEWS] = {
    { "Freq", {
        { "Mode", 17, 1, modeNames, 0 },
        { "Cors", 18, 31, 0, 0 },
        { "Fine", 19, 99, 0, 0 },
        { "Detu", 20, 14, 0, -7 } } },
    { "Level", {
        { "OutL", 16, 99, 0, 0 },
        { "Velo", 15, 7, 0, 0 },
        { "AMS ", 14, 3, 0, 0 },
        { "RScl", 13, 7, 0, 0 } } },
    { "Rates", {
        { "R1  ", 0, 99, 0, 0 },
        { "R2  ", 1, 99, 0, 0 },
        { "R3  ", 2, 99, 0, 0 },
        { "R4  ", 3, 99, 0, 0 } } },
    { "Levels", {
        { "L1  ", 4, 99, 0, 0 },
        { "L2  ", 5, 99, 0, 0 },
        { "L3  ", 6, 99, 0, 0 },
        { "L4  ", 7, 99, 0, 0 } } },
    { "KeyScale", {
        { "BrkP", 8, 99, 0, 0 },
        { "LDep", 9, 99, 0, 0 },
        { "RDep", 10, 99, 0, 0 },
        { "    ", DX7_INDEX_NONE, 0, 0, 0 } } },
    { "Curves", {
        { "LCrv", 11, 3, curveNames, 0 },
        { "RCrv", 12, 3, curveNames, 0 },
        { "    ", DX7_INDEX_NONE, 0, 0, 0 },
        { "    ", DX7_INDEX_NONE, 0, 0, 0 } } },
};

bool dx7RowIsOperator(int row) {
    return row >= DX7_GLOBAL_ROWS && row < DX7_ROWS;
}

int dx7RowOperator(int row) {
    return (row - DX7_GLOBAL_ROWS) / DX7_OPERATOR_VIEWS;
}

int dx7RowView(int row) {
    return (row - DX7_GLOBAL_ROWS) % DX7_OPERATOR_VIEWS;
}

int dx7OperatorRow(int op, int view) {
    return DX7_GLOBAL_ROWS + op * DX7_OPERATOR_VIEWS + view;
}

const Dx7RowDef* dx7RowDef(int row) {
    if (dx7RowIsOperator(row)) {
        return &operatorRows[dx7RowView(row)];
    }
    if (row < 0 || row >= DX7_GLOBAL_ROWS) {
        row = 0;
    }
    return &globalRows[row];
}

int dx7EncoderIndex(int row, int encoder) {
    const Dx7RowDef* def = dx7RowDef(row);
    int index = def->encoders[encoder].index;
    if (index < 0) {
        return index;
    }
    if (dx7RowIsOperator(row)) {
        // VCED stores operator 6 first : DX7 operator N is slot 6 - N
        int slot = 5 - dx7RowOperator(row);
        return slot * 21 + index;
    }
    return index;
}
