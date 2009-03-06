#ifndef _NPR_DRAWABLE_H_
#define _NPR_DRAWABLE_H_

#include "XForm.h"
#include "GQInclude.h"

#include <QHash>
#include <QString>

class CdaMaterial;
class CdaNode;
class NPRGeometry;
class NPRFixedPathSet;
class NPRPrimitive;
class NPRAnimController;
class NPRStyle;

// A drawable node in the scene graph.

class NPRDrawable
{
    public:
        NPRDrawable( const xform& transform, const NPRGeometry* geom );
        NPRDrawable( const xform& transform, const NPRGeometry* geom, 
                     const CdaNode* node, const NPRAnimController* path );

        // Interface to the renderer (all methods must be const)
        void pushTransform() const;
        void popTransform() const;
        void composeTransform( xform& xf ) const;
        void composeTransformInverseTranspose( xform& xf ) const;
        const xform& transform() const { return _transform; }

        const NPRGeometry*  geometry() const { return _geom; }
        const NPRFixedPathSet*   paths() const { return _const_path_set; }

        const CdaMaterial* lookupMaterial( const NPRPrimitive* prim ) const;
        bool               hasOpaque() const;
        bool               hasTranslucent() const;

        const QString&     id() const;
        const CdaNode*     node() const { return _node; }

        // Interface to the application
        void clear();

    protected:
        const NPRGeometry*       _geom;
        const CdaNode*           _node;
        const NPRAnimController* _anim_controller;
        xform                    _transform;

        NPRFixedPathSet*         _path_set;
        const NPRFixedPathSet*   _const_path_set;

};

#endif


