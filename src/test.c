#include "np.h"
#include "ll.h"
#include "log.h"
#include "memory.h"
#include "threadpool.h"
#include "utf8.h"
#include "test.h"

#include <stdlib.h>

void test_dispose(void *ptr)
{
    free(ptr);
}

struct test_thread_context {
    const struct test_group *groupl;
    volatile size_t succ, fail, tot;
};

struct test_metric {
    size_t group, test, generator, instance;
    struct strl test_name, generator_name;
};

static struct message_result fmt_test_metric(char *buff, size_t *p_cnt, void *p_arg, const struct env *env, enum fmt_execute_flags flags)
{
    (void) env;
    struct style *style = ARG_FETCH_PTR(flags & FMT_EXE_FLAG_PTR, p_arg);
    struct test_metric *metric = ARG_FETCH_PTR(flags & FMT_EXE_FLAG_PTR, p_arg);
    if (flags & FMT_EXE_FLAG_PHONY) return MESSAGE_SUCCESS;
    return style ?
        print_fmt(buff, p_cnt, NULL, "%~~uz/%~~uz:%''~~s*/%~~uz:%''~~s*/%~~uz", 
            &style->type_int, metric->group, 
            &style->type_int, metric->test, &style->type_str, STRL(metric->test_name),
            &style->type_int, metric->generator, &style->type_str, STRL(metric->generator_name),
            &style->type_int, metric->instance) :
        print_fmt(buff, p_cnt, NULL, "%uz/%uz:%''s*/%uz:%''s*/%uz", 
            metric->group, 
            metric->test, STRL(metric->test_name),
            metric->generator, STRL(metric->generator_name), 
            metric->instance);
}

struct test_thread_storage {
    uint64_t start;
    void *data;
    struct test_context context;
};

static unsigned test_thread(void *Indl, void *Context, void *Tls)
{
    // w -- group index; x -- test index; y -- generator index; z -- generator instance index
    size_t *indl = Indl, w = indl[0], x = indl[1], y = indl[2], z = indl[3];
    struct test_thread_context *context = Context;
    struct test_tls *tls = Tls;
    struct test_metric metric = { .group = w + 1, .test = x + 1, .test_name = context->groupl[w].test[x].name, .generator = y + 1, .generator_name = context->groupl[w].generator[y].name, .instance = z + 1 };
    unsigned res = 0;
    if (!tls->base.storage)
    {
        if (!array_assert(&tls->log, CODE_METRIC, array_init(&tls->base.storage, NULL, 1, sizeof(struct test_thread_storage), 0, ARRAY_STRICT))) return 0;
        *(struct test_thread_storage *) tls->base.storage = (struct test_thread_storage) { 0 };
    }
    struct test_thread_storage *storage = tls->base.storage;
    if (!storage->data)
    {
        if (!context->groupl[w].generator[y].callback(&storage->data, &z, &tls->log))
        {
            if (tls->base.exec < GENERATOR_THRESHOLD)
            {
                log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_NOTE, "Generator for the test %& reported a failure and will be retriggered elsewhen. Remaining attempts: %~uz.\n", fmt_test_metric, tls->log.style, &metric, GENERATOR_THRESHOLD - tls->base.exec);
                return TASK_YIELD;
            }
            else log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_ERROR, "Number of attempts per generator of the test %& exhausted!\n", fmt_test_metric, tls->log.style, &metric);
        }
        else if (tls->base.exec > 1) log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_NOTE, "Generator for the test %& succeeded after %~uz attempts.\n", fmt_test_metric, tls->log.style, &metric, tls->base.exec);
    }
    if (storage->data) // Do not place 'else' here!
    {
        if (!storage->context.exec) storage->start = get_time();
        storage->context.exec = size_add_sat(storage->context.exec, 1);
        if (storage->context.exec == TEST_THRESHOLD) storage->context.no_retry = 1;
        res = context->groupl[w].test[x].callback(storage->data, &storage->context, tls);
        switch (res)
        {
        case TEST_FAIL:
            log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_ERROR, "Test %& failed!\n", fmt_test_metric, tls->log.style, &metric);
            break;
        case TEST_SUCCESS:
            log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_INFO, "Execution of the test %& took %~T.%?$\n", fmt_test_metric, tls->log.style, &metric, storage->start, get_time(), storage->context.exec > 1, " Attempts made: %~uz.", storage->context.exec);
            break;
        case TEST_YIELD:
            return TASK_YIELD;
        case TEST_RETRY:
            if (storage->context.exec < TEST_THRESHOLD)
            {
                log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_NOTE, "Test %& reported failure and will be retriggered elsewhen. Remaining attempts: %~uz.\n", fmt_test_metric, tls->log.style, &metric, TEST_THRESHOLD - storage->context.exec);
                return TASK_YIELD;
            }
            else log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_ERROR, "Number of attempts per test %& exhausted!\n", fmt_test_metric, tls->log.style, &metric);
            // break;
        default:
            res = 0;
        }
    }
    if (context->groupl[w].dispose) context->groupl[w].dispose(storage->data);
    free(storage);
    size_interlocked_add_sat((volatile void *[]) { &context->fail, &context->succ }[res], 1);
    return res;
}

static unsigned group_thread(void *Indl, void *Context, void *Tls)
{
    // w -- group index; x -- test index; y -- generator index
    size_t *indl = Indl, w = indl[0], x = indl[1], y = indl[2];
    struct test_thread_context *context = Context;
    struct test_tls *tls = Tls;
    size_t cnt = 1;
    for (size_t i = 0; context->groupl[w].generator[y].callback(NULL, &i, &tls->log), i; cnt++);
    size_interlocked_add_sat(&context->tot, cnt);
    return loop_mt(tls->base.pool, test_thread, (struct task_cond) { 0 }, (struct task_aggr) { 0 }, Context, ARG(size_t, 1, 1, 1, cnt), (size_t []) { w, x, y, 0 }, 0, &tls->log);
}

bool test(const struct test_group *groupl, size_t cnt, size_t thread_cnt, struct log *log)
{
    struct thread_pool *pool = thread_pool_create(thread_cnt, sizeof(struct test_tls), 0, log);
    if (!pool) return 0;
    size_t ind = 0;
    for (; ind < thread_cnt; ind++)
        if (!log_dup(&((struct test_tls *) thread_pool_fetch_tls(pool, ind))->log, log)) break;
    bool succ = 0;
    struct test_thread_context context;
    context.groupl = groupl;
    size_store_release(&context.succ, 0);
    size_store_release(&context.fail, 0);
    size_store_release(&context.tot, 0);
    if (ind == thread_cnt)
    {
        size_t i = 0;
        for (; i < cnt; i++)
            if (!loop_mt(pool, group_thread, (struct task_cond) { 0 }, (struct task_aggr) { 0 }, &context, ARG(size_t, 1, groupl[i].test_cnt, groupl[i].generator_cnt), (size_t []) { i, 0, 0 }, 0, log)) break;
        succ = i == cnt;
        uint64_t start = get_time();
        thread_pool_schedule(pool);
        size_t fail = size_load_acquire(&context.fail);
        log_message_fmt(log, CODE_METRIC, fail ? MESSAGE_ERROR : MESSAGE_INFO, "Execution of %~uz test(s) in %~uz thread(s) took %~T. Succeeded: %~uz, failed: %~uz.\n", size_load_acquire(&context.tot), thread_cnt, start, get_time(), size_load_acquire(&context.succ), fail);
        succ = !fail;
    }
    while (ind--) log_close(&((struct test_tls *) thread_pool_fetch_tls(pool, ind))->log);
    thread_pool_dispose(pool);
    return succ;
}

bool perf(struct log *log)
{
    uint64_t start = get_time();

    // Performance tests here

    log_message_fmt(log, CODE_METRIC, MESSAGE_INFO, "Performance tests execution took %~T.\n", start, get_time());
    return 1;
}
