uniform sampler2DRect depth_buffer;
uniform sampler2DRect supersample_locations;
uniform SupersampleParams ss_params;

uniform sampler2D pen_texture;
uniform sampler2D secondary_pen_texture;

uniform vec4 pen_color;
uniform vec4 secondary_pen_color;
uniform float use_secondary_color;

uniform vec4 viewport;

uniform float check_at_spine;

in vec4 spine_position;
in vec3 spine_tangent;
noperspective in vec2 tex_coord;

void main(void)
{
    vec3 spine_window = clipToWindow(spine_position, viewport);

    vec3 test_position = check_at_spine * spine_window + 
                         (1.0 - check_at_spine) * gl_FragCoord.xyz;
    float visibility = getDepthVisibility(test_position, spine_tangent,
                                          depth_buffer, supersample_locations,
                                          ss_params);

    /*if (abs(clip_z) > 1.0)
        visibility = 0.0;*/

    float pen_alpha = texture2D(pen_texture, tex_coord).a * pen_color.a;
    vec4 final_color;
    if (use_secondary_color == 0.0)
    {
        final_color = vec4(pen_color.rgb, pen_alpha * visibility);
    }
    else
    {
        float secondary_pen_alpha = 
            texture2D(secondary_pen_texture, gl_TexCoord[0].xy).a * 
            secondary_pen_color.a;

        vec4 mixed_color = pen_color * visibility + 
                           secondary_pen_color * (1.0 - visibility);
        float mixed_alpha = pen_alpha * visibility + 
                            secondary_pen_alpha * (1.0 - visibility);

        final_color = vec4(mixed_color.rgb, mixed_alpha);
    }

    gl_FragColor = final_color;
}