#include "../np.h"
#include "../test.h"
#include "../memory.h"
#include "np.h"

#include <stdlib.h> 

bool test_np_generator_a(void *p_res, size_t *p_ind, struct log *log)
{
    size_t context = *p_ind, cnt = (size_t) 1 << context;
    if (!p_res)
    {
        if (context < TEST_NP_EXP) ++*p_ind;
        else *p_ind = 0;
        return 1;
    }    
    if (!array_assert(log, CODE_METRIC, array_init(p_res, NULL, fam_countof(struct test_np_a, str, cnt), fam_sizeof(struct test_np_a, str), fam_diffof(struct test_np_a, str, cnt), ARRAY_STRICT))) return 0;
    struct test_np_a *res = *(struct test_np_a **) p_res;
    res->cnt = cnt;
    for (size_t i = 0; i < cnt;)
        for (char j = 0; j < CHAR_MAX && i < cnt; j++)
            for (char k = 0; k < j && i < cnt; k++) res->str[i++] = k;
    return 1;
}

static void *Memrchr_test(void const *Str, int ch, size_t cnt)
{
    const char *str = Str;
    while (cnt) if (str[--cnt] == ch) return (void *) (str + cnt);
    return NULL;
}

unsigned test_np_a(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    struct test_np_a *in = In;
    for (char i = 0; i < CHAR_MAX; i++) for (size_t cnt = in->cnt; cnt;)
    {
        char *a = Memrchr(in->str, i, cnt);
        if (a != Memrchr_test(in->str, i, cnt)) return 0;
        if (!a) break;
        cnt = a - in->str;
    }
    return 1;
}
