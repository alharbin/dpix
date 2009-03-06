uniform sampler2D vert0_tex;
uniform sampler2D vert1_tex;

void main()
{
    vec4 pos;
    vec4 vert;
    if (gl_Vertex.z < 0.5)
    {
        vert = texture2DLod( vert0_tex, gl_Vertex.xy, 0.0 );
    }
    else
    {
        vert = texture2DLod( vert1_tex, gl_Vertex.xy, 0.0 );
    }

    if (vert.w < 0.00001)
        pos = vec4(0.0, 0.0, 0.0, 0.0);
    else
        pos = vec4(vert.xy, 0.0, 1.0);

    gl_Position = pos;
    gl_FrontColor = gl_BackColor = gl_Color;
}


