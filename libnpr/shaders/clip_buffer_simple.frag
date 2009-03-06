uniform vec4 viewport;
uniform mat4 modelview;
uniform mat4 projection;
uniform sampler2DRect vert0_tex;
uniform sampler2DRect vert1_tex;

void main()
{
    // look up the world positions of the segment vertices
    vec2 texcoord = floor(gl_FragCoord.xy);
    vec4 v0_world_pos = texture2DRect(vert0_tex, texcoord);

    // early exit if there are no vertices here to process
    if (v0_world_pos.w < 0.5)
    {
        // no vertex data to process
        gl_FragData[0] = vec4(0.5,0.0,0.0,0.0);
        gl_FragData[1] = vec4(0.5,0.5,0.0,0.0);
        // must write something into fragdata[2] to prevent
        // buffer 2 from getting filled with garbage? (very weird)
        gl_FragData[2] = vec4(0.0,1.0,0.0,0.0);
        return;
    }

    vec4 v1_world_pos = texture2DRect(vert1_tex, texcoord);

    // clip to the near plane
    vec3 v0_clipped_near = v0_world_pos.xyz;
    vec3 v1_clipped_near = v1_world_pos.xyz;
    // project
    /*vec4 v0_pre_div = projection * modelview * vec4(v0_clipped_near,1.0);
    vec4 v1_pre_div = projection * modelview * vec4(v1_clipped_near,1.0);*/

    vec4 v0_pre_div = modelview * vec4(v0_clipped_near,1.0);
    v0_pre_div = projection * v0_pre_div;
    vec4 v1_pre_div = modelview * vec4(v1_clipped_near,1.0);
    v1_pre_div = projection * v1_pre_div;

    // perspective divide
    vec3 v0_clip_pos = v0_pre_div.xyz / v0_pre_div.w;
    vec3 v1_clip_pos = v1_pre_div.xyz / v1_pre_div.w;

    // convert to window coordinates
    /*vec2 v0_screen = (v0_clip_pos.xy + vec2(1.0,1.0)) * 0.5 * viewport.zw;
    vec2 v1_screen = (v1_clip_pos.xy + vec2(1.0,1.0)) * 0.5 * viewport.zw;

    v0_clip_pos = max(min(v0_clip_pos, vec3(1.0,1.0,1.0)), vec3(-1.0,-1.0,-1.0));
    v1_clip_pos = max(min(v1_clip_pos, vec3(1.0,1.0,1.0)), vec3(-1.0,-1.0,-1.0));*/

    gl_FragData[0] = vec4(v0_clip_pos, 1.0);
    gl_FragData[1] = vec4(v1_clip_pos, 1.0);
    /*gl_FragData[0] = vec4(v0_world_pos.xyz, 1.0);
    gl_FragData[1] = vec4(v1_world_pos.xyz, 1.0);*/
    gl_FragData[2] = vec4(1.0,1.0,1.0,1.0);
}

