/*****************************************************************************\

priority_buffer.geom
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Geometry shader for priority buffer rendering.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#extension GL_EXT_geometry_shader4 : enable

uniform sampler2DRect path_start_end_ptrs;
uniform sampler2DRect clip_vert_0_buffer;
uniform sampler2DRect clip_vert_1_buffer;
uniform sampler2DRect offset_buffer;

uniform vec4 viewport;

uniform float max_width;
uniform float row;
uniform float clip_buffer_width;
uniform float atlas_width;
uniform float sample_spacing;

varying vec2 atlas_position;
varying vec4 spine_position;
varying float path_id;

vec2 getVertexOffset(vec2 tangent_a, vec2 tangent_b)
{
    vec2 chord = tangent_a + tangent_b;
    return normalize(vec2(-chord.y, chord.x)) * max_width * 0.5;
}

void emitVertexPair(vec2 v, vec2 vertex_offset, vec4 spine, vec2 sample)
{
    spine_position = spine;

    gl_Position = gl_ProjectionMatrix * vec4(v - vertex_offset, 0.0, 1.0);
    atlas_position = sample;
    EmitVertex();

    gl_Position = gl_ProjectionMatrix * vec4(v + vertex_offset, 0.0, 1.0);
    atlas_position = sample + vec2(0.0, 1.0);
    EmitVertex();
}

void main()
{
    vec2 segment_coord = vec2(gl_PositionIn[0].x, row);
    float segment_index = coordinateToIndex(segment_coord, clip_buffer_width);

    vec4 clip_p = texture2DRect(clip_vert_0_buffer, segment_coord);
    vec4 clip_q = texture2DRect(clip_vert_1_buffer, segment_coord);

    vec2 p = clipToWindow(clip_p, viewport);
    vec2 q = clipToWindow(clip_q, viewport);
    vec2 tangent = q - p;
    float segment_length = length(tangent);

    vec4 offset_texel = texture2DRect(offset_buffer, segment_coord);
    vec4 path_texel = texture2DRect(path_start_end_ptrs, segment_coord);
    float path_start = unpackPathStart(path_texel);
    float path_end = unpackPathEnd(path_texel);
    float num_samples = unpackNumSamples(offset_texel);
    float sample_offset = unpackSampleOffset(offset_texel);
    vec2 atlas_padding = segmentPadding(num_samples, segment_index, 
                                        path_start, path_end);

    vec2 norm_tangent = normalize(tangent);
    vec2 prev_tangent = tangent;
    vec2 next_tangent = tangent;

    // Mitreing
    // Have to check if the neighboring segments are actually on screen before
    // using them to compute tangents. 
    if (segment_index > path_start)
    {
        vec2 prev_coord = indexToCoordinate(segment_index-1.0, clip_buffer_width);
        vec4 prev_clip = texture2DRect(clip_vert_0_buffer, prev_coord);
        if (prev_clip.w > 0.0)
        {
            vec2 p_prev = clipToWindow(prev_clip, viewport);
            prev_tangent = p - p_prev;
        }
    }
    if (segment_index < path_end)
    {
        vec2 next_coord = indexToCoordinate(segment_index+1.0, clip_buffer_width); 
        vec4 next_clip = texture2DRect(clip_vert_1_buffer, next_coord);
        if (next_clip.w > 0.0)
        {
            vec2 q_next = clipToWindow(next_clip, viewport);
            next_tangent = q_next - q;
        }
    }

    // N.B: The p_sample should NOT be offset by (0.5, 0.5), because it is
    // the lower left corner of a quad. 
    vec2 p_sample = indexToCoordinate(sample_offset + atlas_padding.x, atlas_width);
    vec2 q_sample = p_sample + vec2(num_samples, 0.0);

    path_id = path_start;

    // Two vertices at the p end of the segment.
    vec2 vertex_offset = getVertexOffset(prev_tangent, tangent);
    emitVertexPair(p, vertex_offset, clip_p, p_sample);

    // Two vertices at the q end of the segment.
    vertex_offset = getVertexOffset(tangent, next_tangent);
    emitVertexPair(q, vertex_offset, clip_q, q_sample);

    EndPrimitive();
}

