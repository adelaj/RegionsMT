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

static unsigned test_gen_inst_thread(void *Indl, void *Group, void *Tls)
{
    // w -- group index; x -- test index; y -- generator index; z -- generator instance index
    size_t *indl = Indl, w = indl[0], x = indl[1], y = indl[2], z = indl[3];
    const struct test_group *group = Group;
    struct test_tls *tls = Tls;
    //log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_INFO, "Test %~uz:%~uz:%~uz:%~uz assigned to the thread no. %~uz.\n", w, x, y, z, tls->base.tid);
    void *data;
    if (!group->generator[y](&data, &z, &tls->log)) return 0;
    uint64_t start = get_time();
    bool succ = group->test[x](data, &tls->log);
    if (!succ) log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_WARNING, "Test %~uz:%~uz:%~uz:%~uz failed!\n", w, x, y, z);
    else log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_INFO, "Execution of the test %~uz:%~uz:%~uz:%~uz took %~T.\n", w, x, y, z, start, get_time());
    if (group->dispose) group->dispose(data);
    return succ;
}

static unsigned test_gen_thread(void *Indl, void *Group, void *Tls)
{
    // w -- group index; x -- test index; y -- generator index
    size_t *indl = Indl, w = indl[0], x = indl[1], y = indl[2];
    const struct test_group *group = Group;
    struct test_tls *tls = Tls;
    size_t cnt = 1;
    for (size_t i = 0; group->generator[y](NULL, &i, &tls->log), i; cnt++);
    return loop_init(tls->base.pool, test_gen_inst_thread, (struct task_cond) { 0 }, (struct task_aggr) { 0 }, (void *) group, ARG(size_t, 1, 1, 1, cnt), (size_t []) { w, x, y, 0 }, 0, &tls->log);
}

bool test(const struct test_group *groupl, size_t cnt, size_t thread_cnt, struct log *log)
{
    struct thread_pool *pool = thread_pool_create(thread_cnt, sizeof(struct test_tls), 0, log);
    if (!pool) return 0;
    size_t ind = 0;
    for (; ind < thread_cnt; ind++)
        if (!log_dup(&((struct test_tls *) thread_pool_fetch_tls(pool, ind))->log, log)) break;
    bool succ = 0;
    if (ind == thread_cnt)
    {
        size_t i = 0;
        for (; i < cnt; i++)
            if (!loop_init(pool, test_gen_thread, (struct task_cond) { 0 }, (struct task_aggr) { 0 }, (void *) (groupl + i), ARG(size_t, 1, groupl[i].test_cnt, groupl[i].generator_cnt), (size_t []) { i, 0, 0 }, 0, log)) break;
        succ = i == cnt;
        thread_pool_schedule(pool);
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
