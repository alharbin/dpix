// CdaCamera.h

#ifndef _CDA_CAMERA_H_
#define _CDA_CAMERA_H_

#include <QString>
#include <QDomElement>

class CdaCamera
{
public:
    CdaCamera() { clear(); }
    CdaCamera(const QDomElement& element);

    void clear();

public:
    const QString& id() const { return _id; }
    QString			_id;

    QString			_type;
    float			_xfov;
    float			_yfov;
    float			_aspect_ratio;
    float			_znear;
    float			_zfar;
};

#endif