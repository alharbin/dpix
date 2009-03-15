/*****************************************************************************\

polygon_render.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Renders the polygons of the scene with stylized focus.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform vec4              transfer_desat;
uniform vec4              transfer_fade;
uniform vec4              background_color;

uniform int				  light_mode;

// interpolated values
varying vec3			  vert_vec_light;
varying vec3			  vert_normal_camera;
varying vec3			  vert_pos_camera;
varying vec3			  vert_pos_world;
varying vec4			  vert_pos_clip;

void main()
{
    float focus = computeFocus( vert_pos_camera, vert_pos_clip );
    
    vec4 out_col = vec4(0,0,0,0);

    LightingValues light_vals;
    light_vals.light_vec = vert_vec_light;
    light_vals.normal = vert_normal_camera;
    light_vals.light_amb_col = gl_LightSource[0].ambient;
    light_vals.light_diff_col = gl_LightSource[0].diffuse;
    light_vals.light_spec_col = gl_LightSource[0].specular;
    light_vals.mat_amb_col = gl_FrontMaterial.diffuse;
    light_vals.mat_diff_col = gl_FrontMaterial.diffuse;
    light_vals.mat_spec_col = gl_FrontMaterial.specular;
    light_vals.mat_shininess = gl_FrontMaterial.shininess;

    if (light_mode == 0)
    {
        out_col = lambertianLight( light_vals, focus );
    }
    else if (light_mode == 1)
    {
        out_col = frontBackLight( light_vals, focus );
    }
    else if (light_mode == 2)
    {
        out_col = warmCoolLight( light_vals, focus );
    }
    else
    {
        out_col = light_vals.mat_diff_col;
    }
				      
    out_col *= gl_Color;
				      
    out_col = colorFromFocus( focus, transfer_desat, transfer_fade, background_color, out_col );
    
    out_col.a = focus;

    gl_FragColor = out_col;
}
