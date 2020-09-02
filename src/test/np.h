#pragma once

#include "../log.h"

struct test_np_a {
    size_t cnt;
    char str[];
};

#define TEST_NP_EXP 25

bool test_np_generator_a(void *, size_t *, struct log *);
unsigned test_np_a(void *, void *, void *);
#define test_np_dispose_a test_dispose
