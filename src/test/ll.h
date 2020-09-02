#pragma once

#include "../log.h"

struct test_ll_a {
    double a, b; 
    int res_dsc, res_dsc_abs, res_dsc_nan;
};

bool test_ll_generator_a(void *, size_t *, struct log *);
unsigned test_ll_a_1(void *, void *, void *);
unsigned test_ll_a_2(void *, void *, void *);
unsigned test_ll_a_3(void *, void *, void *);
#define test_ll_dispose_a test_dispose

struct test_ll_b { 
    uint32_t a, res_bsf, res_bsr; 
};

bool test_ll_generator_b(void *, size_t *, struct log *);
unsigned test_ll_b(void *, void *, void *);
#define test_ll_dispose_b NULL

void test_ll_perf(struct log *);