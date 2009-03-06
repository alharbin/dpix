#ifdef CUTAWAY
varying vec4 cutawayTexCoordH;
#endif

void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    
#ifdef CUTAWAY
    // get viewport coordinates
    cutawayTexCoordH = gl_ModelViewProjectionMatrix * gl_Vertex;
    // remap to 0..1 space (after perspective divide)
    cutawayTexCoordH.xyw += cutawayTexCoordH.w;
    cutawayTexCoordH.z = (gl_ModelViewMatrix * gl_Vertex).z;
#endif
}
