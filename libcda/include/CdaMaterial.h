/*****************************************************************************\

CdaMaterial.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A wrapper for a COLLADA material node.

libcda is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _CDA_MATERIAL_H_
#define _CDA_MATERIAL_H_

#include <QString>
#include <QDomElement>

class CdaEffect;
class CdaScene;

class CdaMaterial
{
public:
    CdaMaterial() { clear(); }

    CdaMaterial(const QDomElement& element, const CdaScene* scene);

    void clear();

public:
    const QString& id() const { return _id; }
    QString				_id;
    QString             _name;

    QString				_inst_effect_id;
    const CdaEffect*	_inst_effect;
};

#endif