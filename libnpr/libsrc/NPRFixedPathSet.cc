#include "NPRFixedPathSet.h"
#include <algorithm>
#include <assert.h>

// NPRFixedPath

NPRFixedPath::NPRFixedPath( const NPRFixedPath& path) : 
    NPRPrimitive(NPR_LINE_STRIP) 
{ 
    *this = path; 
}

NPRFixedPath::NPRFixedPath( const NPRGeometry* geom ) : 
    NPRPrimitive(NPR_LINE_STRIP), _const_geom(geom), 
    _drawable(0), _attributes() 
{}

NPRFixedPath::NPRFixedPath( const NPRGeometry* geom, 
                            const NPRFixedPathAttr& attr ) : 
    NPRPrimitive(NPR_LINE_STRIP), _const_geom(geom), 
    _drawable(0), _attributes(attr) 
{}

NPRFixedPath::NPRFixedPath( const NPRGeometry* geom, 
                            const NPRDrawable* drawable,
                            const NPRFixedPathAttr& attr ) : 
    NPRPrimitive(NPR_LINE_STRIP), _const_geom(geom), 
    _drawable(drawable), _attributes(attr) 
{}

float NPRFixedPath::computeLength()
{
    int nseg = size()-1;
    float len = 0.0f;
    if (_drawable)
    {
        xform xf;
        _drawable->composeTransform(xf);
        for (int cur_vert = 0; cur_vert < nseg; cur_vert++)
        {
            vec v1 = xf * vert(cur_vert);
            vec v2 = xf * vert(cur_vert+1);
            vec seg = v1 - v2;
            len += sqrtf(seg[0]*seg[0] + seg[1]*seg[1] + seg[2]*seg[2]);
        }
    }
    else 
    {
        for (int cur_vert = 0; cur_vert < nseg; cur_vert++)
        {
            vec seg = vert(cur_vert) - vert(cur_vert+1);
            len += sqrtf(seg[0]*seg[0] + seg[1]*seg[1] + seg[2]*seg[2]);
        }
    }
    _attributes.static_length = len;
    return len;
}

void NPRFixedPath::appendPath( const NPRFixedPath* path )
{
    assert( path->_const_geom == _const_geom );

    for (int i = 0; i < path->size(); i++)
        push_back((*path)[i]);
}

void NPRFixedPath::reverse()
{
    int size = this->size();
    int* data = this->data();
    for (int i = 0; i < size/2; i++)
    {
        int from_back = size - i - 1;
        int temp = data[i];
        data[i] = data[from_back];
        data[from_back] = temp;
    }
}

void NPRFixedPath::orientToMajorAxis()
{
    // Note: this code assumes that the path lies in a plane.
    // If the normal of the plane that the path lies in
    // is mostly towards a negative major axis, it flips the path.

    if (size() > 2)
    {
        vec a = vert(1) - vert(0);
        vec b = vert(2) - vert(1);
        vec normal = a CROSS b;
        int maxindex = 0;
        float maxcoord = abs(normal[0]);
        for (int i = 1; i < 3; i++)
        if (abs(normal[i]) > maxcoord)
        {
            maxcoord = abs(normal[i]);
            maxindex = i;
        }
        if (normal[maxindex] < 0)
            reverse();
    }
}


void NPRFixedPath::debugDump() const
{
    qDebug("Path id: %d", _attributes.static_id);
    qDebug("Path size: %d", size());
    for (int i = 0; i < size(); i++)
    {
        const vec3f& v = vert(i);
        qDebug("%d %f %f %f", value(i), v[0], v[1], v[2]);
    }
}


// NPRFixedPathSet

NPRFixedPathSet::NPRFixedPathSet( const NPRDrawable* drawable )
{
    _drawable = drawable;
    _const_geom = drawable->geometry();
    init();
}

NPRFixedPathSet::NPRFixedPathSet( const NPRGeometry* geom )
{
    _drawable = 0;
    _const_geom = geom;
    init();
}
        
void NPRFixedPathSet::init()
{
    NPRFixedPathAttr attr;

    for (int i = 0; i < _const_geom->primList(NPR_LINES).size(); i++)
    {
        const NPRPrimitive* prim = _const_geom->primList(NPR_LINES)[i];
        stitchLinesIntoPaths(prim, attr);
    }

    attr.type = NPR_CREASE;
    for (int i = 0; i < _const_geom->primList(NPR_LINE_STRIP).size(); i++)
    {
        const NPRPrimitive* prim = _const_geom->primList(NPR_LINE_STRIP)[i];
        NPRFixedPath* newpath = new NPRFixedPath(_const_geom, _drawable, attr);
        for (int j = 0; j < prim->size(); j++ )
        {
            *newpath << prim->at(j);
        }
        addPath(newpath);
    }

    attr.type = NPR_PROFILE;
    for (int i = 0; i < _const_geom->primList(NPR_PROFILES).size(); i++)
    {
        const NPRPrimitive* prim = _const_geom->primList(NPR_PROFILES)[i];
        for (int j = 0; j < prim->size(); j += 2 )
        {
            NPRFixedPath* newpath = new NPRFixedPath(_const_geom, _drawable, attr);
            *newpath << prim->at(j);
            *newpath << prim->at(j + 1);
            addPath(newpath);
        }
    }

    assignStaticIDs();
    assignStaticLengths();
    orientPaths();
}

void NPRFixedPathSet::clear()
{
    _const_geom = 0;
    _drawable = 0;
    while (!_paths.isEmpty())
        delete _paths.takeFirst();
}

void NPRFixedPathSet::assignStaticIDs()
{
    int  num_paths = size();
    for (int i = 0; i < num_paths; i++)
        _paths[i]->attributes().static_id = i+1;
}

void NPRFixedPathSet::orientPaths()
{
    int num_paths = size();
    for (int i = 0; i < num_paths; i++)
        _paths[i]->orientToMajorAxis();
}

void NPRFixedPathSet::assignStaticLengths()
{
    int num_paths = size();
    for (int i = 0; i < num_paths; i++)
	{
		_paths[i]->computeLength();
	}
}

bool compare(const NPRFixedPath *lhs, const NPRFixedPath *rhs)
{
    if (lhs->attributes().static_length != rhs->attributes().static_length)
        return (lhs->attributes().static_length  < 
                rhs->attributes().static_length);
    else
        return (lhs->attributes().static_id < rhs->attributes().static_id );
}


void NPRFixedPathSet::sort()
{
    std::sort(_paths.begin(), _paths.end(), &compare);
}

NPRFixedPath* NPRFixedPathSet::newPath()
{
    NPRFixedPath* newpath = new NPRFixedPath(_const_geom);
    _paths.push_back(newpath);
    return newpath;
}

NPRFixedPath* NPRFixedPathSet::newPath( const NPRFixedPathAttr& attr )
{
    NPRFixedPath* newpath = new NPRFixedPath(_const_geom, attr);
    _paths.push_back(newpath);
    return newpath;
}

void NPRFixedPathSet::addPath( NPRFixedPath* path )
{
    assert( _const_geom == path->geometry() );

    _paths.push_back(path);
}

void NPRFixedPathSet::deletePath( int which )
{
    assert( which >= 0 && which < _paths.size() );

    delete _paths.takeAt(which);
}

// Helpers for stitchLinesIntoPaths

bool angleCloseEnough( const vec& v1, const vec& v2 )
{
    float len1 = len(v1);
    float len2 = len(v2);
    float dotp = v1 DOT v2;
    dotp /= len1 * len2;
    if (dotp > 0.9)
        return true;

    return false;
}

typedef QVector< QList< int > > ConnectivityMatrix;

void makeConnectivityMatrix( const NPRGeometry* geom, const NPRPrimitive* lines, 
                             ConnectivityMatrix& connect )
{
    connect.resize(geom->numVertices());

    for (int j = 0; j < lines->size(); j += 2)
    {
        int a = lines->at(j);
        int b = lines->at(j+1);
        connect[a].append(b);
        connect[b].append(a);
    }
}

void removeEdge( int column, int row, ConnectivityMatrix& connect )
{
    int other_col = connect[column][row];

    // either remove this column's entry or copy the last entry into its spot
    int col_size = connect[column].size();
    if (col_size > 1 && row < col_size-1)
        connect[column][row] = connect[column].takeLast();
    else
        connect[column].removeLast();

    // find the row in the other column
    int other_col_size = connect[other_col].size();
    int other_row = -1;
    for (int i = 0; i < other_col_size; i++)
    {
        if (connect[other_col][i] == column)
        {
            other_row = i;
            break;
        }
    }

    // remove the other row the same way
    if (other_col_size > 1 && other_row < other_col_size-1)
        connect[other_col][other_row] = connect[other_col].takeLast();
    else
        connect[other_col].removeLast();
}

inline const vec& getVert( const NPRGeometry* geom, int v )
{
    return geom->data(GQ_VERTEX)->asVec3()[v];
}

void followEdge( int last_index, const vec& last_segment, ConnectivityMatrix& connect, 
                 const NPRGeometry* geom, NPRFixedPath& path)
{
    const vec& last_v = getVert(geom, last_index);

    int next_row = -1;
    int next_index = -1;
    vec this_segment;
    for (int i = 0; i < connect[last_index].size(); i++)
    {
        next_index = connect[last_index][i];
        this_segment = getVert(geom, next_index) - last_v;

        if (angleCloseEnough( last_segment, this_segment ))
        {
            next_row = i;
            break;
        }
    }

    if (next_row >= 0)
    {
        removeEdge(last_index, next_row, connect);
        path << next_index;
        followEdge(next_index, this_segment, connect, geom, path);
    }
}
 
void reversePath( NPRFixedPath& path )
{
    int len = path.size();
    int middle = floor(len * 0.5f);
    for (int i = 0; i < middle; i++)
    {
        int swap = path[i];
        path[i] = path[len-i-1];
        path[len-i-1] = swap;
    }
}

void NPRFixedPathSet::stitchLinesIntoPaths( const NPRPrimitive* lines, const NPRFixedPathAttr& attr )
{
    // create a sparse connectivity matrix for the edges
    ConnectivityMatrix connect;
    makeConnectivityMatrix( _const_geom, lines, connect );

    // greedily follow edges, create paths, and remove edges from 
    // connectivity matrix
    for (int start = 0; start < connect.size(); start++)
    {
        while (connect[start].size() > 0)
        {
            // add the first segment
            NPRFixedPath* newpath = new NPRFixedPath(_const_geom, _drawable, attr);
            int e1 = start;
            int e2 = connect[start].first();
            (*newpath) << e1 << e2;

            // set up for following
            vec segment = getVert(_const_geom, e2) - getVert(_const_geom, e1);
            removeEdge( start, 0, connect );

            // follow edges until we can't find one that has a close enough angle 
            // append new indices to end of path
            followEdge( e2, segment, connect, _const_geom, *newpath);

            // now reverse the path, and follow in the opposite direction
            // (appending to the end of the reversed path)
            reversePath(*newpath);
            segment = segment * -1.0f;
            followEdge( e1, segment, connect, _const_geom, *newpath);

            addPath(newpath);
        }
    }
}
