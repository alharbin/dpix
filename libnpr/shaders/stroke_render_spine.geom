#extension GL_EXT_geometry_shader4 : enable

uniform vec4 viewport;
uniform float pen_width;
uniform float texture_length;
uniform float length_scale;
uniform float overshoot_scale;

uniform int test_profiles;

varying out vec4 spine_position;
varying out vec3 spine_tangent;
varying out vec2 tex_coord;

varying in vec3 normal[];

vec2 textureOffsets(float segment_length)
{
	float texture_scale = (segment_length / texture_length) / length_scale;
    return vec2(-texture_scale * 0.5 + 0.5, texture_scale * 0.5 + 0.5);
}

bool testProfileEdge(mat3 normal_matrix, mat4 mv_matrix, vec3 n1, vec3 n2, vec4 p)
{
    vec3 face_normal_0 = normal_matrix * n1;
    vec3 face_normal_1 = normal_matrix * n2;
    vec4 camera_to_line = mv_matrix * p;

    float dot0 = dot(camera_to_line.xyz, face_normal_0);
    float dot1 = dot(camera_to_line.xyz, face_normal_1);

    return (dot0 >= 0.0 && dot1 <= 0.0) || (dot0 <= 0.0 && dot1 >= 0.0);
}

void main(void)
{
    vec4 clip_p = gl_ModelViewProjectionMatrix * gl_PositionIn[0];
    vec4 clip_q = gl_ModelViewProjectionMatrix * gl_PositionIn[1];

    if (test_profiles > 0 && 
        !testProfileEdge(gl_NormalMatrix, 
                         gl_ModelViewMatrix,
                         normal[0],
                         normal[1],
                         gl_PositionIn[0]))
    {
        // If we are testing profiles and this one fails the test,
        // send the segment to infinity.
        clip_p.w = 0.0;
        clip_q.w = 0.0;
    }

	vec3 window_p = clipToWindow(clip_p, viewport); 
	vec3 window_q = clipToWindow(clip_q, viewport); 
	
	vec3 tangent = window_q - window_p;
	float segment_length = length(tangent.xy);
	vec2 texture_offsets = textureOffsets(segment_length);	
	
	vec2 perp = normalize(vec2(-tangent.y, tangent.x));
    vec2 window_offset = perp*pen_width;
	
	spine_tangent = normalize(tangent);
	
    float clip_p_w = clip_p.w;
    float clip_q_w = clip_q.w;
	vec4 offset_p = windowToClipVector(window_offset, viewport, clip_p_w);
	vec4 offset_q = windowToClipVector(window_offset, viewport, clip_q_w);

	spine_position = clip_p;
	gl_Position = clip_p + offset_p;
    tex_coord = vec2(texture_offsets.x, 0.0);
	EmitVertex();
	gl_Position = clip_p - offset_p;
    tex_coord = vec2(texture_offsets.x, 1.0);
	EmitVertex();
	
	spine_position = clip_q;
	gl_Position = clip_q + offset_q;
    tex_coord = vec2(texture_offsets.y, 0.0);
	EmitVertex();
	gl_Position = clip_q - offset_q;
    tex_coord = vec2(texture_offsets.y, 1.0);
	EmitVertex();
	
	EndPrimitive();																						
}
