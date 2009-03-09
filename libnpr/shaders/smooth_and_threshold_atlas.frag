uniform sampler2DRect source_buffer;
uniform float overshoot_offset;

// Transfer function vector is defined as (v0, v1, near, far)
float computeTransfer( vec4 transfer, float in_t )
{
    float alpha = clamp((in_t - transfer[2]) / (transfer[3] - transfer[2]), 0.0, 1.0);
    float interp = transfer[0] + (transfer[1] - transfer[0])*alpha;
    return interp;
}

void main()
{
    vec2 center_coord = gl_FragCoord.xy;
    vec4 center_sample = texture2DRect(source_buffer, center_coord);

    float sample_count = 0.0;
    float sample_sum = 0.0;
    for (float i = -6.0; i <= 6.0; i++)
    {
        vec2 offset = vec2(i, 0.0);
        vec4 sample = texture2DRect(source_buffer, center_coord + offset);

        if (idsEqual(sample.rgb, center_sample.rgb))
        {
            sample_count++;
            sample_sum += sample.w;
        }
    }

    float short_rolloff = min(sample_count / 4.0, 1.0);

    float smoothed = short_rolloff * sample_sum / sample_count;

    vec4 smooth_step_up = vec4(0.0, 1.0, 0.0 + overshoot_offset, 0.2 + overshoot_offset);
    
    float thresholded_up = computeTransfer(smooth_step_up, smoothed);
    float thresholded = thresholded_up;

    vec4 out_color = vec4(center_sample.rgb, thresholded);

    gl_FragData[0] = out_color;
}
