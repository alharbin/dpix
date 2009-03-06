#include "NPRDrawable.h"
#include "NPRGeometry.h"
#include "NPRFixedPathSet.h"
#include "NPRAnimController.h"
#include "NPRSettings.h"
#include "NPRStyle.h"

#include "GQInclude.h"

#include "CdaMaterial.h"
#include "CdaEffect.h"
#include "CdaNode.h"

NPRDrawable::NPRDrawable( const xform& transform, const NPRGeometry* geom )
{
    _transform = transform;
    _geom = geom;
    _node = 0;
    _anim_controller = 0;
    _path_set = new NPRFixedPathSet(this);
    _const_path_set = _path_set;
}

NPRDrawable::NPRDrawable( const xform& transform, const NPRGeometry* geom, 
                        const CdaNode* node, const NPRAnimController* path )
{
    _transform = transform;
    _geom = geom;
    _node = node;
    _anim_controller = path;
    _path_set = new NPRFixedPathSet(this);
    _const_path_set = _path_set;
}

void NPRDrawable::clear()
{
    delete _path_set;
    _transform = xform();
    _geom = 0;
    _node = 0;
    _anim_controller = 0;
    _const_path_set = 0;
    _path_set = 0;
}

void NPRDrawable::pushTransform() const
{
    glPushMatrix();
    if (_anim_controller)
        glMultMatrixd( _anim_controller->transform() );
    glMultMatrixd( _transform );
}

void NPRDrawable::popTransform() const
{
    glPopMatrix();
}

void NPRDrawable::composeTransform( xform& xf ) const
{
    if (_anim_controller)
        xf = _anim_controller->transform() * _transform;
    else
        xf = _transform;
}

void NPRDrawable::composeTransformInverseTranspose( xform& xf ) const
{
    if (_anim_controller)
        xf = _anim_controller->transform() * _transform;
    else
        xf = _transform;

    xf = inv(xf);
    xf = transpose(xf);
}

const CdaMaterial* NPRDrawable::lookupMaterial( const NPRPrimitive* prim ) const
{
    if (_node && _node->materials().contains(prim->materialSymbol()))
        return _node->materials().value(prim->materialSymbol());

    return 0;
}

bool NPRDrawable::hasOpaque() const
{
    for (int i = 0; i < NPR_NUM_PRIMITIVES; i++)
    {
        for (int j = 0; j < _geom->primList((NPRPrimitiveType)i).size(); j++)
        {
            const CdaMaterial* material = lookupMaterial(_geom->primList((NPRPrimitiveType)i)[j]);
            if (material)
            {
                const CdaEffect* effect = material->_inst_effect;
                if (effect->_transparency == 0.0f)
                {
                    return true;
                }
            }
            else
            {
                return true;
            }
        }
    }
    return false;
}

bool NPRDrawable::hasTranslucent() const
{
    for (int i = 0; i < NPR_NUM_PRIMITIVES; i++)
    {
        for (int j = 0; j < _geom->primList((NPRPrimitiveType)i).size(); j++)
        {
            const CdaMaterial* material = lookupMaterial(_geom->primList((NPRPrimitiveType)i)[j]);
            if (material)
            {
                const CdaEffect* effect = material->_inst_effect;
                if (effect->_transparency > 0.0f)
                {
                    return true;
                }
            }
        }
    }
    return false;
}


static QString blankId;

const QString& NPRDrawable::id() const
{
    if (_node)
        return _node->id();
    else
        return blankId;
}
