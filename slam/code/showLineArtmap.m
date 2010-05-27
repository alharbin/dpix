function showLineArtmap( imgs )
% Visualize the levels of the line artmap.

% Copyright (c) 2010 by Forrester Cole.
% All rights reserved.
    numframes = size(imgs,1);
    
    figure;
    for i = 1:numframes
        for j = 1:2
            plotind = j + (i-1)*2;
            subplot(numframes, 2, plotind);
            
            imshow(imgs{i,j},[]);
        end
    end
end
