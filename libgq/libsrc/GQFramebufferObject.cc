#include "GQFramebufferObject.h"

#include <QtGlobal>
#include <QFile>
#include <assert.h>

int GQFramebufferObject::_bound_guid = 0;
int GQFramebufferObject::_last_used_guid = 1;

GQFramebufferObject::GQFramebufferObject()
{
    _fbo = -1;
}

GQFramebufferObject::~GQFramebufferObject()
{
    clear();
}

void GQFramebufferObject::clear()
{
    if (_fbo >= 0)
    {
        for (int i = 0; i < _num_color_attachments; i++) {
            delete _color_attachments[i];
        }

        delete _color_attachments;
		
        if (_depth_attachment >= 0)
        {
            glDeleteRenderbuffersEXT(1, (GLuint*)(&_depth_attachment));
        }

        glDeleteFramebuffersEXT(1, (GLuint*)(&_fbo));

        _fbo = -1;
        _attachments = GQ_ATTACH_NONE;
    }
    _width = -1;
    _height = -1;
    _guid = _last_used_guid++;
    if (_guid == 0) _guid = _last_used_guid++;
}

bool GQFramebufferObject::init( int target, int format, uint32 attachments, 
                                   int num_color_attachments, int width, int height )
{
    if (_fbo < 0 ||
        _target != target ||
        _format != format ||
        _attachments != attachments ||
        _num_color_attachments != num_color_attachments ||
        _width != width ||
        _height != height)
    {
         assert(width > 0 && height > 0);
         int max_size = maxFramebufferSize();
         if (width > max_size || height > max_size)
         {
             qCritical("Requested FBO size %d x %d, which exceeds max size %d x %d",
                 width, height, max_size, max_size);
             return false;
         }

         int max_attachments = maxColorAttachments();
         if (num_color_attachments > max_attachments)
         {
             qCritical("Requested FBO with %d color attachments, but only %d are supported",
                 num_color_attachments, max_attachments);
             return false;
         }

        clear();

        _target = target;
        _format = format;
        _width = width;
        _height = height;
        _attachments = attachments;

        // create the fbo
        glGenFramebuffersEXT(1, (GLuint*)(&_fbo));
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);

        // attach the color buffers
        _num_color_attachments = num_color_attachments;
        _color_attachments = new GQTexture2D*[_num_color_attachments];
        for (int i = 0; i < _num_color_attachments; i++)
        {
            _color_attachments[i] = new GQTexture2D;
            _color_attachments[i]->create(_width, _height, _format, GL_RGBA, GL_UNSIGNED_BYTE, NULL, _target);

            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + i, _target, _color_attachments[i]->id(), 0);
        }
        glBindTexture( _target, 0 );
		
        if ( _attachments & GQ_ATTACH_DEPTH)
        {
            // attach a depth buffer
            glGenRenderbuffersEXT(1, (GLuint*)(&_depth_attachment));
            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, _depth_attachment);
            glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, _width, _height);
            glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, _depth_attachment);
        }
        else
        {
            _depth_attachment = -1;
        }
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

        GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            QString die_msg;
            die_msg.sprintf("Could not set up framebuffer object. Tried %dx%d, %d attachments, %d format, %d depth attachment",
                _width, _height, _num_color_attachments, _format, _attachments & GQ_ATTACH_DEPTH);
            qCritical(qPrintable(die_msg));
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            clear();
            return false;
        }

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    }
    return true;
}

bool GQFramebufferObject::initFullScreen( int target, int format, uint32 attachments, int num_color_attachments )
{
    GLint viewport[4];
    glGetIntegerv (GL_VIEWPORT, viewport);

    return init( target, format, attachments, num_color_attachments, viewport[2], viewport[3] );
}

void GQFramebufferObject::bind() const
{
    assert(_fbo >= 0);
    assert(_bound_guid == 0);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);
    glPushAttrib(GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT);
    _bound_guid = _guid;
}

void GQFramebufferObject::unbind() const
{
    assert(_fbo >= 0);
    assert(_bound_guid == _guid);
    glPopAttrib();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    _bound_guid = 0;
}

void GQFramebufferObject::drawBuffer( int which )
{
    assert( which >= 0 && which < _num_color_attachments );
    glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT + which );
}

void GQFramebufferObject::drawToAllBuffers()
{
    GLenum render_targets[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT,
                                GL_COLOR_ATTACHMENT2_EXT, GL_COLOR_ATTACHMENT3_EXT,
                                GL_COLOR_ATTACHMENT4_EXT, GL_COLOR_ATTACHMENT5_EXT,
                                GL_COLOR_ATTACHMENT6_EXT, GL_COLOR_ATTACHMENT7_EXT,
                                GL_COLOR_ATTACHMENT8_EXT, GL_COLOR_ATTACHMENT9_EXT,
                                GL_COLOR_ATTACHMENT10_EXT, GL_COLOR_ATTACHMENT11_EXT,
                                GL_COLOR_ATTACHMENT12_EXT, GL_COLOR_ATTACHMENT13_EXT,
                                GL_COLOR_ATTACHMENT14_EXT, GL_COLOR_ATTACHMENT15_EXT };
    assert(_num_color_attachments > 0 && _num_color_attachments < 16);
    glDrawBuffers(_num_color_attachments, render_targets);
}

GQTexture2D* GQFramebufferObject::colorTexture( int which )
{
    assert( which >= 0 && which < _num_color_attachments );
    return _color_attachments[which];
}

const GQTexture2D* GQFramebufferObject::colorTexture( int which ) const
{
    assert( which >= 0 && which < _num_color_attachments );
    return _color_attachments[which];
}

void GQFramebufferObject::setTexParameteri(unsigned int param, int value)
{
    for (int i = 0; i < _num_color_attachments; i++) {
        _color_attachments[i]->bind();
        glTexParameteri(_target, param, value);
        _color_attachments[i]->unbind();
    }
}

void GQFramebufferObject::setTexParameteriv(unsigned int param, int* value)
{
    for (int i = 0; i < _num_color_attachments; i++) {
        _color_attachments[i]->bind();
        glTexParameteriv(_target, param, (GLint*)value);
        _color_attachments[i]->unbind();
    }
}

void GQFramebufferObject::setTexParameterf(unsigned int param, float value)
{
    for (int i = 0; i < _num_color_attachments; i++) {
        _color_attachments[i]->bind();
        glTexParameterf(_target, param, value);
        _color_attachments[i]->unbind();
    }
}
void GQFramebufferObject::setTexParameterfv(unsigned int param, float* value)
{
    for (int i = 0; i < _num_color_attachments; i++) 
    {
        _color_attachments[i]->bind();
        glTexParameterfv(_target, param, value);
        _color_attachments[i]->unbind();
    }
}

void GQFramebufferObject::setTextureFilter(int filter_min, int filter_mag)
{
    for (int i = 0; i < _num_color_attachments; i++)
    {
        _color_attachments[i]->bind();
        setTexParameteri(GL_TEXTURE_MAG_FILTER, filter_mag);
        setTexParameteri(GL_TEXTURE_MIN_FILTER, filter_min);
        _color_attachments[i]->unbind();
    }
}

void GQFramebufferObject::setTextureWrap(int wrap_s, int wrap_t)
{
    for (int i = 0; i < _num_color_attachments; i++)
    {
        _color_attachments[i]->bind();
        setTexParameteri(GL_TEXTURE_WRAP_S, wrap_s);
        setTexParameteri(GL_TEXTURE_WRAP_T, wrap_t);
        _color_attachments[i]->unbind();
    }
}

void GQFramebufferObject::readColorTexturei( int which, int format, GQImage& image ) const 
{
    assert( which >= 0 && which < _num_color_attachments );
    assert(format == GL_RGB || format == GL_RGBA);
    int chan = 4;
    if (format == GL_RGB)
        chan = 3;
    image.resize( _width, _height, chan );

    _color_attachments[which]->bind();
    glGetTexImage( _target, 0, format, GL_UNSIGNED_BYTE, 
                   image.raster());
    _color_attachments[which]->unbind();
}

void GQFramebufferObject::readColorTexturef( int which, int format, GQFloatImage& image ) const 
{
    assert( which >= 0 && which < _num_color_attachments );
    assert(format == GL_RGB || format == GL_RGBA);
    int chan = 4;
    if (format == GL_RGB)
        chan = 3;
    image.resize( _width, _height, chan );

    _color_attachments[which]->bind();
    glGetTexImage( _target, 0, format, GL_FLOAT, 
                   image.raster());
    _color_attachments[which]->unbind();
}	

void GQFramebufferObject::loadColorTexturei( int which, const GQImage& image )
{
    assert( which >= 0 && which < _num_color_attachments );
    _color_attachments[which]->bind();
    int format = GL_RGBA;
    if (image.chan() == 3)
        format = GL_RGB;
    glTexSubImage2D( _target, 0, 0,0,_width,_height, format, GL_UNSIGNED_BYTE, 
                   image.raster());
    _color_attachments[which]->unbind();
}


void GQFramebufferObject::loadColorTexturef( int which, const GQFloatImage& image )
{
    assert( which >= 0 && which < _num_color_attachments );
    _color_attachments[which]->bind();
    int format = GL_RGBA;
    if (image.chan() == 3)
        format = GL_RGB;
    glTexSubImage2D( _target, 0, 0,0,_width,_height, format, GL_FLOAT, 
                   image.raster());
    _color_attachments[which]->unbind();
}

void GQFramebufferObject::readSubColorTexturei( int which, int x, int y, int width, int height,
                                                  int format, GQImage& image ) const 
{
    // N.B.: on several cards (8800GTS, 7800GT) this call returns bogus results
    // if the FBO width > 4k
    assert( which >= 0 && which < _num_color_attachments );
    assert(format == GL_RGB || format == GL_RGBA);
    assert(_bound_guid == _guid);
    int chan = 4;
    if (format == GL_RGB)
        chan = 3;
    image.resize( width, height, chan );

    GLint last_buffer;
    glGetIntegerv( GL_READ_BUFFER, &last_buffer );
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer( GL_COLOR_ATTACHMENT0_EXT + which );
    glReadPixels( x, y, width, height, format, GL_UNSIGNED_BYTE, image.raster() );
    glReadBuffer( last_buffer );

    reportGLError();
}

void GQFramebufferObject::readSubColorTexturef( int which, int x, int y, int width, int height,
                                                  int format, GQFloatImage& image ) const 
{
    assert( which >= 0 && which < _num_color_attachments );
    assert(format == GL_RGB || format == GL_RGBA);
    assert(_bound_guid == _guid);
    int chan = 4;
    if (format == GL_RGB)
        chan = 3;
    image.resize( width, height, chan );

    GLint last_buffer;
    glGetIntegerv( GL_READ_BUFFER, &last_buffer );
    glReadBuffer( GL_COLOR_ATTACHMENT0_EXT + which );
    glReadPixels( x, y, width, height, format, GL_FLOAT, image.raster() );
    glReadBuffer( last_buffer );
}

void GQFramebufferObject::readDepthBuffer( GQFloatImage& image ) const 
{
    readSubDepthBuffer( 0, 0, _width, _height, image );
}

void GQFramebufferObject::readSubDepthBuffer( int x, int y, int width, int height, GQFloatImage& image ) const 
{
    assert(_attachments & GQ_ATTACH_DEPTH);
    image.resize( width, height, 1 );

    glReadPixels( x, y, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, image.raster() );
}

void GQFramebufferObject::saveColorTextureToFile( int which, const QString& filename ) const 
{
    if (filename.endsWith("float") || filename.endsWith("pfm"))
    {
        GQFloatImage img;
        readColorTexturef( which, GL_RGBA, img );
        img.save(filename);
    }
    else 
    {
        GQImage img;
        readColorTexturei( which, GL_RGBA, img );
        img.save(filename);
    }
}

void GQFramebufferObject::saveDepthBufferToFile( const QString& filename ) const 
{
    GQFloatImage img;
    readDepthBuffer( img );
    img.save(filename);
}

int GQFramebufferObject::maxFramebufferSize()
{
    GLint size;
    glGetIntegerv( GL_MAX_TEXTURE_SIZE, &size );
    return size;
}

int GQFramebufferObject::maxColorAttachments()
{
    GLint num;
    glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS_EXT, &num);
    return num;
}


// GQSingleFBO

bool GQSingleFBO::init( int target, int format, uint32 attachments, 
                                   int num_color_attachments, int width, int height )
{
    if (_fbo < 0 ||
        _target != target ||
        _format != format ||
        _attachments != attachments ||
        _num_color_attachments != num_color_attachments ||
        _width != width ||
        _height != height)
    {
         assert(width > 0 && height > 0);
         int max_size = maxFramebufferSize();
         if (width > max_size || height > max_size)
         {
             qCritical("Requested FBO size %d x %d, which exceeds max size %d x %d",
                 width, height, max_size, max_size);
             return false;
         }

        clear();

        _target = target;
        _format = format;
        _width = width;
        _height = height;
        _attachments = attachments;

        // create the fbo
        glGenFramebuffersEXT(1, (GLuint*)(&_fbo));
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);

        // create the color buffers, but only bind the first
        _num_color_attachments = num_color_attachments;
        _color_attachments = new GQTexture2D*[_num_color_attachments];
        for (int i = 0; i < _num_color_attachments; i++)
        {
            _color_attachments[i] = new GQTexture2D;
            _color_attachments[i]->create(_width, _height, _format, GL_RGBA, GL_UNSIGNED_BYTE, NULL, _target);
        }

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, _target, _color_attachments[0]->id(), 0);
        _current_color_attachment = 0;

        glBindTexture( _target, 0 );
		
        if ( _attachments & GQ_ATTACH_DEPTH)
        {
            // attach a depth buffer
            glGenRenderbuffersEXT(1, (GLuint*)(&_depth_attachment));
            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, _depth_attachment);
            glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, _width, _height);
            glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, _depth_attachment);
        }
        else
        {
            _depth_attachment = -1;
        }
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

        reportGLError();

        GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            qCritical("Could not set up frame buffer object.");
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            clear();
            return false;
        }

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    }
    return true;
}


void GQSingleFBO::drawBuffer( int which )
{
    assert(which >= 0 && which < _num_color_attachments );
    assert(_bound_guid == _guid);

    _current_color_attachment = which;
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, _target, _color_attachments[which]->id(), 0);
    glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT);
}

void GQSingleFBO::drawToAllBuffers()
{
    // multiple render targets is not supported by this class
    assert(0);
}

