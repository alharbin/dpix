/*****************************************************************************\

CdaLight.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A wrapper for a COLLADA light.

libcda is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _CDA_LIGHT_H_
#define _CDA_LIGHT_H_

#include <QString>
#include <QDomElement>

#include "CdaTypes.h"

class CdaLight
{
public:
    CdaLight() { clear(); }

    CdaLight(const QDomElement& element);

    void clear();

public:
    const QString& id() const { return _id; }
    QString			_id;

    QString			_type;
    CdaColor4		_color;
};

#endif