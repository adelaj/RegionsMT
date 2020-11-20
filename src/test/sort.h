#pragma once

#include "../log.h"

struct test_sort_a {
    size_t cnt, arr[];
};

struct test_sort_b {
    size_t cnt, ucnt;
    double arr[];
};

struct test_sort_c {
    size_t cnt, arr[];
};

struct test_sort_d {
    size_t cnt;
    uint64_t arr[];
};

// Maximal array size is '1 << TEST_SORT_EXP'
#define TEST_SORT_EXP 25

#define TEST_HASH_MAX 1599

bool test_sort_generator_a_1(void *, size_t *, struct log *);
bool test_sort_generator_a_2(void *, size_t *, struct log *);
bool test_sort_generator_a_3(void *, size_t *, struct log *);
unsigned test_sort_a(void *, void *, void *);
#define test_sort_dispose_a test_dispose

bool test_sort_generator_b_1(void *, size_t *, struct log *);
unsigned test_sort_b_1(void *, void *, void *);
unsigned test_sort_b_2(void *, void *, void *);
#define test_sort_dispose_b test_dispose

bool test_sort_generator_c_1(void *, size_t *, struct log *);
unsigned test_sort_c_1(void *, void *, void *);
unsigned test_sort_c_2(void *, void *, void *);
#define test_sort_dispose_c test_dispose

bool test_sort_generator_d_1(void *, size_t *, struct log *);
unsigned test_sort_d_1(void *, void *, void *);
#define test_sort_dispose_d test_dispose

bool test_sort_generator_e_1(void *, size_t *, struct log *);
unsigned test_sort_e_1(void *, void *, void *);
void test_sort_dispose_e(void *);
