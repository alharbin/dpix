/*****************************************************************************\

float_depth_buffer.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Rendering depth to a deep floating-point buffer.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

varying vec4 vert_pos_clip;

void main()
{
    vec4 vert_pos_post_div = vert_pos_clip / vert_pos_clip.w;
    float clip_depth = vert_pos_post_div.z;
    clip_depth = 0.5 * (clip_depth + 1.0);

    // Hacky simulation of glPolygonOffset
    float dx = dFdx(clip_depth);
    float dy = dFdy(clip_depth);
    float maxchange = max(abs(dx), abs(dy));

    float depth_unit = 0.000001 * (1.0 + (1.0 - clip_depth));
    float depth_factor = 1.0;
    float depth_offset = depth_factor * maxchange + depth_unit;
    clip_depth += depth_offset;
    gl_FragData[0] = vec4(maxchange, clip_depth, clip_depth, clip_depth);
}
