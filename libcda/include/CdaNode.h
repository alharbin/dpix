// CdaNode.h

#ifndef _CDA_NODE_H_
#define _CDA_NODE_H_

#include "CdaTypes.h"

#include <QString>
#include <QDomElement>
#include <QHash>
#include <vector>
using std::vector;

class CdaGeometry;
class CdaCamera;
class CdaLight;
class CdaMaterial;
class CdaScene;

typedef QHash<QString, const CdaMaterial*> CdaMaterialHash;

class CdaNode
{
public:
    CdaNode();
    CdaNode(const QDomElement& element, const CdaScene* scene, CdaNode* parent);
    ~CdaNode();

    void clear();
    const QString&          id() const { return _id; }
    const int               uid() const { return _uid; }
    const QString           name() const;
    const QString&          nodeId() const { return _inst_node_id; }
    const CdaGeometry*      geometry() const { return _inst_geometry; }
    const CdaGeometry*      path() const { return _inst_path; }
    const CdaCamera*        camera() const { return _inst_camera; }
    const CdaLight*         light() const { return _inst_light; }
    const CdaMaterialHash&  materials() const { return _inst_materials; }

    void setId(const QString &id) { _id = id; }
    void setUid(const int uid) { _uid = uid; }
    void setName(const QString &name) { _name = name; }
    void setNodeId(const QString& inst_node_id) { _inst_node_id = inst_node_id; }
    void setGeometry(const CdaGeometry* inst_geometry) { _inst_geometry = inst_geometry; }
    void setPath(const CdaGeometry* inst_path) { _inst_path = inst_path; }
    void setCamera(const CdaCamera* inst_camera) { _inst_camera = inst_camera; }
    void setLight(const CdaLight* inst_light) { _inst_light = inst_light; }
    void setMaterials(const CdaMaterialHash& hash) { _inst_materials = hash; }
    
    int numChildren() const { return _children.size(); }
    void addChild(CdaNode* child);
    void clearChildren();
    const CdaNode* child(int i) const { return _children[i]; }
    CdaNode* child(int i) { return _children[i]; }
    CdaNode* childSorted(int i);
    
    const CdaNode* parent() const { return _parent; }
    CdaNode* parent() { return _parent; }
    void setParent(CdaNode *parent) { _parent = parent; }

    const CdaXform& matrix() const { return _matrix; }
    void setMatrix(const CdaXform& matrix) { _matrix = matrix; }
    
private:
    void sortChildren();
    static bool sortNodes(const CdaNode* node1, const CdaNode* node2);
    
    QString				_id; // must be unique per scene
    int _uid;
    QString             _name; // may be reused

    // some or all of these pointers will be NULL
    QString             _inst_node_id;
    const CdaGeometry*	_inst_geometry;
    const CdaGeometry*  _inst_path;
    const CdaCamera*	_inst_camera;
    const CdaLight*		_inst_light;
    CdaMaterialHash     _inst_materials;

    QList<CdaNode*>	    _children;
    QList<CdaNode*>     _sorted_children;
    bool _children_changed;
    CdaNode*            _parent;

    CdaXform			_matrix;

};

#endif