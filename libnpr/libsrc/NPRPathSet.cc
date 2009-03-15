/*****************************************************************************\

NPRPathSet.cc
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "NPRPathSet.h"
#include <algorithm>
#include <assert.h>

QList<NPRPath*>  NPRPath::_free_pool;

void NPRPathAttr::copyFrom( const NPRFixedPath* source )
{
    type = source->attributes().type;
    static_id = source->attributes().static_id;
    model_id = source->attributes().model_id;
    source_path = source;
}

void NPRPath::clear()
{
    _verts.clear();
    _attrs.clear();
    _attributes = NPRPathAttr();
}

void NPRPath::reset()
{
    _verts.clear();
    _attrs.clear();
}

void NPRPath::copyFromFixedPath( const NPRFixedPath* source)
{
    _attributes.source_path = source;
    _attributes.static_id = source->attributes().static_id;
    _attributes.model_id = source->attributes().model_id;
    _attributes.type = source->attributes().type;

    for (int i = 0; i < source->size(); i++)
    {
        _verts.push_back(source->vert(i));
        NPRVertAttr attr;
        attr.source1 = i;
        attr.source2 = i;
        attr.interp_t = 0;
        _attrs.push_back(attr);
    }
}

void NPRPath::addVert( const vec& v )
{
    _verts.push_back(v);
    _attrs.push_back(NPRVertAttr());
}

void NPRPath::addVert( const vec& v, float visibility )
{
    NPRVertAttr attr;
    attr.visibility = visibility;
    _verts.push_back(v);
    _attrs.push_back(attr);
}

void NPRPath::addVert( const NPRPath* source, int which )
{
    assert(source->attributes().source_path == _attributes.source_path);

    _verts.push_back(source->_verts[which]);
    _attrs.push_back(source->_attrs[which]);
}

void NPRPath::addInterpolatedVert( const NPRPath* source, int v1, int v2, float t )
{
    assert(source->attributes().source_path == _attributes.source_path);
    assert( source->vertAttr(v1).interp_t <= 0 && source->vertAttr(v2).interp_t <= 0);

    vec v = source->vert(v1) * (1.0f - t) + source->vert(v2) * t;
    NPRVertAttr attr;
    const NPRVertAttr& a1 = source->vertAttr(v1);
    const NPRVertAttr& a2 = source->vertAttr(v2);
    attr.interp_t = t;
    attr.source1 = a1.source1;
    attr.source2 = a2.source1;
    attr.priority = a1.priority * (1.0f - t) + a2.priority * t;
    attr.visibility = a1.visibility * (1.0f - t) + a2.visibility * t;

    _verts.push_back(v);
    _attrs.push_back(attr);
}

float NPRPath::length() const
{
    int nseg = size()-1;
    float len = 0.0f;
    for (int cur_vert = 0; cur_vert < nseg; cur_vert++)
    {
        vec seg = vert(cur_vert) - vert(cur_vert+1);
        len += sqrtf(seg[0]*seg[0] + seg[1]*seg[1] + seg[2]*seg[2]);
    }
    return len;
}

void NPRPath::transform( const xform& xf )
{
    for (int i = 0; i < _verts.size(); i++)
        _verts[i] = xf * _verts[i];
}

void NPRPath::addRef()
{
    _ref_count++;
}

void NPRPath::decRef()
{
    assert(_ref_count > 0);
    _ref_count--;
    if (_ref_count == 0)
        deletePath(this);
}

NPRPath* NPRPath::newPath()
{
    NPRPath* newpath = 0;
    if (_free_pool.size() > 0)
    {
        newpath = _free_pool.takeFirst();
        newpath->clear();
    }
    else
    {
        newpath = new NPRPath();
    }
    return newpath;
}

NPRPath* NPRPath::newPath(const NPRPathAttr& attr)
{
    NPRPath* newpath = newPath();
    newpath->_attributes = attr;
    return newpath;
}

NPRPath* NPRPath::newPath(const NPRFixedPath* source)
{
    NPRPath* newpath = newPath();
    newpath->copyFromFixedPath(source);
    return newpath;
}

void NPRPath::deletePath(NPRPath* path)
{
    assert(path->_ref_count == 0);
    path->_ref_count = 0;
    _free_pool.append(path);
}
        
void NPRPathSet::addPath( NPRPath* path )
{
    _paths.append(path);
    path->addRef();
}

void NPRPathSet::clear()
{
    while (!_paths.isEmpty())
        _paths.takeFirst()->decRef();
}

NPRPathSet& NPRPathSet::operator=(NPRPathSet& orig)
{
    clear();

    _paths = orig._paths;
    for (int i = 0; i < _paths.size(); i++)
        _paths[i]->addRef();

    return *this;
}


bool compare(const NPRPath *lhs, const NPRPath *rhs)
{
    /*if(lhs->attributes.manualLevel != rhs->attributes.manualLevel)
        return (lhs->attributes.manualLevel < rhs->attributes.manualLevel );
    else */

    /*if (lhs->attributes.model_id != rhs->attributes.model_id)
        return (lhs->attributes.model_id >
                rhs->attributes.model_id);
    else*/ 
    if (lhs->attributes().source_path && rhs->attributes().source_path &&
        lhs->attributes().source_path->attributes().static_length != 
            rhs->attributes().source_path->attributes().static_length)
        return (lhs->attributes().source_path->attributes().static_length < 
                    rhs->attributes().source_path->attributes().static_length);
    else
        return (lhs->attributes().static_id < rhs->attributes().static_id );
}


void NPRPathSet::sort()
{
    std::sort(_paths.begin(), _paths.end(), &compare);
}
