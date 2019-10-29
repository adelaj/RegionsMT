#include "np.h"
#include "ll.h"
#include "log.h"
#include "memory.h"
#include "utf8.h"
#include "test.h"

#include <stdlib.h>

bool test(const struct test_group *group_arr, size_t cnt, struct log *log)
{
    bool succ = 0;
    uint64_t all = get_time();
    size_t test_data_sz = 0;
    for (size_t i = 0; i < cnt; i++) if (test_data_sz < group_arr[i].test_sz) test_data_sz = group_arr[i].test_sz;
    void *test_data = NULL;
    if (!array_init(&test_data, NULL, test_data_sz, 1, 0, ARRAY_STRICT)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
    else
    {
        succ = 1;
        for (size_t i = 0; i < cnt; i++)
        {
            uint64_t start = get_time();
            const struct test_group *group = group_arr + i;
            for (size_t j = 0; j < group->test_generator_cnt; j++)
            {
                size_t context = 0;
                do {
                    size_t ind = context;
                    if (group->test_generator[j](test_data, &context, log))
                    {
                        for (size_t k = 0; k < group->test_cnt; k++)
                        {
                            bool res = group->test[k](test_data, log);
                            if (!res) log_message_fmt(log, CODE_METRIC, MESSAGE_WARNING, "Test no. %~uz of the group no. %~uz failed under the input data instance no. %~uz of the generator no. %~uz!\n", k + 1, i + 1, ind + 1, j + 1);
                            succ &= res;
                        }
                        if (group->test_dispose) group->test_dispose(test_data);
                        continue;
                    }
                    succ = 0;
                } while (context);
            }
            log_message_fmt(log, CODE_METRIC, MESSAGE_INFO, "Tests execution of the group no. %~uz took %~T.\n", i + 1, start, get_time());
        }
        log_message_fmt(log, CODE_METRIC, MESSAGE_INFO, "Tests execution took %~T.\n", all, get_time());
        free(test_data);
    }
    return succ;
}

bool perf(struct log *log)
{
    uint64_t start = get_time();

    // Performance tests here

    log_message_fmt(log, CODE_METRIC, MESSAGE_INFO, "Performance tests execution took %~T.\n", start, get_time());
    return 1;
}
