/*****************************************************************************\

CdaEffect.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A wrapper class for a COLLADA effect node.

libcda is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _CDA_EFFECT_H_
#define _CDA_EFFECT_H_

#include <QString>
#include <QDomElement>

#include "CdaTypes.h"

class CdaEffect
{
public:
    CdaEffect() { clear(); }
    CdaEffect(const QDomElement& element);

    void clear();

public:
    const QString& id()const  { return _id; }
    
    QString			_id;

    QString			_type;

    CdaColor4		_emission;
    CdaColor4		_ambient;
    CdaColor4		_diffuse;
    CdaColor4		_specular;
    float			_shininess;
    CdaColor4		_reflective;
    float			_reflectivity;
    CdaColor4		_transparent;
    float			_transparency;
    float			_index_of_refraction;
};

#endif