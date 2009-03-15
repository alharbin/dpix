/*****************************************************************************\

stroke_render_spine.vert
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Simple vertex shader for spine-test visibility stroke rendering.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

varying vec3 normal;

void main() {

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = gl_Vertex;
    normal = gl_Normal;
}
