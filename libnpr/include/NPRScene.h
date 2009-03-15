/*****************************************************************************\

NPRScene.h
Authors: Forrester Cole (fcole@cs.princeton.edu)
         Michael Burns (mburns@cs.princeton.edu)       
Copyright (c) 2009 Forrester Cole

The main scenegraph class for libnpr.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _NPR_SCENE_H_
#define _NPR_SCENE_H_

#include <QList>
#include <QString>
#include <QHash>
#include <QList>
#include <QVector>
#include <QDir>

#include "Vec.h"
#include "XForm.h"
#include "NPRUtility.h"
#include "NPRLight.h"
#include "NPRFixedPathSet.h"

#include "CdaTypes.h"
#include "CdaModelScene.h"
#include "CdaNode.h"

#include "GQInclude.h"
#include "GQStats.h"

class NPRDrawable;
class NPRStyle;
class NPRAnimController;
class NPRFixedPathSet;
class NPRGeometry;


class NPRScene
{
    public:
        typedef int NodeRef;
        typedef int DrawableRef;
        typedef QList<NPRDrawable*> DrawableList;
        typedef QList<NPRGeometry*> GeometryList;
        typedef QList<NodeRef> NodeRefList;
        typedef QList<DrawableRef> DrawableRefList;
        typedef QHash<QString, NPRGeometry*> StringGeometryHash;
        typedef QHash<NPRDrawable*, int> DrawableHash;
        typedef QList<NodeRefList> NodeRefListList;
        typedef QVector<DrawableList> DrawableListVector;
        typedef QList<DrawableRefList> DrawableRefListList;
        typedef QList<NPRAnimController*> AnimControllerList;

    public:
        NPRScene();
        ~NPRScene();

    	// Interface to the renderer (all methods must be const)
        int                 numDrawables() const { return _drawables.size(); }
        const NPRDrawable*  drawable(int which) const { return _drawables[which]; }
        const NPRStyle*     drawableStyle(int which) const { Q_UNUSED(which); return _global_style; }

        bool                isDrawablePotentiallyVisible( int which ) const;
        bool                isDrawableSelected(int which) const;

        int                 numSelectedDrawables() const { return _partition_nodes_list.last().size(); }
        int                 numPartitions() const { return _partition_nodes_list.size(); }
        const QList<int>&   drawableIndices(int partition, unsigned int flags) const;

        const QList<const NPRFixedPath*>& sortedPaths() const { return _sorted_path_list; }
        
        const xform&        cameraTransform() const { return _camera_transform; }
        const xform&        cameraInverseTransform() const { return _camera_inverse_transform; }
        const vec&          cameraPosition() const { return _camera_pos; }
        const vec&          cameraDirection() const { return _camera_dir; }
        const vec&          cameraUp() const { return _camera_up; }

        void                boundingSphere( vec& center, float& radius ) const;
        float               sceneRadius() const { return _bsphere_radius; }

        bool                hasViewDependentPaths() const { return _has_view_dependent_paths; }

        int				    numLights() const { return 1; }
        const NPRLight*     light(int which) const { Q_UNUSED(which); return &_light; }

        bool                areVBOsEnabled() const { return _enable_vbos; }

        const vec&		    focalPoint() const { return _focal_point; }
        
        const NPRStyle*     globalStyle() const { return _global_style; }

        // Interface to the application
        void        clear();
        bool        load( const QString& filename );
        bool        load( const QDomElement& element, const QDir& path );
        bool        save( QDomDocument& doc, QDomElement& element, const QDir& path );

        NPRLight*   light(int which) { Q_UNUSED(which); return &_light; }
        NPRStyle*   style() { return _global_style; }

        void	    setFocalPoint( const vec& p ) { _focal_point = p; }

        void	    setLight( const NPRLight& light ) { _light = light; }
    	void	    setStyle( NPRStyle* style );

        void        computePotentiallyVisibleSet();

        void        setCameraTransform( const xform& xf );
        void        setFieldOfView( float rad );
        void        setAnimationSpeed(float speed);
        void        setAnimationFrame(unsigned int frame);

        void        selectTrees(const NodeRefList &list);
        void        deselectTrees(const NodeRefList &list);
        void        newPartition();
        bool        deletePartition(int index);

        void        updateDrawableLists();
        void        updateVBOs();

        void        updateSortedPaths();

        void        recordStats( GQStats& stats );

        QAbstractItemModel* itemModel();

    protected:
        bool loadCollada( const QString& filename );

        void sortDrawables(); 

        void traverseAndFindInstances( const CdaNode* root, const CdaXform& current_xf, 
                                       const NPRAnimController* animation_path, int depth, DrawableList &drawables_list );

        void debugPartitions();

    protected:
        CdaModelScene*      _cda_scene;
        QString             _model_filename;

        DrawableList        _drawables;
        GeometryList        _geometries;
        StringGeometryHash  _id_to_geometry_map;
        DrawableListVector  _id_to_drawables_list_map;
        DrawableHash        _drawable_partition_map;
        NodeRefListList     _partition_nodes_list;
        DrawableRefListList _partitions;
        DrawableRefListList _partitions_opaque;
        DrawableRefListList _partitions_translucent;

        AnimControllerList  _anim_controllers;

        QList<bool>         _drawables_pvs;

        QList<const NPRFixedPath*> _sorted_path_list;

        xform               _camera_transform;
        xform               _camera_inverse_transform;
        vec                 _camera_pos;
        vec                 _frozen_camera_pos;
        vec                 _camera_dir;
        vec                 _camera_up;
        float               _fovy;

        bool                _has_view_dependent_paths;

        vec3f               _bsphere_center;
        float               _bsphere_radius;

        int                 _max_scene_depth;

        NPRLight            _light;
        NPRStyle*           _global_style;

        vec				    _focal_point;

        bool                _enable_vbos;
};



#endif // _NPR_SCENE_H_

