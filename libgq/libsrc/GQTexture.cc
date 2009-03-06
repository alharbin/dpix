#include "GQTexture.h"
#include "GQInclude.h"
#include <assert.h>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>


GQTexture::~GQTexture()
{
    clear();
}

bool GQTexture::is3D(const QString& filename )
{
    if (filename.endsWith(".3dt"))
        return true;
    return false;
}

void GQTexture::clear()
{
    if (_id > 0)
    {
        glDeleteTextures(1, (GLuint*)(&_id) );
    }
    _id = -1;
}


GQTexture2D::GQTexture2D()
{
    _id = -1;
}
        
bool GQTexture2D::genTexture(int width, int height, int internalFormat, int format, int type, const void *data)
{
    glGenTextures(1, (GLuint*)(&_id));
    glBindTexture(_target, _id);
    
    glTexImage2D(_target, 0, internalFormat, width, height, 0, format, type, data);

    int wrapMode = _target == GL_TEXTURE_2D ? GL_REPEAT : GL_CLAMP;
    
    glTexParameteri(_target, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(_target, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    int error = glGetError();
    if (error)
    {
        printf("\nNPRTexture2D::createGLTexture : GL error: %s\n", gluErrorString(error));
        return false;
    } 

    return true;
}
        
bool GQTexture2D::load( const QString& filename )
{
    GQImage image;
    if (!image.load(filename))
        return false;
    
    return create(image, GL_TEXTURE_2D);
}

bool GQTexture2D::create( const GQImage& image, int target )
{
    GLenum format;
    if (image.chan() == 1) 
        format = GL_ALPHA;
    else if (image.chan() == 2)
        format = GL_LUMINANCE_ALPHA;
    else if (image.chan() == 3) 
        format = GL_RGB;
    else if (image.chan() == 4)
        format = GL_RGBA;

    return create(image.width(), image.height(), format, format, GL_UNSIGNED_BYTE, image.raster(), target);
}

bool GQTexture2D::create(int width, int height, int internalFormat, int format, int type, const void *data, int target)
{
    _target = target;
    _width = width;
    _height = height;
    
    return genTexture(width, height, internalFormat, format, type, data);
}

bool GQTexture2D::bind() const
{
    glBindTexture(_target, _id);
    return true;
}

void GQTexture2D::unbind() const
{
    glBindTexture(_target, 0);
}

void GQTexture2D::enable() const
{
    glEnable(_target);
}

void GQTexture2D::disable() const
{
    glDisable(_target);
}

int GQTexture2D::target() const
{
    return _target;
}


GQTexture3D::GQTexture3D()
{
    _id = -1;
    _num_slices = 0;
    _slices = NULL;
    _wrap_mode_s = GL_REPEAT;
    _wrap_mode_t = GL_REPEAT;
    _wrap_mode_r = GL_CLAMP_TO_EDGE;
}

GQTexture3D::~GQTexture3D()
{
    if (_slices)
        delete [] _slices;
}

bool GQTexture3D::load( const QString& filename )
{
    QFile file(filename);

    //open the file, check if successful
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug("GQTexture3D::load - Error: Unable to open file %s\n", qPrintable(filename));
        return false;
    }

    //read number of slices from file
    QTextStream in_text(&file);
    in_text >> _num_slices;
    if (_num_slices < 0 || _num_slices > 255)
    {
        qDebug("GQTexture3D::load - Error: Bad number of slices: %d\n", _num_slices);
        return false;
    }

    _slices = new GQImage[_num_slices];

    // now get each slice individually
    // get the path from the original file

    QString path = QFileInfo(filename).path();

    for (int i = 0; i < _num_slices; i++)
    {   
        QString buf;
        in_text >> buf;

        // make file name
        QString fullname = path + "/" + buf;

        qDebug("Loading %s\n", qPrintable(fullname));

        if (!_slices[i].load( fullname ))
        {
            qWarning("GQTexture3D::load - Failed to load slice %s\n", qPrintable(fullname));
            return false;               
        }
    }

    return createGLTexture();
}

bool GQTexture3D::createGLTexture()
{
    assert( glGetError() == 0);

    int width, height, depth;
    int channels = _slices[0].chan();
    width = _slices[0].width();
    height = _slices[0].height();
    depth = _num_slices;

    int maxsize;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, (GLint*)(&maxsize));

    if (width > maxsize || height > maxsize || depth > maxsize)
    {
        printf("GQTexture3D::bind: Texture too big: %dx%dx%d, max is %d\n",
               width, height, depth, maxsize);
        return false;
    }

    glGenTextures(1, (GLuint*)(&_id));
    glBindTexture(GL_TEXTURE_3D, _id);

    unsigned char* texels = new unsigned char[width*height*depth*channels];
    for (int k = 0; k < depth; k++)
    {
        assert( _slices[k].width() == (unsigned)width );
        assert( _slices[k].height() == (unsigned)height );

        unsigned char* raster = _slices[k].raster();
        int rasterIndex = 0;
        int rasterOffset = k*height*width*channels;
        for (int j = 0; j < height; j++)
        {
            for (int i = 0; i < width; i++)
            {
                for (int c = 0; c < channels; c++)
                {    
                    texels[rasterIndex + rasterOffset] = raster[rasterIndex];
                    rasterIndex++;
                }
            }
        }
    }

    GLenum format = GL_RGB;
    if (_slices[0].chan() == 1) 
        format = GL_ALPHA;
    else if (_slices[0].chan() == 4)
        format = GL_RGBA;

    assert( format == GL_ALPHA );

#ifdef WIN32
    PFNGLTEXIMAGE3DPROC glTexImage3D;
    glTexImage3D = (PFNGLTEXIMAGE3DPROC) wglGetProcAddress("glTexImage3D");
#endif        
    //gluBuild3DMipmaps(GL_TEXTURE_3D, GL_ALPHA, width, height, depth, format,
    //                                       GL_UNSIGNED_BYTE, texels);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_ALPHA, width, height, depth, 0,
                 format, GL_UNSIGNED_BYTE, texels);

      // need to change this to use mipmapping later
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, _wrap_mode_s);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, _wrap_mode_t);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, _wrap_mode_r);  


    delete [] texels;

    int error = glGetError();
    if (error)
    {
        printf("\nNPRTexture3D::createGLTexture : GL error: %s\n", gluErrorString(error));
        return false;
    } 

    return true;
}

bool GQTexture3D::bind() const
{
    assert( glGetError() == 0);

    glBindTexture(GL_TEXTURE_3D, _id);

  
    int error = glGetError();
    if (error)
    {
        printf("\nNPRTexture3D::bind : GL error: %s\n", gluErrorString(error));
        return false;
    } 

    return true;
}

void GQTexture3D::unbind() const
{
    glBindTexture(GL_TEXTURE_3D, 0);
}

void GQTexture3D::enable() const
{
    glEnable(GL_TEXTURE_3D);
}

void GQTexture3D::disable() const
{
    glDisable(GL_TEXTURE_3D);
}

int GQTexture3D::target() const
{
    return GL_TEXTURE_3D;
}

bool GQTexture3D::create(int width, int height, int depth, int internalFormat, int format, int type, const void *data)
{
    _num_slices = depth;
    _slices = 0;
	
    _wrap_mode_s = GL_REPEAT;
    _wrap_mode_t = GL_REPEAT;
    _wrap_mode_r = GL_REPEAT;

    glGenTextures(1, (GLuint*)(&_id));
    bind();
    glTexImage3D( GL_TEXTURE_3D, 0, internalFormat, width, height, depth,
                  0, format, type, data );

    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, _wrap_mode_s);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, _wrap_mode_t);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, _wrap_mode_r);  
    unbind();

    int error = glGetError();
    if (error)
    {
        printf("\nNPRTexture3D::create: GL error: %s\n", gluErrorString(error));
        return false;
    } 

    return true;
}



