/*****************************************************************************\

NPRLight.cc
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/
#include "NPRLight.h"
#include "NPRUtility.h"

NPRLight::NPRLight()
{
    _ambient_color = vec4( 0.1f, 0.1f, 0.1f, 1.0f );
    _diffuse_color = vec4( 0.8f, 0.8f, 0.8f, 1.0f );
    _specular_color = vec4( 0.5f, 0.5f, 0.5f, 1.0f );
    _enable_diffuse = true;
    _enable_specular = true;
    _enable_ambient = true;
    _mode = NPR_LIGHT_LAMBERTIAN;
}
    
bool NPRLight::load( const QDomElement& element )
{
    _mode = (NPRLightMode)(element.attribute("mode").toInt());

    QDomElement ambient = element.firstChildElement("ambient");
    if (!ambient.isNull())
        _enable_ambient = ambient.attribute("enabled").toInt();

    QDomElement diffuse = element.firstChildElement("diffuse");
    if (!diffuse.isNull())
        _enable_diffuse = diffuse.attribute("enabled").toInt();

    QDomElement specular = element.firstChildElement("specular");
    if (!specular.isNull())
        _enable_specular = specular.attribute("enabled").toInt();

    return true;
}

bool NPRLight::save( QDomDocument& doc, QDomElement& element )
{
    element.setAttribute("mode", (int)_mode);
    QDomElement ambient = doc.createElement("ambient");
    ambient.setAttribute("enabled", (int)_enable_ambient);
    element.appendChild(ambient);
    QDomElement diffuse = doc.createElement("diffuse");
    diffuse.setAttribute("enabled", (int)_enable_diffuse);
    element.appendChild(diffuse);
    QDomElement specular = doc.createElement("specular");
    specular.setAttribute("enabled", (int)_enable_specular);
    element.appendChild(specular);

    return true;
}

void NPRLight::applyToGL(int which) const 
{
    vec4 ambient, diffuse, specular;
    if (_enable_ambient)
        ambient = _ambient_color;
    if (_enable_diffuse)
        diffuse = _diffuse_color;
    if (_enable_specular)
        specular = _specular_color;

    glLightfv(GL_LIGHT0 + which, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0 + which, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0 + which, GL_SPECULAR, specular);

    vec4 light_pos = vec4(_light_dir[0], _light_dir[1], _light_dir[2], 0.0f); 
    glLightfv(GL_LIGHT0 + which, GL_POSITION, light_pos);
}
