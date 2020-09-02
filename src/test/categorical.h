#pragma once

#include "../log.h"

struct test_categorical_a {
    double pv, qas;
    size_t dimx, dimy, cnt, tbl[];
};

#define REL_ERROR 1e-6

bool test_categorical_generator_a(void *, size_t *, struct log *);
unsigned test_categorical_a(void *, void *, void *);
#define test_categorical_dispose_a test_dispose
