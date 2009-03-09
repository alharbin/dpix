uniform sampler2D vis_focus_texture;
uniform sampler2D vis_defocus_texture;
uniform sampler2D invis_focus_texture;
uniform sampler2D invis_defocus_texture;

uniform vec4 vis_focus_color;
uniform vec4 vis_defocus_color;
uniform vec4 invis_focus_color;
uniform vec4 invis_defocus_color;

vec4 computePenColor( float visibility, float focus, vec2 tex_coord )
{
    float vis_focus_alpha = texture2D(vis_focus_texture, tex_coord).a * 
                            vis_focus_color.a;
    float vis_defocus_alpha = texture2D(vis_defocus_texture, tex_coord).a * 
                              vis_defocus_color.a;
    float invis_focus_alpha = texture2D(invis_focus_texture, tex_coord).a * 
                              invis_focus_color.a;
    float invis_defocus_alpha = texture2D(invis_defocus_texture, tex_coord).a * 
                                invis_defocus_color.a;

    float mixed_vis_alpha = mix(vis_focus_alpha, vis_defocus_alpha, focus);
    float mixed_invis_alpha = mix(invis_focus_alpha, invis_defocus_alpha, focus);
    float mixed_alpha = mix(mixed_vis_alpha, mixed_invis_alpha, 1.0 - visibility);

    vec4 mixed_vis_color = mix(vis_focus_color, vis_defocus_color, focus);
    vec4 mixed_invis_color = mix(invis_focus_color, invis_defocus_color, focus);
    vec4 mixed_color = mix(mixed_vis_color, mixed_invis_color, 1.0 - visibility);

    return vec4(mixed_color.rgb, mixed_alpha);
}