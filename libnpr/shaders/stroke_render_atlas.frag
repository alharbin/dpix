uniform sampler2DRect segment_atlas;

uniform mat4  inverse_projection;

varying vec2 atlas_position;
varying vec4 spine_position;
varying float path_start;

void main()
{
    vec4 camera_pos_w = inverse_projection * spine_position;
    vec3 camera_pos = camera_pos_w.xyz / camera_pos_w.w;

    float visibility = texture2DRect(segment_atlas, atlas_position).w;
    float focus = computeFocus(camera_pos, spine_position);

    vec4 final_color = computePenColor(visibility, focus, gl_TexCoord[0].xy);

    gl_FragColor = final_color;
    /*vec4 color = colorWheel(fmod(path_start*2.0, 3.14));
    color[3] = visibility;
    gl_FragColor = color;*/
}

