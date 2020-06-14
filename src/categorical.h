#pragma once

#include "common.h"
#include "mt.h"

#include <gsl/gsl_rng.h>

struct categorical_supp {
    uint8_t *phen_bits;
    size_t *filter, *tbl, *phen_val, *phen_mar, *outer;
};

struct categorical_adj_average_supp {
    uint8_t *phen_bits;
    size_t *filter, *tbl, *phen_mar, *phen_perm, *phen_filter, *outer;
    struct categorical_snp_data *snp_data;    
};

void mar_init(size_t *, size_t *, size_t *, size_t *, size_t, size_t);
void ymar_init(size_t *, size_t *, size_t, size_t);
void outer_chisq_init(size_t *, size_t *, size_t *, size_t, size_t);
bool outer_combined_init(size_t *, size_t *, size_t *, size_t, size_t, size_t);
double stat_exact(size_t *, size_t *, size_t *);
double qas_or(size_t *);
double stat_chisq(size_t *, size_t *, size_t, size_t, size_t);
double qas_fisher(size_t *, size_t *, size_t *, size_t *, size_t *, size_t, size_t, size_t);

struct array_result categorical_init(struct categorical_supp *, size_t, size_t);
struct mt_result categorical_impl(struct categorical_supp *, uint8_t *, size_t *, size_t, size_t, enum mt_flags);
void categorical_close(struct categorical_supp *);

struct array_result categorical_adj_average_init(struct categorical_adj_average_supp *, size_t, size_t, size_t);
struct adj_result categorical_adj_average(struct categorical_adj_average_supp *, uint8_t *, size_t *, size_t, size_t, size_t, size_t, size_t, gsl_rng *, enum mt_flags);
void maver_adj_close(struct categorical_adj_average_supp *);
