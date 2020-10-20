#pragma once

#include "../log.h"

struct test_ll_a { 
    uint32_t a, res_bsf, res_bsr; 
};

bool test_ll_generator_a(void *, size_t *, struct log *);
unsigned test_ll_a(void *, void *, void *);
#define test_ll_dispose_a NULL

void test_ll_perf(struct log *);