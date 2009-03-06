// CdaCamera.cc

#include "CdaCamera.h"

#include <assert.h>

void CdaCamera::clear()
{
    _id = QString("");
    _type = QString("");
    _xfov = -1;
    _yfov = -1;
    _aspect_ratio = -1;
    _znear = -1;
    _zfar = -1;
}

CdaCamera::CdaCamera(const QDomElement& element)
{
    clear();

    _id = element.attribute("id");

    QDomElement optics = element.firstChildElement("optics");
    assert(!optics.isNull());
    QDomElement tech_common = optics.firstChildElement("technique_common");
    assert(!tech_common.isNull());
    QDomElement cam = tech_common.firstChildElement();
    assert(!cam.isNull());
    _type = cam.tagName();

    // temporary, only handle perspective camera:
    assert(_type == "perspective");

    QDomElement e = cam.firstChildElement("xfov");
    if (!e.isNull())
        _xfov = e.text().toFloat();

    e = cam.firstChildElement("yfov");
    if (!e.isNull())
        _yfov = e.text().toFloat();

    e = cam.firstChildElement("aspect_ratio");
    if (!e.isNull())
        _aspect_ratio = e.text().toFloat();

    e = cam.firstChildElement("znear");
    if (!e.isNull())
        _znear = e.text().toFloat();

    e = cam.firstChildElement("zfar");
    if (!e.isNull())
        _zfar = e.text().toFloat();
}