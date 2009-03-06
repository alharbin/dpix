uniform vec4              transfer_focus;
uniform vec4              transfer_desat;
uniform vec4              transfer_fade;
uniform vec4              background_color;

uniform float			  model_size;

// camera focus params
uniform float			  focal_length;
uniform vec3			  c_poa;

// screen focus params
uniform vec2			  s_poa;
uniform float			  aspect_ratio;

// world focus params
uniform vec3			  w_poa;

#define NPR_FOCUS_NONE 0
#define NPR_FOCUS_CAMERA 1
#define NPR_FOCUS_WORLD 2
#define NPR_FOCUS_SCREEN 3
#define NPR_FOCUS_OBJECT 4
#define NPR_FOCUS_CONFIDENCE 5
#define NPR_FOCUS_COLOR 6

uniform int				  focus_mode;
uniform int				  light_mode;

// interpolated values
varying vec3			  vert_vec_light;
varying vec3			  vert_normal_camera;
varying vec3			  vert_pos_camera;
varying vec3			  vert_pos_world;
varying vec4			  vert_pos_clip;

// cutaway
#define NPR_CUTAWAY_NONE 0
#define NPR_CUTAWAY_DISCARD 1
uniform int cutawayMode;
uniform sampler2D texCutaway;
uniform sampler2D texCutawayDilated;
varying vec4 cutawayTexCoordH;

void main()
{
    vec3 backNormal = vec3(0, 0, 0);
    if (cutawayMode == NPR_CUTAWAY_DISCARD) {
        vec2 cutawayTexCoord = cutawayTexCoordH.xy / cutawayTexCoordH.w;
        if (gl_FrontFacing) {
            vec4 cutawayValue = texture2D(texCutaway, cutawayTexCoord);
            if (gl_FragCoord.z < cutawayValue.w)
                discard;
        }
        else {
            vec4 cutawayValue = texture2D(texCutawayDilated, cutawayTexCoord);
            backNormal = cutawayValue.xyz;
            if (gl_FragCoord.z < cutawayValue.w)
                discard;
        }
    }
    
    float focus = 0.0;
    if (focus_mode == NPR_FOCUS_WORLD)
    {
        float dist_to_poa = length(c_poa - vert_pos_camera);
        float norm_dist = dist_to_poa / model_size;
        focus = computeTransfer(transfer_focus, norm_dist);
    }
    else if (focus_mode == NPR_FOCUS_SCREEN)
    {
        vec3 clip = vert_pos_clip.xyz / vert_pos_clip.w;
        vec2 offset = s_poa - clip.xy;
        offset.y /= aspect_ratio;

        float dist = length(offset) / 2.0;

        focus = computeTransfer(transfer_focus, dist);
    }
    
    vec4 out_col = vec4(0,0,0,0);

    LightingValues light_vals;
    light_vals.light_vec = vert_vec_light;
    if (gl_FrontFacing)
        light_vals.normal = vert_normal_camera;
    else
        light_vals.normal = backNormal;
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

    out_col.a = float(!gl_FrontFacing);

    gl_FragColor = out_col;
}
