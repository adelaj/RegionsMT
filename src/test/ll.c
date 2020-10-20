#include "../np.h"
#include "../ll.h"
#include "../cmp.h"
#include "ll.h"

#include <string.h>

bool test_ll_generator_a(void *p_res, size_t *p_ind, struct log *log)
{
    (void) log;
    static const struct test_ll_a data[] = {
        { 0b0, UINT32_MAX, UINT32_MAX },
        { 0b1, 0, 0 },
        { 0b10, 1, 1 },
        { 0b100, 2, 2 },
        { 0b1001, 0, 3 },
        { 0b10010, 1, 4 },
        { 0b100100, 2, 5 },
        { 0b1001000, 3, 6 },
        { 0b10010000, 4, 7 },
        { 0b100100000, 5, 8 },
        { 0b1001000000, 6, 9 },
        { 0b10010000000, 7, 10 },
        { 0b100100000000, 8, 11 },
        { 0b1001000000000, 9, 12 },
        { 0b10010000000000, 10, 13 },
        { 0b100100000000000, 11, 14 },
        { 0b1001000000000000, 12, 15 },
        { 0b10010000000000000, 13, 16 },
        { 0b100100000000000000, 14, 17 },
        { 0b1001000000000000000, 15, 18 },
    };
    if (p_res) *(const struct test_ll_a **) p_res = data + *p_ind;
    else if (++*p_ind >= countof(data)) *p_ind = 0;
    return 1;
}

unsigned test_ll_a(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    const struct test_ll_a *in = In;
    uint32_t res_bsr = bsr(in->a), res_bsf = bsf(in->a);
    if (res_bsr != in->res_bsr || res_bsf != in->res_bsf) return 0;
    return 1;
}
