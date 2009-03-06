uniform vec4 viewport;

uniform sampler2DRect depth_buffer;
uniform sampler2DRect supersample_locations;
uniform SupersampleParams ss_params;

varying vec4 sample_clip_pos;
varying float path_id;

void main()
{
    // convert from clip to window coordinates
    vec3 sample_window_pos, sample_window_tangent;
    vec3 sample_post_div = sample_clip_pos.xyz / sample_clip_pos.w;

    sample_window_pos.xy = (sample_post_div.xy + vec2(1.0,1.0)) * 0.5 * viewport.zw;
    sample_window_pos.z = 0.5 * (sample_post_div.z + 1.0);

    sample_window_tangent = normalize(dFdx(sample_window_pos));

    // compute visibility for this sample
    float visibility = getDepthVisibility( sample_window_pos, sample_window_tangent, 
                                          depth_buffer, supersample_locations, 
                                          ss_params );

    float strength = visibility;
    vec3 id_color = idToColor(path_id);

    gl_FragData[0] = vec4(id_color, strength);
}
