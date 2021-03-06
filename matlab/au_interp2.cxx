// B = vgg_interp2(A, X, Y[, method[, oobv]])

// $Id: vgg_interp2.cxx,v 1.4 2008/03/30 19:25:28 ojw Exp $
// Rewritten by ojw 20/9/06
// awf added derivatives, 12/02/16

#include <math.h>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "au_mex.h"

enum method_t {
  nearest,
  linear,
  cubic
};

method_t get_method(char const* buffer);

template <class T>
bool try_cast_1(mlx_inputs& in, mlx_outputs& out);
  
template <class T, class Out_t>
bool try_cast_2(mlx_inputs& in, mlx_outputs& out);
  
template<class T, class U> 
void interp_nearest(mlx_array<U>& B, mlx_array<U>& B_grad, mlx_array<T>& A, mlx_array<mlx_double>& X, mlx_array<mlx_double>& Y, U oobv);

template<bool do_grad, class T, class U> 
void interp_linear(mlx_array<U>& B, mlx_array<U>& B_grad, mlx_array<T>& A, mlx_array<mlx_double>& X, mlx_array<mlx_double>& Y, U oobv);

template<bool do_grad, class T, class U> 
void interp_cubic(mlx_array<U>& B, mlx_array<U>& B_grad, mlx_array<T>& A, mlx_array<mlx_double>& X, mlx_array<mlx_double>& Y, U oobv);

void mlx_function(mlx_inputs& in, mlx_outputs& out)
{
  mlx_assert(in.nrhs >= 3 && in.nrhs <= 5);
  mlx_assert(out.nlhs <= 2);
  
//  mexPrintf("omp procs = %d\n", omp_get_num_procs());

  if (!(  try_cast_1<mlx_logical>(in, out) ||
          try_cast_1<mlx_uint8>(in, out) ||
          try_cast_1<mlx_uint16>(in, out) ||
          try_cast_1<mlx_int16>(in, out) ||
          try_cast_1<mlx_single>(in, out) ||
          try_cast_1<mlx_double>(in, out)))
    mexErrMsgTxt("Unhandled input type (first argument)");
}

template <class T>
bool try_cast_1(mlx_inputs& in, mlx_outputs& out)
{
  // Check input type, fail silently if no match
  if (!mlx_isa<T>(in[0]))
    return false;

  // Check output type
  if (in.nrhs < 5)
    // It's double 
    mlx_assert((try_cast_2<T, mlx_double>(in, out)));
  else {
    if (!(  try_cast_2<T, mlx_logical>(in, out) ||
            try_cast_2<T, mlx_uint8>(in, out) ||
            try_cast_2<T, mlx_uint16>(in, out) ||
            try_cast_2<T, mlx_single>(in, out) ||
            try_cast_2<T, mlx_double>(in, out)))
    mexErrMsgTxt("Onhandled output type (from oobv)");
  }
  return true;
} 
          
template <class T, class Out_t>
bool try_cast_2(mlx_inputs& in, mlx_outputs& out)
{
  // Get input, type should match already
  mlx_array<T> A(in[0]);
  
  // Get the out of bounds value (oobv), should match Out_t as checked above.
  Out_t oobv;
  if (in.nrhs >= 5) {
    mlx_scalar<Out_t> oobv_array(in[4], mlx_array_nothrow);
    if (!oobv_array) return false;
    oobv = oobv_array[0];
  } else
    oobv = (Out_t)mxGetNaN();
  
  mlx_array<mlx_double> X(in[1]);
  mlx_array<mlx_double> Y(in[2]);
  
  mlx_assert(X.size == Y.size);
  mlx_assert(X.size.n <= 2);

  // Get the interpolation method
  method_t method = linear;
  if (in.nrhs > 3) {
    mlx_string s(in[3]);
    method = get_method(s.c_str());
  }
  
  // Create output array
  int ndims = A.size.n;
  std::vector<int> out_dim_buf(ndims+1);
  out_dim_buf[0] = X.size[0];
  out_dim_buf[1] = X.size[1];
  int nchannels = 1;
  for (int i = 2; i < ndims; i++) {
    out_dim_buf[i] = A.size[i];
    nchannels *= A.size[i];
  }

  mlx_dims out_dims(ndims, out_dim_buf.data());
  mlx_make_array<Out_t> ans(out_dims);
  out[0] = ans;
  
  // If second output argument, return gradients.
  // Same dimensions with 2 planes.
  mlx_array<Out_t> B_grad;
  if (out.nlhs > 1) {
    out_dim_buf[ndims] = 2;
    mlx_dims out_grad_dims(ndims+1, out_dim_buf.data());
  
    out[1] = mlx_make_array<Out_t>(out_grad_dims);
    B_grad.init(out[1], false);
  }

  switch (method) {
    case nearest:
      interp_nearest(ans, B_grad, A, X, Y, oobv);
      break;
    case linear:
      if (B_grad)
        interp_linear<true>(ans, B_grad, A, X, Y, oobv);
      else
        interp_linear<false>(ans, B_grad, A, X, Y, oobv);
      break;
    case cubic:
      if (B_grad)
        interp_cubic<true>(ans, B_grad, A, X, Y, oobv);
      else
        interp_cubic<false>(ans, B_grad, A, X, Y, oobv);
      break;
  }
  
  return true;
}

// Function for correct rounding
// Add these to use numeric_limits class
#include <limits>
using namespace std;
template<class U, class T> 
inline U saturate_cast(T val)
{
  if (numeric_limits<U>::is_integer && !numeric_limits<T>::is_integer) {
    if (numeric_limits<U>::is_signed)
      return val > 0 ? (val > (T)numeric_limits<U>::max() ? numeric_limits<U>::max() : static_cast<U>(val + 0.5)) : (val < (T)numeric_limits<U>::min() ? numeric_limits<U>::min() : static_cast<U>(val - 0.5));
    else
      return val > 0 ? (val > (T)numeric_limits<U>::max() ? numeric_limits<U>::max() : static_cast<U>(val + 0.5)) : 0;
  }
  return static_cast<U>(val);
}

// Nearest neighbour
template<class T, class Out_t>
void interp_nearest(mlx_array<Out_t>& B, mlx_array<Out_t>& B_grad, 
        mlx_array<T>& A, mlx_array<mlx_double>& X, mlx_array<mlx_double>& Y, Out_t oobv)
{
  int num_points = X.numel();

  int h = A.size[0];
  int w = A.size[1];
  int col = A.numel() / (w*h);

  int end = num_points * col;
  int step = h * w;

  double dw = (double)w + 0.5;
  double dh = (double)h + 0.5;
  
  // For each of the interpolation points
  int i, j, k;
#pragma omp parallel for if (num_points > 1000) num_threads(omp_get_num_procs()) default(shared) private(i,j,k)
  for (i = 0; i < num_points; i++) {
    double Xi = X[i];
    double Yi = Y[i];
    
    if (Xi >= 0.5 && Xi < dw && Yi >= 0.5 && Yi < dh) {
      // Find nearest neighbour
      k = h * int(Xi-0.5) + int(Yi-0.5);
      //mexPrintf("%d: %g %g -> %g\n", i, Xi, Yi, A[k]);
      for (j = i; j < end; j += num_points, k += step)
        B[j] = saturate_cast<Out_t, T>(A[k]);
    } else {
      // Out of bounds
      for (j = i; j < end; j += num_points)
        B[j] = oobv;
    }
  }

  if (B_grad)
    mexErrMsgTxt("Gradient not implemented for nearest.  Would love to know what you mean...");
}

// Linear interpolation
template <bool do_grad, class T, class Out_t>
void interp_linear(mlx_array<Out_t>& B, mlx_array<Out_t>& B_grad, 
       mlx_array<T>& A, mlx_array<mlx_double>& X, mlx_array<mlx_double>& Y, Out_t oobv)
{
  int num_points = X.numel();

  int h = A.size[0];
  int w = A.size[1];
  int col = A.numel() / (w*h);
  
  int end = num_points * col;
  int step = h * w;

  double dw = (double)w;
  double dh = (double)h;

  // For each of the interpolation points
  int i, j, k;
#pragma omp parallel for if (num_points > 300) num_threads(omp_get_num_procs()) default(shared) private(i,j,k)
  for (i = 0; i < num_points; i++) {
    
    if (X[i] < 1 || Y[i] < 1 || X[i] > dw || Y[i] > dh) {
      // Out of bounds
      for (j = i; j < end; j += num_points) {
        B[j] = oobv;
        if (do_grad) {
          B_grad[j] = oobv;
          B_grad[j+end] = oobv;
        }
      }
      continue;
    }

    bool X_inside = X[i] < dw;
    bool Y_inside = Y[i] < dh;

    if (X_inside && Y_inside) {
      // Linearly interpolate
      int x = (int)X[i];
      int y = (int)Y[i];
      double u = X[i] - x;
      double v = Y[i] - y;
      k = h * (x - 1) + y - 1;
      for (j = i; j < end; j += num_points, k += step) {
        /*
          syms A00 A01 A10 A11 u v real
          out1 = A00 + (A01 - A00) * u;
          out2 = out1 + ((A10 - out1) + (A11 - A10) * u) * v;
          diff(out2, u)
          diff(out2, v)
         */
        double A00 = A[k];
        double A01 = A[k+h];
        double A10 = A[k+1];
        double A11 = A[k+h+1];
        double out1 = A00 + (A01 - A00) * u;
        double out2 = out1 + ((A10 - out1) + (A11 - A10) * u) * v;
        B[j] = saturate_cast<Out_t, double>(out2);
        if (do_grad) {
          // TODO: u,v = 0 means this returns one element of the subgradient
          //       Possibly better would be to return 0 if in the subgradient
          B_grad[j] = saturate_cast<Out_t, double>(A01 - A00 + v*(A00 - A01 - A10 + A11));
          B_grad[j+end] = saturate_cast<Out_t, double>(A10 - A00 + u*(A00 - A01 - A10 + A11));
        }
      }
    }
    else if (X_inside) {
      // The Y coordinate is on the boundary
      // Avoid reading outside the buffer to avoid crashes
      // Linearly interpolate along X
      int x = (int)X[i];
      double u = X[i] - x;
      k = h * x - 1;
      for (j = i; j < end; j += num_points, k += step) {
        B[j] = saturate_cast<Out_t, double>(A[k] + (A[k+h] - A[k]) * u);
        if (do_grad) {
          B_grad[j] = saturate_cast<Out_t, double>(A[k+h] - A[k]);
          B_grad[j+end] = 0;
        }
      }
    }
    else if (Y_inside) {
      // The X coordinate is on the boundary
      // Avoid reading outside the buffer to avoid crashes
      // Linearly interpolate along Y
      int y = (int)Y[i];
      double v = Y[i] - y;
      k = h * (w - 1) + y - 1;
      for (j = i; j < end; j += num_points, k += step) {
        B[j] = saturate_cast<Out_t, double>(A[k] + (A[k+1] - A[k]) * v);
        if (do_grad) {
          B_grad[j] = 0;
          B_grad[j+end] = saturate_cast<Out_t, double>(A[k+1] - A[k]);
        }
      }
    } else {
      // The X and Y coordinates are on the boundary
      // Avoid reading outside the buffer to avoid crashes
      // Output the last value in the array
      k = h * w - 1;
      for (j = i; j < end; j += num_points, k += step) {
        B[j] = saturate_cast<Out_t, double>(A[k]);
        if (do_grad) {
          B_grad[j] = 0;
          B_grad[j+end] = 0;
        }
      }
    }
  }
}

// Hermite cubic spline interpolation
template<bool do_grad, class T, class Out_t>
void interp_cubic(mlx_array<Out_t>& B, mlx_array<Out_t>& B_grad, 
       mlx_array<T>& A, mlx_array<mlx_double>& X, mlx_array<mlx_double>& Y, Out_t oobv)
{
  int num_points = X.numel();

  int h = A.size[0];
  int w = A.size[1];
  int col = A.numel() / (w*h);

  int end = num_points * col;
  int step = h * w;

  double dw = (double)w - 1;
  double dh = (double)h - 1;

  // For each of the interpolation points
  int i, j, m, n;
#pragma omp parallel for if (num_points > 100) num_threads(omp_get_num_procs()) default(shared) private(i,j,m,n)
  for (i = 0; i < num_points; i++) {    
    if (X[i] >= 2 && X[i] < dw && Y[i] >= 2 && Y[i] < dh) {
      // Bicubicly interpolate
      int x = (int)X[i];
      int y = (int)Y[i];
      double u = X[i] - x;
      double v = Y[i] - y;
      double u2 = u * u;
      double v2 = v * v;
      double u3 = u2 * u;
      double v3 = v2 * v;
      int k = h * (x - 2) + y - 2;
      double b[4];
      double bv[4]; // db_dv
      for(j = i; j < end; j += num_points, k += step) {
        for (m = 0, n = k; m < 4; m++, n += h) {
          double c0 = (double)A[n+0];
          double c1 = (double)A[n+1];
          double c2 = (double)A[n+2];
          double c3 = (double)A[n+3];
          double a = (c3 + c1) - (c2 + c0);
          b[m] = v3 * a + v2 * ((c0 - c1) - a) + v * (c2 - c0) + c1;
          if (do_grad)
            bv[m] =  3*v2*a + 2*v*((c0 - c1) - a) + (c2 - c0);
        }
        double a = (b[3] + b[1]) - (b[2] + b[0]);
        double av = do_grad ? (bv[3] + bv[1]) - (bv[2] + bv[0]) : 0;

        B[j] = saturate_cast<Out_t, double>
                (u3 * a + u2 * ((b[0] - b[1]) - a) + u * (b[2] - b[0]) + b[1]);
        if (do_grad) {
          B_grad[j] = saturate_cast<Out_t, double>
                (3*u2 * a + 2*u* ((b[0] - b[1]) - a) + (b[2] - b[0]));
          B_grad[j+end] = saturate_cast<Out_t, double>
                (u3 * av + u2 * ((bv[0] - bv[1]) - av) + u * (bv[2] - bv[0]) + bv[1]);
        }
      }
    } else {
      // Out of bounds
      for (j = i; j < end; j += num_points)
        B[j] = oobv;
    }
  }
}

method_t get_method(char const* buffer)
{
  // Remove '*' from the start
  if (*buffer == '*')
    ++buffer;

  // Remove 'bi' from the start
  if (buffer[0] == 'b' && buffer[1] == 'i')
    buffer += 2;

  switch (*buffer) {
    case 'n':
      return nearest;
    case 'l':
      return linear;
    case 'c':
      return cubic;
  }

  mexErrMsgTxt("Unsupported interpolation method");
  return nearest; // suppress warning.
}

