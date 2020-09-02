#include "../categorical.h"
#include "../np.h"
#include "../log.h"
#include "../test.h"
#include "../memory.h"
#include "categorical.h"

#include <string.h>
#include <math.h>

bool test_categorical_generator_a(void *p_res, size_t *p_ind, struct log *log)
{
    struct {
        double pv, qas;
        size_t *tbl, dimx, dimy, cnt;
    } data[] = {
        // fisher.test(matrix(c(1, 10, 20, 21), 2, 2))$p.value
        { 0.03441376438044415, 0., MATCX(2, (size_t []) { 1, 10, 20, 21 }) }, // Exact Fisher
        // chisq.test(matrix(c(10, 20, 100, 20), 2, 2), correct = F)$p.value
        { 3.040003413570398e-08, 0., MATCX(2, (size_t []) { 10, 20, 100, 20 }) } // Chi-Square
    };
    if (!p_res)
    {
        if (++*p_ind >= countof(data)) *p_ind = 0;
        return 1;
    }    
    size_t ind = *p_ind;
    if (!array_assert(log, CODE_METRIC, array_init(p_res, NULL, fam_countof(struct test_categorical_a, tbl, data[ind].cnt), fam_sizeof(struct test_categorical_a, tbl), fam_diffof(struct test_categorical_a, tbl, data[ind].cnt), ARRAY_STRICT))) return 0;
    struct test_categorical_a *res = *(struct test_categorical_a **) p_res;
    res->pv = data[ind].pv;
    res->qas = data[ind].qas;
    res->dimx = data[ind].dimx;
    res->dimy = data[ind].dimy;
    res->cnt = data[ind].cnt;
    memcpy(res->tbl, data[ind].tbl, data[ind].cnt * sizeof(*res->tbl));
    return 1;
}

unsigned test_categorical_a(void *In, void *Context, void *Tls)
{
    (void) Context;
    unsigned res = TEST_RETRY;
    struct test_categorical_a *in = In;
    struct test_tls *tls = Tls;
    size_t *xmar = NULL, *ymar = NULL, *outer = NULL;
    if (array_assert(&tls->log, CODE_METRIC, array_init(&outer, NULL, in->cnt, sizeof(*outer), 0, ARRAY_STRICT)) &&
        array_assert(&tls->log, CODE_METRIC, array_init(&xmar, NULL, in->dimx, sizeof(*xmar), 0, ARRAY_STRICT | ARRAY_CLEAR)) &&
        array_assert(&tls->log, CODE_METRIC, array_init(&ymar, NULL, in->dimy, sizeof(*ymar), 0, ARRAY_STRICT | ARRAY_CLEAR)))
    {
        size_t mar = 0;
        mar_init(in->tbl, xmar, ymar, &mar, in->dimx, in->dimy);
        double pv = outer_combined_init(outer, xmar, ymar, mar, in->dimx, in->dimy) ? stat_chisq(in->tbl, outer, mar, in->dimx, in->dimy) : stat_exact(in->tbl, xmar, ymar);
        res = fabs(pv - in->pv) >= pv * REL_ERROR;
        if (!res) log_message_fmt(&tls->log, CODE_METRIC, MESSAGE_ERROR, "Relative error is too large!\n");
    }    
    free(xmar);
    free(ymar);
    free(outer);
    return res;
}
