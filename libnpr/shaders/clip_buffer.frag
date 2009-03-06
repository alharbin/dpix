uniform vec4 viewport;
uniform mat4 modelview;
uniform mat4 projection;
uniform mat4 inverse_projection;
uniform sampler2DRect vert0_tex;
uniform sampler2DRect vert1_tex;
uniform sampler2DRect face_normal0_tex;
uniform sampler2DRect face_normal1_tex;
uniform sampler2DRect path_start_end_ptrs;
uniform vec3 view_pos;
uniform vec3 view_dir;
uniform float sample_step;
uniform float buffer_width;

// Clipping routines adapted from NPRPathResample.cc
// Clipping is done in clip space (NPRPathResample is in window space)
// The original code didn't care about keeping the correct Z, so
// it did all the clipping in the post-projection cube with w=1.
// For ease we do the same here, then multiply the final coordinates
// by w so that they are interpolated correctly later on. 

struct segment
{
    vec3 p1;
    vec3 p2;
    bool on_screen;
};
    
const float epsilon = 0.00001;
const float xmin = -1.1;
const float xmax = 1.1;
const float ymin = -1.1;
const float ymax = 1.1;

// this is a conservative offscreen rejection test ... catches most cases 
bool segmentOffScreen(vec3 p0, vec3 p1)
{
    return ( (p0[0] < xmin && p1[0] < xmin) ||
             (p0[0] > xmax && p1[0] > xmax) ||
             (p0[1] < ymin && p1[1] < ymin) ||
             (p0[1] > ymax && p1[1] > ymax) );
}

bool pointOffScreen(vec3 p)
{
    return  (p[0] < xmin ||
             p[0] > xmax ||
             p[1] < ymin ||
             p[1] > ymax );
}


vec3 clipMinMaxX(vec3 outv, vec3 inv)
{
    vec3 ret = outv;
    if (outv.x < xmin)
    {
        float t = (xmin - outv.x) / (inv.x - outv.x);
        ret = t * inv + (1.0 - t) * outv;
    }
    else if (outv.x > xmax)
    {
        float t = (xmax - inv.x) / (outv.x - inv.x);
        ret = t * outv + (1.0 - t) * inv;
    }
    return ret;
}

vec3 clipMinMaxY(vec3 outv, vec3 inv)
{
    vec3 ret = outv;
    if (outv.y < ymin)
    {
        float t = (ymin - outv.y) / (inv.y - outv.y);
        ret = t * inv + (1.0 - t) * outv;
    }
    else if (outv.y > ymax)
    {
        float t = (ymax - inv.y) / (outv.y - inv.y);
        ret = t * outv + (1.0 - t) * inv;
    }
    return ret;
}

vec3 clipSegmentOneOut(vec3 off_screen, vec3 on_screen)
{
    vec3 outv = off_screen;

    // first clip against the x coords
    outv = clipMinMaxX(outv, on_screen);
    
    // now clip against the y coords using the newly clipped point
    outv = clipMinMaxY(outv, on_screen);

    return outv;
}


segment clipToMin(float min, segment inseg, float p1val, float p2val)
{
    float minPos = min + epsilon;
    float minNeg = min - epsilon;
    segment outseg = segment(inseg.p1,inseg.p2,inseg.on_screen);
    
    // trivial reject
    if ((p1val < minPos && p2val < minPos) || inseg.on_screen == false)
    {
        outseg.on_screen = false;
    }
     
    // cut at min
    if (p1val < minPos)
    {
        float t = (min - p1val) / (p2val - p1val);
        outseg.p1 = t * inseg.p2 + (1.0 - t) * inseg.p1;
    }
    else if (p2val < minPos)
    {
        float t = (min - p2val) / (p1val - p2val);
        outseg.p2 = t * inseg.p1 + (1.0 - t) * inseg.p2;
    }
    return outseg;
}

segment clipToMax(float max, segment inseg, float p1val, float p2val)
{
    float maxPos = max + epsilon;
    float maxNeg = max - epsilon;
    segment outseg = segment(inseg.p1,inseg.p2,inseg.on_screen);

    // trivial reject
    if ((p1val > maxNeg && p2val > maxNeg) || inseg.on_screen == false)
    {
        outseg.on_screen = false;
    }
     
    // cut at max
    if (p1val > maxNeg)
    {
        float t = (max - p2val) / (p1val - p2val);
        outseg.p1 = t * inseg.p1 + (1.0 - t) * inseg.p2;
    }
    else if (p2val > maxNeg)
    {
        float t = (max - p1val) / (p2val - p1val);
        outseg.p2 = t * inseg.p2 + (1.0 - t) * inseg.p1;
    }
    return outseg;
}
                        
segment clipSegmentBothOut(vec3 p1, vec3 p2)
{
    segment seg = segment(p1, p2, true);

    seg = clipToMin(xmin, seg, seg.p1.x, seg.p2.x);
    seg = clipToMax(xmax, seg, seg.p1.x, seg.p2.x);
    seg = clipToMin(ymin, seg, seg.p1.y, seg.p2.y);
    seg = clipToMax(ymax, seg, seg.p1.y, seg.p2.y);

    return seg;
}

vec3 clipSegmentToNear(vec3 off_screen, vec3 on_screen)
{
    // see http://members.tripod.com/~Paul_Kirby/vector/Vplanelineint.html
    
    vec3 a = off_screen;
    vec3 b = on_screen;
    vec3 c = view_pos + view_dir;
    vec3 n = view_dir;
    float t = dot((c - a), n) / dot((b - a), n);  
    
    vec3 clipped = a + (b - a) * t; 
    return clipped; 
}

bool pointBeyondNear(vec3 p)
{
    vec3 offset = p - view_pos;
    bool beyond = dot(offset, view_dir) > 0.0;
    return beyond; 
}

bool testProfileEdge( vec2 texcoord, vec3 world_position )
{
    // This should really be the inverse transpose of the modelview matrix, but
    // that only matters if the camera has a weird anisotropic scale or skew.
    vec4 face_normal_0 = modelview * texture2DRect(face_normal0_tex, texcoord);
    vec4 face_normal_1 = modelview * texture2DRect(face_normal1_tex, texcoord);
    vec4 camera_to_line = modelview * vec4(world_position, 1.0);

    float dot0 = dot(camera_to_line, face_normal_0);
    float dot1 = dot(camera_to_line, face_normal_1);

    return (dot0 >= 0.0 && dot1 <= 0.0) || (dot0 <= 0.0 && dot1 >= 0.0);
}

void main()
{
    // look up the world positions of the segment vertices
    vec2 texcoord = gl_FragCoord.xy;
    vec4 v0_world_pos = texture2DRect(vert0_tex, texcoord);

    // early exit if there are no vertices here to process
    if (v0_world_pos.w < 0.5)
    {
        // no vertex data to process
        gl_FragData[0] = vec4(0.5,0.0,0.0,0.0);
        gl_FragData[1] = vec4(0.5,0.5,0.0,0.0);
        // must write something into fragdata[2] to prevent
        // buffer 2 from getting filled with garbage? (very weird)
        gl_FragData[2] = vec4(0.0,1.0,0.0,0.0);
        return;
    }

    vec4 v1_world_pos = texture2DRect(vert1_tex, texcoord);

    // clip to the near plane
    vec3 v0_clipped_near = v0_world_pos.xyz;
    vec3 v1_clipped_near = v1_world_pos.xyz;
    bool v0_beyond_near = pointBeyondNear(v0_world_pos.xyz);
    bool v1_beyond_near = pointBeyondNear(v1_world_pos.xyz);

    if ( !v0_beyond_near && !v1_beyond_near )
    {
        // segment entirely behind the camera
        gl_FragData[0] = vec4(0.0,1.0,0.0,0.0);
        gl_FragData[1] = vec4(0.0,0.0,1.0,0.0); 
        gl_FragData[2] = vec4(0.0,1.0,0.0,0.0);
        return;
    }
    else if ( !v0_beyond_near )
    {
        v0_clipped_near = clipSegmentToNear( v0_world_pos.xyz, v1_clipped_near );
    }
    else if ( !v1_beyond_near )
    {
        v1_clipped_near = clipSegmentToNear( v1_world_pos.xyz, v0_clipped_near );
    }

    // If this segment is a profile edge, test to see if it should be turned on.
    if (v1_world_pos.w > 0.5)
    {
        bool profile_on = testProfileEdge(texcoord, v0_clipped_near);
        if (!profile_on)
        {
            // Profile edge should be off.
            gl_FragData[0] = vec4(0.0,1.0,0.5,0.0);
            gl_FragData[1] = vec4(0.0,0.5,1.0,0.0); 
            gl_FragData[2] = vec4(0.0,1.0,0.0,0.0);
            return;
        }
    }

    // project
    vec4 v0_pre_div = projection * modelview * vec4(v0_clipped_near,1.0);
    vec4 v1_pre_div = projection * modelview * vec4(v1_clipped_near,1.0);

    // perspective divide
    vec3 v0_clip_pos = v0_pre_div.xyz / v0_pre_div.w;
    vec3 v1_clip_pos = v1_pre_div.xyz / v1_pre_div.w;

    // clip to frustum
    bool v0_on_screen = !pointOffScreen( v0_clip_pos ); 
    bool v1_on_screen = !pointOffScreen( v1_clip_pos ); 

    if ( !v0_on_screen && !v1_on_screen )
    {
        segment ret = clipSegmentBothOut(v0_clip_pos, v1_clip_pos);
        if (ret.on_screen == false)
        {
            // segment entirely off screen: BLUE / MAGENTA / BLACK
            gl_FragData[0] = vec4(0.0,0.0,1.0,0.0);
            gl_FragData[1] = vec4(1.0,0.0,1.0,0.0);
            gl_FragData[2] = vec4(0.0,0.0,0.0,0.0);
            return;
        }
        v0_clip_pos = ret.p1;
        v1_clip_pos = ret.p2;
    }
    else if ( !v0_on_screen )
    {
        v0_clip_pos = clipSegmentOneOut( v0_clip_pos, v1_clip_pos );
    }
    else if ( !v1_on_screen )
    {
        v1_clip_pos = clipSegmentOneOut( v1_clip_pos, v0_clip_pos );
    }

    // convert to window coordinates
    vec2 v0_screen = (v0_clip_pos.xy + vec2(1.0,1.0)) * 0.5 * viewport.zw;
    vec2 v1_screen = (v1_clip_pos.xy + vec2(1.0,1.0)) * 0.5 * viewport.zw;

    float segment_screen_length = length(v0_screen - v1_screen);

    // scale the length by sample_step to get the number of samples
    float num_samples = segment_screen_length / sample_step;
    num_samples = ceil(num_samples);

    // Unproject and reproject the final clipped positions
    // so that interpolation is perspective correct later on.
    vec4 v0_world = inverse_projection * vec4(v0_clip_pos, 1.0);
    vec4 v1_world = inverse_projection * vec4(v1_clip_pos, 1.0);
    vec4 v0_clipped_pre_div = projection * v0_world;
    vec4 v1_clipped_pre_div = projection * v1_world;

    // Add some padding to the number of samples so that filters
    // that work along the segment length (such as overshoot)
    // have some room to work with at the end of each segment.
    vec4 path_texel = texture2DRect(path_start_end_ptrs, texcoord);
    vec2 padding = segmentPadding(num_samples, 
        coordinateToIndex(texcoord, buffer_width),
        unpackPathStart(path_texel),
        unpackPathEnd(path_texel));
    float total_padding = padding.x + padding.y;

    gl_FragData[0] = v0_clipped_pre_div;
    gl_FragData[1] = v1_clipped_pre_div;
    gl_FragData[2] = packOffsetTexel(num_samples, segment_screen_length,
                                     num_samples + total_padding, segment_screen_length);
}

