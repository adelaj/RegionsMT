#include "../np.h"
#include "../ll.h"
#include "../lde.h"
#include "../memory.h"
#include "../tblproc.h"
#include "lde.h"
#include "categorical.h"

#include <stdlib.h>

enum tbl_gen_col_type {
    TBL_GEN_COL_IND = 0,
    TBL_GEN_COL_GEN = 1,
};

struct tbl_gen_head_context {
    size_t cl_cnt, phen_cnt;
    size_t *cl_type, *gen;
    uint8_t *cl_bits, *gen_bits;
};

bool lde_run(const char *path_gen, const char *path_out, struct log *log)
{
    uint8_t *gen = NULL;
    struct gen_context gen_context = { 0 };
    size_t gen_skip = 1, snp_cnt = 0, gen_length = 0;
    if (!tbl_read(path_gen, 0, tbl_gen_selector2, NULL, &gen_context, &gen, &gen_skip, &snp_cnt, &gen_length, ',', log)) goto error;

    FILE *f = fopen(path_out, "w");
    if (!f) goto error;

    for (size_t i = 0; i < snp_cnt; i++)
    {
        for (size_t j = size_sub_sat(i, 80); j < i; j++)
        {
            double lde = lde_impl(gen + gen_context.phen_cnt * i, gen + gen_context.phen_cnt * j, gen_context.phen_cnt);
            //unsigned res = -lde > .9;
            fprintf(f ,"%zu,%zu,%.15f\n%zu,%zu,%.15f\n", j + 1, i + 1, lde, i + 1, j + 1, lde);
        }
    }

    Fclose(f);

error:
    free(gen);
    return 1;
}
