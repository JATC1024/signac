#define ARMA_USE_CXX11
#define ARMA_NO_DEBUG

#include <RcppArmadillo.h>
#include <RcppParallel.h>
#include <tbb/tbb.h>
#include <cmath>
#include <unordered_map>
#include <fstream>
#include <string>
#include <hdf5.h>
#include "CommonUtil.h"
#include <mutex>

using namespace Rcpp;
using namespace arma;
using namespace RcppParallel;

// [[Rcpp::depends(RcppParallel)]]
// [[Rcpp::depends(RcppArmadillo)]]
// [[Rcpp::depends(Rhdf5lib)]]

// [[Rcpp::export]]
arma::sp_mat FastCreateSparseMat(int nrow, int ncol) {
    return arma::speye(nrow, ncol);
}

// [[Rcpp::export]]
Rcpp::List FastStatsOfSparseMat(const arma::sp_mat &mat) {
    return Rcpp::List::create(mat.n_rows, mat.n_cols, mat.n_elem, mat.n_nonzero);
}

// [[Rcpp::export]]
arma::sp_mat FastCreateFromTriplet(const arma::urowvec &vec1, const arma::urowvec &vec2, const arma::colvec &vec_val) {
    arma::umat loc = arma::join_vert(vec1, vec2);
    arma::sp_mat sp(loc, vec_val);
    return sp;
}

// [[Rcpp::export]]
arma::sp_mat FastConvertToSparseMat(const SEXP &s) {
    return Rcpp::as<arma::sp_mat>(s);
}

// [[Rcpp::export]]
Rcpp::List FastConvertToTripletMat(const SEXP &s) {
    return Rcpp::simple_triplet_matrix(Rcpp::as<arma::sp_mat>(s));
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatSqrt(const arma::sp_mat &mat) {
    return arma::sqrt(mat);
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatMult(const arma::sp_mat &mat1, const arma::sp_mat &mat2) {
    return mat1 * mat2;
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatAddition(const arma::sp_mat &mat1, const arma::sp_mat &mat2) {
    return mat1 + mat2;
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatMultWithNum(const arma::sp_mat &mat, const int &k) {
    return k * mat;
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatSymmatl(const arma::sp_mat &mat) {
    return arma::symmatl(mat);
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatTranspose(const arma::sp_mat &mat) {
    return arma::trans(mat);
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatTrimatu(const arma::sp_mat &mat) {
    return arma::trimatu(mat);
}

// [[Rcpp::export]]
int FastSparseMatTrace(const arma::sp_mat &mat) {
    return arma::trace(mat);
}

// [[Rcpp::export]]
arma::sp_mat FastConvertToDiagonalSparseMat(arma::sp_mat &mat) {
    mat.diag().ones();
    return mat;
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatSquare(const arma::sp_mat &mat) {
    return arma::square(mat);
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatRepmat(const arma::sp_mat &mat, const int &i, const int &j) {
    return arma::repmat(mat, i, j);
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatSign(const arma::sp_mat &mat) {
    return arma::sign(mat);
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatMultSD(const arma::sp_mat &mat1, const arma::mat &mat2) {
    arma::sp_mat temp2(mat2);
    arma::sp_mat result(mat1 * temp2);
    return result;
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatMultDS(const arma::mat &mat1, const arma::sp_mat &mat2) {
    arma::sp_mat temp1(mat1);
    arma::sp_mat result(temp1 * mat2);
    return result;
}

// [[Rcpp::export]]
arma::sp_mat FastSparseMatMultDD(const arma::mat &mat1, const arma::mat &mat2) {
    arma::sp_mat temp1(mat1);
    arma::sp_mat temp2(mat2);
    arma::sp_mat result(temp1 * temp2);
    return result;
}

// [[Rcpp::export]]
arma::sp_mat FastGetRowOfSparseMat(const arma::sp_mat &mat, const int &i) {
    int irow = i;
    PerformRIndex(i, (int)mat.n_rows, irow);
    return mat.row(irow - 1);
}

// [[Rcpp::export]]
arma::sp_mat FastGetColOfSparseMat(const arma::sp_mat &mat, const int &j) {
    int icol = j;
    PerformRIndex(j, (int)mat.n_cols, icol);
    return mat.col(icol);
}

// [[Rcpp::export]]
arma::sp_mat FastGetRowsOfSparseMat(const arma::sp_mat &mat, const int &start, const int &end) {
    int irow_start = start;
    int irow_end = end;
    PerformRMultiIndex(start, (int)mat.n_rows, irow_start, end, (int)mat.n_rows, irow_end);
    return mat.rows(irow_start, irow_end);
}

// [[Rcpp::export]]
arma::sp_mat FastGetColsOfSparseMat(const arma::sp_mat &mat, const int &start, const int &end) {
    int icol_start = start;
    int icol_end = end;
    PerformRMultiIndex(start, (int)mat.n_cols, icol_start, end, (int)mat.n_cols, icol_end);
    return mat.cols(icol_start, icol_end);
}

// [[Rcpp::export]]
arma::sp_mat FastGetSubSparseMat(const arma::sp_mat &mat, const arma::urowvec &rrvec, const arma::ucolvec &ccvec) {
    arma::urowvec rvec(rrvec.size());
    PerformRVector(rrvec, (int)mat.n_rows, rvec);
    arma::ucolvec cvec(ccvec.size());
    PerformRVector(ccvec, (int)mat.n_cols, cvec);

    std::size_t total_rows = rvec.size();
    std::size_t total_cols = cvec.size();

    bool found = false;
    std::size_t n = 0;
    std::size_t p = 0;
    std::size_t found_idx = 0;

    arma::vec new_val(mat.n_nonzero);
    arma::uvec new_rvec(mat.n_nonzero);
    arma::uvec new_cvec(total_cols + 1);
    new_cvec(p) = 0;

    for (auto const& j: cvec) {
        for (std::size_t k = mat.col_ptrs[j]; k < mat.col_ptrs[j + 1]; k++) {
            found = false;
            found_idx = 0;
            while (!found && found_idx < total_rows) {
                if (mat.row_indices[k] == rvec.at(found_idx)) {
                    found = true;
                }
                found_idx++;
            }

            if (found) {
                new_val(n) = mat.values[k];
                new_rvec(n) = found_idx - 1;
                n++;
            }
        }

        p++;
        new_cvec(p) = n;
    }
    new_cvec(p) = n ;

    new_val.reshape(n, 1);
    new_rvec.reshape(n, 1);

    return arma::sp_mat(new_rvec, new_cvec, new_val, total_rows, total_cols);
}

// [[Rcpp::export]]
arma::sp_mat FastGetSubSparseMatByRows(const arma::sp_mat &mat, const arma::urowvec &rvec) {
    arma::ucolvec cvec(mat.n_cols);
    for(int i = 0; i< mat.n_cols; i++) {
        cvec(i) = i;
    }
    return FastGetSubSparseMat(mat, rvec, cvec);
}

// [[Rcpp::export]]
arma::sp_mat FastGetSubSparseMatByCols(const arma::sp_mat &mat, const arma::ucolvec &cvec) {
    arma::urowvec rvec(mat.n_rows);
    for(int i = 0; i< mat.n_rows; i++) {
        rvec(i) = i;
    }
    return FastGetSubSparseMat(mat, rvec, cvec);
}

// [[Rcpp::export]]
Rcpp::NumericVector FastGetSumSparseMatByRows(const arma::sp_mat &mat, const arma::urowvec &rvec) {
    Rcpp::NumericVector result(rvec.size());
    arma::urowvec rrvec(rvec.size());
    PerformRVector(rvec, (int)mat.n_rows, rrvec);

    //tbb::mutex m;
    tbb::parallel_for( tbb::blocked_range<int>(0, rvec.size()),
    [&](tbb::blocked_range<int> r)
    {
       for (int i=r.begin(); i<r.end(); ++i)
       {
           for (arma::sp_mat::const_row_iterator rij = mat.begin_row(i); rij != mat.end_row(i); ++rij) {
               //m.lock();
               result[rij.row()] += (*rij);
               //m.unlock();
           }
       }
    });

    return result;
}

// [[Rcpp::export]]
Rcpp::NumericVector FastGetSumSparseMatByCols(const arma::sp_mat &mat, const arma::ucolvec &cvec) {
    Rcpp::NumericVector result(cvec.size());
    arma::ucolvec ccvec(cvec.size());
    PerformRVector(cvec, (int)mat.n_cols, ccvec);

    //tbb::mutex m;
    tbb::parallel_for( tbb::blocked_range<int>(0, cvec.size()),
    [&](tbb::blocked_range<int> r)
    {
       for (int i=r.begin(); i<r.end(); ++i)
       {
           for (arma::sp_mat::const_col_iterator cij = mat.begin_col(i); cij != mat.end_col(i); ++cij) {
               //m.lock();
               result[cij.col()] += (*cij);
               //m.unlock();
           }
       }
    });

    return result;
}

// [[Rcpp::export]]
Rcpp::NumericVector FastGetSumSparseMatByAllRows(arma::sp_mat &mat) {
    Rcpp::NumericVector result(mat.n_rows);
    tbb::mutex m;
    tbb::parallel_for( tbb::blocked_range<int>(0, mat.n_cols),
    [&](tbb::blocked_range<int> r)
    {
       for (int i=r.begin(); i<r.end(); ++i)
       {
           for (arma::sp_mat::const_col_iterator cij = mat.begin_col(i); cij != mat.end_col(i); ++cij) {
               m.lock();
               result[cij.row()] += (*cij);
               m.unlock();
           }
       }
    });
    return result;
}

// [[Rcpp::export]]
Rcpp::NumericVector FastGetSumSparseMatByAllRowsV2(arma::sp_mat &mat) {
    Rcpp::NumericVector result(mat.n_rows);
    //tbb::mutex m;
    tbb::parallel_for( tbb::blocked_range<int>(0, mat.n_rows),
    [&](tbb::blocked_range<int> r)
    {
       for (int i=r.begin(); i<r.end(); ++i)
       {
           for (arma::sp_mat::const_row_iterator rij = mat.begin_row(i); rij != mat.end_row(i); ++rij) {
               //m.lock();
               result[rij.row()] += (*rij);
               //m.unlock();
           }
       }
    });
    return result;
}

// [[Rcpp::export]]
Rcpp::NumericVector FastGetSumSparseMatByAllCols(arma::sp_mat &mat) {
    Rcpp::NumericVector result(mat.n_cols);
    //tbb::mutex m;
    tbb::parallel_for( tbb::blocked_range<int>(0, mat.n_cols),
    [&](tbb::blocked_range<int> r)
    {
        for (int i=r.begin(); i<r.end(); ++i)
        {
            for (arma::sp_mat::const_col_iterator cij = mat.begin_col(i); cij != mat.end_col(i); ++cij) {
                //m.lock();
                result[cij.col()] += (*cij);
                //m.unlock();
            }
        }
    });
    return result;
}
