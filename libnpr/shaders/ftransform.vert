/*****************************************************************************\

ftransform.vert
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

void main(void)
{
    gl_Position = ftransform();
}