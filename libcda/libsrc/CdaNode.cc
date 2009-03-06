// CdaNode.cc

#include <QTextStream>
#include <QMap>
#include <QDebug>

#include "CdaNode.h"
#include "CdaGeometry.h"
#include "CdaCamera.h"
#include "CdaLight.h"
#include "CdaMaterial.h"
#include "CdaScene.h"
#include "CdaUtility.h"
#include <assert.h>

CdaNode::CdaNode()
{
    clear();
}

CdaNode::~CdaNode()
{
    clear();    
}

void CdaNode::clear()
{
    _uid = -1;
    _id = QString("");
    _inst_node_id = QString();
    _inst_geometry = NULL;
    _inst_path = NULL;
    _inst_camera = NULL;
    _inst_light = NULL;
    _inst_materials.clear();

    for (int i = 0; i < (int)(_children.size()); i++)
        delete _children[i];

    _children.clear();
    _sorted_children.clear();
    _children_changed = false;
    _parent = NULL;

    _matrix = CdaXform::identity();
}

CdaNode::CdaNode(const QDomElement& element, const CdaScene* scene, CdaNode* parent)
{
    clear();
    _parent = parent;

    _id = element.attribute("id");
    _name = element.attribute("name");

    QDomElement e = element.firstChildElement("instance_node");
    if (!e.isNull())
    {
        _inst_node_id = trimHash(e.attribute("url"));
    }

    e = element.firstChildElement("instance_geometry");
    if (!e.isNull())
    {
        QString gid = trimHash(e.attribute("url"));
        _inst_geometry = scene->findLibraryGeometry(gid);
        QDomElement bind_mat = e.firstChildElement("bind_material");
        if (!bind_mat.isNull())
        {
            QDomElement tech_common = bind_mat.firstChildElement("technique_common");
            assert(!tech_common.isNull());
            QDomElement im = tech_common.firstChildElement("instance_material");
            while (!im.isNull())
            {
                QString symbol = im.attribute("symbol");
                QString targetid = trimHash(im.attribute("target"));
                const CdaMaterial* targetmat = scene->findLibraryMaterial(targetid);
                _inst_materials.insert( symbol, targetmat );

                im = im.nextSiblingElement("instance_material");
            }
        }
    }

    e = element.firstChildElement("instance_camera");
    QString eid;
    if (!e.isNull())
    {
        eid = trimHash(e.attribute("url"));
        _inst_camera = scene->findLibraryCamera(eid);
    }

    e = element.firstChildElement("instance_light");
    if (!e.isNull())
    {
        eid = trimHash(e.attribute("url"));
        _inst_light = scene->findLibraryLight(eid);
    }

    e = element.firstChildElement("matrix");
    if (!e.isNull())
    {
        QString es = e.text();
        QTextStream stream(&es);

        stream >> _matrix[0] >> _matrix[4] >> _matrix[8] >> _matrix[12]
        >> _matrix[1] >> _matrix[5] >> _matrix[9] >> _matrix[13]
        >> _matrix[2] >> _matrix[6] >> _matrix[10] >> _matrix[14]
        >> _matrix[3] >> _matrix[7] >> _matrix[11] >> _matrix[15];
    }

    e = element.firstChildElement("node");
    while (!e.isNull())
    {
        addChild(new CdaNode(e, scene, this));

        e = e.nextSiblingElement("node");
    }
    
    QDomElement extra_e = element.firstChildElement("extra");
    if (!extra_e.isNull()) {
        QDomElement dpix = extra_e.firstChildElement("technique");
        if (!dpix.isNull() && dpix.attribute("profile") == "DPIX") {
            e = dpix.firstChildElement("instance_path");
            if (!e.isNull()) {
                QString gid = trimHash(e.attribute("url"));
                _inst_path = scene->findLibraryGeometry(gid);
            }
        }
    }
}

const QString CdaNode::name() const
{
    if (!_name.isEmpty())
        return _name;
    else if (_inst_geometry)
        return _inst_geometry->_id;
    else
        return "Group";
}

CdaNode* CdaNode::childSorted(int i)
{
    if (_children_changed)
        sortChildren();
 
    return _sorted_children[i];
}

void CdaNode::addChild(CdaNode *child)
{
    _children << child;
    _children_changed = true;
}

void CdaNode::clearChildren()
{
    _children.clear();
    _sorted_children.clear();
    _children_changed = false;
}

bool CdaNode::sortNodes(const CdaNode* node1, const CdaNode* node2)
{
    if (node1->name() == "Scene")
        return true;
    if (node2->name() == "Scene")
        return false;
    
    return node1->name() < node2->name();
}

void CdaNode::sortChildren()
{
    _sorted_children = _children;
    qSort(_sorted_children.begin(), _sorted_children.end(), CdaNode::sortNodes);
    
    _children_changed = false;
}
