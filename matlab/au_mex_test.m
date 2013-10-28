
%% Test au_mex.h

disp('mexing');
mex('au_mex_example_1.cxx');

disp('testing');
A = rand(2,3);
B = rand(2,3);
O = au_mex_example_1(A, B);

au_test_equal O (A+B)

%% Check for leaks
disp('Check for leaks...');
mex('au_mex_test_leak.cxx');
!tasklist /FI "imagename eq matlab.exe"
for k=1:10
    for j=1:10000
        au_mex_test_leak();
    end
    !tasklist /NH /FI "imagename eq matlab.exe"
end
disp('Done. If memory increased continually, report bug at awful.codeplex.com.');
