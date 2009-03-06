vec3 clipToWindow(vec4 clip, vec4 viewport)
{
    vec3 post_div = clip.xyz / clip.w;
    vec2 xypos = (post_div.xy + vec2(1.0,1.0)) * 0.5 * viewport.zw;
    return vec3(xypos, post_div.z*0.5 + 0.5);
}

vec4 windowToClipVector(vec2 window, vec4 viewport, float clip_w)
{
    vec2 xypos = (window / viewport.zw)*2.0;
    return vec4(xypos, 0.0, 0.0)*clip_w;
}