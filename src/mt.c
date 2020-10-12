#include "ll.h"
#include "mt.h"
#include "sort.h"
#include "memory.h"
#include "strproc.h"

// Warning! 'lev' and 'phen' may point to the same memory region! The same may apply to 'filter' and 'grouping'
struct array_result levels_from_phen_impl(size_t *lev, size_t *phen, char *buff, size_t *p_cnt, uint8_t *filter, uint8_t *grouping)
{
    uintptr_t *ptr = NULL;
    size_t cnt = *p_cnt;
    struct array_result res = pointers(&ptr, phen, cnt, sizeof(*phen), str_off_cmp, buff);
    if (!res.status) return res;
    size_t ucnt = 0, i = 0, ip = 0;
    for (; i < cnt; i++)
    {
        ip = (size_t) ((ptr[i] - (uintptr_t) phen) / sizeof(*phen));
        if (bt(filter, ip)) break;
        lev[ip] = SIZE_MAX;
    }
    for (size_t j = i + 1; j < cnt; j++)
    {
        size_t jp = (size_t) ((ptr[j] - (uintptr_t) phen) / sizeof(*phen));
        if (bt(filter, jp))
        {
            size_t tmp = ucnt;
            if (str_off_cmp((void *) ptr[j], (void *) ptr[i], buff)) ucnt++;
            lev[ip] = bt(grouping, ip) ? tmp : SIZE_MAX;
            i = j;
            ip = jp;
        }
        else lev[jp] = SIZE_MAX;
    }
    if (i < cnt)
    {
        lev[ip] = bt(grouping, ip) ? ucnt : SIZE_MAX;
        ucnt++;
    }
    free(ptr);
    *p_cnt = ucnt;
    return (struct array_result) { .status = ARRAY_SUCCESS_UNTOUCHED };
}

struct array_result levels_from_phen(size_t **p_lev, size_t *phen, char *buff, size_t *p_cnt, uint8_t *filter, uint8_t *grouping)
{
    struct array_result res1 = array_init(p_lev, NULL, *p_cnt, sizeof(**p_lev), 0, ARRAY_STRICT);
    if (!res1.status) return res1;
    struct array_result res2 = levels_from_phen_impl(*p_lev, phen, buff, p_cnt, filter, grouping);
    if (res2.status) return res1;
    free(*p_lev);
    return res1;
}

