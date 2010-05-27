% RES = histoMatch(MTX, N, X)
%
% Modify elements of MTX so that normalized histogram matches that
% specified by vectors X and N, where N contains the histogram counts
% and X the histogram bin positions (see histo).

% Eero Simoncelli, 7/96.

% Modified to take different arguments by Forrester Cole, May 17 2010.

function res = histoMatch(from, to)
[N, X] = hist(to, round(length(to)/ 8));

res = matchToHist(from, N, X);

function res = matchToHist(mtx, N, X)

if(min(N) < 0)
    error('Negative values in histogram')
end

extraN = mean(N)/(1e8);
N = N(:)';
X = X(:)';
N = N + extraN;   %% HACK: no empty bins ensures nC strictly monotonic

nStep = X(2) - X(1);
nC = [0, cumsum(N)]/sum(N);
nX = [X(1)-nStep/2, X+nStep/2];

mtxLength = numel(mtx);
oC = 1/(mtxLength + 1) * [1 : mtxLength];

nnX = interp1(nC, nX, oC, 'linear');

[Y, I] = sort(mtx);
res = mtx;
res(I) = nnX;
