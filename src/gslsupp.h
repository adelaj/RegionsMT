#pragma once

#include "common.h"

#include <gsl/gsl_multifit.h>

double log_fact(size_t);
double log_choose(size_t, size_t);
double pdf_hypergeom(size_t, size_t, size_t, size_t);
double gamma_inc_P(double, double);
double gamma_inc_Q(double, double);

double cdf_gamma_Q(double, double, double);
double cdf_chisq_Q(double, double);

bool multifit_linear_tsvd(const gsl_matrix *, const gsl_vector *, double tol, gsl_vector *, double *, size_t *, gsl_multifit_linear_workspace *);
