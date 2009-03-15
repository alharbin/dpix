/*****************************************************************************\

pos_and_normal.frag
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Draws position and normal buffers.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

varying vec3 vert_pos_world;
varying vec3 vert_pos_camera;
varying vec4 vert_pos_clip;
varying vec3 vert_normal_world;
varying vec3 vert_normal_camera;

void main()
{
    vec3 world_norm = normalize(vert_normal_world);
    vec3 camera_norm = normalize(vert_normal_camera);

	gl_FragData[0] = vec4(vert_pos_world, 1.0);
	gl_FragData[1] = vec4(world_norm, 0.0);

    float clip_depth = vert_pos_clip.z / vert_pos_clip.w;
    clip_depth = 0.5 * (clip_depth + 1.0);

    // Hacky simulation of glPolygonOffset
    float depth_unit = 0.0001 * (1.0 + (1.0 - clip_depth));
    float depth_offset = depth_unit / abs(vert_normal_camera.z) + depth_unit;
    clip_depth += depth_offset;
	gl_FragData[2] = vec4(clip_depth, clip_depth, clip_depth, 1.0);

	gl_FragData[3] = vec4(vert_pos_camera, 1.0);
	gl_FragData[4] = vec4(camera_norm, 0.0);
}
