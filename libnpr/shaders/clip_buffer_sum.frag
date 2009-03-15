/*****************************************************************************\

clip_buffer_sum.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A fragment program that computes the segment atlas offsets using 
an exclusive scan (parallel all-prefix sum) operation.

The real-valued segment length is also summed separately
from the number of samples to allow texture parameterization
based on arclength.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2DRect last_pass_buf;
uniform float step_size;
uniform float buffer_width;

void main()
{
    vec2  this_coord = floor(gl_FragCoord.xy);
    float this_index = coordinateToIndex(this_coord, buffer_width);

    vec4  last_texel = texture2DRect(last_pass_buf, this_coord);
    float this_samples = unpackSampleOffset(last_texel);
    float original_samples = unpackNumSamples(last_texel);
    float this_length = unpackArcLengthOffset(last_texel);
    float original_length = unpackArcLength(last_texel);

    // We want the zero indexed all-prefix-sum, not the one indexed.
    // That is, the value of the 0th element should be 0, not the value 
    // of the first sum. 
    // So we shift the index by 1 on the first pass (there's probably a better way...)
    if (step_size < 1.00001)
    {
        this_index -= 1.0;
        if (this_index >= 0.0)
        {
            this_coord = indexToCoordinate(this_index, buffer_width);
            vec4 this_texel = texture2DRect(last_pass_buf, this_coord);
            this_samples = unpackSampleOffset(this_texel);
            this_length = unpackArcLengthOffset(this_texel);
        }
        else
        {
            this_samples = 0.0;
            this_length = 0.0;
        }
    }

    float step_samples = 0.0;
    float step_length = 0.0;

    float step_index = this_index - step_size;
    if (step_index >= 0.0)
    {
        vec2 step_coord = indexToCoordinate(step_index, buffer_width);
        vec4 step_texel = texture2DRect(last_pass_buf, step_coord);
        step_samples = unpackSampleOffset(step_texel);
        step_length = unpackArcLengthOffset(step_texel);
    }

    float out_samples = this_samples + step_samples;
    float out_length = this_length + step_length;

    gl_FragColor = packOffsetTexel(original_samples, original_length, 
                                   out_samples, out_length);
}




