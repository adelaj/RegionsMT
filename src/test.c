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
    size_t *indl = Indl, tn = indl[0], gn = indl[1], gi = indl[2];
    const struct test_group *group = Group;
    struct test_tls *tls = Tls;
    log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_INFO, "Test %~uz:%~uz:%~uz from the group %\"~s* assigned to the thread no. %~uz.\n", tn, gn, gi, STRL(group->name), tls->base.tid);
    void *data;
    if (!group->generator[gn](&data, &gi, &tls->log)) return 0;
    uint64_t start = get_time();
    bool succ = group->test[tn](data, &tls->log);
    if (!succ) log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_WARNING, "Test %~uz:%~uz:%~uz from the group %\"~s* failed!\n", tn, gn, gi, STRL(group->name));
    else log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_INFO, "Execution of the test %~uz:%~uz:%~uz from the group %\"~s* took %~T.\n", tn, gn, gi, STRL(group->name), start, get_time());
    if (group->dispose) group->dispose(data);
    return succ;
}

static unsigned test_gen_thread(void *Indl, void *Group, void *Tls)
{
    size_t *indl = Indl, tn = indl[0], gn = indl[1];
    const struct test_group *group = Group;
    struct test_tls *tls = Tls;
    log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_INFO, "Test %~uz:%~uz from the group %\"~s* assigned to thread %~uz!\n", tn, gn, STRL(group->name), tls->base.tid);
    return 1;
    /*
    size_t cnt = 1;
    for (size_t ind = 0; group->generator[gn](NULL, &ind, &tls->log), ind; cnt++);
    return loop_init(tls->base.pool, test_gen_inst_thread, (struct task_cond) { 0 }, (struct task_aggr) { 0 }, (void *) group, ARG(size_t, 1, 1, cnt), (size_t []) { tn, gn, 0 }, 0, &tls->log);
    */
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
            if (!loop_init(pool, test_gen_thread, (struct task_cond) { 0 }, (struct task_aggr) { 0 }, (void *) (groupl + i), ARG(size_t, groupl[i].test_cnt, groupl[i].generator_cnt), NULL, 0, log)) break;
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
