/*****************************************************************************\

focus_common.glsl
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Common routines for stylized focus.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#define NPR_FOCUS_NONE 0
#define NPR_FOCUS_SCREEN 1
#define NPR_FOCUS_WORLD 2

uniform vec4              focus_transfer;
uniform vec2			  focus_2d_poa;
uniform vec3			  focus_3d_poa;
uniform float			  screen_aspect_ratio;
uniform float			  model_size;
uniform int     		  focus_mode;

// Transfer function vector is defined as (v0, v1, near, far)
float computeTransfer( vec4 transfer, float in_t )
{
    float alpha = clamp((in_t - transfer[2]) / (transfer[3] - transfer[2]), 0.0, 1.0);
    float interp = transfer[0] + (transfer[1] - transfer[0])*alpha;
    return interp;
}

vec4 colorFromFocus( float focus, vec4 transfer_desat, vec4 transfer_fade, 
                       vec4 background_color, vec4 in_color )
{
    // Desaturate first.
    float desat = computeTransfer(transfer_desat, focus);
    float max_channel = max(max(in_color[0], in_color[1]), in_color[2]);

    vec4 out_color;
    
    out_color = in_color + desat*(vec4(max_channel, max_channel, max_channel, 1) - in_color);

    // Fade out.
    float alpha = computeTransfer(transfer_fade, focus);
    float beta = 1.0 - alpha;

    out_color = out_color*beta + background_color*alpha;

    return out_color;
}

float computeFocus( vec3 camera_pos, vec4 clip_pos )
{
    float dist = 0.0;
    if (focus_mode == NPR_FOCUS_WORLD)
    {
        float dist_to_poa = length(focus_3d_poa - camera_pos);
        dist = dist_to_poa / (model_size * 2.0);
    }
    else if (focus_mode == NPR_FOCUS_SCREEN)
    {
        vec3 ndc = clip_pos.xyz / clip_pos.w;
        vec2 offset = focus_2d_poa - ndc.xy;
        offset.y /= screen_aspect_ratio;

        dist = length(offset) / 2.0;
    }

    float focus = computeTransfer(focus_transfer, dist);

    return focus; 
}