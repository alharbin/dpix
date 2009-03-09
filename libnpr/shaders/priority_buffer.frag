uniform sampler2DRect segment_atlas;
uniform vec4  transfer_width;

uniform vec4  viewport;
uniform float max_width;

uniform mat4  inverse_projection;

varying vec2 atlas_position;
varying vec4 spine_position;
varying float path_id;

void main()
{
    float visibility = texture2DRect(segment_atlas, atlas_position).w;
    if (visibility < 0.001)
        discard;

    vec4 camera_pos_w = inverse_projection * spine_position;
    vec3 camera_pos = camera_pos_w.xyz / camera_pos_w.w;
    float focus = computeFocus( camera_pos, spine_position );
    float pixel_width = computeTransfer(transfer_width, focus);
    float width_scale = (pixel_width / max_width) * visibility;

    if (abs(mod(atlas_position.y, 1.0) - 0.5)*2.0 > width_scale)
        discard;

    vec4 color = vec4(idToColor(path_id), pixel_width/255.0);

    gl_FragColor = color;
}

