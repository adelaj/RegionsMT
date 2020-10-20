#pragma once

#include "../test.h"

struct test_cmp_a {
    double a, b;
    int res_dsc, res_dsc_abs, res_dsc_nan;
};

bool test_cmp_generator_a(void *, size_t *, struct log *);
unsigned test_cmp_a_1(void *, void *, void *);
unsigned test_cmp_a_2(void *, void *, void *);
unsigned test_cmp_a_3(void *, void *, void *);
#define test_cmp_dispose_a test_dispose