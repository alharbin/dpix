uniform sampler2DRect depth_buffer;
uniform sampler2DRect supersample_locations;

struct SupersampleParams
{
    float buffer_scale;
    float t_scale;
    float s_scale;
    float one_over_count;
    int sample_count;
};
uniform SupersampleParams ss_params;

float getDepthVisibility(vec3 screen_pos, vec3 screen_tangent)
{
    float visibility = 0.0;
    vec2 bitangent = vec2(-screen_tangent.y, screen_tangent.x);
    for (int i = 0; i < ss_params.sample_count; i++)
    {
        vec2 offset = texture2DRect(supersample_locations, 
                                    vec2(float(i)+0.5, 0.5)).xy;
        offset = offset - vec2(0.5, 0.5);
        offset = offset * vec2(ss_params.t_scale, ss_params.s_scale);

        vec2 position = screen_pos.xy * ss_params.buffer_scale + 
                        screen_tangent.xy * offset.x +
                        bitangent * offset.y;

        vec4 model_depth = texture2DRect(depth_buffer, position);

        if (screen_pos.z <= model_depth.z)
            visibility += ss_params.one_over_count;
    }

    return visibility;
}