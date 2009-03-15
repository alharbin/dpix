/*****************************************************************************\

CdaEffect.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

libcda is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include <QTextStream>
#include "CdaEffect.h"
#include <assert.h>

void CdaEffect::clear()
{
    _id = QString("");
    _type = QString("");

    _emission = CdaColor4(0,0,0,0);
    _ambient = CdaColor4(0,0,0,0);
    _diffuse = CdaColor4(0,0,0,0);
    _specular = CdaColor4(0,0,0,0);
    _shininess = 1;
    _reflective = CdaColor4(0,0,0,0);
    _reflectivity = 0;
    _transparent = CdaColor4(0,0,0,0);
    _transparency = 0;
    _index_of_refraction = 1;
}

CdaColor4 getColorFromElement( const QDomElement& element )
{
    CdaColor4 out = CdaColor4(1,1,1,1);
    QDomElement c = element.firstChildElement("color");
    if (!c.isNull())
    {
        QString s = c.text();
        QTextStream stream(&s);
        stream >> out[0] >> out[1] >> out[2] >> out[3];
    }
    return out;
}

float getFloatFromElement( const QDomElement& element )
{
    float out;
    QDomElement c = element.firstChildElement("float");
    assert(!c.isNull());
    out = c.text().toFloat();
    return out;
}

CdaEffect::CdaEffect(const QDomElement& element)
{
    clear();

    _id = element.attribute("id");

    QDomElement profile = element.firstChildElement("profile_COMMON");
    assert(!profile.isNull());
    QDomElement tech = profile.firstChildElement("technique");
    assert(!tech.isNull());
    QDomElement model = tech.firstChildElement();
    assert(!model.isNull());

    _type = model.tagName();
    QDomElement e = model.firstChildElement("emission");
    if (!e.isNull())
        _emission = getColorFromElement( e );

    e = model.firstChildElement("ambient");
    if (!e.isNull())
        _ambient = getColorFromElement( e );

    e = model.firstChildElement("diffuse");
    if (!e.isNull())
        _diffuse = getColorFromElement( e );

    e = model.firstChildElement("specular");
    if (!e.isNull())
        _specular = getColorFromElement( e );

    e = model.firstChildElement("shininess");
    if (!e.isNull())
        _shininess = getFloatFromElement( e );

    e = model.firstChildElement("reflective");
    if (!e.isNull())
        _reflective = getColorFromElement( e );

    e = model.firstChildElement("reflectivity");
    if (!e.isNull())
        _reflectivity = getFloatFromElement( e );

    e = model.firstChildElement("transparent");
    if (!e.isNull())
        _transparent = getColorFromElement( e );

    e = model.firstChildElement("transparency");
    if (!e.isNull())
        _transparency = getFloatFromElement( e );

    e = model.firstChildElement("index_of_refraction");
    if (!e.isNull())
        _index_of_refraction = getFloatFromElement( e );
}
