// CdaMaterial.cc

#include "CdaMaterial.h"
#include "CdaEffect.h"
#include "CdaScene.h"
#include "CdaUtility.h"
#include <assert.h>

void CdaMaterial::clear()
{
    _id = QString("");
    _inst_effect = NULL;
}

CdaMaterial::CdaMaterial(const QDomElement& element, const CdaScene* scene)
{
    clear();

    _id = element.attribute("id");
    _name = element.attribute("name");

    QDomElement ie = element.firstChildElement("instance_effect");
    assert(!ie.isNull());

    _inst_effect_id = trimHash(ie.attribute("url"));
    _inst_effect = scene->findLibraryEffect(_inst_effect_id);
}
