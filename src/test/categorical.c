#include "../categorical.h"
#include "../np.h"
#include "../log.h"
#include "../test.h"
#include "../memory.h"
#include "categorical.h"

#include <string.h>
#include <math.h>

bool test_categorical_generator_a(void *dst, size_t *p_context, struct log *log)
{
    struct test_categorical_a data[] = {
        { MATCX(2, (size_t[]) { 1, 10, 20, 21 }), 1.4632678190897159 }, // Exact Fisher
        { MATCX(2, (size_t[]) { 10, 20, 100, 20 }), 7.5171259287287606 } // Chi-Square
    };
    size_t *tbl, ind = *p_context;
    if (!array_init(&tbl, NULL, data[ind].cnt, sizeof(*tbl), 0, ARRAY_STRICT)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
    else
    {
        memcpy(dst, data + ind, sizeof(*data));
        memcpy(tbl, data[ind].tbl, data[ind].cnt * sizeof(*tbl));
        ((struct test_categorical_a *) dst)->tbl = tbl;
        if (++*p_context >= countof(data)) *p_context = 0;
        return 1;
    }
    return 0;
}

void test_categorical_disposer_a(void *In)
{
    struct test_categorical_a *in = In;
    free(in->tbl);
}

bool test_categorical_a(void *In, struct log *log)
{
    bool succ = 0;
    struct test_categorical_a *in = In;
    size_t *xmar = NULL, *ymar = NULL, *outer = NULL;
    if (!array_init(&outer, NULL, in->cnt, sizeof(*outer), 0, ARRAY_STRICT) ||
        !array_init(&xmar, NULL, in->dimx, sizeof(*xmar), 0, ARRAY_STRICT | ARRAY_CLEAR) ||
        !array_init(&ymar, NULL, in->dimy, sizeof(*ymar), 0, ARRAY_STRICT | ARRAY_CLEAR)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
    else
    {
        double nlpv, qas;
        size_t mar = 0;
        mar_init(in->tbl, xmar, ymar, &mar, in->dimx, in->dimy);
        if (outer_combined_init(outer, xmar, ymar, mar, in->dimx, in->dimy))
        {
            nlpv = stat_chisq(in->tbl, outer, mar, in->dimx, in->dimy);
            qas = qas_fisher(in->tbl, xmar, ymar, mar, in->dimx, in->dimy);
        }
        else
        {
            nlpv = stat_exact(in->tbl, xmar, ymar);
            qas = qas_lor(in->tbl);
        }
        if (fabs(nlpv - in->nlpv) >= nlpv * REL_ERROR) log_message_fmt(log, CODE_METRIC, MESSAGE_ERROR, "Relative error is too large!\n");
        else succ = 1;
    }    
    free(xmar);
    free(ymar);
    free(outer);
    return succ;
}
