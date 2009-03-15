/*****************************************************************************\

stroke_render_atlas.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Stroke rendering using the segment atlas.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2DRect visibility_atlas;
uniform sampler2DRect priority_atlas;

uniform mat4  inverse_projection;

varying vec2 atlas_position;
varying vec4 spine_position;
varying float path_start;

void main()
{
    vec4 camera_pos_w = inverse_projection * spine_position;
    vec3 camera_pos = camera_pos_w.xyz / camera_pos_w.w;

    float visibility = texture2DRect(visibility_atlas, atlas_position).w;
    float elision = texture2DRect(priority_atlas, atlas_position).w;
    float focus = computeFocus(camera_pos, spine_position);

    vec4 pen_color = computePenColor(visibility, focus, gl_TexCoord[0].xy);
    pen_color.a *= elision;

    gl_FragColor = pen_color;
    /*vec4 color = colorWheel(fmod(path_start*2.0, 3.14));
    color[3] = visibility;
    gl_FragColor = color;*/
}

