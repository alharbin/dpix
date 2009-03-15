/*****************************************************************************\

NPRGLDraw.cc
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "NPRUtility.h"
#include "NPRGLDraw.h"
#include "NPRScene.h"
#include "NPRStyle.h"
#include "NPRLight.h"
#include "NPRSettings.h"
#include "GQTexture.h"
#include "GQShaderManager.h"
#include "GQFramebufferObject.h"
#include "GQStats.h"

#include "NPRGeometry.h"
#include "NPRDrawable.h"
#include "NPRFixedPathSet.h"

#include "CdaMaterial.h"
#include "CdaEffect.h"
#include "CdaTypes.h"

#include <assert.h>

bool NPRGLDraw::_is_initialized = false;
GQTexture2D* NPRGLDraw::_supersample_texture = NULL;

void NPRGLDraw::handleGLError(const char* file, int line)
{
    GLint error = glGetError();
    if (error != 0)
    {
        QString errormsg = "NPRGLDraw::handleGLError: " +
            QString((const char*)gluErrorString(error));
        if (file)
            errormsg = errormsg + QString(" (in %1:%2)").arg(file).arg(line);

        qCritical("%s\n", qPrintable(errormsg));
        qFatal("%s\n", qPrintable(errormsg));
    }
} 


void NPRGLDraw::drawMesh(const NPRScene& scene, int which, int draw_mode )
{
    const NPRDrawable* drawable = scene.drawable(which);

    drawDrawablePolygons(drawable, draw_mode);
}

void NPRGLDraw::drawMeshes(const NPRScene& scene, const QList<int>* list, int draw_mode )
{
    const NPRGeometry* current_geom = 0;
    int count = (list) ? list->size() : scene.numDrawables();
    for (int i = 0; i < count; i++)
    {
        int index = (list) ? (*list)[i] : i;
        if (scene.isDrawablePotentiallyVisible(index))
        {
            const NPRDrawable* drawable = scene.drawable(index); 

            const NPRPrimPointerList& triangles = 
                drawable->geometry()->primList(NPR_TRIANGLES);
            const NPRPrimPointerList& tri_strips = 
                drawable->geometry()->primList(NPR_TRIANGLE_STRIP);
            const NPRPrimPointerList& lines = 
                drawable->geometry()->primList(NPR_LINES);
            const NPRPrimPointerList& line_strips = 
                drawable->geometry()->primList(NPR_LINE_STRIP);
            const NPRPrimPointerList& profiles = 
                drawable->geometry()->primList(NPR_PROFILES);

            bool triangles_to_draw = (triangles.size() > 0 || tri_strips.size() > 0) && 
                                (draw_mode & NPR_DRAW_POLYGONS);
            bool lines_to_draw = (lines.size() > 0 || line_strips.size() > 0) && 
                                  (draw_mode & NPR_DRAW_LINES);
            bool profiles_to_draw = (profiles.size() > 0) && 
                                (draw_mode & NPR_DRAW_PROFILES);

            if (triangles_to_draw || lines_to_draw || profiles_to_draw)
            {
                glMatrixMode(GL_MODELVIEW);
                drawable->pushTransform();

                if (current_geom != drawable->geometry())
                {
                    if (current_geom)
                        current_geom->unbind();
                    current_geom = drawable->geometry();
                    current_geom->bind();
                    GQStats::instance().addToCounter("Polygon binds", 1);
                }

                if (triangles_to_draw)
                {
                    glColor3f(1.0f, 1.0f, 1.0f);

                    drawPrimList(GL_TRIANGLES, drawable, triangles, draw_mode);
                    drawPrimList(GL_TRIANGLE_STRIP, drawable, tri_strips, draw_mode);
                }
                if (lines_to_draw || profiles_to_draw)
                {
                    const NPRPenStyle* pen_style = scene.drawableStyle(index)->penStyle(0);
                    float width = pen_style->stripWidth();
                    const vec& color = pen_style->color();
                    glLineWidth(width*2);
                    glColor3fv(color);

                    if (lines_to_draw)
                    {
                        drawPrimList(GL_LINES, drawable, lines, draw_mode);
                        drawPrimList(GL_LINE_STRIP, drawable, line_strips, draw_mode);
                    }
                    if (profiles_to_draw)
                    {
                        drawPrimList(GL_LINES, drawable, profiles, draw_mode);
                    }
                }

                drawable->popTransform();
            }
        }
    }
    if (current_geom)
    {
        current_geom->unbind();
    }

    NPRGLDraw::handleGLError();
}

void NPRGLDraw::drawMeshesDepth( const NPRScene& scene )
{
    int mode = NPR_DRAW_POLYGONS | NPR_OPAQUE;
    if (NPRSettings::instance().get(NPR_ENABLE_TRANSPARENT_POLYGONS) == true)
        mode |= NPR_TRANSLUCENT;

    drawMeshes(scene, 0, mode);

    NPRGLDraw::handleGLError();
}

void setGLMaterial( const CdaMaterial* material )
{
    if (material)
    {
        // set appropriate material values
        const CdaEffect* effect = material->_inst_effect;
        CdaColor4 diffuse = effect->_diffuse;
        CdaColor4 specular = effect->_specular;
        CdaColor4 ambient = effect->_ambient;
        diffuse[3] *= (1.0f - effect->_transparency);
        specular[3] *= (1.0f - effect->_transparency);
        ambient[3] *= (1.0f - effect->_transparency);
        
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse );
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular ); 
        glMaterialfv(GL_FRONT, GL_AMBIENT, ambient ); 
        glMaterialf(GL_FRONT, GL_SHININESS, effect->_shininess ); 
    }
    else
    {
        // set some default material values
        vec4f white = vec4f(1.0f, 1.0f, 1.0f, 1.0f);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, white );
        glMaterialfv(GL_FRONT, GL_SPECULAR, white ); 
        glMaterialfv(GL_FRONT, GL_AMBIENT, white ); 
        glMaterialf(GL_FRONT, GL_SHININESS, 20 ); 
    }
}

bool isMaterialOpaque( const CdaMaterial* material )
{
    if (material && material->_inst_effect->_transparency == 0.0f)
    {
        return true;
    }
    return false;
}

void NPRGLDraw::drawPrimList(int mode, const NPRDrawable* drawable, 
                             const NPRPrimPointerList& prims, int draw_mode)
{
    // last_material has to be initialized non-zero, because 0 represents "no material".
    const CdaMaterial* last_material = (const CdaMaterial*)1; 

    for (int i = 0; i < prims.size(); i++)
    {
        const NPRPrimitive* prim = prims[i];
        const CdaMaterial* material = drawable->lookupMaterial(prim);
        bool opaque = isMaterialOpaque(material);
        if (((draw_mode & NPR_OPAQUE) && opaque) || 
            ((draw_mode & NPR_TRANSLUCENT) && !opaque) )
        {
            if (material != last_material)
            {
                setGLMaterial(material);
                last_material = material;
            }
            glDrawElements(mode, prim->size(), GL_UNSIGNED_INT, prim->constData());
        }
    }
}

void NPRGLDraw::drawDrawablePolygons( const NPRDrawable* drawable, int draw_mode )
{
    const NPRPrimPointerList& triangles = drawable->geometry()->primList(NPR_TRIANGLES);

    if (triangles.size() > 0)
    {
        glMatrixMode(GL_MODELVIEW);
        drawable->pushTransform();

        GQStats::instance().addToCounter("Polygon binds", 1);

        drawable->geometry()->bind();

        glColor3f(1.0f, 1.0f, 1.0f);

        drawPrimList(GL_TRIANGLES, drawable, triangles, draw_mode);

        drawable->geometry()->unbind();
        drawable->popTransform();
    }
}

void NPRGLDraw::applyMaterialToGL( const CdaMaterial* material )
{
    Q_UNUSED(material);
}

void NPRGLDraw::setPerModelPolygonUniforms( const GQShaderRef* shader, const NPRScene& scene, int which )
{
    if (shader && shader->getName() == "polygon_render")
    {
        const NPRStyle* style = scene.drawableStyle(which);

        float transfer[4];
        float selection_scale = 0.0f;
        if (scene.numSelectedDrawables() > 0 && !(scene.isDrawableSelected(which)))
            selection_scale = 0.2f;

        style->transfer(NPRStyle::FILL_FADE).toArray(transfer);
        transfer[0] = transfer[0] * (1.0 - selection_scale) + selection_scale;
        transfer[1] = transfer[1] * (1.0 - selection_scale) + selection_scale;
        glUniform4fv(shader->uniformLocation("transfer_fade"), 1, transfer);
    }
}

void NPRGLDraw::drawPaperQuad( const NPRStyle* style )
{
    const GQTexture* paper_texture = style->paperTexture();
    
    float transfer[4];
    style->transfer(NPRStyle::PAPER_PARAMS).toArray(transfer);
        
    // early exit if no paper texture or paper texture is totally transparent
    if (paper_texture == NULL || transfer[0] == 0.0f)
        return;
        
    paper_texture->bind();
    paper_texture->enable();
    
    // get viewport dimensions to calculate texture coordinates
    float viewport[4];
    glGetFloatv(GL_VIEWPORT, viewport);
    
    float scaled_width = viewport[2] / (float)(paper_texture->width()); 
    float scaled_height = viewport[3] / (float)(paper_texture->height());
    scaled_width *= 1 - transfer[1]*0.875f;
    scaled_height *= 1 - transfer[1]*0.875f;
    
    // scale texture appropriately
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    glScalef(scaled_width, scaled_height, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1.0f, 1.0f, 1.0f, transfer[0]);
    glDisable(GL_DEPTH_TEST);

    drawFullScreenQuad( GL_TEXTURE_2D );  

    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    paper_texture->disable();
    
}

void NPRGLDraw::visualizeFBO(const GQFramebufferObject& fbo, int which)
{
    fbo.colorTexture(which)->bind();
    fbo.colorTexture(which)->enable();

    drawFullScreenQuad(fbo.target());

    fbo.colorTexture(which)->disable();
}


void NPRGLDraw::drawBackgroundTex( const NPRStyle* style )
{
    const GQTexture* background_tex = style->backgroundTexture();

    float rescale = 1.0f;

    if (!background_tex)
        return;

    background_tex->bind();
    background_tex->enable();

    // get viewport dimensions to calculate texture coordinates
    float viewport[4];
    glGetFloatv(GL_VIEWPORT, viewport);
    
    float scaled_width = viewport[2] / (float)(background_tex->width()); 
    float scaled_height = viewport[3] / (float)(background_tex->height());
    
    // scale texture appropriately
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    glScalef(scaled_width*rescale, scaled_height*rescale, 1.0f);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    drawFullScreenQuad( GL_TEXTURE_2D );  

    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    background_tex->disable();
}

void NPRGLDraw::drawFullScreenQuad( int texture_mode )
{
    NPRGLDraw::handleGLError();

    GLint main_viewport[4];
    glGetIntegerv (GL_VIEWPORT, main_viewport);
    int width  = main_viewport[2];
    int height = main_viewport[3];

    // draw full screen quad. start by setting projection and modelview to identity
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // turn off depth testing
/*    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);*/

    if (texture_mode == GL_TEXTURE_2D)
    {
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1, 0);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1, 1);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0, 1);
        glVertex2f(-1.0f, 1.0f);
        glEnd();
    }
    else
    {
        glColor4f(1.0, 1.0, 1.0, 1.0);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2i(width, 0);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2i(width, height);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2i(0, height);
        glVertex2f(-1.0f, 1.0f);
        glEnd();
    }

    // restore matrix state
	
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    NPRGLDraw::handleGLError();
}

void NPRGLDraw::drawFullScreenQuadFBO( const GQFramebufferObject& fbo )
{
    NPRGLDraw::handleGLError();

    // draw full screen quad. start by setting projection and modelview to identity
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if (fbo.target() == GL_TEXTURE_2D)
    {
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1, 0);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1, 1);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0, 1);
        glVertex2f(-1.0f, 1.0f);
        glEnd();
    }
    else
    {
        int width = fbo.width();
        int height = fbo.height();
        glColor4f(1.0, 1.0, 1.0, 1.0);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2i(width, 0);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2i(width, height);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2i(0, height);
        glVertex2f(-1.0f, 1.0f);
        glEnd();
    }

    // restore matrix state
	
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    NPRGLDraw::handleGLError();
}

void NPRGLDraw::drawPosAndNormalBuffer( const NPRScene& scene )
{ 
    NPRGLDraw::clearGLState();
    GQShaderRef shader = GQShaderManager::bindProgram("pos_and_normal");
	
    // set up for zbuff render
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    
    // clear everything
    glClearColor(1.0, 1.0, 1.0, 1.0);
    //glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    NPRGLDraw::drawMeshes( scene );
}
        
void NPRGLDraw::drawPosAndNormalBufferFBO( const NPRScene& scene, 
                        GQFramebufferObject& fbo, float supersample_factor )
{
    GLint main_viewport[4];
    glGetIntegerv (GL_VIEWPORT, main_viewport);
    // The shader writes world position, world normal, camera depth, 
    // camera position, camera normal buffers.
	fbo.init( GL_TEXTURE_RECTANGLE_ARB, GL_RGBA32F_ARB, GQ_ATTACH_DEPTH, 5,
                              main_viewport[2]*supersample_factor, main_viewport[3]*supersample_factor);

    fbo.bind();
    fbo.drawToAllBuffers();

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0,0,fbo.width(), fbo.height());
	
    drawPosAndNormalBuffer( scene );

    glPopAttrib();

    fbo.unbind();
}

int NPRGLDraw::drawDepthBufferFBO(const NPRScene& scene, 
                                  GQFramebufferObject& fbo, float supersample_factor )
{
    GLint main_viewport[4];
    glGetIntegerv (GL_VIEWPORT, main_viewport);
    int format = GL_RGBA32F_ARB;
	// While the ALPHA32F texture appears to work on windows, it doesn't on mac, and
	// it is apparently not officially allowed by the FBO extension spec.
    //int format = GL_ALPHA32F_ARB;
	bool success = fbo.init(GL_TEXTURE_RECTANGLE_ARB, format, GQ_ATTACH_DEPTH, 1,
                             main_viewport[2]*supersample_factor, 
                             main_viewport[3]*supersample_factor);
    if (!success)
        return NPR_FAILURE;

    fbo.setTexParameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fbo.setTexParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //fbo.setTexParameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //fbo.setTexParameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    fbo.bind();
    fbo.drawToAllBuffers();

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0,0,fbo.width(), fbo.height());
	
    NPRGLDraw::clearGLState();
    GQShaderRef shader = GQShaderManager::bindProgram("float_depth_buffer");

    // set up for zbuff render
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    // clear everything
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    NPRGLDraw::drawMeshesDepth( scene );

    glPopAttrib();

    fbo.unbind();

    return NPR_SUCCESS;
}

void NPRGLDraw::clearGLScreen(const vec& color, float depth)
{
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(color[0], color[1], color[2], 0.0);
    glClearDepth(depth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void NPRGLDraw::clearGLDepth(float depth)
{
    glDepthMask(GL_TRUE);
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void NPRGLDraw::clearGLState()
{
    glDisable(GL_DITHER);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_NORMALIZE);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(0);

    for (int i = 0; i < 8; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_3D, 0);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
    }
    glActiveTexture(GL_TEXTURE0);
}

void NPRGLDraw::setUniformSSParams(const GQShaderRef& shader,
                                   const GQTexture2D& depth_buffer)
{
    if (!_is_initialized)
        init();

    NPRSettings& settings = NPRSettings::instance();

    int supersample_count = settings.get(NPR_LINE_VISIBILITY_SUPERSAMPLE);
    float depth_scale = settings.get(NPR_SEGMENT_ATLAS_DEPTH_SCALE);
    float kernel_scale_x = settings.get(NPR_SEGMENT_ATLAS_KERNEL_SCALE_X);
    float kernel_scale_y = settings.get(NPR_SEGMENT_ATLAS_KERNEL_SCALE_Y);

    shader.setUniform1f("ss_params.buffer_scale", depth_scale);
    shader.setUniform1f("ss_params.t_scale", 
        kernel_scale_x * depth_scale);
    shader.setUniform1f("ss_params.s_scale", 
        kernel_scale_y * depth_scale);
    shader.setUniform1f("ss_params.one_over_count", 
        1.0f / (float)supersample_count);
    shader.setUniform1i("ss_params.sample_count", supersample_count);

    shader.bindNamedTexture("supersample_locations", 
                            _supersample_texture);
    shader.bindNamedTexture("depth_buffer", 
                            &depth_buffer);
}

void NPRGLDraw::setUniformViewParams(const GQShaderRef& shader)
{
    GLfloat viewport[4];
    GLfloat mv[16],proj[16]; 
    glGetFloatv(GL_VIEWPORT, viewport);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);

    int viewport_id = shader.uniformLocation("viewport");
    if (viewport_id >= 0)
        glUniform4fv(viewport_id, 1, viewport);

    int modelview_id = shader.uniformLocation("modelview");
    if (modelview_id >= 0)
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, mv);

    int projection_id = shader.uniformLocation("projection");
    if (projection_id >= 0)
        glUniformMatrix4fv(projection_id, 1, GL_FALSE, proj);

    int inverse_projection_id = shader.uniformLocation("inverse_projection");
    if (inverse_projection_id >= 0)
    {
        XForm<float> inverse_projection = XForm<float>(proj);
        invert(inverse_projection);
        glUniformMatrix4fv(inverse_projection_id, 1, GL_FALSE, inverse_projection);
    }
}

void NPRGLDraw::setUniformPolygonParams(const GQShaderRef& shader, 
                                        const NPRScene& scene) 
{
    bool lighting_enabled = NPRSettings::instance().get(NPR_ENABLE_LIGHTING);
    shader.setUniform3fv("light_dir", scene.light(0)->lightDir() );
    if (lighting_enabled)
        shader.setUniform1i("light_mode", scene.light(0)->mode());
    else
        shader.setUniform1i("light_mode", -1);

    const NPRStyle* style = scene.globalStyle();

    float transfer[4];

    style->transfer(NPRStyle::FILL_DESAT).toArray(transfer);
    shader.setUniform4fv("transfer_desat", transfer);

    style->transfer(NPRStyle::FILL_FADE).toArray(transfer);
    shader.setUniform4fv("transfer_fade", transfer);

    shader.setUniform4f("background_color", 
        style->backgroundColor()[0],
        style->backgroundColor()[1],
        style->backgroundColor()[2],
        1.0);
}


void NPRGLDraw::setUniformFocusParams(const GQShaderRef& shader, 
                                      const NPRScene& scene)
{
    const NPRStyle* style = scene.globalStyle();
    float transfer[4];

    // cameraTransform is actually the transformation from camera to
    // world space (i.e., the position of the camera in world), 
    // so we want the inverse here.
    vec camera_poa = scene.cameraInverseTransform() * scene.focalPoint();

    style->transfer(NPRStyle::FOCUS_TRANSFER).toArray(transfer);
    shader.setUniform4fv("focus_transfer", transfer);
    shader.setUniform1i("focus_mode", NPRSettings::instance().get(NPR_FOCUS_MODE));

    float radius = scene.sceneRadius();

    shader.setUniform1f("model_size", radius);
    shader.setUniform3fv("focus_3d_poa", camera_poa);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    shader.setUniform1f("screen_aspect_ratio", (float)viewport[2] / (float)viewport[3]);
    // transform screen to clip coordinates
    float cx = scene.focalPoint()[0] / ((float)viewport[2]*0.5) - 1;
    float cy = scene.focalPoint()[1] / ((float)viewport[3]*0.5) - 1;
    shader.setUniform2f("focus_2d_poa", cx, cy );
}

void NPRGLDraw::init()
{
    initSupersampleTexture();
    _is_initialized = true;
}

void NPRGLDraw::initSupersampleTexture()
{
    srand(1000);

    GQImage random_img(64, 64, 3);
    random_img.raster()[0] = 128;
    random_img.raster()[1] = 128;
    random_img.raster()[2] = 128;
    for (int i = 3; i < 64*64*3; i++)
    {
        unsigned char r = ((float)rand() / (float)RAND_MAX) * 255.0f;
        random_img.raster()[i] = r;
    }

    _supersample_texture = new GQTexture2D();
    _supersample_texture->create(random_img, GL_TEXTURE_RECTANGLE_ARB);
}
