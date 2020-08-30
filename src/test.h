#pragma once

#include "log.h"

typedef bool (*test_generator_callback)(void *, size_t *, struct log *);
typedef void (*test_dispose_callback)(void *);
typedef bool (*test_callback)(void *, struct log *);

#define FSTRL(F) { F, STRI(#F) } 
#define TEST_GENERATOR_THRESHOLD 255

struct test_group {
    test_dispose_callback dispose;
    struct {
        struct test_generator {
            test_generator_callback callback;
            struct strl name;
        } *generator;
        size_t generator_cnt;
    };
    struct {
        struct test {
            test_callback callback;
            struct strl name;
        } *test;
        size_t test_cnt;
    };
};

void test_dispose(void *);
bool test(const struct test_group *, size_t, size_t, struct log *);