/*****************************************************************************\

clip_buffer_viz.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Debug visualization code for the clipping buffer. 

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

uniform sampler2DRect vert0_tex;
uniform sampler2DRect vert1_tex;

void main()
{
    bool even = mod(gl_FragCoord.x, 2.0) < 1.0;

    vec2 texcoord = vec2(floor(gl_FragCoord.x)*0.5, floor(gl_FragCoord.y));
    vec4 color;
    if (even)
        color = texture2DRect( vert0_tex, texcoord);
    else
        color = texture2DRect( vert1_tex, texcoord);

    gl_FragColor = vec4(color.xyz, 1.0);
}



