function [out] = makeLinearFilteredNoise( in_size, mean, var, levels )

% Make 1.5D filtered noise at multiple pyramid levels with the specified
% mean and variance.
%
% [out] = makeLinearFilteredNoise( in_size, mean, var, levels )
%   in_size: the size of the output noise images.
%   mean, var: the mean and variances of the output.
%   levels: the number of filter levels to apply. Each filter (roughly) doubles
%           in width, so if levels = 3 then the noise will be convolved with
%           a 3, 5, and 9 pixel wide filter. 

% Copyright (c) 2010 by Forrester Cole.
% All rights reserved.

    gaussian = normpdf(-3:0.01:3,0,1);
    
    % make gaussian white noise
    randn('state',0);
    rand('state',0);
	ntex = rand(in_size);
    
    ntex = [ntex ntex ntex];
    for level = 1:levels
        filtsize = 2^level + 1;
        gaussianfilt = imresize( gaussian, [1, filtsize] );
        gaussianfilt = gaussianfilt / sum(gaussianfilt);

        filt = conv2(ntex, gaussianfilt);
        
         %renormalize the filtered version
        shape = size(filt);
        rs = reshape( filt, 1, numel(filt) );
        rs = histoMatch(rs, ones(100,1));
        ntex = reshape( rs, shape );
        
        center = size(filt) * 0.5;
        offset = in_size * 0.5;
        filt = filt(:, (center(2)-offset(2)):(center(2)+offset(2)-1));
        
        filtered{level} = filt;
    end
    out = filtered;
end
