/*****************************************************************************\

median_filter_atlas.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A median filter for the segment atlas.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2DRect source_buffer;

const int MAX_SAMPLES = 9;
float sample_array[MAX_SAMPLES];
int sample_count = 0;

#define swap(a, b)				temp = a; a = min(a, b); b = max(temp, b);

void bubbleSortSamples()
{
    float temp;
    for (int i = 0; i < sample_count; i++)
    {
        for (int j = 0; j < sample_count-1; j++)
        {
            swap(sample_array[i], sample_array[j]);
        }
    }
}

void main()
{
    vec2 center_coord = gl_FragCoord.xy;
    vec4 center_sample = texture2DRect(source_buffer, center_coord);
    
    if (!idsEqual(center_sample.rgb, vec3(0.0,0.0,0.0)))
    {
        float to_left = floor((float(MAX_SAMPLES))/2.0);
        for (int i = 0; i < MAX_SAMPLES; i++)
        {
            vec2 offset = vec2(float(i) - to_left, 0.0);
            vec4 sample = texture2DRect(source_buffer, center_coord + offset);

            if (idsEqual(sample.rgb, center_sample.rgb))
            {
                sample_array[sample_count] = sample.w;
                sample_count++;
            }
        }

        bubbleSortSamples();
    }

    float median = sample_array[sample_count / 2];

    vec4 filtered = vec4(center_sample.rgb, median);

    gl_FragData[0] = filtered;
}
