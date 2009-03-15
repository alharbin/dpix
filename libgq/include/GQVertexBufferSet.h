/*****************************************************************************\

GQVertexBufferSet.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

An abstraction to manage OpenGL vertex arrays and vertex buffer objects. 
A buffer set is stored on the CPU side by default, but may be copied to
the GPU using copyDataToVBOs().

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _GQ_VERTEX_BUFFER_SET_H
#define _GQ_VERTEX_BUFFER_SET_H

#include <QVector>
#include <QHash>
#include <QList>

#include "GQShaderManager.h"
#include "GQFramebufferObject.h"

enum GQVertexBufferSemantic { GQ_VERTEX, GQ_NORMAL, GQ_COLOR, GQ_TEXCOORD, GQ_NUM_SEMANTICS };
const QString GQSemanticNames[GQ_NUM_SEMANTICS] = { "vertex", "normal", "color", "texcoord" }; 

class GQVertexBufferSet
{
    public:
        GQVertexBufferSet() { clear(); }
        ~GQVertexBufferSet() { clear(); }

        void add( GQVertexBufferSemantic semantic, int usage_mode, int width, const QVector<float>* data );
        void add( GQVertexBufferSemantic semantic, int usage_mode, int width, const QVector<uint8>* data );
        void add( GQVertexBufferSemantic semantic, int usage_mode, int width, int format, int length );
        void add( const QString& name, int usage_mode, int width, const QVector<float>* data );
        void add( const QString& name, int usage_mode, int width, const QVector<uint8>* data );
        void add( const QString& name, int usage_mode, int width, int format, int length );

        int  numBuffers() const { return _buffers.size(); }

        int  vboId( GQVertexBufferSemantic semantic ) const;
        int  vboId( const QString& name ) const;

        void setStartingElement( int element ) { _starting_element = element; }
        void setElementStride( int stride ) { _element_stride = stride; }

        void clear();

        void bind() const;
        void bind( const GQShaderRef& current_shader ) const;
        void unbind() const;
        bool isBound() const { return _guid == _bound_guid; }

        void copyDataToVBOs();
        void deleteVBOs();

        void copyFromFBO( const GQFramebufferObject& fbo, int fbo_buffer, const QString& vbo_name );
        void copyFromFBO( const GQFramebufferObject& fbo, int fbo_buffer, GQVertexBufferSemantic vbo_semantic );
        void copyFromSubFBO( const GQFramebufferObject& fbo, int fbo_buffer, int x, int y, 
                             int width, int height, const QString& vbo_name );
        void copyFromSubFBO( const GQFramebufferObject& fbo, int fbo_buffer, int x, int y, 
                             int width, int height, GQVertexBufferSemantic vbo_semantic );

    protected:
        class BufferInfo {
        public:
            void init( const QString& name, int usage_mode, int width, 
                       int data_type, int length, 
                       const QVector<float>* float_data, const QVector<uint8>* uint8_data );

            const uint8* dataPointer() const;
            int          dataSize() const;
        public:
            QString _name;
            GQVertexBufferSemantic _semantic;
            int _data_type;
            int _type_size;
            int _width;
            int _vbo_id;
            int _vbo_size;
            int _usage_mode;
            bool _normalize;

            const QVector<float>* _float_data;
            const QVector<uint8>* _uint8_data;
        };

        QHash<QString, BufferInfo*> _buffer_hash;
        QList<BufferInfo>           _buffers;

        int _starting_element;
        int _element_stride;

        int _guid;

        static bool              _gl_buffer_object_bound;
        static int               _last_used_guid;
        static int               _bound_guid;
        static QList<int>        _bound_attribs;

    protected:
        void add( const BufferInfo& buffer_info );

        void bindBuffer( const BufferInfo& info, int attrib ) const;
};

#endif