/*****************************************************************************\

stroke_render_atlas.geom
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Stroke rendering using the segment atlas.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#extension GL_EXT_geometry_shader4 : enable

uniform sampler2DRect path_start_end_ptrs;
uniform sampler2DRect clip_vert_0_buffer;
uniform sampler2DRect clip_vert_1_buffer;
uniform sampler2DRect offset_buffer;

uniform vec4 viewport;

uniform float pen_width;
uniform float texture_length;
uniform float length_scale;
uniform float row;
uniform float clip_buffer_width;
uniform float atlas_width;
uniform float sample_spacing;

uniform float overshoot_scale;

uniform int use_artmaps;

varying out vec2 atlas_position;
varying out vec4 spine_position;
varying out float path_start;

// The pen textures are parameterized with the fixed point at the middle
// of the path. Using the midpoint helps temporal coherence because
// the maximum speed at which the texture slides is 
// half as fast when using the midpoint as opposed to one of the
// endpoints.

vec3 getTextureOffsets(float path_start, float path_end, 
                       float segment_index, float segment_length)
{
  if (path_start == path_end)
    {
      if(use_artmaps == 1)
	{
	  float level = segment_length * length_scale / texture_length;
	  float t = ceil(log2(level));
	  float fact = pow(2,t);
	  return vec3(0, fact, 2.0*level/fact-1.0);
	}
      else
	{
	  float texture_scale = (segment_length / texture_length) / length_scale;
	  return vec3(-texture_scale * 0.5 + 0.5, texture_scale * 0.5 + 0.5, 0.0);
	}
    }
  else
    {
      vec2 mid_coord = indexToCoordinate((path_start + path_end)*0.5, clip_buffer_width);
      vec2 this_coord = indexToCoordinate(segment_index, clip_buffer_width); 
      vec4 mid_texel = texture2DRect(offset_buffer, mid_coord);
      vec4 this_texel = texture2DRect(offset_buffer, this_coord);
                         
      float arc_length = unpackArcLengthOffset(this_texel) - 
	unpackArcLengthOffset(mid_texel);

      if(use_artmaps == 1)
	{
	  vec2 start_coord = indexToCoordinate(path_start, clip_buffer_width);
	  vec4 start_texel = texture2DRect(offset_buffer, start_coord);
	  vec2 end_coord = indexToCoordinate(path_end, clip_buffer_width);
	  vec4 end_texel = texture2DRect(offset_buffer, end_coord);
	  float path_length = unpackArcLengthOffset(end_texel) -
	    unpackArcLengthOffset(start_texel);
	  vec2 offsets = vec2(arc_length, arc_length + segment_length) / path_length;

	  float level = path_length * length_scale / texture_length;
	  float t = ceil(log2(level));
	  float fact = pow(2,t);
	  return vec3(fact * offsets, 2.0*level/fact-1.0);
	}
      else
	{
	  vec2 offsets = vec2(arc_length, arc_length + segment_length);
	  return vec3((offsets / texture_length) / length_scale,0.0);
	}
    }
}

vec2 getVertexOffset(vec2 tangent_a, vec2 tangent_b)
{
    vec2 chord = tangent_a + tangent_b;
    return normalize(vec2(-chord.y, chord.x)) * pen_width;
}

void emitVertexPair(vec2 v, vec2 vertex_offset, vec2 sample, vec2 tex_offset)
{
    gl_Position = gl_ProjectionMatrix * vec4(v - vertex_offset, 0.0, 1.0);
    gl_TexCoord[0] = vec4(tex_offset.x, 0.0, tex_offset.y, 0.0); 
    atlas_position = sample;
    EmitVertex();

    gl_Position = gl_ProjectionMatrix * vec4(v + vertex_offset, 0.0, 1.0);
    gl_TexCoord[0] = vec4(tex_offset.x, 1.0, tex_offset.y, 0.0); 
    atlas_position = sample + vec2(0.0, 1.0);
    EmitVertex();
}

vec2 overshootAmounts( float index, vec2 padding )
{
    float size = 5.0;
    float f = mod(index, size);
    float overshoot = (10.0 + 10.0 * (f / 5.0)) * overshoot_scale;
    float left = min(overshoot, padding.x * sample_spacing);
    float right = min(overshoot, padding.y * sample_spacing);
    return vec2(left, right);
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
    path_start = unpackPathStart(path_texel);
    float path_end = unpackPathEnd(path_texel);
    float num_samples = unpackNumSamples(offset_texel);
    float sample_offset = unpackSampleOffset(offset_texel);
    vec2 atlas_padding = segmentPadding(num_samples, segment_index, 
                                        path_start, path_end);

    vec3 tex_offsets = getTextureOffsets(path_start, path_end,
                                         segment_index, segment_length);

    vec2 norm_tangent = normalize(tangent);
    vec2 prev_tangent = norm_tangent;
    vec2 next_tangent = norm_tangent;
    
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
            prev_tangent = normalize(p - p_prev);
        }
    }
    if (segment_index < path_end)
    {
        vec2 next_coord = indexToCoordinate(segment_index+1.0, clip_buffer_width); 
        vec4 next_clip = texture2DRect(clip_vert_1_buffer, next_coord);
        if (next_clip.w > 0.0)
        {
            vec2 q_next = clipToWindow(next_clip, viewport);
            next_tangent = normalize(q_next - q);
        }
    }

    // N.B: The p_sample should NOT be offset by (0.5, 0.5), because it is
    // the lower left corner of a quad. 
    vec2 p_sample = indexToCoordinate(sample_offset + atlas_padding.x, atlas_width);
    vec2 q_sample = p_sample + vec2(num_samples, 0.0);

    // Compute overshoot.
    vec2 overshoot = overshootAmounts(segment_index, atlas_padding);
    p -= prev_tangent * overshoot.x;
    p_sample -= vec2(overshoot.x / sample_spacing, 0.0);
    q += next_tangent * overshoot.y;
    q_sample += vec2(overshoot.y / sample_spacing, 0.0);

    // Two vertices at the p end of the segment.
    vec2 vertex_offset = getVertexOffset(prev_tangent, norm_tangent);
    spine_position = clip_p;
    emitVertexPair(p, vertex_offset, p_sample, tex_offsets.xz); 

    // Two vertices at the q end of the segment.
    vertex_offset = getVertexOffset(norm_tangent, next_tangent);
    spine_position = clip_q;
    emitVertexPair(q, vertex_offset, q_sample, tex_offsets.yz); 

    EndPrimitive();
}

