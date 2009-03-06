uniform sampler2DRect segment_atlas;

uniform sampler2D pen_texture;
uniform sampler2D secondary_pen_texture;

uniform vec4 pen_color;
uniform vec4 secondary_pen_color;
uniform float use_secondary_color;

varying vec2 atlas_position;
varying float path_start;

void main()
{
    float visibility = texture2DRect(segment_atlas, atlas_position).w;

    float pen_alpha = texture2D(pen_texture, gl_TexCoord[0].xy).a * pen_color.a;
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
    /*vec4 color = colorWheel(fmod(path_start*2.0, 3.14));
    color[3] = visibility;
    gl_FragColor = color;*/
}

