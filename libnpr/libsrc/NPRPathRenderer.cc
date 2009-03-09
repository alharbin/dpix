#include "NPRPathRenderer.h"
#include "NPRSegmentAtlas.h"
#include "NPRStyle.h"
#include "NPRScene.h"
#include "NPRSettings.h"
#include "NPRGLDraw.h"

#include "GQShaderManager.h"
#include "GQStats.h"

#if 1
#define __MY_START_TIMER __START_TIMER
#define __MY_STOP_TIMER __STOP_TIMER
#define __MY_TIME_CODE_BLOCK __TIME_CODE_BLOCK
#else
#define __MY_START_TIMER __START_TIMER_AFTER_GL_FINISH
#define __MY_STOP_TIMER __STOP_TIMER_AFTER_GL_FINISH
#define __MY_TIME_CODE_BLOCK __TIME_CODE_BLOCK_AFTER_GL_FINISH
#endif

NPRPathRenderer::NPRPathRenderer()
{
    clear();
}

void NPRPathRenderer::clear()
{
    _blank_texture.clear();
    _quad_vertices_vbo.clear();

    _is_initialized = false;
}

void NPRPathRenderer::init()
{
    makeBlankTexture();
    makeQuadRenderingVBO();

    _is_initialized = true;
}

void NPRPathRenderer::drawSimpleLines(const NPRScene& scene)
{
    if (!_is_initialized)
        init();

    NPRGLDraw::clearGLState();

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    int mode = NPR_DRAW_LINES | NPR_OPAQUE | NPR_TRANSLUCENT;

    NPRGLDraw::drawMeshes( scene, 0, mode );
}
        
// Drawing strokes without access to a segment atlas. This
// is algorithm 1 in the TVCG paper. 
void NPRPathRenderer::drawStrokes(const NPRScene& scene,
                                  const GQTexture2D& depth_buffer)
{
    if (!_is_initialized)
        init();

    NPRGLDraw::clearGLState();

    GQShaderRef shader = GQShaderManager::bindProgram("stroke_render_spine");
    // If NPR_CHECK_LINE_VISIBILITY is false, the depth buffer should be
    // blank. 
    NPRSettings& settings = NPRSettings::instance();
    shader.bindNamedTexture("depth_buffer", &depth_buffer);
    shader.setUniform1f("check_at_spine", 
        settings.get(NPR_CHECK_LINE_VISIBILITY_AT_SPINE));
    setUniformPenStyleParams(shader, *(scene.globalStyle()));

    NPRGLDraw::setUniformSSParams(shader);
    NPRGLDraw::setUniformViewParams(shader);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int mode = NPR_DRAW_LINES | NPR_OPAQUE | NPR_TRANSLUCENT;

    shader.setUniform1i("test_profiles", 0);
    NPRGLDraw::drawMeshes(scene, 0, mode );

    if (settings.get(NPR_EXTRACT_PROFILES))
    {
        int mode = NPR_DRAW_PROFILES | NPR_OPAQUE | NPR_TRANSLUCENT;
        shader.setUniform1i("test_profiles", 1);
        NPRGLDraw::drawMeshes(scene, 0, mode );
    }
}

// Drawing strokes with access to a segment atlas. This
// is algorithm 2 in the TVCG paper.
void NPRPathRenderer::drawStrokes(const NPRScene& scene, 
                                  const NPRSegmentAtlas& atlas)
{
    if (!_is_initialized)
        init();

    GQShaderRef shader = GQShaderManager::bindProgram("stroke_render_atlas");
    setUniformPenStyleParams(shader, *(scene.globalStyle()));

    NPRSegmentAtlas::AtlasBufferId visibility_buffer = 
        NPRSegmentAtlas::VISIBILITY_ID;
    if (NPRSettings::instance().get(NPR_CHECK_LINE_PRIORITY))
    {
        visibility_buffer = NPRSegmentAtlas::PRIORITY_ID;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawQuads(shader, atlas, visibility_buffer);
}

void NPRPathRenderer::drawPriorityBuffer(const NPRScene& scene, 
                                         const NPRSegmentAtlas& atlas )
{
    if (!_is_initialized)
        init();

    GQShaderRef shader = GQShaderManager::bindProgram("priority_buffer");
    NPRGLDraw::setUniformFocusParams(shader, scene);
    setUniformPriorityBufferParams(shader, scene);

    glDisable(GL_BLEND);

    drawQuads(shader, atlas, NPRSegmentAtlas::VISIBILITY_ID);
}
        


void NPRPathRenderer::setUniformPenStyleParams(const GQShaderRef& shader, 
                                               const NPRStyle& style)
{
    const NPRPenStyle* vis_focus = style.penStyle("Base Style");
    const NPRPenStyle* vis_defocus = style.penStyle("Defocused");
    const NPRPenStyle* invis_focus = style.penStyle("Invisible");
    const NPRPenStyle* invis_defocus = style.penStyle("Defocused Invis.");

    /*
    float overshoot = style.transfer(NPRStyle::LINE_OVERSHOOT).v1;
    shader.setUniform1f("overshoot_scale", overshoot );
    */

    // We have to set some texture for the pen even if the user hasn't
    // selected anything to avoid a crash on mac.
    const GQTexture* vis_focus_tex = &_blank_texture;
    const GQTexture* vis_defocus_tex = &_blank_texture;
    const GQTexture* invis_focus_tex = &_blank_texture;
    const GQTexture* invis_defocus_tex = &_blank_texture;

    if (vis_focus->texture()) vis_focus_tex = vis_focus->texture();
    if (vis_defocus->texture()) vis_defocus_tex = vis_defocus->texture();
    if (invis_focus->texture()) invis_focus_tex = invis_focus->texture();
    if (invis_defocus->texture()) invis_defocus_tex = invis_defocus->texture();

    shader.bindNamedTexture("vis_focus_texture", vis_focus_tex);
    shader.bindNamedTexture("vis_defocus_texture", vis_defocus_tex);
    shader.bindNamedTexture("invis_focus_texture", invis_focus_tex);
    shader.bindNamedTexture("invis_defocus_texture", invis_defocus_tex);

    float invis_opacity_factor = style.drawInvisibleLines() ? 1.0f : 0.0f;
    
    vec4f vis_focus_color(vis_focus->color()[0], vis_focus->color()[1],
                          vis_focus->color()[2], vis_focus->opacity());
    vec4f vis_defocus_color(vis_defocus->color()[0], vis_defocus->color()[1],
                          vis_defocus->color()[2], vis_defocus->opacity());
    vec4f invis_focus_color(invis_focus->color()[0], invis_focus->color()[1],
                          invis_focus->color()[2], 
                          invis_focus->opacity()*invis_opacity_factor);
    vec4f invis_defocus_color(invis_defocus->color()[0], invis_defocus->color()[1],
                          invis_defocus->color()[2], 
                          invis_defocus->opacity()*invis_opacity_factor);

    shader.setUniform4fv("vis_focus_color", vis_focus_color);
    shader.setUniform4fv("vis_defocus_color", vis_defocus_color);
    shader.setUniform4fv("invis_focus_color", invis_focus_color);
    shader.setUniform4fv("invis_defocus_color", invis_defocus_color);
        
    shader.setUniform1f("pen_width", vis_focus->stripWidth());

    shader.setUniform1f("texture_length", vis_focus_tex->width());
    shader.setUniform1f("length_scale", vis_focus->lengthScale());
}

void NPRPathRenderer::setUniformPriorityBufferParams(const GQShaderRef& shader,
                                    const NPRScene& scene)
{
    const NPRStyle* style = scene.globalStyle();

    float focused_width = style->penStyle("Base Style")->elisionWidth();
    float defocused_width = style->penStyle("Defocused")->elisionWidth();

    shader.setUniform1f("max_width", std::max(focused_width, defocused_width));

    float transfer[4];
    style->transfer(NPRStyle::LINE_ELISION).toArray(transfer, 
        focused_width, defocused_width);
    shader.setUniform4fv("transfer_width", transfer);

    GLfloat proj[16]; 
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    XForm<float> inverse_projection = XForm<float>(proj);
    invert(inverse_projection);
    shader.setUniformMatrix4fv("inverse_projection", inverse_projection);
}

void NPRPathRenderer::drawQuads(const GQShaderRef& shader, 
                       const NPRSegmentAtlas& atlas,
                       NPRSegmentAtlas::AtlasBufferId visibility_buffer)
{
    if (!_is_initialized)
        init();

    if (atlas.totalSegments() == 0)
        return;

    __MY_START_TIMER("Draw Quads");

    const NPRSettings& settings = NPRSettings::instance();

    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    GLfloat viewport[4];
    glGetFloatv(GL_VIEWPORT, viewport);
    gluOrtho2D(viewport[0], viewport[2], viewport[1], viewport[3]);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    bool wireframe = settings.get(NPR_VIEW_TRI_STRIPS);
    if (wireframe)
    {
        glLineWidth(1.0);
        glPolygonMode(GL_FRONT, GL_LINE);
    }

    shader.bindNamedTexture("path_start_end_ptrs", 
        atlas.pathBuffer(NPRSegmentAtlas::PATH_START_END_ID));
    shader.bindNamedTexture("offset_buffer", 
        atlas.offsetBuffer());
    shader.bindNamedTexture("clip_vert_0_buffer", 
        atlas.clipBuffer(NPRSegmentAtlas::CLIP_VERTEX_0_ID));
    shader.bindNamedTexture("clip_vert_1_buffer", 
        atlas.clipBuffer(NPRSegmentAtlas::CLIP_VERTEX_1_ID));

    shader.bindNamedTexture("segment_atlas", 
        atlas.atlasBuffer(visibility_buffer));

    shader.setUniform4fv("viewport", viewport );
    shader.setUniform1f("clip_buffer_width", atlas.clipBufferWidth() );
    shader.setUniform1f("atlas_width", atlas.atlasWrapWidth() );
    if (shader.uniformLocation("sample_spacing") >= 0)
    {
        shader.setUniform1f("sample_spacing", atlas.sampleSpacing() );
    }

    _quad_vertices_vbo.bind();

    int segments_remaining = atlas.totalSegments();
    int row = 0;
    while(segments_remaining > 0)
    {
        int count = min(segments_remaining, atlas.clipBufferWidth());
        shader.setUniform1f("row", row);
        glDrawArrays(GL_POINTS, 0, count);
        segments_remaining -= count;
        row++;
    }

    _quad_vertices_vbo.unbind();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    if (wireframe)
    {
        glPolygonMode(GL_FRONT, GL_FILL);
    }

    __MY_STOP_TIMER("Draw Quads");
}
        
// Creates a blank dummy texture for rendering quads when the user
// hasn't selected a texture.
void NPRPathRenderer::makeBlankTexture()
{
    int size = 16;
    GQImage blank_img(size, size, 4);
    for (int i = 0; i < size*size*4; i++)
    {
        blank_img.raster()[i] = 255;
    }

    _blank_texture.create(blank_img);
}

// Creates a string of vertices for rendering quads along line segments.
// The vertex positions will be adjusted in a shader.
void NPRPathRenderer::makeQuadRenderingVBO()
{
    QVector<float> vertices;
    int count = GQFramebufferObject::maxFramebufferSize();

    for (int i = 0; i < count; i++)
    {
        vertices.push_back(i);
        vertices.push_back(0);
    }
    _quad_vertices_vbo.clear();
    _quad_vertices_vbo.add(GQ_VERTEX, GL_STATIC_DRAW, 2, &vertices);
    _quad_vertices_vbo.copyDataToVBOs();

    NPRGLDraw::handleGLError();
}

