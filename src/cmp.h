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