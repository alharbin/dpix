#ifndef NPR_PATH_RENDERER_H_
#define NPR_PATH_RENDERER_H_

#include "GQImage.h"
#include "GQTexture.h"
#include "GQVertexBufferSet.h"
#include "GQShaderManager.h"

#include "NPRSegmentAtlas.h"

class NPRScene;
class NPRStyle;

class NPRPathRenderer
{
    public:
        NPRPathRenderer();

        void clear();

        void drawSimpleLines(const NPRScene& scene);

        void drawStrokes(const NPRScene& scene,
                         const GQTexture2D& depth_buffer);
        void drawStrokes(const NPRScene& scene, 
                         const NPRSegmentAtlas& atlas );
        void drawPriorityBuffer(const NPRScene& scene, 
                                const NPRSegmentAtlas& atlas );

    protected:
        void init();

        void drawQuads(const GQShaderRef& shader, 
                       const NPRSegmentAtlas& atlas,
                       NPRSegmentAtlas::AtlasBufferId visibility_buffer);

        void makeQuadRenderingVBO();
        void makeBlankTexture();

        void setUniformPenStyleParams(const GQShaderRef& shader, 
                                      const NPRStyle& style);
        void setUniformPriorityBufferParams(const GQShaderRef& shader, 
                                      const NPRScene& scene);

    protected:
        bool         _is_initialized;

        GQTexture2D  _blank_texture;
        GQVertexBufferSet _quad_vertices_vbo;
};

#endif /*NPR_PATH_RENDERER_H_*/
