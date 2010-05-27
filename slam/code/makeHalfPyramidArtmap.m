function [out_imgs] = makeHalfPyramidArtmap( texture, full_width, step_size )
    
% Create a half-pyramid self similar line artmap from an input exemplar
% texture.
%
% [out_imgs] = makeHalfPyramidArtmap( texture, full_width, step_size )
%   texture: the exemplar. Dimensions must be powers of 2.
%   full_width: the width of the base of the pyramid.
%   step_size: the pixel interval between levels of the pyramid.
%              Should be a power of 2 no smaller than the height of 
%              the texture, or the synthesis algorithm may break.

% The algorithm implemented by this code is described in:
%  "Self-Similar Texture for Coherent Line Stylization,"
%  Pierre Benard, Forrester Cole, Aleksey Golovinskiy, Adam Finkelstein,
%  NPAR 2010.
%
% Copyright (c) 2010 by Forrester Cole.
% All rights reserved.
 

    pad_factor = 2;
    texture = padImage(double(texture), pad_factor);
    nchan = size(texture,1);
    
    Nsc = log2(nchan) - 2;
    Nor = 4;
    Na = 9;
    synparams = textureAnalysis15D( texture, Nsc, Nor, Na );
    statg0 = synparams.pixelStats;
    mean0 = statg0(1); var0 =  statg0(2);

    noise = makeLinearFilteredNoise( size(texture), mean0, var0, 2 );
   
    half_width = full_width/2;
    sizes = half_width:step_size:full_width;
    out_imgs = cell(length(sizes),2);
    
    for i = 1:length(sizes)
        width = sizes(i);
        scaled_size = [nchan width];

        coarse_resized = imresize( noise{2}, scaled_size);
        fine_resized = imresize( [noise{2} noise{2}], scaled_size);
        
        alpha = (width - half_width) / (half_width);
        
        seed = fine_resized * alpha + coarse_resized * (1 - alpha);
        
        syn = textureSynthesis15D(synparams, seed, 15);
        
        syn = unpadImage(syn, pad_factor);
        seed = unpadImage(seed, pad_factor);
            
        out_imgs{i,1} = syn;
        out_imgs{i,2} = seed;
    end
  
    %showImgs(out_imgs);
end

function imgOut = padImage(imgIn, factor)
    oldHeight = size(imgIn, 1);
    newHeight = oldHeight*factor;
    imgOut = zeros(newHeight, size(imgIn, 2));
    imgOut(newHeight/2  - oldHeight/2 + 1: newHeight/2  + oldHeight/2, :) = imgIn;
end

function imgOut = unpadImage(imgIn, factor)
    oldHeight = size(imgIn, 1);
    newHeight = oldHeight / factor;
    imgOut = imgIn(oldHeight/2  - newHeight/2 + 1: oldHeight/2  + newHeight/2, :);
end
