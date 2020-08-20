#pragma once

#include "log.h"

typedef bool (*test_generator_callback)(void *, size_t *, struct log *);
typedef void (*test_dispose_callback)(void *);
typedef bool (*test_callback)(void *, struct log *);

struct test_group {
    const struct strl name;
    test_dispose_callback dispose;
    struct {
        test_generator_callback *generator;
        size_t generator_cnt;
    };
    struct {
        test_callback *test;
        size_t test_cnt;
    };
};

void test_dispose(void *);
bool test(const struct test_group *, size_t, size_t, struct log *);