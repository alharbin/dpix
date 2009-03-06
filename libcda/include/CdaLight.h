// CdaLight.h

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