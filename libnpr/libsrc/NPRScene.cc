#include "NPRScene.h"

#include "NPRAnimController.h"
#include "NPRDrawable.h"
#include "NPRGeometry.h"
#include "NPRLight.h"
#include "NPRStyle.h"
#include "NPRSettings.h"
#include "NPRGLDraw.h"

#include "CdaGeometry.h"

#include "Vec.h"
#include "XForm.h"
#include <QList>
#include <QDebug>
#include <QStringList>
#include <assert.h>

const int CURRENT_VERSION = 1;

NPRScene::NPRScene()
{
    _global_style = NULL;
    _cda_scene = 0;    

    clear();

    // Have to initialize this after the clear.
    _global_style = new NPRStyle();
}

NPRScene::~NPRScene()
{
    clear();
}

void NPRScene::clear()
{
    if (_cda_scene)
    {
        delete _cda_scene;
    }
    _cda_scene = 0;

    while (!_drawables.isEmpty())
        delete _drawables.takeLast();

    while (!_geometries.isEmpty())
        delete _geometries.takeLast();

    while (!_anim_controllers.isEmpty())
        delete _anim_controllers.takeLast();

    _id_to_geometry_map.clear();
    _id_to_drawables_list_map.clear();

    // clear partitions
    _partition_nodes_list.clear();
    _partitions.clear();
    _partitions_opaque.clear();
    _partitions_translucent.clear();
    // insert default partition (node list may be empty)
    newPartition();
    // insert current selection partition
    newPartition();

    _camera_transform = xform();

    _has_view_dependent_paths = false;

    _bsphere_center = vec(0,0,0);
    _bsphere_radius = -1;

    _enable_vbos = false;

    _model_filename = QString();

    _fovy = 0;

    if (_global_style)
        delete _global_style;

    _global_style = NULL;
}

void NPRScene::setStyle( NPRStyle* style )
{
	if (_global_style)
		delete _global_style;
	_global_style = style;
}

void NPRScene::setFieldOfView( float rad ) 
{ 
    _fovy = rad; 
}

bool NPRScene::isDrawablePotentiallyVisible( int which ) const 
{ 
    return _drawables_pvs[which] || !NPRSettings::instance().get(NPR_COMPUTE_PVS);
}

void NPRScene::computePotentiallyVisibleSet()
{
    int visible_count = 0;
    if (NPRSettings::instance().get(NPR_COMPUTE_PVS))
    {
        __START_TIMER("Compute PVS");

        _drawables_pvs.clear();
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        float aspect_ratio = (float)(viewport[2]) / (float)(viewport[3]);
        float tan_fovy_2 = tanf(_fovy/2.0f); 
        // find radius of cone that extends to corner of screen
        float to_corner = sqrt(aspect_ratio*aspect_ratio + 1);
        to_corner *= tan_fovy_2;
        float angle = atanf(to_corner);
        float sin_cone = sinf(angle);
        float cos_cone = cosf(angle);

        const vec& viewpos = cameraPosition();
        const vec& viewdir = cameraDirection();
        vec yardstick(1,1,1);

        for (int i = 0; i < _drawables.size(); i++)
        {
            bool size_check_passed = false;
            bool cone_check_passed = false;

            const NPRDrawable* drawable = _drawables[i];
            xform xf, xf_rot;
            drawable->composeTransform(xf);
            xf_rot = rot_only(xf);

            // Very simple and approximate bounding sphere size check.
            vec center = xf * drawable->geometry()->bsphereCenter();
            float radius = drawable->geometry()->bsphereRadius();

            vec scaled_yardstick = xf_rot * yardstick;
            float max_scale = max(fabsf(scaled_yardstick[0]), 
                                  max(fabsf(scaled_yardstick[1]), 
                                      fabsf(scaled_yardstick[2])));
            float scaled_radius = max_scale * radius;
            float dist = len(viewpos - center);
            float width = dist * tan_fovy_2;
            float ratio = scaled_radius / width;

            float approx_pix = viewport[3] * ratio;
            size_check_passed = approx_pix > 0.0f;

            if (!(scaled_radius < 100000 && scaled_radius > 0))
            {
                qWarning("Bogus drawable bounding sphere found.");
            }

            if (!size_check_passed)
            {
                _drawables_pvs.append(false);
                continue;
            }

            // Conservative frustum cone test.

            float offset_amt = radius / sin_cone;
            vec apex_offset = viewdir * (-offset_amt);
            vec cone_apex = viewpos + apex_offset;
            vec to_bsphere = center - cone_apex;
            dist = len(to_bsphere);
            float dotdir = (viewdir DOT to_bsphere) / dist;

            cone_check_passed = dotdir > cos_cone;

            if (!cone_check_passed)
            {
                _drawables_pvs.append(false);
                continue;
            }

            _drawables_pvs.append(true);
            visible_count++;
        }

        __STOP_TIMER("Compute PVS");

        __SET_COUNTER("PVS Drawables", visible_count);
    }
}

bool NPRScene::save( QDomDocument& doc, QDomElement& element, const QDir& path )
{
    element.setAttribute("version", CURRENT_VERSION);

    QDomElement model = doc.createElement("model");
    QString relative_filename = path.relativeFilePath(_model_filename);
    model.setAttribute("filename", relative_filename);
	element.appendChild(model);

    QDomElement light = doc.createElement("light");
    _light.save(doc, light);
    element.appendChild(light);

    QDomElement style = doc.createElement("style");
    _global_style->save(doc, style);
    element.appendChild(style);

    QDomElement focal_point = doc.createElement("focal_point");
    focal_point.setAttribute("x", _focal_point[0]);
    focal_point.setAttribute("y", _focal_point[1]);
    focal_point.setAttribute("z", _focal_point[2]);
    element.appendChild(focal_point);

    return true;
}

bool NPRScene::load( const QDomElement& element, const QDir& path )
{
    int version = element.attribute("version").toInt();
    if (version != CURRENT_VERSION)
    {
        qWarning("NPRScene::load: scene XML out of date (%d, current is %d)\n", 
            version, CURRENT_VERSION);
        return false;
    }
    QDomElement model = element.firstChildElement("model");
    assert(!model.isNull());

    QString relative_filename = model.attribute("filename");
    QString abs_filename = path.absoluteFilePath(relative_filename);

    bool ret = load(abs_filename);
    if (!ret)
    {
        qWarning("NPRScene::load: could not load %s\n", 
            qPrintable(abs_filename));
        return false;
    }

    QDomElement light = element.firstChildElement("light");
    if (!light.isNull())
    {
        bool light_ret = _light.load(light);
        if (!light_ret)
        {
            qWarning("NPRScene::load: could not load light from XML\n");
            clear();
            return false;
        }
    }

    QDomElement style = element.firstChildElement("style");
    if (!style.isNull())
    {
        NPRStyle* new_style = new NPRStyle();
        bool style_ret = new_style->load(style);
        if (!style_ret)
        {
            qWarning("NPRScene::load: could not load style from XML\n");
            delete new_style;
            clear();
            return false;
        }
        setStyle(new_style);
    }

    QDomElement focal_point = element.firstChildElement("focal_point");
    if (!focal_point.isNull())
    {
        _focal_point[0] = focal_point.attribute("x").toFloat();
        _focal_point[1] = focal_point.attribute("y").toFloat();
        _focal_point[2] = focal_point.attribute("z").toFloat();
    }

    return true;
}

bool NPRScene::load( const QString& filename )
{
    clear();

    if (CdaScene::isColladaFile(filename))
    {
        if (!loadCollada(filename))
            return false;
    }
    else
    {
        return false;
    }

    _model_filename = filename;
    _global_style = new NPRStyle();

    sortDrawables();
    updateDrawableLists();
    updateSortedPaths();

    return true;
}

bool NPRScene::loadCollada( const QString& filename )
{
    _cda_scene = new CdaModelScene();
    bool ret = _cda_scene->load(filename);

    if (ret)
    {
        for (int i = 0; i < _cda_scene->numLibraryGeometries(); i++)
        {
            const CdaGeometry* geom = _cda_scene->libraryGeometry(i);
            NPRGeometry* newgeom = new NPRGeometry(geom);
            _geometries.push_back(newgeom);
            _id_to_geometry_map.insert(geom->_id, newgeom);
        }

        _max_scene_depth = 0;

        _id_to_drawables_list_map.resize(_cda_scene->numNodes());
        traverseAndFindInstances( _cda_scene->root(), CdaXform(), NULL, 
            _max_scene_depth, _drawables );

        if (_drawables.size() == 0)
        {
            qWarning("NPRScene::load - No drawable nodes loaded.\n");
            return false;
        }

        CdaScene::findBoundingSphere( _cda_scene, _cda_scene->root(), 
            _bsphere_center, _bsphere_radius );

        return true;
    }
    else
    {
        delete _cda_scene;
        _cda_scene = 0;

        return false;
    }
}

void NPRScene::traverseAndFindInstances(const CdaNode* root, 
                                        const CdaXform& current_xf, 
                                        const NPRAnimController* animation_path, 
                                        int depth, DrawableList &drawables_list)
{
    assert( _cda_scene );

    if (depth > _max_scene_depth)
        _max_scene_depth = depth;

    if (root->path())
    {
        vec va, vb;
        CdaScene::findBoundingAABB(_cda_scene, root, va, vb);
        vec center = (va + vb) / 2.0f;
        const NPRGeometry* geom = _id_to_geometry_map.value( root->path()->_id );
        assert(geom);
        NPRAnimController* new_controller = new NPRAnimController(geom, center);
        _anim_controllers.append(new_controller);
        animation_path = new_controller;
    }

    // traverse the scene, grab the models and lights, and push
    // them into our intermediate lists
    if (root->numChildren() > 0 || !root->nodeId().isEmpty() )
    {
        for (int i = 0; i < root->numChildren(); i++) {
            DrawableList child_drawables;
            traverseAndFindInstances( root->child(i), current_xf * root->matrix(), animation_path, depth + 1, child_drawables );
            _id_to_drawables_list_map[root->uid()] << child_drawables;
            drawables_list << child_drawables;
        }

        if (!root->nodeId().isEmpty()) {
            const CdaNode *inst_node = _cda_scene->findLibraryNode(root->nodeId());
            if (inst_node) {
                DrawableList child_drawables;
                traverseAndFindInstances(inst_node, current_xf * root->matrix(), animation_path, depth + 1, child_drawables );
                _id_to_drawables_list_map[root->uid()] << child_drawables;
                drawables_list << child_drawables;
            }
        }
    }

    if (root->geometry())
    {
        const NPRGeometry* geom = _id_to_geometry_map.value( root->geometry()->_id );
        assert(geom);

        NPRDrawable* newdrawable = new NPRDrawable( current_xf * root->matrix(), geom, root, animation_path );
        _id_to_drawables_list_map[root->uid()] << newdrawable;
        drawables_list << newdrawable;
        _drawables_pvs << true;
    }
}

bool compareDrawables(const NPRDrawable* lhs, const NPRDrawable* rhs)
{
    return (int)(lhs->geometry()) < (int)(rhs->geometry());
}

void NPRScene::sortDrawables()
{
    std::sort(_drawables.begin(), _drawables.end(), &compareDrawables);
}

const QList<int>& NPRScene::drawableIndices(int partition, unsigned int flags) const
{
    // TODO clean up on aisle 7
    if (flags & NPR_OPAQUE && flags & NPR_TRANSLUCENT)
        return _partitions[partition];
    else if (flags & NPR_OPAQUE)
        return _partitions_opaque[partition];
    else if (flags & NPR_TRANSLUCENT)
        return _partitions_translucent[partition];
    else
        return _partitions[partition];
}

void NPRScene::boundingSphere( vec& center, float& radius ) const
{
    center = _bsphere_center;
    radius = _bsphere_radius;
}

void NPRScene::setCameraTransform( const xform& xf )
{
    _camera_transform = xf;
    _camera_inverse_transform = xf;
    invert(_camera_inverse_transform);

    _camera_pos = xf * vec(0,0,0);
    if (!NPRSettings::instance().get(NPR_FREEZE_LINES))
        _frozen_camera_pos = _camera_pos;

    xform rot_only = xf;
    rot_only[12] = 0; rot_only[13] = 0; rot_only[14] = 0;
    _camera_dir = rot_only * vec(0,0,-1);
    _camera_up = rot_only * vec(0,1,0);
}

void NPRScene::setAnimationSpeed(float speed)
{
    for (int i = 0; i < _anim_controllers.size(); i++)
        _anim_controllers[i]->setSpeed(speed);
}

void NPRScene::setAnimationFrame(unsigned int frame)
{
    for (int i = 0; i < _anim_controllers.size(); i++)
        _anim_controllers[i]->setFrame(frame);
}

void NPRScene::selectTrees(const NodeRefList &list)
{
    NodeRefList &current_node_list = _partition_nodes_list.last();
    current_node_list << list;
}

void NPRScene::deselectTrees(const NodeRefList &list)
{
    NodeRefList &current_node_list = _partition_nodes_list.last();
    for (int i = 0; i < list.size(); i++) {
        current_node_list.removeAll(list[i]);
    }
}

void NPRScene::newPartition()
{
    _partition_nodes_list << NodeRefList();
    _partitions << DrawableRefList();
    _partitions_opaque << DrawableRefList();
    _partitions_translucent << DrawableRefList();

    // do not need to update because drawable lists should be unchanged
}

bool NPRScene::deletePartition(int index)
{
    // Do not delete first or last partition
    if (index <= 0 || index >= _partition_nodes_list.size() - 1)
        return false;
    else {
        _partition_nodes_list.removeAt(index);
        _partitions.removeAt(index);
        _partitions_opaque.removeAt(index);
        _partitions_translucent.removeAt(index);
    }

    updateDrawableLists();

    return true;
}

bool NPRScene::isDrawableSelected(int which) const
{
    return _drawable_partition_map[_drawables[which]] == (_partition_nodes_list.size() - 1);
}

void NPRScene::updateDrawableLists()
{
    for (int i = 0; i < _drawables.size(); i++) {
        _drawable_partition_map[_drawables[i]] = 0;
    }
    for (int i = 1; i < _partition_nodes_list.size(); i++) {
        const NodeRefList &partition_nodes = _partition_nodes_list[i];
        for (int j = 0; j < partition_nodes.size(); j++) {
            const DrawableList &drawables_list = _id_to_drawables_list_map[partition_nodes[j]];
            for (int k = 0; k < drawables_list.size(); k++) {
                _drawable_partition_map[drawables_list[k]] = i;
            }
        }
    }
    for (int i = 0; i < _partition_nodes_list.size(); i++) {
        _partitions[i].clear();
        _partitions_opaque[i].clear();
        _partitions_translucent[i].clear();
    }
    for (int i = 0; i < _drawables.size(); i++) {
        int partition_num = _drawable_partition_map[_drawables[i]];
        bool has_opaque = _drawables[i]->hasOpaque();
        bool has_translucent = _drawables[i]->hasTranslucent();

        _partitions[partition_num] << i;

        if (has_opaque)        
            _partitions_opaque[partition_num] << i;

        if (has_translucent)
            _partitions_translucent[partition_num] << i;
    }

    //    debugPartitions();
}

void NPRScene::debugPartitions()
{
    qDebug() << numPartitions() << "partitions:";
    for (int i = 0; i < _partition_nodes_list.size(); i++) {
        qDebug() << _partition_nodes_list[i];
    }
}

bool comparePaths(const NPRFixedPath* lhs, const NPRFixedPath* rhs)
{
    return (int)(lhs->cachedLength()) < (int)(rhs->cachedLength());
}

void NPRScene::updateSortedPaths()
{
    // Get all the paths currently in the scene.
    for (int m = 0; m < numDrawables(); m++)
    {
        const NPRFixedPathSet* pathset = drawable(m)->paths();
        if (!pathset)
            continue;

        for (int i = 0; i < pathset->size(); i++)
        {
            const NPRFixedPath* path = (*pathset)[i]; 

            _sorted_path_list.append(path);
        }
    }

    // Sort the paths, currently by static length,
    // which is set in the path at construction time.
    std::sort(_sorted_path_list.begin(), _sorted_path_list.end(), comparePaths);
}

void NPRScene::recordStats( GQStats& stats )
{
    int prim_sets[NPR_NUM_PRIMITIVES];
    int prim_counts[NPR_NUM_PRIMITIVES];
    int inst_prim_sets[NPR_NUM_PRIMITIVES];
    int inst_prim_counts[NPR_NUM_PRIMITIVES];
    int vertex_count = 0;
    int inst_vertex_count = 0;
    memset(prim_sets, 0, sizeof(prim_sets));
    memset(prim_counts, 0, sizeof(prim_counts));
    memset(inst_prim_sets, 0, sizeof(inst_prim_sets));
    memset(inst_prim_counts, 0, sizeof(inst_prim_counts));

    int inst_path_sets = 0;
    int inst_path_counts = 0;
    int len_2_paths = 0;
    int total_path_segments = 0;

    int type_strides[NPR_NUM_PRIMITIVES] = {3, 0, 2, 0, 2};

    int num_geoms = _geometries.size();
    for (int i = 0; i < num_geoms; i++)
    {
        const NPRGeometry* geom = _geometries[i];

        vertex_count += geom->numVertices();

        for (int j = 0; j < NPR_NUM_PRIMITIVES; j++)
        {
            const NPRPrimPointerList& prims = geom->primList((NPRPrimitiveType)j);

            prim_sets[j] += prims.size();
            for (int k = 0; k < prims.size(); k++)
            {
                int count = (type_strides[j] > 0) ? prims[k]->size() / type_strides[j] : 1;
                prim_counts[j] += count;
            }
        }
    }

    for (int i = 0; i < _drawables.size(); i++)
    {
        const NPRGeometry* geom = _drawables[i]->geometry();
        
        inst_vertex_count += geom->numVertices();

        for (int j = 0; j < NPR_NUM_PRIMITIVES; j++)
        {
            const NPRPrimPointerList& prims = geom->primList((NPRPrimitiveType)j);

            inst_prim_sets[j] += prims.size();
            for (int k = 0; k < prims.size(); k++)
            {
                int count = (type_strides[j] > 0) ? prims[k]->size() / type_strides[j] : 1;
                inst_prim_counts[j] += count;
            }
        }

        const NPRFixedPathSet* paths = _drawables[i]->paths();
        if (paths)
        {
            inst_path_sets++;
            for (int j = 0; j < paths->size(); j++)
            {
                if (paths->at(j)->size() == 2)
                    len_2_paths++;
                inst_path_counts++;
                total_path_segments += paths->at(j)->size() - 1;
            }
        }
    }

    stats.beginConstantGroup("Scene");
    stats.setConstant("Tree Depth", _max_scene_depth);
    stats.setConstant("Drawable Nodes", _drawables.size());
    stats.beginConstantGroup("Library Geometry (# sets)");
    stats.setConstant("vertices", QString("%1").arg(vertex_count));
    stats.setConstant("triangles", QString("%1 (%2)").arg(prim_counts[NPR_TRIANGLES]).arg(prim_sets[NPR_TRIANGLES]));
    stats.setConstant("triangle strips", QString("%1 (%2)").arg(prim_counts[NPR_TRIANGLE_STRIP]).arg(prim_sets[NPR_TRIANGLE_STRIP]));
    stats.setConstant("lines", QString("%1 (%2)").arg(prim_counts[NPR_LINES]).arg(prim_sets[NPR_LINES]));
    stats.setConstant("line strips", QString("%1 (%2)").arg(prim_counts[NPR_LINE_STRIP]).arg(prim_sets[NPR_LINE_STRIP]));
    stats.setConstant("profile edges", QString("%1 (%2)").arg(prim_counts[NPR_PROFILES]).arg(prim_sets[NPR_PROFILES]));
    stats.endConstantGroup();
    stats.beginConstantGroup("Instanced Geometry (# sets)");
    stats.setConstant("vertices", QString("%1").arg(inst_vertex_count));
    stats.setConstant("triangles", QString("%1 (%2)").arg(inst_prim_counts[NPR_TRIANGLES]).arg(inst_prim_sets[NPR_TRIANGLES]));
    stats.setConstant("triangle strips", QString("%1 (%2)").arg(inst_prim_counts[NPR_TRIANGLE_STRIP]).arg(inst_prim_sets[NPR_TRIANGLE_STRIP]));
    stats.setConstant("lines", QString("%1 (%2)").arg(inst_prim_counts[NPR_LINES]).arg(inst_prim_sets[NPR_LINES]));
    stats.setConstant("line strips", QString("%1 (%2)").arg(inst_prim_counts[NPR_LINE_STRIP]).arg(inst_prim_sets[NPR_LINE_STRIP]));
    stats.setConstant("profile edges", QString("%1 (%2)").arg(inst_prim_counts[NPR_PROFILES]).arg(inst_prim_sets[NPR_PROFILES]));
    stats.endConstantGroup();
    stats.beginConstantGroup("Paths");
    stats.setConstant("paths", QString("%1 (%2)").arg(inst_path_counts).arg(inst_path_sets));
    stats.setConstant("paths of size 2", len_2_paths);
    stats.setConstant("total path segments (plus profiles)", QString("%1 (%2)").arg(total_path_segments).arg(total_path_segments + inst_prim_counts[NPR_PROFILES]));
    stats.endConstantGroup();
    stats.endConstantGroup();
}

QAbstractItemModel* NPRScene::itemModel()
{
    if (_cda_scene)
        return _cda_scene;

    return 0;
}


void NPRScene::updateVBOs()
{
    // It appears that it is actually faster to *not* use VBOs for 
    // small bits of geometry, at least with some cards.
    const int MINIMUM_VERTICES = 20;

    bool enable = NPRSettings::instance().get(NPR_ENABLE_VBOS);

    if (!_enable_vbos && enable)
    {
        for (int i = 0; i < _geometries.size(); i++)
        {
            if (_geometries[i]->data(GQ_VERTEX)->size() > MINIMUM_VERTICES)
            {
                _geometries[i]->copyToVBO();
            }
        }
        _enable_vbos = true;
    }
    else if (_enable_vbos && !enable)
    {
        for (int i = 0; i < _geometries.size(); i++)
        {
            _geometries[i]->deleteVBO();
        }
        _enable_vbos = false;
    }
}
