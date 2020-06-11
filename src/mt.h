#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Common structures for multiple testing
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
