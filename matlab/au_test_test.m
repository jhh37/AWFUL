function au_test_test

% Unit "test" for au_test_equal
% Doesn't really test, as I can't capture the fprintf output.

% awf, may13

fprintf(2, 'au_test_test: these should pass\n');
au_test_equal 16+1 17
au_test_equal 0 sin(0)
au_test_equal 0.01 sin(0.01) 0.01
syms w x
au_test_equal w w
au_test_equal w/w 1

au_test_equal 0 sin(1)
disp('au_test_test: that should have failed');
au_test_equal w/w x
disp('au_test_test: that should have failed');
au_test_equal('[0 1]', 'sin([0 1])');
disp('au_test_test: that should have failed');

disp('** Testing au_test_regexp **')
au_test_regexp(sprintf('%.3f', 4), '\d+\.\d')
disp('au_test_test: That should have passed');
au_test_regexp(sprintf('%.0f', 4), '\d+\.\d')
disp('au_test_test: That should have failed');

% Test a long string
longstr = mat2str(rand(4,4), 5);
au_assert length(longstr)>80
au_test_regexp(longstr, '\d+\.\d')
disp('au_test_test: That should have passed, and truncated the string');

% Test printing
au_test_equal rand(3,4) rand(3,4,5) 1e-5 print