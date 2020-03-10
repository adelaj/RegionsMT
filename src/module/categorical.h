#pragma once

#include "../categorical.h"
#include "../common.h"
#include "../log.h"
#include "../tblproc.h"

struct gen_context {
    size_t gen_cap, gen_cnt, phen_cnt;
};

void print_cat(FILE *, struct categorical_res);

bool tbl_gen_selector2(struct tbl_col *, size_t, size_t, void *tbl, void *);
bool categorical_run_chisq(const char *, const char *, const char *, struct log *log);
bool categorical_run_adj(const char *, const char *, const char *, const char *, size_t, uint64_t, struct log *);