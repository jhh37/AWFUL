
au_test_begin au_interp2

%%
A = randn(2,3,2,2)*3;

Xrange = .1:.7:2.3;
Yrange = .2:.45:4;
[X,Y] = meshgrid(Xrange, Yrange);

B = au_interp2(A, X, Y, 'n');
vgg_B = vgg_interp2(A, X, Y, 'n');

au_test_equal B vgg_B


%
for oobv = {true, single(11.0), double(11.0), uint8(13), uint16(15)}
  s = oobv{1};
  B = au_interp2(A, X, Y, 'n', s);
  au_test_equal class(B) class(s)

  vgg_B = vgg_interp2(A, X, Y, 'n', s);
  au_test_equal B vgg_B
end


B(:,:,1)
vgg_B(:,:,1)

au_test_end au_interp2


%% Test grad
A = randn(5,5,2,3)*3;

Xrange = 1+[ 1.1 1.1 1.7 1.7 1.0 1.9 ];
Yrange = 1+[ 1.3 1.7 1.1 1.8 1.9 1.0 ];
Xrange = Xrange(5);
Yrange = Yrange(5);

[X,Y] = meshgrid(Xrange, Yrange);

for OP = ['c','l']
  OP
  [B, DB] = au_interp2(A, X, Y, OP);
  
  delta = 1e-7;
  dbdx_fd = (au_interp2(A, X+delta, Y, OP) - B)./delta;
  dbdy_fd = (au_interp2(A, X, Y+delta, OP) - B)./delta;
  DB_fd = cat(length(size(DB)), dbdx_fd, dbdy_fd);
  
  au_test_equal DB DB_fd 1e-5
end

%%
rand('seed', 2.0)
Z = uint8(rand(5) * 64);
R = 2:.3:4; 
[xx,yy] = meshgrid(R);
hold off; 
image(1:5.5,1:5,Z);
hold on
image(R,R,au_interp2(Z,xx,yy,'l'));
axis equal
colormap prism

%%
disp('** TIMIMGS **');
N=1000;
ToNS = N * numel(xx) / 1e9;
for cast = {@single, @double}
  for func = {@au_interp2, @au_interp2_omp, @vgg_interp2, @interp2, }
    tic
    for k=1:N
      M1 = func{1}(cast{1}(Z), xx, yy, 'l', cast{1}(1.0));
    end
    fprintf('Etime (%s, %s) = %.1fnsec\n', ...
      func2str(cast{1}), func2str(func{1}), toc/ToNS);
  end
end
