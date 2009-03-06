// CdaLight.cc

#include <QTextStream>
#include <CdaLight.h>
#include <assert.h>

void CdaLight::clear()
{
    _id = QString("");
    _type = QString("");
    _color = CdaColor4(1,1,1,1);
}

CdaLight::CdaLight(const QDomElement& element)
{
    clear();

    _id = element.attribute("id");

    QDomElement tech_common = element.firstChildElement("technique_common");
    assert(!tech_common.isNull());
    QDomElement le = tech_common.firstChildElement();
    assert(!le.isNull());
    _type = le.tagName();

    QDomElement ce = le.firstChildElement("color");
    assert(!le.isNull());

    QString cs = ce.text();
    QTextStream stream(&cs);
    stream >> _color[0] >> _color[1] >> _color[2];
}