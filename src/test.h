#pragma once

#include "log.h"
#include "threadpool.h"

typedef bool (*test_generator_callback)(void *, size_t *, struct log *);
typedef void (*test_dispose_callback)(void *);

#define FSTRL(F) { F, STRI(#F) } 
#define GENERATOR_THRESHOLD 256
#define TEST_THRESHOLD 256

enum {
    TEST_FAIL = TASK_FAIL,
    TEST_SUCCESS = TASK_SUCCESS,
    TEST_YIELD = TASK_YIELD,
    TEST_RETRY = TASK_CNT
};

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
            task_callback callback;
            struct strl name;
        } *test;
        size_t test_cnt;
    };
};

struct test_tls {
    struct tls_base base;
    struct log log;
};

void test_dispose(void *);
bool test(const struct test_group *, size_t, size_t, struct log *);