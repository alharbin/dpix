/*****************************************************************************\

stroke_render_spine.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Fragment shader for spine-test visibility stroke rendering. 

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform mat4  inverse_projection;
uniform vec4 viewport;

uniform float check_at_spine;

varying vec4 spine_position;
varying vec3 spine_tangent;
varying vec2 tex_coord;

void main(void)
{
    vec3 spine_window = clipToWindow(spine_position, viewport);
    vec4 camera_pos_w = inverse_projection * spine_position;
    vec3 camera_pos = camera_pos_w.xyz / camera_pos_w.w;

    vec3 test_position = check_at_spine * spine_window + 
                         (1.0 - check_at_spine) * gl_FragCoord.xyz;
    float visibility = getDepthVisibility(test_position, spine_tangent);
    float focus = computeFocus( camera_pos, spine_position);

    vec4 final_color = computePenColor( visibility, focus, tex_coord);

    gl_FragColor = final_color;
}
