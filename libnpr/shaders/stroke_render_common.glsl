/*****************************************************************************\

stroke_render_common.glsl
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Common functions for stroke rendering (e.g., computing pen color)

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler3D vis_focus_texture;
uniform sampler3D vis_defocus_texture;
uniform sampler3D invis_focus_texture;
uniform sampler3D invis_defocus_texture;

uniform vec4 vis_focus_color;
uniform vec4 vis_defocus_color;
uniform vec4 invis_focus_color;
uniform vec4 invis_defocus_color;

uniform vec4 color_transfer;
uniform vec4 opacity_transfer;
uniform vec4 texture_transfer;

vec4 computePenColor( float visibility, float focus, vec2 tex_coord )
{
    vec3 tex_coord_3d = vec3(tex_coord,0.0);
    float vis_focus_pen = texture3D(vis_focus_texture, tex_coord_3d).r;
    float invis_focus_pen = texture3D(invis_focus_texture, tex_coord_3d).r;
    float vis_defocus_pen = texture3D(vis_defocus_texture, tex_coord_3d).r;
    float invis_defocus_pen = texture3D(invis_defocus_texture, tex_coord_3d).r;

    float color_mix = computeTransfer(color_transfer, focus);
    float opacity_mix = computeTransfer(opacity_transfer, focus);
    float texture_mix = computeTransfer(texture_transfer, focus);

    float mixed_vis_pen = mix(vis_focus_pen, vis_defocus_pen, texture_mix);
    float mixed_invis_pen = mix(invis_focus_pen, invis_defocus_pen, 
                                texture_mix);
    float mixed_pen = mix(mixed_invis_pen, mixed_vis_pen, visibility);

    float mixed_vis_alpha = mix(vis_focus_color.a, vis_defocus_color.a, 
                                opacity_mix);
    float mixed_invis_alpha = mix(invis_focus_color.a, invis_defocus_color.a, 
                                  opacity_mix);
    float mixed_alpha = mix(mixed_invis_alpha, mixed_vis_alpha, visibility);

    vec4 mixed_vis_color = mix(vis_focus_color, vis_defocus_color, color_mix);
    vec4 mixed_invis_color = mix(invis_focus_color, invis_defocus_color, 
                                 color_mix);
    vec4 mixed_color = mix(mixed_invis_color, mixed_vis_color, visibility);

    return vec4(mixed_color.rgb, mixed_pen * mixed_alpha);
}
