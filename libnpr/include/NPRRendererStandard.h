
/******************************************************************************\
 *                                                                            *
 *  filename : NPRRendererStandard.h                                          *
 *  authors  : Forrester Cole												  *
 *																			  *
 *  NPR rendering engine designed for NPR line drawing. Based on the original *
 *  libnpr by R. Keith Morley and Adam Finkelstein.							  *
 *                                                                            *
\******************************************************************************/

#ifndef _NPR_RENDERER_STANDARD_H_
#define _NPR_RENDERER_STANDARD_H_

#include "NPRRenderer.h"
#include "NPRSegmentAtlas.h"
#include "NPRPathRenderer.h"
#include "NPRPathSet.h"

class NPRScene;
class NPRStyle;
class GQShaderRef;

typedef enum
{
    NPR_SPINE_TEST,
    NPR_SEGMENT_ATLAS,
    NPR_NUM_LINE_VISIBILITY_METHODS
} NPRLineVisibilityMethod;

class NPRRendererStandard : public NPRRenderer
{
  public:
    NPRRendererStandard();

    void clear();

    void drawScene( const NPRScene& scene );
    void drawSceneDepth( const NPRScene& scene );

    void resize( int width, int height );

  protected:
    void setLineVisibilityMethod( NPRLineVisibilityMethod method );

    void drawPolygons(const NPRScene& scene);

    void drawDepthBuffer( const NPRScene& scene );
    void drawPriorityBuffer( const NPRScene& scene );
    void visualizeDepthBuffer();
    void visualizePriorityBuffer();

    const GQTexture2D* depthBuffer() const 
        { return _depth_buffer.colorTexture(0); }
    const GQTexture2D* priorityBuffer() const 
        { return _priority_buffer.colorTexture(0); }

  private:
    NPRLineVisibilityMethod _line_viz_method; 

    NPRSegmentAtlas     _segment_atlas;
    NPRPathRenderer     _path_renderer;

    GQFramebufferObject _depth_buffer;
    GQFramebufferObject _priority_buffer;
};

#endif // _NPR_RENDERER_STANDARD_H_

