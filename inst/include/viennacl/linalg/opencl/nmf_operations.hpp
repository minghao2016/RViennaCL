#ifndef VIENNACL_LINALG_OPENCL_NMF_OPERATIONS_HPP_
#define VIENNACL_LINALG_OPENCL_NMF_OPERATIONS_HPP_

/* =========================================================================
 Copyright (c) 2010-2016, Institute for Microelectronics,
 Institute for Analysis and Scientific Computing,
 TU Wien.
 Portions of this software are copyright by UChicago Argonne, LLC.

 -----------------
 ViennaCL - The Vienna Computing Library
 -----------------

 Project Head:    Karl Rupp                   rupp@iue.tuwien.ac.at

 (A list of authors and contributors can be found in the manual)

 License:         MIT (X11), see file LICENSE in the base directory
 ============================================================================= */

/** @file viennacl/linalg/opencl/vector_operations.hpp
 @brief Implementations of NMF operations using OpenCL
 */

#include "viennacl/linalg/opencl/kernels/nmf.hpp"
#include "viennacl/linalg/opencl/nmf_operations.hpp"

#include "viennacl/linalg/host_based/nmf_operations.hpp"

#include <Rcpp.h>
namespace viennacl
{
namespace linalg
{
namespace opencl
{

/** @brief The nonnegative matrix factorization (approximation) algorithm as suggested by Lee and Seung. Factorizes a matrix V with nonnegative entries into matrices W and H such that ||V - W*H|| is minimized.
 *
 * @param V     Input matrix
 * @param W     First factor
 * @param H     Second factor
 * @param conf  A configuration object holding tolerances and the like
 */
template<typename NumericT>
void nmf(viennacl::matrix_base<NumericT> const & V,
         viennacl::matrix_base<NumericT>       & W,
         viennacl::matrix_base<NumericT>       & H,
         viennacl::linalg::nmf_config const & conf)
{
  viennacl::ocl::context & ctx = const_cast<viennacl::ocl::context &>(viennacl::traits::opencl_handle(V).context());

  const std::string NMF_MUL_DIV_KERNEL = "el_wise_mul_div";

  viennacl::linalg::opencl::kernels::nmf<NumericT>::init(ctx);

  vcl_size_t k = W.size2();
  conf.iters_ = 0;

  if (viennacl::linalg::norm_frobenius(W) <= 0)
    W = viennacl::scalar_matrix<NumericT>(W.size1(), W.size2(), NumericT(1), ctx);

  if (viennacl::linalg::norm_frobenius(H) <= 0)
    H = viennacl::scalar_matrix<NumericT>(H.size1(), H.size2(), NumericT(1), ctx);

  viennacl::matrix_base<NumericT> wn(V.size1(), k, W.row_major(), ctx);
  viennacl::matrix_base<NumericT> wd(V.size1(), k, W.row_major(), ctx);
  viennacl::matrix_base<NumericT> wtmp(V.size1(), V.size2(), W.row_major(), ctx);

  viennacl::matrix_base<NumericT> hn(k, V.size2(), H.row_major(), ctx);
  viennacl::matrix_base<NumericT> hd(k, V.size2(), H.row_major(), ctx);
  viennacl::matrix_base<NumericT> htmp(k, k, H.row_major(), ctx);

  viennacl::matrix_base<NumericT> appr(V.size1(), V.size2(), V.row_major(), ctx);

  NumericT last_diff = 0;
  NumericT diff_init = 0;
  bool stagnation_flag = false;

  for (vcl_size_t i = 0; i < conf.max_iterations(); i++)
  {
    conf.iters_ = i + 1;
    {
      hn = viennacl::linalg::prod(trans(W), V);
      htmp = viennacl::linalg::prod(trans(W), W);
      hd = viennacl::linalg::prod(htmp, H);

      viennacl::ocl::kernel & mul_div_kernel = ctx.get_kernel(viennacl::linalg::opencl::kernels::nmf<NumericT>::program_name(), NMF_MUL_DIV_KERNEL);
      viennacl::ocl::enqueue(mul_div_kernel(H, hn, hd, cl_uint(H.internal_size1() * H.internal_size2())));
    }
    {
      wn = viennacl::linalg::prod(V, trans(H));
      wtmp = viennacl::linalg::prod(W, H);
      wd = viennacl::linalg::prod(wtmp, trans(H));

      viennacl::ocl::kernel & mul_div_kernel = ctx.get_kernel(viennacl::linalg::opencl::kernels::nmf<NumericT>::program_name(), NMF_MUL_DIV_KERNEL);

      viennacl::ocl::enqueue(mul_div_kernel(W, wn, wd, cl_uint(W.internal_size1() * W.internal_size2())));
    }

    if (i % conf.check_after_steps() == 0)  //check for convergence
    {
      appr = viennacl::linalg::prod(W, H);

      appr -= V;
      NumericT diff_val = viennacl::linalg::norm_frobenius(appr);

      if (i == 0)
        diff_init = diff_val;

      if (conf.print_relative_error())
        Rcpp::Rcout << diff_val / diff_init << std::endl;

      // Approximation check
      if (diff_val / diff_init < conf.tolerance())
        break;

      // Stagnation check
      if (std::fabs(diff_val - last_diff) / (diff_val * NumericT(conf.check_after_steps())) < conf.stagnation_tolerance()) //avoid situations where convergence stagnates
      {
        if (stagnation_flag)    // iteration stagnates (two iterates with no notable progress)
          break;
        else
          // record stagnation in this iteration
          stagnation_flag = true;
      } else
        // good progress in this iteration, so unset stagnation flag
        stagnation_flag = false;

      // prepare for next iterate:
      last_diff = diff_val;
    }
  }
}

} //namespace opencl
} //namespace linalg
} //namespace viennacl

#endif /* VIENNACL_LINALG_OPENCL_NMF_OPERATIONS_HPP_ */
