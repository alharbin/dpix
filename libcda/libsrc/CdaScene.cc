// CdaScene.cc

#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QString>

#include "CdaTypes.h"
#include "CdaScene.h"
#include "CdaCamera.h"
#include "CdaEffect.h"
#include "CdaNode.h"
#include "CdaMaterial.h"
#include "CdaGeometry.h"
#include "CdaLight.h"

#include "unzip.h"

#include "bsphere.h"

#include <assert.h>

CdaScene::CdaScene()
{
    _root = NULL;
    clear();
}

CdaScene::~CdaScene()
{
    clear();
}

void CdaScene::clear()
{
    if (_root)
    {
        delete _root;
        _root = NULL;
    }

    while (!_library_nodes.isEmpty())
        delete _library_nodes.takeFirst();

    while (!_library_geometries.isEmpty())
        delete _library_geometries.takeFirst();
        
    while (!_library_cameras.isEmpty())
        delete _library_cameras.takeFirst();

    while (!_library_lights.isEmpty())
        delete _library_lights.takeFirst();

    while (!_library_materials.isEmpty())
        delete _library_materials.takeFirst();

    while (!_library_effects.isEmpty())
        delete _library_effects.takeFirst();
};


// I/O functions

bool CdaScene::load( const QString& filename )
{
    if (filename.endsWith("kmz", Qt::CaseInsensitive))
    {
        return loadKMZ(filename);
    }
    else if (filename.endsWith("dae", Qt::CaseInsensitive))
    {
        return loadDAE(filename);
    }
    else
    {
        qWarning("CdaScene::load: unrecognized file type: %s\n", qPrintable(filename));
        return false;
    }
}
    
bool CdaScene::isColladaFile( const QString& filename )
{
    if (filename.endsWith("kmz", Qt::CaseInsensitive) ||
        filename.endsWith("dae", Qt::CaseInsensitive))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CdaScene::loadKMZ(const QString& filename)
{
    unzFile uf = NULL;
    uf = unzOpen(qPrintable(filename));

    if (uf == NULL)
    {
        qWarning("CdaScene::load: could not open: %s\n", qPrintable(filename));
        return false;
    }

    unz_global_info gi;
    int err = unzGetGlobalInfo (uf,&gi);

    for (uint32 i=0;i<gi.number_entry;i++)
    {
        char filename_inzip[256];
        unz_file_info file_info;
        err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
        if (err!=UNZ_OK)
        {
            qWarning("CdaScene::load: error %d with zipfile in unzGetCurrentFileInfo\n",err);
            return false;
        }
        QString name = QString(filename_inzip);
        if (name.endsWith("dae", Qt::CaseInsensitive))
        {
            // parse the dae file
            QByteArray ba = QByteArray(file_info.uncompressed_size, '0');
            unzOpenCurrentFile(uf);
            int bytes_read = unzReadCurrentFile(uf, ba.data(), file_info.uncompressed_size);
            if (bytes_read != (int)file_info.uncompressed_size)
            {
                qWarning("CdaScene::load: error reading %s in %s\n", filename_inzip, qPrintable(filename));
                return false;
            }
            unzCloseCurrentFile(uf);

            parseDAE(ba);
        }
        unzGoToNextFile(uf);
    }

    unzClose(uf);

    return true;
}

bool CdaScene::loadDAE(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    qDebug("CdaScene::load: Opened %s", qPrintable(filename));

    QByteArray data = file.readAll();

    return parseDAE( data );
}

bool CdaScene::parseDAE(const QByteArray& data)
{    
    // open the file and read into memory

    QDomDocument doc("collada");
    QString error_str;
    int error_line;
    int error_column;
    if (!doc.setContent(data, &error_str, &error_line, &error_column))
    {
        qWarning("CdaScene::load: DOM read failure: %s at row %d, col %d (look at dom_error_dump.txt)\n", 
			qPrintable(error_str), error_line, error_column);
        QFile debug_dump("dom_error_dump.txt");
        debug_dump.open(QIODevice::WriteOnly);
        debug_dump.write(data);
        debug_dump.close();
        return false;
    }

    QDomElement doc_e = doc.documentElement();

    // start reading the libraries
    QDomElement lib_effects = doc_e.firstChildElement("library_effects");
    if (!lib_effects.isNull())
    {
        QDomElement effect_e = lib_effects.firstChildElement("effect");
        while(!effect_e.isNull())
        {
            CdaEffect* new_effect = new CdaEffect(effect_e);
            _library_effects.push_back(new_effect);
            effect_e = effect_e.nextSiblingElement("effect");
        }
    }

    qDebug("CdaScene::load: Read %d effects", (int)_library_effects.size());

    // materials must be read after effects because they can include "instance_effect" nodes
    QDomElement lib_materials = doc_e.firstChildElement("library_materials");
    if (!lib_materials.isNull())
    {
        QDomElement material_e = lib_materials.firstChildElement("material");
        while(!material_e.isNull())
        {
            CdaMaterial* new_material = new CdaMaterial(material_e, this);
            _library_materials.push_back(new_material);
            material_e = material_e.nextSiblingElement("material");
        }
    }

    qDebug("CdaScene::load: Read %d materials", (int)_library_materials.size());

    QDomElement lib_lights = doc_e.firstChildElement("library_lights");
    if (!lib_lights.isNull())
    {
        QDomElement light_e = lib_lights.firstChildElement("light");
        while(!light_e.isNull())
        {
            CdaLight* new_light = new CdaLight(light_e);
            _library_lights.push_back(new_light);
            light_e = light_e.nextSiblingElement("light");
        }
    }

    qDebug("CdaScene::load: Read %d lights", (int)_library_lights.size());

    QDomElement lib_cameras = doc_e.firstChildElement("library_cameras");
    if (!lib_cameras.isNull())
    {
        QDomElement camera_e = lib_cameras.firstChildElement("camera");
        while(!camera_e.isNull())
        {
            CdaCamera* new_camera = new CdaCamera(camera_e);
            _library_cameras.push_back(new_camera);
            camera_e = camera_e.nextSiblingElement("camera");
        }
    }

    qDebug("CdaScene::load: Read %d cameras", (int)_library_cameras.size());

    QDomElement lib_geometries = doc_e.firstChildElement("library_geometries");
    if (!lib_geometries.isNull())
    {
        QDomElement geometry_e = lib_geometries.firstChildElement("geometry");
        while(!geometry_e.isNull())
        {
            CdaGeometry* new_geometry = new CdaGeometry(geometry_e);
            _library_geometries.push_back(new_geometry);
            geometry_e = geometry_e.nextSiblingElement("geometry");
        }
    }

    qDebug("CdaScene::load: Read %d geometries", (int)_library_geometries.size());

    QDomElement lib_nodes = doc_e.firstChildElement("library_nodes");
    if (!lib_nodes.isNull())
    {
        QDomElement node_e = lib_nodes.firstChildElement("node");
        while(!node_e.isNull())
        {
            CdaNode* new_node = new CdaNode(node_e, this, NULL);
            _library_nodes.push_back(new_node);
            node_e = node_e.nextSiblingElement("node");
        }
    }

    qDebug("CdaScene::load: Read %d library nodes", (int)_library_nodes.size());

    // we ignore all but the first visual scene
    QDomElement scene_e = doc_e.firstChildElement("library_visual_scenes");
    assert(!scene_e.isNull());
    scene_e = scene_e.firstChildElement("visual_scene");

    // recursively create the node hierarchy
    _root = new CdaNode(scene_e, this, NULL);

    qDebug("CdaScene::load: Read root node");
    
    traverseAndAssignUid(_root);
    for (int i = 0; i < _library_nodes.size(); i++) {
        traverseAndAssignUid(_library_nodes[i]);
    }
    
    qDebug("CdaScene::load: success!\n");

    return true;
}

const CdaNode* CdaScene::findLibraryNode(QString id) const
{
    for (int i = 0; i < _library_nodes.size(); i++)
    {
        if (_library_nodes[i]->id() == id)
            return _library_nodes[i];
    }
    return NULL;
}

const CdaGeometry* CdaScene::findLibraryGeometry(QString id) const
{
    for (int i = 0; i < _library_geometries.size(); i++)
    {
        if (_library_geometries[i]->id() == id)
            return _library_geometries[i];
    }
    return NULL;
}

const CdaCamera* CdaScene::findLibraryCamera(QString id) const
{
    for (int i = 0; i < _library_cameras.size(); i++)
    {
        if (_library_cameras[i]->id() == id)
            return _library_cameras[i];
    }
    return NULL;
}

const CdaLight* CdaScene::findLibraryLight(QString id) const
{
    for (int i = 0; i < _library_lights.size(); i++)
    {
        if (_library_lights[i]->id() == id)
            return _library_lights[i];
    }
    return NULL;
}

const CdaMaterial* CdaScene::findLibraryMaterial(QString id) const
{
    for (int i = 0; i < _library_materials.size(); i++)
    {
        if (_library_materials[i]->id() == id)
            return _library_materials[i];
    }
    return NULL;
}

const CdaEffect* CdaScene::findLibraryEffect(QString id) const
{
    for (int i = 0; i < _library_effects.size(); i++)
    {
        if (_library_effects[i]->id() == id)
            return _library_effects[i];
    }
    return NULL;
}

CdaNode* CdaScene::traverseAndFindById( CdaNode* root, const QString& id )
{
    for (int i = 0; i < root->numChildren(); i++)
    {
        CdaNode* ret = traverseAndFindById(root->child(i), id);
        if (ret)
            return ret;
    }
    if (root->id() == id)
        return root;

    return NULL;
}

const CdaNode* CdaScene::findSceneNode(QString id) const
{
    return traverseAndFindById(_root, id);
}

  
// add library elements
    
void CdaScene::addLibraryLight( CdaLight* light )
{
    assert(!findLibraryLight(light->id()));

    _library_lights.push_back(light);
}

// add a node instance
void CdaScene::addNodeInstance( const QString& parent_id, CdaNode* node)
{
    assert(!findSceneNode(node->id()));
    assert(!findLibraryNode(node->id()));

    CdaNode* parent = traverseAndFindById( _root, parent_id );
    assert(parent);

    parent->addChild(node);
}

// Helper functions

void CdaScene::traverseAndGetVerts(const CdaScene *scene, const CdaNode* root, vector<CdaVec3>& verts, const CdaXform& current_xf )
{
    if (root->numChildren() > 0 || !root->nodeId().isEmpty())
    {
        for (int i = 0; i < root->numChildren(); i++)
            if (!root->child(i)->id().startsWith("path"))
                traverseAndGetVerts(scene, root->child(i), verts, current_xf * root->matrix() );

        if (!root->nodeId().isEmpty()) {
            const CdaNode* inst_node = scene->findLibraryNode(root->nodeId());
            if (inst_node)
                traverseAndGetVerts(scene, inst_node, verts, current_xf * root->matrix() );
        }
    }
    else
    {
        if (root->geometry())
        {
            const int num_geom_verts = root->geometry()->data(CDA_VERTEX).length();
            const CdaVec3 *geom_verts = root->geometry()->data(CDA_VERTEX).asVec3();
            for (int i = 0; i < num_geom_verts; i++)
                verts.push_back( current_xf * geom_verts[i] );
        }
    }
}

void CdaScene::findBoundingSphere(const CdaScene *scene, const CdaNode* root, CdaVec3& center, float& radius )
{
    // use the trimesh library's miniball code to compute the exact bounding sphere
    // of all the geometry underneath node root
    vector<CdaVec3> transformed_verts;

    traverseAndGetVerts(scene, root, transformed_verts, CdaXform() );

    Miniball<3,float> mb;
    mb.check_in(transformed_verts.begin(), transformed_verts.end());
    mb.build();
    center = mb.center();
    radius = sqrt(mb.squared_radius());
}

void CdaScene::findBoundingAABB(const CdaScene *scene, const CdaNode* root, CdaVec3& lower_corner, CdaVec3& upper_corner )
{
    vector<CdaVec3> transformed_verts;
    traverseAndGetVerts(scene, root, transformed_verts, CdaXform() );

    CdaVec3 ll = CdaVec3(10e6, 10e6, 10e6);
    CdaVec3 uu = CdaVec3(-10e6, -10e6, -10e6);
    for (uint32 i = 0; i < transformed_verts.size(); i++)
    {
        CdaVec3 vert = transformed_verts[i];
        ll[0] = min(vert[0], ll[0]);
        ll[1] = min(vert[1], ll[1]);
        ll[2] = min(vert[2], ll[2]);
        uu[0] = max(vert[0], uu[0]);
        uu[1] = max(vert[1], uu[1]);
        uu[2] = max(vert[2], uu[2]);
    }
    lower_corner = ll;
    upper_corner = uu;
}

void CdaScene::traverseAndFlatten(const CdaScene *scene, const CdaNode* root, CdaNode* new_root, const CdaXform& current_xf )
{
    if (root->numChildren() > 0 || !root->nodeId().isEmpty())
    {
        for (int i = 0; i < root->numChildren(); i++)
            traverseAndFlatten(scene, root->child(i), new_root, current_xf * root->matrix() );

        if (!root->nodeId().isEmpty()) {
            const CdaNode *inst_node = scene->findLibraryNode(root->nodeId());
            if (inst_node)
                traverseAndFlatten(scene, inst_node, new_root, current_xf * root->matrix() );
        }
    }
    else
    {
        CdaNode* new_node = new CdaNode();
        new_node->setId(QString("node%1").arg(new_root->numChildren()));
        new_node->setMatrix(current_xf * root->matrix());

        if (root->camera())
            new_node->setCamera(root->camera());

        if (root->geometry())
        {
            new_node->setGeometry(root->geometry());
            new_node->setMaterials(root->materials());
        }

        if (root->light())
            new_node->setLight(root->light());

        new_root->addChild(new_node);
    }
}

void CdaScene::expandAndFlattenNodes()
{
    // expand all node instances and flatten the hierarchy to 
    // a root node and the root's children

    CdaNode* new_root = new CdaNode();
    new_root->setId(_root->id());

    // walk the tree, keeping track of the current xform
    // and adding leaf nodes when we find them
    traverseAndFlatten(this, _root, new_root, _root->matrix() );

    // destroy the old hierarchy
    delete _root;

    for (int i = 0; i < _library_nodes.size(); i++)
        delete _library_nodes[i];
    _library_nodes.clear();

    _root = new_root;
}

void CdaScene::traverseAndAssignUid(CdaNode* root)
{
    root->setUid(_node_list.size());
    _node_list << root;
    
    for (int i = 0; i < root->numChildren(); i++) {
        traverseAndAssignUid(root->child(i));
    }
}

