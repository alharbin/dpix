
/*****************************************************************************\
*                                                                           *
*  filename: NPRStyle.h                                                     *
*  authors : r. keith                                                       *
*                                                                           *
*  class to hold all _style attributes for drawing a class of paths.         *
*                                                                           *
\*****************************************************************************/

#ifndef _NPR_STYLE_H_
#define _NPR_STYLE_H_

#include <stdio.h>
#include "GQTexture.h"
#include "Vec.h"
#include <QString>
#include <QDomElement>
#include <vector>
using std::vector;

class NPRTransfer 
{
public:
    NPRTransfer() : v1(0), v2(0), vnear(0), vfar(0) {}

    void load( const QDomElement& element );
    // legacy reading code
    void loadV1( const char* buf );
    void save( QDomDocument& doc, QDomElement& element );

    void set( float v1, float v2, float vnear, float vfar );
    void print() const;

    float compute( float f ) const;

    void toArray( float ar[4]) const;

public:
    float v1, v2, vnear, vfar;
};


class NPRPenStyle
{
public:
    NPRPenStyle();
    ~NPRPenStyle() { clear(); }

    void clear();
    void copyFrom( const NPRPenStyle& style );
    void load( const QDomElement& element );
    void save( QDomDocument& doc, QDomElement& element );

    const QString&      name() const                 { return _name; }
    const vec&          color() const                { return _color; }
    const QString&      textureFile() const          { return _texture_file; }
    const GQTexture*    texture() const              { return _texture; }
    GQTexture*          texture()                    { return _texture; }

    float               stripWidth() const           { return _strip_width; }
    float               endcapWidth() const          { return _endcap_width; }
    int                 endcapLength() const         { return _endcap_length; }
    float               lengthScale() const          { return _length_scale; }

    void setName( const QString& name ) { _name = name; }
    void setColor( const vec& color ) { _color = color; }
    bool setTexture( const QString& filename );
    bool setTexture( GQTexture* texture ) { _texture = texture; return true; }

    void setStripWidth( float width ) { _strip_width = width; }
    void setEndcapWidth( float width ) { _endcap_width = width; }
    void setEndcapLength( int length ) { _endcap_length = length; }
    void setLengthScale( float scale ) { _length_scale = scale; }

protected:
    QString     _name;
    vec         _color;
    QString     _texture_file;
    GQTexture* _texture;

    float       _strip_width;
    float       _endcap_width;  // fraction of base strip width
    int         _endcap_length; // length of endcap in triangles
    float       _length_scale;  // texture scale along the line's length
};


class NPRStyle
{
public:
    NPRStyle();
    ~NPRStyle() { clear(); }

    void clear();
    bool load( const QString& filename );
    bool load( const QDomElement& root );
    bool save( const QString& filename );
    bool save( QDomDocument& doc, QDomElement& root );

    void loadDefaults();

    bool loadPaperTexture(const QString& filename);
    GQTexture* paperTexture() { return _paper_texture; }
    const GQTexture* paperTexture() const { return _paper_texture; }

    bool loadBackgroundTexture(const QString& filename);
    GQTexture* backgroundTexture() { return _background_texture; }
    const GQTexture* backgroundTexture() const { return _background_texture; }


    // enumerated set of transfer functions
    typedef enum 
    {
        FOCUS_TRANSFER,

        // rendering from focus values                      
        LINE_OPACITY,
        LINE_TEXTURE,
        LINE_WIDTH,
        LINE_OVERSHOOT,           

        COLOR_FADE,
        COLOR_DESAT,
        COLOR_BLUR,      

        PRIORITY_WIDTH,
        PRIORITY_COUNT,

        PAPER_PARAMS,

        SC_THRESHOLD,
        RV_THRESHOLD,

        NUM_TRANSFER_FUNCTIONS
    } TransferFunc;

    inline const NPRTransfer& transfer( TransferFunc func ) const { return _transfers[func]; }
    NPRTransfer& transferRef( TransferFunc func );
    NPRTransfer& transferByName( const QString& name );

    inline const vec&   backgroundColor() const      { return _background_color; }
    inline float        priorityMinWidth() const     { return _priority_min_width; }
    inline float        priorityMaxWidth() const     { return _priority_max_width; }
    inline float        scThreshold() const     { return _sc_threshold; }
    inline float        rvThreshold() const     { return _rv_threshold; }
    inline float        appRidgeThreshold() const     { return _app_ridge_threshold; }

    inline void         setSCThreshold( float threshold ) { _sc_threshold = threshold; }
    inline void         setRVThreshold( float threshold ) { _rv_threshold = threshold; }
    inline void         setAppRidgeThreshold( float threshold ) { _app_ridge_threshold = threshold; }

    void         setBackgroundColor( const vec& color ) { _background_color = color; }

    bool         isPathStyleDirty() { return _path_style_dirty; }
    void         setPathStyleClean() { _path_style_dirty = false; }

    int                 numPenStyles() const { return (int)(_pen_styles.size()); }
    const NPRPenStyle*  penStyle(int which) const { return _pen_styles[which]; }
    NPRPenStyle*        penStyle(int which) { return _pen_styles[which]; }
    const NPRPenStyle*  penStyle (const QString& name) const;
    NPRPenStyle*        penStyle(const QString& name);
    bool                hasPenStyle(const QString& name) const;
    void                mapLineTypesToPenStyles(vector<int>& map) const;

    void                addPenStyle(NPRPenStyle* style);
    bool                deletePenStyle( int which );
    bool                deletePenStyle( const QString& name );

protected:
    bool loadTexture(QString& output_name, GQTexture*& output_ptr, 
                           const QString& field_name, const QString& filename);
protected:
    NPRTransfer _transfers[NUM_TRANSFER_FUNCTIONS];
    vector<NPRPenStyle*> _pen_styles;

    vec         _background_color;       

    QString     _paper_file;
    GQTexture* _paper_texture;

    QString     _background_file;
    GQTexture* _background_texture;
    
    float _priority_min_width;
    float _priority_max_width;
    float _sc_threshold;
    float _rv_threshold;
    float _app_ridge_threshold;

    bool _path_style_dirty;

};


#endif // _NPR_STYLE_H_
