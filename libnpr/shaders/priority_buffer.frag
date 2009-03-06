uniform sampler2DRect segment_atlas;
uniform vec4  transfer_focus;
uniform vec4  transfer_width;

uniform vec4  viewport;
uniform float max_width;
uniform float scene_radius;

uniform vec3  camera_focal_point;
uniform vec3  focal_point;
uniform mat4  inverse_projection;

#define NPR_FOCUS_NONE 0
#define NPR_FOCUS_CAMERA 1
#define NPR_FOCUS_WORLD 2
#define NPR_FOCUS_SCREEN 3
#define NPR_FOCUS_OBJECT 4
#define NPR_FOCUS_CONFIDENCE 5
#define NPR_FOCUS_COLOR 6

uniform int focus_mode;

varying vec2 atlas_position;
varying vec4 spine_position;
varying float path_id;

void main()
{
    float visibility = texture2DRect(segment_atlas, atlas_position).w;
    if (visibility < 0.001)
        discard;

    float width_scale = transfer_width[0];
    if (focus_mode == NPR_FOCUS_SCREEN)
    {
        vec2 offset = clipToWindow(spine_position, viewport) - focal_point.xy;
        float distance = length(offset) / max(viewport[2], viewport[3]);

        float focus = computeTransfer(transfer_focus, distance);   
        width_scale = computeTransfer(transfer_width, focus);
    }
    else if (focus_mode == NPR_FOCUS_WORLD)
    {
        vec4 position_w = inverse_projection * spine_position;
        vec3 position = position_w.xyz / position_w.w;
        float distance = length(position - camera_focal_point);

        float focus = computeTransfer(transfer_focus, distance / (scene_radius));
        width_scale = computeTransfer(transfer_width, focus);
    }
        
    width_scale *= visibility;

    float pixel_width = width_scale*max_width;
    if (pixel_width < 1.0)
    {
        width_scale = 1.0 / max_width;
        pixel_width = 1.0;
    }

    if (abs(mod(atlas_position.y, 1.0) - 0.5)*2.0 > width_scale)
        discard;

    vec4 color = vec4(idToColor(path_id), pixel_width/255.0);

    gl_FragColor = color;
}

