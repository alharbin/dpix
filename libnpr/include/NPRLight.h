#ifndef NPRLIGHT_H_
#define NPRLIGHT_H_

#include "Vec.h"
#include <QDomDocument>

typedef enum
{
    NPR_LIGHT_LAMBERTIAN,
    NPR_LIGHT_FRONT_BACK,
    NPR_LIGHT_WARM_COOL,

    NPR_NUM_LIGHT_MODES
} NPRLightMode;

class NPRLight
{
public:
    NPRLight();

    bool load( const QDomElement& element );
    bool save( QDomDocument& doc, QDomElement& element );

    void setLightDir( const vec& dir )       { _light_dir = dir; }
    void setDiffuseColor( const vec4& col )  { _diffuse_color = col; }
    void setSpecularColor( const vec4& col ) { _specular_color = col; }
    void setAmbientColor( const vec4& col )  { _ambient_color = col; }
    void setMode( const NPRLightMode& mode ) { _mode = mode; }
    void setEnableDiffuse(bool enable)       { _enable_diffuse = enable; }
    void setEnableSpecular(bool enable)      { _enable_specular = enable; }
    void setEnableAmbient(bool enable)       { _enable_ambient = enable; }

    const vec& lightDir() const          { return _light_dir; }
    NPRLightMode mode() const            { return _mode; }
    bool isDiffuseEnabled() const        { return _enable_diffuse; }
    bool isSpecularEnabled() const       { return _enable_specular; }
    bool isAmbientEnabled() const        { return _enable_ambient; }

    void applyToGL(int which) const;

protected:
    vec  _light_dir;
    vec4 _diffuse_color;
    vec4 _specular_color;
    vec4 _ambient_color;

    bool _enable_diffuse;
    bool _enable_specular;
    bool _enable_ambient;

    NPRLightMode _mode;
};

#endif /*NPRLIGHT_H_*/
