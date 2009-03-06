#ifdef CUTAWAY
uniform sampler2D texCutaway;
uniform float depth;
varying vec4 cutawayTexCoordH;
#endif

void main()
{
//    gl_FragDepth = gl_FragCoord.z - 5.0 * clamp(3.0 * fwidth(gl_FragCoord.z), 0.00001, 0.0001);
    gl_FragDepth = gl_FragCoord.z - clamp(3.0 * fwidth(gl_FragCoord.z), 0.00001, 0.0001);

#ifdef CUTAWAY
    vec2 cutawayTexCoord = cutawayTexCoordH.xy / cutawayTexCoordH.w;
    vec4 cutawayValue = texture2D(texCutaway, cutawayTexCoord);
    vec4 outColor = vec4(0.0, 0.0, 0.0, 0.7);
    if (cutawayValue.w > gl_FragCoord.z) {
//        outColor = vec4(0.0, 0.0, 0.0, 0.35);
        outColor.a *= 1.0 - smoothstep(0.0, depth + 0.000001, cutawayValue.w - gl_FragCoord.z);
    }
#else
    vec4 outColor = vec4(0.0, 0.0, 0.0, 1.0);
#endif
    
    gl_FragColor = outColor;
}
