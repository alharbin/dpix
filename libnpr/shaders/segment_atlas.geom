/*****************************************************************************\

segment_atlas.geom
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Geometry shader for drawing the segment atlas itself.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#extension GL_EXT_geometry_shader4 : enable

uniform sampler2DRect path_start_end_ptrs;
uniform sampler2DRect clip_vert_0_buffer;
uniform sampler2DRect clip_vert_1_buffer;
uniform sampler2DRect offset_buffer;

uniform float clip_buffer_width;
uniform float atlas_width;

varying vec4 sample_clip_pos;
varying float path_id;

void emitLineSegment(float offset, float width, vec4 clip_p, vec4 clip_q)
{
    vec2 vertex_position = indexToCoordinate(offset, atlas_width);
    vertex_position += vec2(0.5, 0.5);

    sample_clip_pos = clip_p;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex_position, 0.0, 1.0);
    EmitVertex();

    // N.B.: the second vertex is positioned num_samples
    // to the right of the first coordinate, *not* at the coordinate 
    // corresponding to the index offset + width.
    // This allows the second vertex to fall off into the gutter of the atlas.

    vertex_position = vertex_position + vec2(width, 0.0);

    sample_clip_pos = clip_q;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex_position, 0.0, 1.0);
    EmitVertex();

    EndPrimitive();
}

void main()
{
    float segment_index = gl_PositionIn[0].x;
    vec2 segment_coord = indexToCoordinate(segment_index, clip_buffer_width);

    vec4 path_texel = texture2DRect(path_start_end_ptrs, segment_coord);
    float path_start = unpackPathStart(path_texel);
    float path_end = unpackPathEnd(path_texel);

    vec4 offset_texel = texture2DRect( offset_buffer, segment_coord ); 
    float num_samples = unpackNumSamples(offset_texel);
    float sample_offset = unpackSampleOffset(offset_texel);

    vec2 padding = segmentPadding(num_samples, segment_index, 
                                  path_start, path_end);

    // path_id stays the same for each vertex, so we just set
    // it once.
    path_id = path_start;

    vec4 clip_p = texture2DRect( clip_vert_0_buffer, segment_coord );
    vec4 clip_q = texture2DRect( clip_vert_1_buffer, segment_coord );

    // Each segment potentially gets padding to its left and right 
    // along with the real values in the middle. 
    emitLineSegment(sample_offset, padding.x, clip_p, clip_p);
    emitLineSegment(sample_offset + padding.x, num_samples, 
                    clip_p, clip_q);
    emitLineSegment(sample_offset + padding.x + num_samples, 
                    padding.y, clip_q, clip_q);

}

