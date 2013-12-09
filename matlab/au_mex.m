% Help for au_mex.h
% See https://awful.codeplex.com/wikipage?title=au_mex.h
%
% -- mlx_inputs --
% Manage the "rhs" values on a mexFunction
%   mlx_inputs ins(nrhs, prhs);
%   ins[0] .. // range-checked access
%
% -- mlx_outputs --
%   mlx_outputs outs(nlhs, plhs);
%   outs[0] = // range-checked assignment from mlx_cast<>
%
% -- mlx_cast --
%   mlx_cast<double> A(ins[0]); // Assign from inputs[0]
%   mlx_cast<double> A(prhs[0]); // Assign from prhs[0]
%   mlx_assert(A.rows == 3); // Access rows
%   mlx_assert(A.numel() == 3); // Access total number of elements
%   mlx_assert(A.size[2] == 3); // Access 3rd dimension
%   double const* p = A.data;
%   A[0] = 1.0; // 0-based linear access
%   A.put1(1) = 1.0;  // 1-based put
%
% -- mlx_make_array : mlx_cast --
%   Constructor for mlx_cast of form:
%   mlx_make_array<mlx_uint8> img(480, 640);
%   mlx_make_array<mlx_uint8> img(mlx_dims(...));

help au_mex

