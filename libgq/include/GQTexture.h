/******************************************************************************\
 *                                                                            *
 *  filename : GQTexture.h                                                   *
 *  author   : Forrester Cole                                                 *
 *                                                                            *
 *  Classes to manage 2D and 3D OpenGL textures. I/O is handled by            *
 *  GQImage.                                                                 *
 *                                                                            *
\******************************************************************************/


#ifndef _GQ_TEXTURE_H_
#define _GQ_TEXTURE_H_

#include "GQImage.h"
#include "GQInclude.h"

#include <QString>

class GQTexture
{
    public:
        static bool is3D(const QString& filename);

        virtual ~GQTexture();

        virtual bool load( const QString& filename ) = 0;

        virtual bool bind() const = 0;
        virtual void unbind() const = 0;
        virtual void enable() const = 0;
        virtual void disable() const = 0;        
        
        virtual unsigned int width() const = 0;
        virtual unsigned int height() const = 0;
        virtual unsigned int depth() const = 0;
        
        int id() const { return _id; }
        virtual int target() const = 0;
        
        void clear();

    protected:
        int _id;
};

class GQTexture2D : public GQTexture
{
    public:
        GQTexture2D();

        bool load( const QString& filename );
        bool create( const GQImage& image, int target = GL_TEXTURE_2D );
        bool create(int width, int height, int internalFormat, int format, int type, const void *data, int target = GL_TEXTURE_2D);
        bool bind() const;
        void unbind() const;
        void enable() const; 
        void disable() const;
        
        unsigned int width() const { return _width; }
        unsigned int height() const { return _height; }
        unsigned int depth() const { return 1; }

        int target() const;

    protected:
        bool genTexture(int width, int height, int internalFormat, int format, int type, const void *data);
    
    protected:
        int _target;
        int _width;
        int _height;
};

class GQTexture3D : public GQTexture
{
    public:
        GQTexture3D();
        ~GQTexture3D();

        bool load( const QString& filename );
        bool create(int width, int height, int depth, int internalFormat, int format, int type, const void *data);
        bool bind() const;
        void unbind() const;
        void enable() const; 
        void disable() const;      
        
        unsigned int width() const { return _slices[0].width(); }
        unsigned int height() const { return _slices[1].height(); }
        unsigned int depth() const { return _num_slices; }        

        int target() const;

	protected:
		bool createGLTexture();	

    protected:
        GQImage* _slices;
        int       _num_slices;

		int		  _wrap_mode_s;
		int		  _wrap_mode_t;
		int		  _wrap_mode_r;
};


#endif /*_GQ_TEXTURE_H_*/
