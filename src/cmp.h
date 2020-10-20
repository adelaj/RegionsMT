#pragma once

int size_stable_cmp_dsc(const void *, const void *, void *);
int size_stable_cmp_asc(const void *, const void *, void *);

bool size_cmp_dsc(const void *, const void *, void *);
bool size_cmp_asc(const void *, const void *, void *);

int flt64_stable_cmp_dsc(const void *, const void *, void *);
int flt64_stable_cmp_asc(const void *, const void *, void *);

int flt64_stable_cmp_dsc_abs(const void *, const void *, void *);
int flt64_stable_cmp_asc_abs(const void *, const void *, void *);

int flt64_stable_cmp_dsc_nan(const void *, const void *, void *);
int flt64_stable_cmp_asc_nan(const void *, const void *, void *);

int char_cmp(const void *, const void *, void *);

bool str_eq(const void *, const void *, void *);
bool stro_str_eq(const void *, const void *, void *);
bool stro_eq(const void *, const void *, void *);

int stro_stable_cmp(const void *, const void *, void *);
bool stro_cmp(const void *, const void *, void *);

int str_strl_stable_cmp(const void *, const void *, void *);