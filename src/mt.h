#pragma once

#include <stddef.h>
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////
//
//  Common structures and routines for multiple testing
//

enum mt_alt {
    ALT_A = 0,
    ALT_CD,
    ALT_D,
    ALT_R,
    ALT_CNT
};

#define GEN_CNT 3

enum mt_flags {
    TEST_CD = 1 << ALT_CD,
    TEST_D = 1 << ALT_D,
    TEST_R = 1 << ALT_R,
    TEST_A = 1 << ALT_A,
};

struct mt_result {
    double pv[ALT_CNT], qas[ALT_CNT];
};

struct adj_result {
    double pv[ALT_CNT];
};

struct array_result levels_from_phen_impl(size_t *, size_t *, char *buff, size_t *, uint8_t *, uint8_t *);
struct array_result levels_from_phen(size_t **, size_t *, char *, size_t *, uint8_t *, uint8_t *);
