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

struct test_tls {
    struct tls_base base;
    struct log log;
};

struct test_context {
    const struct test_group *groupl;
    volatile size_t succ, fail, tot;
};

static unsigned test_thread(void *Indl, void *Context, void *Tls)
{
    // w -- group index; x -- test index; y -- generator index; z -- generator instance index
    size_t *indl = Indl, w = indl[0], x = indl[1], y = indl[2], z = indl[3];
    struct test_context *context = Context;
    struct test_tls *tls = Tls;
    void *data;
    uint64_t start = get_time();
    if (!context->groupl[w].generator[y].callback(&data, &z, &tls->log)) return 0;
    bool succ = context->groupl[w].test[x].callback(data, &tls->log);
    if (!succ) log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_WARNING, "Test %~uz:%''~s*:%''~s*:%~uz failed!\n", w + 1, STRL(context->groupl[w].test[x].name), STRL(context->groupl[w].generator[y].name), z + 1);
    else log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_INFO, "Execution of the test %~uz:%''~s*:%''~s*:%~uz took %~T.\n", w + 1, STRL(context->groupl[w].test[x].name), STRL(context->groupl[w].generator[y].name), z + 1, start, get_time());
    if (context->groupl[w].dispose) context->groupl[w].dispose(data);
    size_interlocked_add_sat((volatile void *[]) { &context->fail, &context->succ }[succ], 1);
    return succ;
}

static unsigned group_thread(void *Indl, void *Context, void *Tls)
{
    // w -- group index; x -- test index; y -- generator index
    size_t *indl = Indl, w = indl[0], x = indl[1], y = indl[2];
    struct test_context *context = Context;
    struct test_tls *tls = Tls;
    size_t cnt = 1;
    for (size_t i = 0; context->groupl[w].generator[y].callback(NULL, &i, &tls->log), i; cnt++);
    size_interlocked_add_sat(&context->tot, cnt);
    return loop_init(tls->base.pool, test_thread, (struct task_cond) { 0 }, (struct task_aggr) { 0 }, Context, ARG(size_t, 1, 1, 1, cnt), (size_t []) { w, x, y, 0 }, 0, &tls->log);
}

bool test(const struct test_group *groupl, size_t cnt, size_t thread_cnt, struct log *log)
{
    struct thread_pool *pool = thread_pool_create(thread_cnt, sizeof(struct test_tls), 0, log);
    if (!pool) return 0;
    size_t ind = 0;
    for (; ind < thread_cnt; ind++)
        if (!log_dup(&((struct test_tls *) thread_pool_fetch_tls(pool, ind))->log, log)) break;
    bool succ = 0;
    struct test_context context;
    context.groupl = groupl;
    size_store_release(&context.succ, 0);
    size_store_release(&context.fail, 0);
    size_store_release(&context.tot, 0);
    if (ind == thread_cnt)
    {
        size_t i = 0;
        for (; i < cnt; i++)
            if (!loop_init(pool, group_thread, (struct task_cond) { 0 }, (struct task_aggr) { 0 }, &context, ARG(size_t, 1, groupl[i].test_cnt, groupl[i].generator_cnt), (size_t []) { i, 0, 0 }, 0, log)) break;
        succ = i == cnt;
        uint64_t start = get_time();
        thread_pool_schedule(pool);
        log_message_fmt(log, CODE_METRIC, MESSAGE_INFO, "Execution of %~uz test(s) in %~uz thread(s) took %~T. Succeeded: %~uz, failed: %~uz.\n", size_load_acquire(&context.tot), thread_cnt, start, get_time(), size_load_acquire(&context.succ), size_load_acquire(&context.fail));
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
