#pragma once

#include "common.h"

#include <gsl/gsl_rng.h>

#define ALT_CNT 4
#define GEN_CNT 3

enum categorical_flags {
    TEST_TYPE_CODOMINANT = 1,
    TEST_TYPE_RECESSIVE = 2,
    TEST_TYPE_DOMINANT = 4,
    TEST_TYPE_ALLELIC = 8
};

struct categorical_supp {
    uint8_t *phen_bits;
    size_t *filter, *tbl, *phen_val, *phen_mar, *outer;
};

struct categorical_res {
    double pv[ALT_CNT], qas[ALT_CNT];
};

struct maver_adj_supp {
    uint8_t *phen_bits;
    size_t *filter, *tbl, *phen_mar, *phen_perm, *outer;
    struct categorical_snp_data *snp_data;    
};

struct maver_adj_res {
    double pv[ALT_CNT];
    size_t rpl[ALT_CNT];
};

size_t filter_init(size_t *, uint8_t *, size_t);

void mar_init(size_t *, size_t *, size_t *, size_t *, size_t, size_t);
void ymar_init(size_t *, size_t *, size_t, size_t);
void outer_chisq_init(size_t *, size_t *, size_t *, size_t, size_t);
bool outer_combined_init(size_t *, size_t *, size_t *, size_t, size_t, size_t);
double stat_exact(size_t *, size_t *, size_t *);
double qas_or(size_t *);
double stat_chisq(size_t *, size_t *, size_t, size_t, size_t);
double qas_fisher(size_t *, size_t *, size_t *, size_t *, size_t *, size_t, size_t, size_t);

bool categorical_init(struct categorical_supp *, size_t, size_t);
struct categorical_res categorical_impl(struct categorical_supp *, uint8_t *, size_t *, size_t, size_t, enum categorical_flags);
void categorical_close(struct categorical_supp *);

bool maver_adj_init(struct maver_adj_supp *, size_t, size_t, size_t);
struct maver_adj_res maver_adj_impl(struct maver_adj_supp *, uint8_t *, size_t *, size_t, size_t, size_t, size_t, size_t, gsl_rng *, enum categorical_flags);
void maver_adj_close(struct maver_adj_supp *);
