#include "ll.h"
#include "gslsupp.h"

#include <math.h>
#include <gsl/gsl_sf.h>
#include <gsl/gsl_blas.h>

double log_fact(size_t n)
{
    gsl_sf_result res;
    if (n <= UINT_MAX) 
    {
        if (gsl_sf_lnfact_e((unsigned) n, &res) == GSL_SUCCESS) return res.val;
    }
    else
    {
        if (gsl_sf_lngamma_e((double) n + 1., &res) == GSL_SUCCESS) return res.val;
    }
    return nan(__func__);
}

double log_choose(size_t n, size_t m)
{
    if (m > n) return nan(__func__);
    if (m == n || !m) return 0.;
    return m > (n >> 1) ? log_fact(n) - log_fact(n - m) - log_fact(m) : log_fact(n) - log_fact(m) - log_fact(n - m); // Addition here may be not associative
}

double pdf_hypergeom(size_t k, size_t n1, size_t n2, size_t t)
{
    size_t car, n12 = size_add(&car, n1, n2);
    if (car) return nan(__func__);
    if (t > n12) t = n12;
    if (k > n1 || k > t || (t > n2 && k < t - n2)) return 0.;
    return exp(log_choose(n1, k) + log_choose(n2, t - k) - log_choose(n12, t));
}

double gamma_inc_P(double a, double x)
{
    gsl_sf_result res;
    if (gsl_sf_gamma_inc_P_e(a, x, &res) == GSL_SUCCESS) return res.val;
    return nan(__func__);
}

double gamma_inc_Q(double a, double x)
{
    gsl_sf_result res;
    if (gsl_sf_gamma_inc_Q_e(a, x, &res) == GSL_SUCCESS) return res.val;
    return nan(__func__);
}

double cdf_gamma_Q(double x, double a, double b)
{
    if (x <= 0.) return 1.;
    double y = x / b;
    return y < a ? 1. - gamma_inc_P(a, y) : gamma_inc_Q(a, y);
}

double cdf_chisq_Q(double x, double df)
{
    return cdf_gamma_Q(x, .5 * df, 2.);
}

// Warning! This function contains code, copy-pasted from the GSL library and adapted to meet our requirements
// This part should be distributed under the terms of the GNU GPL version 3 or later!
static void multifit_linear_solve(const gsl_matrix *X, const gsl_vector *y, double tol, double lambda, size_t *rank, gsl_vector *c, double *rnorm, double *snorm, gsl_multifit_linear_workspace *ws)
{
    size_t n = X->size1, p = X->size2;
    double lambda_sq = lambda * lambda;
    double rho_ls = 0.; /* contribution to rnorm from OLS */

    size_t j, p_eff;

    /* these inputs are previously computed by multifit_linear_svd() */
    gsl_matrix_view A = gsl_matrix_submatrix(ws->A, 0, 0, n, p);
    gsl_matrix_view Q = gsl_matrix_submatrix(ws->Q, 0, 0, p, p);
    gsl_vector_view S = gsl_vector_subvector(ws->S, 0, p);

    /* workspace */
    gsl_matrix_view QSI = gsl_matrix_submatrix(ws->QSI, 0, 0, p, p);
    gsl_vector_view xt = gsl_vector_subvector(ws->xt, 0, p);
    gsl_vector_view D = gsl_vector_subvector(ws->D, 0, p);
    gsl_vector_view t = gsl_vector_subvector(ws->t, 0, n);

    /*
    * Solve y = A c for c
    * c = Q diag(s_i / (s_i^2 + lambda_i^2)) U^T y
    */

    /* compute xt = U^T y */
    gsl_blas_dgemv(CblasTrans, 1.0, &A.matrix, y, 0.0, &xt.vector);

    if (n > p)
    {
        /*
        * compute OLS residual norm = || y - U U^T y ||;
        * for n = p, U U^T = I, so no need to calculate norm
        */
        gsl_vector_memcpy(&t.vector, y);
        gsl_blas_dgemv(CblasNoTrans, -1.0, &A.matrix, &xt.vector, 1.0, &t.vector);
        rho_ls = gsl_blas_dnrm2(&t.vector);
    }

    if (lambda > 0.0)
    {
        /* xt <-- [ s(i) / (s(i)^2 + lambda^2) ] .* U^T y */
        for (j = 0; j < p; ++j)
        {
            double sj = gsl_vector_get(&S.vector, j);
            double f = (sj * sj) / (sj * sj + lambda_sq);
            double *ptr = gsl_vector_ptr(&xt.vector, j);

            /* use D as workspace for residual norm */
            gsl_vector_set(&D.vector, j, (1.0 - f) * (*ptr));

            *ptr *= sj / (sj * sj + lambda_sq);
        }

        /* compute regularized solution vector */
        gsl_blas_dgemv(CblasNoTrans, 1.0, &Q.matrix, &xt.vector, 0.0, c);

        /* compute solution norm */
        *snorm = gsl_blas_dnrm2(c);

        /* compute residual norm */
        *rnorm = gsl_blas_dnrm2(&D.vector);

        if (n > p)
        {
            /* add correction to residual norm (see eqs 6-7 of [1]) */
            *rnorm = sqrt((*rnorm) * (*rnorm) + rho_ls * rho_ls);
        }

        /* reset D vector */
        gsl_vector_set_all(&D.vector, 1.0);
    }
    else
    {
        /* Scale the matrix Q, QSI = Q S^{-1} */

        gsl_matrix_memcpy(&QSI.matrix, &Q.matrix);

        {
            double s0 = gsl_vector_get(&S.vector, 0);
            p_eff = 0;

            for (j = 0; j < p; j++)
            {
                gsl_vector_view column = gsl_matrix_column(&QSI.matrix, j);
                double sj = gsl_vector_get(&S.vector, j);
                double alpha;

                if (sj <= tol * s0)
                {
                    alpha = 0.0;
                }
                else
                {
                    alpha = 1.0 / sj;
                    p_eff++;
                }

                gsl_vector_scale(&column.vector, alpha);
            }

            *rank = p_eff;
        }

        gsl_blas_dgemv(CblasNoTrans, 1.0, &QSI.matrix, &xt.vector, 0.0, c);

        /* Unscale the balancing factors */
        gsl_vector_div(c, &D.vector);

        *snorm = gsl_blas_dnrm2(c);
        *rnorm = rho_ls;
    }
}

bool multifit_linear_tsvd(const gsl_matrix *X, const gsl_vector *y, double tol, gsl_vector *c, double *p_chisq, size_t *p_rank, gsl_multifit_linear_workspace *ws)
{
    double rnorm, snorm;
    if (gsl_multifit_linear_bsvd(X, ws)) return 0;
    multifit_linear_solve(X, y, tol, -1., p_rank, c, &rnorm, &snorm, ws);
    *p_chisq = rnorm * rnorm;
    return 1;
}