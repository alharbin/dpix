/*****************************************************************************\

bilateral_filter_atlas.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A bilateral filter for the segment atlas.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2DRect source_buffer;

void main()
{
    vec2 center_coord = gl_FragCoord.xy;
    vec4 center_sample = texture2DRect(source_buffer, center_coord);

    float bilateral_threshold = 1.0;//0.5;
    float sample_count = 0.0;
    float sample_sum = 0.0;
    for (float i = -4.0; i <= 4.0; i++)
    {
        vec2 offset = vec2(i, 0.0);
        vec4 sample = texture2DRect(source_buffer, center_coord + offset);

        float difference = abs(sample.w - center_sample.w);
        if (idsEqual(sample.rgb, center_sample.rgb) &&
            difference < bilateral_threshold)
        {
            sample_count++;
            sample_sum += sample.w;
        }
    }

    sample_sum = sample_sum * max(min(sample_count/4.0 - 0.2, 1.0), 0.0);

    vec4 averaged = vec4(center_sample.rgb, sample_sum / sample_count);

    gl_FragData[0] = averaged;
}
