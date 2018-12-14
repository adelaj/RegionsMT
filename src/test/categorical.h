#pragma once

#include "../log.h"

struct test_categorical_a {
    size_t *tbl, dimx, dimy, cnt;
    double nlpv, qas;
};

#define REL_ERROR 1e-6

bool test_categorical_generator_a(void *, size_t *, struct log *);
void test_categorical_disposer_a(void *);
bool test_categorical_a(void *, struct log *);
