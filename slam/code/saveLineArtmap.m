function saveLineArtmap( imgs, out_name )

% Write the line artmap to disk. One image is generated per level, and a
% '3dt' text file is generated listing the number and name of each artmap
% texture.
%
% saveLineArtmap( imgs, out_name )
%   imgs: the artmap structure, as returned from makeHalfPyramidArtmap
%   out_name: the prefix for writing the texture. 
%           Example: out_name = 'myartmap' will produce 'myartmap.3dt'
%           and 'myartmap001.png', etc.

% Copyright (c) 2010 by Forrester Cole.
% All rights reserved.

    stretch_images = 1;

    num_maplevels = size(imgs,1);
    max_size = size(imgs{1,1},2);
    for i = 2:num_maplevels
        max_size = max(size(imgs{i,1},2),max_size);
    end
    
    file = fopen(sprintf('%s.3dt', out_name), 'w');
    fprintf(file, '%d\n', num_maplevels);
    
    for i = 1:num_maplevels
        
        img = imgs{i,1};
        if stretch_images
            img = imresize(img, [size(img,1), max_size]);
            filename = sprintf('%s%03d.png', out_name, i);
        else
            filename = sprintf('%s%03d.png', out_name, size(img,2));
        end
        imwrite(cast(img, 'uint8'), filename);
        
        fprintf(file, '%s\n', filename);
    end
    
    fclose(file);
    
end
