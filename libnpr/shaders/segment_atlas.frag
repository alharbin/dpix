/*****************************************************************************\

segment_atlas.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Standard fragment program that writes visibility into the segment atlas.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform vec4 viewport;

varying vec4 sample_clip_pos;
varying float path_id;

void main()
{
    // Convert from clip to window coordinates.
    vec3 sample_window_pos, sample_window_tangent;
    vec3 sample_post_div = sample_clip_pos.xyz / sample_clip_pos.w;

    sample_window_pos.xy = (sample_post_div.xy + vec2(1.0,1.0)) * 0.5 * viewport.zw;
    sample_window_pos.z = 0.5 * (sample_post_div.z + 1.0);

    sample_window_tangent = normalize(dFdx(sample_window_pos));

    // Get the visibility and id for this sample.
    float visibility = getDepthVisibility(sample_window_pos, sample_window_tangent);

    float strength = visibility;
    vec3 id_color = idToColor(path_id);

    gl_FragData[0] = vec4(id_color, strength);
}
