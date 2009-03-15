/*****************************************************************************\

NPRSegmentAtlas.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Segment atlas creation and storage. These routines correspond to "algorithm 2"
in the TVCG paper.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef NPR_SEGMENT_ATLAS_H_
#define NPR_SEGMENT_ATLAS_H_

#include "GQFramebufferObject.h"
#include "GQVertexBufferSet.h"

class NPRScene;
class NPRStyle;
class GQTexture;

class NPRSegmentAtlas
{
    public:
        typedef enum {
            PATH_VERTEX_0_ID,
            PATH_VERTEX_1_ID,
            FACE_NORMAL_0_ID,
            FACE_NORMAL_1_ID,
            PATH_START_END_ID,
            NUM_PATH_BUFFERS
        } PathBufferId;

        typedef enum {
            CLIP_VERTEX_0_ID,
            CLIP_VERTEX_1_ID,
            SEGMENT_LENGTHS_ID,
            NUM_CLIP_BUFFERS
        } ClipBufferId;

        typedef enum {
            VISIBILITY_ID,
            PRIORITY_ID,
            NUM_ATLAS_BUFFERS
        } AtlasBufferId;

        typedef enum {
            BILATERAL_FILTER,
            MEDIAN_FILTER,
            SMOOTH_AND_THRESHOLD_FILTER,
            NUM_ATLAS_FILTER_TYPES
        } AtlasFilterType;

    public:
        NPRSegmentAtlas();
        ~NPRSegmentAtlas();

        void clear();

        void draw( const NPRScene& scene, const GQTexture2D& depth_buffer );
        void visualize( const NPRScene& scene, const GQTexture2D& depth_buffer );

        void checkPriority( const GQTexture2D& priority_buffer );
        void filter(AtlasBufferId which, AtlasFilterType type, const NPRStyle* style);

        void setDumpImagesNextFrame() { _dump_next_frame = true; }

        const GQTexture2D* pathBuffer(PathBufferId which) const 
            { return _path_verts_fbo.colorTexture(which); }
        int  pathBufferWidth() const { return _path_verts_fbo.width(); }

        const GQTexture2D* clipBuffer(ClipBufferId which) const 
            { return _clip_fbo.colorTexture(which); }
        int  clipBufferWidth() const { return _clip_fbo.width(); }

        const GQTexture2D* offsetBuffer() const 
            { return _sum_fbo.colorTexture(_sum_result_buffer_id); }
        int  offsetBufferWidth() const { return _sum_fbo.width(); }

        const GQTexture2D* depthBuffer() const { return _depth_fbo.colorTexture(0); }
        const GQTexture2D* atlasBuffer(AtlasBufferId which) const; 
        int  atlasWidth() const;
        int  atlasWrapWidth() const;

        int  totalSegments() const { return _total_segments; }
        int  totalSamples() const { return _total_samples; }
        float sampleSpacing() const { return _sample_step; }
        int  maxSegments();

    protected:
        bool init( const NPRScene& scene );
        void drawClipBuffer( const NPRScene& scene );
        void visualizeClippedLines();

        void setDrawProfiles( bool draw );

        void sumSegmentLengths();

        void drawSegmentAtlas(AtlasBufferId target, const GQTexture2D& reference_texture);
        
        void refreshPathData( const NPRScene& scene );

        void makePathVertexTexture( const NPRScene& scene );
        bool makePathVertexFBO( const NPRScene& scene );
        void makeSegmentAtlasVBO();

    protected:
        float        _sample_step;
        int          _total_segments;
        int          _total_samples;
        int          _atlas_wrap_width;

        bool         _draw_profiles;
        bool         _dump_next_frame;
        bool         _path_data_dirty;
        bool         _is_initialized;
        bool         _is_smoothed_atlas_current[NUM_ATLAS_BUFFERS];

        GQFramebufferObject _path_verts_fbo;
        GQFramebufferObject _depth_fbo;

        GQFramebufferObject _clip_fbo;
        GQFramebufferObject _sum_fbo;
        int                 _sum_result_buffer_id;

        GQVertexBufferSet   _atlas_source_vbo;
        GQFramebufferObject _atlas_fbo;
        GQFramebufferObject _filtered_atlas_fbo;

        GQFramebufferObject _clip_viz_fbo;
        GQVertexBufferSet   _clip_viz_vbo;

        GQVertexBufferSet   _quad_vertices_vbo;
};

#endif /*NPR_SEGMENT_ATLAS_H_*/
