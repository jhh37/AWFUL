
%% SCRIPT  au_prmat_test

A = [
  .123e-100 -.123e-100
  .123e-10 -.123e-10
  .123e-5 -.123e-5
  .123e-4 -.123e-4
  .123e-3 -.123e-3
  .123e-2 -.123e-2
  .123e-1 -.123e-1
  .2 -.3;
  9.123 -9.123;
  45.123 -45.123;
  245.123 -245.123;
  1245.123 -1245.123;
  12451.23 -12451.23;
  124512.3 -124512.3;
  1245123. -1245123.;
  1.2451e7 -1.2451e7;
  1.2451e8 -1.2451e8;
  1.2451e9 -1.2451e9;
  1.245e10 -1.245e10;
  1.245e11 -1.245e11;
  1.245e111 -1.245e111;
  ];


B = full(sprand(5,3, .4));
C = diag([2 nan 4 -inf]);
au_prmat(B,A,C,A)
%au_prmat(A,B,C, 'colwidth', 2)
