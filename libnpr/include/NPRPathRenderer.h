/*****************************************************************************\

NPRPathRenderer.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Stroke rendering functions for the various rendering methods supported.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

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
                       int draw_mode);

        void makeQuadRenderingVBO();
        void makeBlankTextures();

        void setUniformPenStyleParams(const GQShaderRef& shader, 
                                      const NPRStyle& style);
        void setUniformPriorityBufferParams(const GQShaderRef& shader, 
                                      const NPRScene& scene);

    protected:
        bool         _is_initialized;

        GQTexture2D  _blank_white_texture;
        GQTexture2D  _blank_white_texture_rect;
        GQVertexBufferSet _quad_vertices_vbo;
};

#endif /*NPR_PATH_RENDERER_H_*/
