#include <lgr_linear_algebra.hpp>
#include <Omega_h_profile.hpp>
#include <lgr_for.hpp>
#include <Omega_h_reduce.hpp>

// DEBUG
#include <Omega_h_print.hpp>

namespace lgr {

void matvec(GlobalMatrix mat, GlobalVector vec, GlobalVector result) {
  OMEGA_H_TIME_FUNCTION;
  auto const n = result.size();
  auto f = OMEGA_H_LAMBDA(int const row) {
    double value = 0.0;
    auto const begin = mat.rows_to_columns.a2ab[row];
    auto const end = mat.rows_to_columns.a2ab[row + 1];
    for (auto row_col = begin; row_col < end; ++row_col) {
      auto const col = mat.rows_to_columns.ab2b[row_col];
      value += mat.entries[row_col] * vec[col];
    }
    result[row] = value;
  };
  parallel_for(n, std::move(f));
}

double dot(GlobalVector a, GlobalVector b) {
  OMEGA_H_TIME_FUNCTION;
  auto const n = a.size();
  auto const first = Omega_h::IntIterator(0);
  auto const last = Omega_h::IntIterator(n);
  double const init = 0.0;
  auto const op = Omega_h::plus<double>();
  auto transform = OMEGA_H_LAMBDA(int const row) -> double {
    return a[row] * b[row];
  };
  return Omega_h::transform_reduce(first, last, init, op, std::move(transform));
}

void axpy(double a, GlobalVector x, GlobalVector y, GlobalVector result) {
  OMEGA_H_TIME_FUNCTION;
  auto f = OMEGA_H_LAMBDA(int const i) {
    result[i] = a * x[i] + y[i];
  };
  parallel_for(result.size(), std::move(f));
}

int conjugate_gradient(
    GlobalMatrix A,
    GlobalVector b,
    GlobalVector x,
    double max_residual_magnitude) {
  OMEGA_H_TIME_FUNCTION;
  std::cout << "A = " << read(A.entries) << '\n';
  std::cout << "b = " << read(b) << '\n';
  std::cout << "x_0 = " << read(x) << '\n';
  auto const n = x.size();
  GlobalVector r(n, "CG/r");
  matvec(A, x, r); // r = A * x
  axpy(-1.0, r, b, r); // r = -r + b, r = b - A * x
  std::cout << "r_0 = " << read(r) << '\n';
  GlobalVector p(n, "CG/p");
  Omega_h::copy_into(read(r), p); // p = r
  auto rsold = dot(r, r);
  std::cout << "|r|_0 = " << rsold << '\n';
  if (std::sqrt(rsold) < max_residual_magnitude) {
    return 0;
  }
  GlobalVector Ap(n, "CG/Ap");
  for (int i = 0; i < b.size(); ++i) {
    matvec(A, p, Ap);
    auto const alpha = rsold / dot(p, Ap);
    axpy(alpha, p, x, x); // x = x + alpha * p
    std::cout << "x = " << read(x) << '\n';
    axpy(alpha, Ap, r, r); // r = r + alpha * Ap
    std::cout << "r = " << read(r) << '\n';
    auto const rsnew = dot(r, r);
    std::cout << "rsnew = " << rsnew << '\n';
    if (std::sqrt(rsnew) < max_residual_magnitude) {
      return i + 1;
    }
    auto const beta = rsnew / rsold;
    axpy(beta, p, r, p); // p = r + (rsnew / rsold) * p
    std::cout << "p = " << read(p) << '\n';
    rsold = rsnew;
  }
  return b.size() + 1;
}

}
