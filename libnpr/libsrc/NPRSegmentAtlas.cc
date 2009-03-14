#include "NPRSegmentAtlas.h"
#include "NPRGLDraw.h"
#include "GQShaderManager.h"
#include "NPRScene.h"
#include "GQStats.h"
#include "NPRSettings.h"
#include "NPRDrawable.h"
#include "NPRStyle.h"
#include <assert.h>

#include <QVector>

//#define USE_NV_PERF_SDK
#ifdef USE_NV_PERF_SDK
#include "NVPerfSDK.h"
#endif

#if 1
#define __MY_START_TIMER __START_TIMER
#define __MY_STOP_TIMER __STOP_TIMER
#define __MY_TIME_CODE_BLOCK __TIME_CODE_BLOCK
#else
#define __MY_START_TIMER __START_TIMER_AFTER_GL_FINISH
#define __MY_STOP_TIMER __STOP_TIMER_AFTER_GL_FINISH
#define __MY_TIME_CODE_BLOCK __TIME_CODE_BLOCK_AFTER_GL_FINISH
#endif
    
const int MAXIMUM_SAMPLES = 1 << 20;
const int MAXIMUM_SEGMENT_LENGTH = 1 << 10;

static int DUMP_IMAGES = 0;

inline void copyToBuffer(float* buffer, int offset, float a, float b, float c, float d)
{
    buffer[offset + 0] = a;
    buffer[offset + 1] = b;
    buffer[offset + 2] = c;
    buffer[offset + 3] = d;
}

inline void copyToBuffer(float* buffer, int offset, vec3f vec, float w)
{
    buffer[offset + 0] = vec[0];
    buffer[offset + 1] = vec[1];
    buffer[offset + 2] = vec[2];
    buffer[offset + 3] = w;
}

inline void copyToBuffer(QVector<uint8>& buffer, int offset, uint8 a, uint8 b, uint8 c)
{
    buffer[offset + 0] = a;
    buffer[offset + 1] = b;
    buffer[offset + 2] = c;
}



NPRSegmentAtlas::NPRSegmentAtlas()
{
    clear();

#ifdef USE_NV_PERF_SDK
    NVPMInit();
#endif
}

NPRSegmentAtlas::~NPRSegmentAtlas()
{
    clear();

#ifdef USE_NV_PERF_SDK
    NVPMShutdown();
#endif
}

void NPRSegmentAtlas::clear()
{
    _depth_fbo.clear();
    _clip_fbo.clear();
    _sum_fbo.clear();
    _atlas_fbo.clear();
    _path_verts_fbo.clear();
    _clip_viz_fbo.clear();

    _clip_viz_vbo.clear();
    _atlas_source_vbo.clear();

    _quad_vertices_vbo.clear();

    _sample_step = 2.0f;
    _total_segments = 0;
    _total_samples = 0;

    _dump_next_frame = false;
    _draw_profiles = true;
    _path_data_dirty = true;

    for (int i = 0; i < NUM_ATLAS_BUFFERS; i++)
        _is_smoothed_atlas_current[i] = false;


    _is_initialized = false;
}

const GQTexture2D* NPRSegmentAtlas::atlasBuffer(AtlasBufferId which) const
{
    if (_is_smoothed_atlas_current[which])
        return _filtered_atlas_fbo.colorTexture(which);
    else
        return _atlas_fbo.colorTexture(which);
}

int NPRSegmentAtlas::atlasWidth() const
{
    return _atlas_fbo.width();
}

int NPRSegmentAtlas::atlasWrapWidth() const
{
    return _atlas_wrap_width;
}

int NPRSegmentAtlas::maxSegments()
{
    if (_is_initialized)
        return _clip_fbo.width()*_clip_fbo.height();
    else 
        return 0;
}

const int RGBA_FLOAT_FORMAT = GL_RGBA_FLOAT32_ATI;
const int RGBA_FLOAT_FORMAT_HALF = GL_RGBA16F_ARB;
const int ALPHA_FLOAT = GL_ALPHA32F_ARB;
const int TEXTURE_FORMAT = GL_TEXTURE_RECTANGLE_ARB;

static bool initFBOHelper(QString name, GQFramebufferObject* fbo, int num_buffers, 
                   int width, int height, int format = RGBA_FLOAT_FORMAT)
{
    bool success = fbo->init(TEXTURE_FORMAT, format, 
                             GQ_ATTACH_NONE, num_buffers,
                             width, height);
    if (!success)
    {
        qCritical("NPRSegmentAtlas::initFBOHelper: failed to initialize %s fbo.\n", 
                  qPrintable(name));
        return false;
    }

    fbo->setTextureWrap(GL_CLAMP, GL_CLAMP);
    fbo->setTextureFilter(GL_NEAREST, GL_NEAREST);
    return true;
}
        
bool NPRSegmentAtlas::init( const NPRScene& scene )
{
    int width = GQFramebufferObject::maxFramebufferSize();
    int height = ceil((float)MAXIMUM_SAMPLES / (float)width);
    _atlas_wrap_width = width - MAXIMUM_SEGMENT_LENGTH;

    bool success = makePathVertexFBO(scene);
    if (!success)
        return false;

    makeSegmentAtlasVBO();

    initFBOHelper("clip_fbo", &_clip_fbo, 3, 
                  _path_verts_fbo.width(), _path_verts_fbo.height());
    initFBOHelper("clip_length_sum_fbo", &_sum_fbo, 2, 
                  _path_verts_fbo.width(), _path_verts_fbo.height());
    initFBOHelper("segment_atlas", &_atlas_fbo, NUM_ATLAS_BUFFERS, 
                  width, height, GL_RGBA);

    _is_initialized = true;
    return true;
}

void NPRSegmentAtlas::draw( const NPRScene& scene, const GQTexture2D& depth_buffer )
{
    __MY_TIME_CODE_BLOCK("Sample Buffer Draw");

    if (!_is_initialized)
    {
        if (!init(scene))
            return;
    }

    if (_dump_next_frame)
        DUMP_IMAGES = 1;

    NPRSettings& settings = NPRSettings::instance();
    setDrawProfiles(settings.get(NPR_EXTRACT_PROFILES));

    if (scene.hasViewDependentPaths() || _path_data_dirty)
    {
        refreshPathData(scene);
    }

    drawClipBuffer( scene );
    sumSegmentLengths();
    drawSegmentAtlas(VISIBILITY_ID, depth_buffer);

    if (_dump_next_frame)
    {
        DUMP_IMAGES = 0;
        _dump_next_frame = false;
    }
}


void NPRSegmentAtlas::visualize( const NPRScene& scene, const GQTexture2D& depth_buffer )
{
    draw(scene, depth_buffer);

    __MY_START_TIMER("viz clipped lines");
    visualizeClippedLines();
    __MY_STOP_TIMER("viz clipped lines");
}

        
void NPRSegmentAtlas::checkPriority( const GQTexture2D& priority_buffer )
{
    assert(_is_initialized);
    if (!_is_initialized)
        return;

    drawSegmentAtlas(PRIORITY_ID, priority_buffer);
}
        
void NPRSegmentAtlas::filter(AtlasBufferId which, AtlasFilterType type, const NPRStyle* style)
{
    Q_UNUSED(style);

    __MY_TIME_CODE_BLOCK("smooth atlas");

    initFBOHelper("smoothed_segment_atlas", &_filtered_atlas_fbo, 
        NUM_ATLAS_BUFFERS, _atlas_fbo.width(), _atlas_fbo.height(),
        GL_RGBA);

    GQShaderRef shader;
    switch (type)
    {
        case BILATERAL_FILTER : 
            shader = GQShaderManager::bindProgram("bilateral_filter_atlas"); break;
        case MEDIAN_FILTER :
            shader = GQShaderManager::bindProgram("median_filter_atlas"); break;
        case SMOOTH_AND_THRESHOLD_FILTER :
            shader = GQShaderManager::bindProgram("smooth_and_threshold_atlas"); 
            shader.setUniform1f("overshoot_offset", 1.0);
            break;

        default:
            assert(0); return;
    }

    shader.bindNamedTexture("source_buffer", 
                            _atlas_fbo.colorTexture(which));

    _filtered_atlas_fbo.bind();
    _filtered_atlas_fbo.drawBuffer(which);

    glViewport(0,0,_filtered_atlas_fbo.width(),
               _filtered_atlas_fbo.height());

    glClearColor(0.0,0.0,0.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    NPRGLDraw::drawFullScreenQuad( _filtered_atlas_fbo.target() );

    _filtered_atlas_fbo.unbind();

    _is_smoothed_atlas_current[which] = true;
}

void NPRSegmentAtlas::setDrawProfiles( bool draw )
{
    if (draw != _draw_profiles)
    {
        _draw_profiles = draw;
        _path_data_dirty = true;
    }
}

void NPRSegmentAtlas::refreshPathData( const NPRScene& scene )
{
    makePathVertexFBO(scene);
    makeSegmentAtlasVBO();
}

void NPRSegmentAtlas::drawClipBuffer( const NPRScene& scene )
{
    __MY_TIME_CODE_BLOCK("draw clip buf");

    NPRGLDraw::handleGLError();
    NPRGLDraw::clearGLState();
    NPRGLDraw::clearGLScreen(vec(0,0,0), 1.0);

    GQShaderRef shader = GQShaderManager::bindProgram("clip_buffer");

    NPRGLDraw::setUniformViewParams(shader);
    shader.setUniform3fv("view_pos", scene.cameraPosition() );
    shader.setUniform3fv("view_dir", scene.cameraDirection() );
    shader.setUniform1f("sample_step", _sample_step );
    shader.setUniform1f("buffer_width", _clip_fbo.width());

    NPRGLDraw::handleGLError();

    shader.bindNamedTexture("vert0_tex", 
        _path_verts_fbo.colorTexture(PATH_VERTEX_0_ID));
    shader.bindNamedTexture("vert1_tex", 
        _path_verts_fbo.colorTexture(PATH_VERTEX_1_ID));
    shader.bindNamedTexture("face_normal0_tex", 
        _path_verts_fbo.colorTexture(FACE_NORMAL_0_ID));
    shader.bindNamedTexture("face_normal1_tex", 
        _path_verts_fbo.colorTexture(FACE_NORMAL_1_ID));
    shader.bindNamedTexture("path_start_end_ptrs", 
        _path_verts_fbo.colorTexture(PATH_START_END_ID)); 
    
    _clip_fbo.bind();
    _clip_fbo.drawToAllBuffers();

    glViewport(0,0,_clip_fbo.width(),_clip_fbo.height());

    glClearColor(0.5,0.5,0.5,0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    NPRGLDraw::drawFullScreenQuad( _clip_fbo.target() );

    _clip_fbo.unbind();

    if (DUMP_IMAGES)
    {
        _path_verts_fbo.saveColorTextureToFile(0, "verts0.bmp");
        _path_verts_fbo.saveColorTextureToFile(1, "verts1.bmp");
        _clip_fbo.saveColorTextureToFile(0, "clippedv0img.bmp"); 
        _clip_fbo.saveColorTextureToFile(1, "clippedv1img.bmp");
        _clip_fbo.saveColorTextureToFile(2, "numsamplesimg.bmp");
	}

    NPRGLDraw::handleGLError();
}

void NPRSegmentAtlas::visualizeClippedLines()
{
    if (DUMP_IMAGES)
    {
        _clip_fbo.saveColorTextureToFile(0, "v0img2.bmp");
        _clip_fbo.saveColorTextureToFile(1, "v1img2.bmp");
        _clip_fbo.saveColorTextureToFile(2, "numsamplesimg2.bmp");
    }

    if (_clip_viz_vbo.numBuffers() == 0)
    {
        // Set up the VBO and FBO for the clipping debug visualization.
        initFBOHelper("clip_viz_fbo", &_clip_viz_fbo, 1, 
            _clip_fbo.width()*2, _clip_fbo.height(),
            RGBA_FLOAT_FORMAT_HALF);

        int max_segments = _clip_fbo.width()*_clip_fbo.height();
        _clip_viz_vbo.add( GQ_VERTEX, GL_STREAM_COPY, 4, GL_FLOAT, max_segments*4*2 );
        _clip_viz_vbo.copyDataToVBOs();
    }

    NPRGLDraw::clearGLState();

    GQShaderRef shader = GQShaderManager::bindProgram("clip_buffer_viz");

    shader.bindNamedTexture("vert0_tex", _clip_fbo.colorTexture(CLIP_VERTEX_0_ID));
    shader.bindNamedTexture("vert1_tex", _clip_fbo.colorTexture(CLIP_VERTEX_1_ID));

    NPRGLDraw::handleGLError();

    _clip_viz_fbo.bind();

    glViewport(0,0,_clip_fbo.width()*2, _clip_fbo.height());

    glDisable(GL_DEPTH_TEST);
    NPRGLDraw::drawFullScreenQuad(GL_TEXTURE_RECTANGLE_ARB);

    _clip_viz_vbo.copyFromFBO(_clip_viz_fbo, 0, GQ_VERTEX);
    NPRGLDraw::handleGLError();

    _clip_viz_fbo.unbind();

    if (DUMP_IMAGES)
    {
        _clip_viz_fbo.saveColorTextureToFile(0, "clipviz.bmp");
    }

    shader.unbind();

    NPRGLDraw::clearGLState();
    NPRGLDraw::clearGLScreen(vec(0.5,0.5,0.5), 1.0);

    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
   
    _clip_viz_vbo.bind();
    
    glDrawArrays(GL_LINES, 0, _total_segments*2);
    
    _clip_viz_vbo.unbind();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    NPRGLDraw::handleGLError();
}

void NPRSegmentAtlas::sumSegmentLengths()
{
    __MY_TIME_CODE_BLOCK("sum lengths");
    
    NPRGLDraw::handleGLError();

    NPRGLDraw::clearGLState();
    NPRGLDraw::clearGLScreen(vec(0,0,0), 1.0);

    GQShaderRef shader = GQShaderManager::bindProgram("clip_buffer_sum");

    shader.bindNamedTexture("last_pass_buf", 
                            _clip_fbo.colorTexture(SEGMENT_LENGTHS_ID));
    shader.setUniform1f("buffer_width", _clip_fbo.width());

    _sum_fbo.bind();

    // We add one additional sample to capture the final sum value.
    int num_rows = ceil( (float)(_total_segments + 1) / 
                         (float)_clip_fbo.width() );

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0,0,_clip_fbo.width(),num_rows);
    glDisable(GL_DEPTH_TEST);

    int cur_buffer = 0;
    int step_size = 1;
    while (step_size < _total_segments)
    {
        _sum_fbo.drawBuffer(cur_buffer);
        shader.setUniform1f("step_size", step_size);

        NPRGLDraw::drawFullScreenQuad(GL_TEXTURE_RECTANGLE_ARB);

        shader.bindNamedTexture("last_pass_buf", 
                                _sum_fbo.colorTexture(cur_buffer));
        cur_buffer = (cur_buffer + 1) % 2;

        step_size = step_size << 1;
    }

    _sum_result_buffer_id = (cur_buffer + 1) % 2;

    if (DUMP_IMAGES)
    {
        GQFloatImage img;
        _sum_fbo.readColorTexturef(_sum_result_buffer_id, GL_RGBA, img);
        img.scaleValues(0.1f);
        img.save("sumfbo.bmp");
    }

    // read back the total number of samples in all segments 
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT + _sum_result_buffer_id);
    float returned_value;
    glReadPixels(_clip_fbo.width()-1, num_rows-1, 1, 1, GL_RED, GL_FLOAT, 
                 &returned_value);

    glPopAttrib();

    _sum_fbo.unbind();

    // Sanity check: total samples should be an integer
    assert(floor(returned_value) == returned_value);
    _total_samples = returned_value;

    __SET_COUNTER("total samples", _total_samples);

    // Print a warning if we don't have enough room in the atlas.
    if (_total_samples + 1 >= MAXIMUM_SAMPLES)
    {
        qWarning("Ran out of space in segment atlas. Need %d, have %d\n", 
            _total_samples+1, MAXIMUM_SAMPLES);
    }
}

void NPRSegmentAtlas::drawSegmentAtlas(AtlasBufferId target, 
                                       const GQTexture2D& reference_texture)
{
    __MY_TIME_CODE_BLOCK("draw segment atlas");
    
    NPRGLDraw::handleGLError();

    int sample_buf_width = _atlas_fbo.width();
    int sample_buf_height = _atlas_fbo.height();

    NPRGLDraw::clearGLState();
    GQShaderRef shader;
    switch (target) 
    {
        case VISIBILITY_ID : 
            shader = GQShaderManager::bindProgram("segment_atlas"); 
            NPRGLDraw::setUniformSSParams(shader, reference_texture);
            break;
        case PRIORITY_ID : 
            shader = GQShaderManager::bindProgram("segment_atlas_priority"); 
            shader.bindNamedTexture("priority_buffer", &reference_texture);
            break;
        default:
            assert(0);
            return;
    }
    shader.bindNamedTexture("path_start_end_ptrs", 
        _path_verts_fbo.colorTexture(PATH_START_END_ID));
    shader.bindNamedTexture("clip_vert_0_buffer", 
        _clip_fbo.colorTexture(CLIP_VERTEX_0_ID));
    shader.bindNamedTexture("clip_vert_1_buffer", 
        _clip_fbo.colorTexture(CLIP_VERTEX_1_ID));
    shader.bindNamedTexture("offset_buffer", 
        _sum_fbo.colorTexture(_sum_result_buffer_id));

    shader.setUniform1f("clip_buffer_width", _clip_fbo.width() );
    shader.setUniform1f("atlas_width", _atlas_wrap_width );

    NPRGLDraw::setUniformViewParams(shader);

    _atlas_fbo.bind();
    _atlas_fbo.drawBuffer(target);

    glViewport(0,0,sample_buf_width,sample_buf_height);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0,sample_buf_width,0,sample_buf_height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glLineWidth(1.0f);
    glPointSize(1.0f);

    _atlas_source_vbo.bind(shader);

    __MY_START_TIMER("draw sample row lines");

    glMatrixMode(GL_PROJECTION);
    glDrawArrays(GL_POINTS, 0, _total_segments);

    __MY_STOP_TIMER("draw sample row lines");

    _atlas_source_vbo.unbind();

    NPRGLDraw::handleGLError();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    _atlas_fbo.unbind();

    _is_smoothed_atlas_current[target] = false;

    NPRGLDraw::handleGLError();

    if (DUMP_IMAGES)
    {
        GQFloatImage img;
        _atlas_fbo.readColorTexturef(0, GL_RGBA, img);
        img.scaleValues(0.1f);
        img.save("samplefbo.bmp");
    }
}

// Creates the textures representing the 3D positions of the line vertices.
// These positions will be projected and clipped using a fragment program.
bool NPRSegmentAtlas::makePathVertexFBO( const NPRScene& scene )
{
    __TIME_CODE_BLOCK("Make Path Vertex FBO");

    const QList<const NPRFixedPath*>& path_list = scene.sortedPaths();

    // Count the number of paths we need to include.
    int total_profile_segments = 0;
    int total_non_profile_segments = 0;
    for (int i = 0; i < path_list.size(); i++)
    {
        int num_segments = path_list.at(i)->size() - 1;
        if (path_list.at(i)->attributes().type == NPR_PROFILE)
            total_profile_segments += num_segments;
        else
            total_non_profile_segments += num_segments;
    }

    _total_segments = total_non_profile_segments;
    if (_draw_profiles)
        _total_segments += total_profile_segments;

    // Set up the FBO to handle that number of paths. Return false if we can't
    // handle it due to texture size restrictions.
    //
    // Set up the buffer to be the smallest square texture that will fit the
    // data.
    float sqrt_total_segments = ceil(sqrt((float)_total_segments));
    int clip_buf_width = sqrt_total_segments;
    int clip_buf_height = clip_buf_width;

    if (clip_buf_height > clip_buf_width)
    {
        qCritical("Too many segments for clip buffer (%d, max is %d)\n", 
            _total_segments, clip_buf_width*clip_buf_width);
        return false;
    }
    int max_segments = clip_buf_width*clip_buf_height;

    int target = TEXTURE_FORMAT;
    int format = RGBA_FLOAT_FORMAT;
    _path_verts_fbo.init( target, format, GQ_ATTACH_NONE, NUM_PATH_BUFFERS, 
                          clip_buf_width, clip_buf_height );

    NPRGLDraw::handleGLError(__FILE__, __LINE__);

    GQFloatImage vertex_0_img, vertex_1_img, 
                 face_normal_0_img, face_normal_1_img, 
                 path_start_end_img;
    vertex_0_img.resize(clip_buf_width, clip_buf_height, 4);
    vertex_1_img.resize(clip_buf_width, clip_buf_height, 4);
    face_normal_0_img.resize(clip_buf_width, clip_buf_height, 4);
    face_normal_1_img.resize(clip_buf_width, clip_buf_height, 4);
    path_start_end_img.resize(clip_buf_width, clip_buf_height, 4);
    float* vertex_0_buf = vertex_0_img.raster();
    float* vertex_1_buf = vertex_1_img.raster();
    float* face_normal_0_buf = face_normal_0_img.raster();
    float* face_normal_1_buf = face_normal_1_img.raster();
    float* path_start_end_buf = path_start_end_img.raster();
    for (int i = 0; i < max_segments*4; i++)
    {
        vertex_0_buf[i] = 0.0f;
        vertex_1_buf[i] = 0.0f;
        face_normal_0_buf[i] = 0.0f;
        face_normal_1_buf[i] = 0.0f;
        path_start_end_buf[i] = 0.0f;
    }

    // Load the paths into the images. 
    int segment_counter = 0;
    for (int m = 0; m < path_list.size(); m++)
    {
        const NPRFixedPath* path = path_list.at(m);

        xform model_xform, model_inverse_transpose;
        path->drawable()->composeTransform(model_xform);
        path->drawable()->composeTransformInverseTranspose(model_inverse_transpose);

        int nverts = path->size();

        if (path->attributes().type == NPR_PROFILE && _draw_profiles == false)
            continue;

        int path_start = segment_counter;
        int path_end = segment_counter + nverts - 2;
        vec3 path_start_end_vec = vec3(path_start, path_end, 0);

        for (int j = 0; j < nverts-1; j++)
        {
            int offset = segment_counter*4;
            vec3 v0 = model_xform * path->vert(j);
            vec3 v1 = model_xform * path->vert(j+1);

            float v1_w = 0.0f;

            if (path->attributes().type == NPR_PROFILE)
            {
                v1_w = 1.0f;

                assert(nverts == 2);
                vec3 n0 = model_inverse_transpose * path->normal(j);
                vec3 n1 = model_inverse_transpose * path->normal(j+1);

                // Hack: some (sketchup) models seem to have flipped normals.
                // We therefore make sure that the two normals align with each
                // other by checking the dot product. This will cause profiles
                // along edges with angles > 90 to fail, but those edges are
                // usually creases anyway.

                if ((n0 DOT n1) < 0)
                    n0 *= -1.0f;

                copyToBuffer(face_normal_0_buf, offset, n0, 0.0f);
                copyToBuffer(face_normal_1_buf, offset, n1, 0.0f);
            }

            copyToBuffer(vertex_0_buf, offset, v0, 1.0f);
            copyToBuffer(vertex_1_buf, offset, v1, v1_w);
            copyToBuffer(path_start_end_buf, offset, path_start_end_vec, 0.0f);

            segment_counter++;
        }
    }

    _path_verts_fbo.loadColorTexturef(PATH_VERTEX_0_ID, vertex_0_img );
    _path_verts_fbo.loadColorTexturef(PATH_VERTEX_1_ID, vertex_1_img );
    _path_verts_fbo.loadColorTexturef(FACE_NORMAL_0_ID, face_normal_0_img );
    _path_verts_fbo.loadColorTexturef(FACE_NORMAL_1_ID, face_normal_1_img );
    _path_verts_fbo.loadColorTexturef(PATH_START_END_ID, path_start_end_img );
    _path_verts_fbo.setTextureFilter(GL_NEAREST, GL_NEAREST);
    _path_verts_fbo.setTextureWrap(GL_CLAMP, GL_CLAMP);
   

    _path_data_dirty = false;

    return true;
}

// Creates the vertex array that will be used as the source data for rendering
// the segment atlas. A vertex program will move these vertices to the correct 
// locations in the atlas by reading the clip buffer and the clip length
// sum buffer.
void NPRSegmentAtlas::makeSegmentAtlasVBO()
{
    QVector<float> vertices;

    for (int i = 0; i < _total_segments; i++)
    {
        /*for (int j = 0; j < 2; j++)
        {
            // The vertices themselves have x coordinate equal to the
            // segment index, and y coordinate equal to the vertex index
            // in the segment (0 or 1).
            vertices.push_back(i);
            vertices.push_back(j);
        }*/
        vertices.push_back(i);
        vertices.push_back(0);
    }

    _atlas_source_vbo.clear();
    _atlas_source_vbo.add( GQ_VERTEX, GL_STATIC_DRAW, 2, &vertices);
    _atlas_source_vbo.copyDataToVBOs();

    NPRGLDraw::handleGLError();
}
