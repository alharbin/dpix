// CdaScene.h

#ifndef _CDA_SCENE_H_
#define _CDA_SCENE_H_

#include <QList>
#include <QString>
#include <QByteArray>
#include "CdaTypes.h"
#include <vector>
using std::vector;

class CdaNode;
class CdaGeometry;
class CdaCamera;
class CdaLight;
class CdaMaterial;
class CdaEffect;

class CdaScene
{
public:
    CdaScene();
    ~CdaScene();

    void clear();
    bool load( const QString& filename );
    static bool isColladaFile( const QString& filename );

    const CdaNode*		root() const { return _root; }
    const CdaNode*		findSceneNode(QString id) const;
    CdaNode*            getNode(int uid) const { return _node_list[uid]; }
    int                 numNodes() const { return _node_list.size(); }

    // get library elements
    int					numLibraryNodes() const				{ return _library_nodes.size(); }
    int					numLibraryGeometries() const		{ return _library_geometries.size(); }
    int					numLibraryCameras() const			{ return _library_cameras.size(); }
    int					numLibraryLights() const			{ return _library_lights.size(); }
    int					numLibraryMaterials() const			{ return _library_materials.size(); }
    int					numLibraryEffects() const			{ return _library_effects.size(); }

    const CdaNode*		libraryNode(int which) const		{ return _library_nodes[which]; }
    const CdaGeometry*	libraryGeometry(int which) const	{ return _library_geometries[which]; }
    const CdaCamera*	libraryCamera(int which) const		{ return _library_cameras[which]; }
    const CdaLight*		libraryLight(int which) const		{ return _library_lights[which]; }
    const CdaMaterial*	libraryMaterial(int which) const	{ return _library_materials[which]; }
    const CdaEffect*	libraryEffect(int which) const		{ return _library_effects[which]; }

    const CdaNode*		findLibraryNode(QString id) const;
    const CdaGeometry*	findLibraryGeometry(QString id) const;
    const CdaCamera*	findLibraryCamera(QString id) const;
    const CdaLight*		findLibraryLight(QString id) const;
    const CdaMaterial*	findLibraryMaterial(QString id) const;
    const CdaEffect*	findLibraryEffect(QString id) const;

    // add library elements
    void                addLibraryLight( CdaLight* light );

    // add a node
    void                addNodeInstance( const QString& parent_id, CdaNode* node);

public:
    // helper functions
    static void			findBoundingSphere(const CdaScene *scene, const CdaNode* root, CdaVec3& center, float& radius );
    static void			findBoundingAABB(const CdaScene *scene, const CdaNode* root, CdaVec3& lower_corner, CdaVec3& upper_corner );
    void				expandAndFlattenNodes();

protected:
    static void traverseAndGetVerts(const CdaScene *scene, const CdaNode* root, vector<CdaVec3>& verts, const CdaXform& current_xf );
    static void traverseAndFlatten(const CdaScene *scene, const CdaNode* root, CdaNode* new_root, const CdaXform& current_xf );
    static CdaNode* traverseAndFindById(CdaNode* root, const QString& id);
    
    void traverseAndAssignUid(CdaNode* root);
    
protected: 
    bool				loadKMZ(const QString& filename);
    bool				loadDAE(const QString& filename);

    bool				parseDAE(const QByteArray& data);

protected:
    // collada scene root node
    CdaNode*				_root;

    // collada libraries
    QList<CdaNode*>		_library_nodes;
    QList<CdaGeometry*>	_library_geometries;
    QList<CdaCamera*>	_library_cameras;
    QList<CdaLight*>	_library_lights;
    QList<CdaMaterial*>	_library_materials;
    QList<CdaEffect*>	_library_effects;
    
    // node list
    QList<CdaNode*>     _node_list;

};

#endif

