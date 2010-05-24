/*****************************************************************************\

NPRGeometry.cc
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/
#include "NPRGeometry.h"
#include "CdaGeometry.h"
#include "GQInclude.h"
#include <assert.h>
#include "bsphere.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

const NPRGeometry*  NPRGeometry::_currently_bound_geom = 0;

NPRGeometry::NPRGeometry()
{
    clear();
}

NPRGeometry::NPRGeometry( const CdaGeometry* cda_geometry )
{
    clear();
    convertFromCdaGeometry( this, cda_geometry );
}


void NPRGeometry::clear()
{
    while (!_sources.isEmpty())
        delete _sources.takeFirst();

    for (int i = 0; i < NPR_NUM_PRIMITIVES; i++)
    {
        while (!_primitives[i].isEmpty())
            delete _primitives[i].takeFirst();
    }

    for (int i = 0; i < GQ_NUM_VERTEX_BUFFER_TYPES; i++)
        _semantic_list[i] = 0;

    _source_hash.clear();

    _bsphere_center = vec(0,0,0);
    _bsphere_radius = -1;

    _vertex_buffer_set.clear();
}

void NPRGeometry::reset()
{
    for (int i = 0; i < _sources.size(); i++)
        _sources[i]->clear();

    for (int i = 0; i < NPR_NUM_PRIMITIVES; i++)
    {
        while (!_primitives[i].isEmpty())
            delete _primitives[i].takeFirst();
    }
    _vertex_buffer_set.deleteVBOs();
}

void NPRGeometry::addData( const QString& name, int width )
{
    int current_length = 0;
    if (_sources.size() > 0)
        current_length = _sources[0]->length();

    NPRDataSource* newsource = new NPRDataSource(width);
    newsource->resize(width*current_length);
    _sources.push_back(newsource);
    _source_hash[name] = newsource;
    for (int i = 0; i < GQ_NUM_VERTEX_BUFFER_TYPES; i++)
    {
        if (name == GQVertexBufferNames[i])
        {
            _semantic_list[i] = newsource;
            break;
        }
    }

    _vertex_buffer_set.add(name, newsource->width(), *newsource);
}

void NPRGeometry::addData( const QString& name, NPRDataSource* source )
{
    int current_length = 0;
    if (_sources.size() > 0)
        current_length = _sources[0]->length();

    assert(_sources.size() == 0 || source->length() == current_length);

    _sources.push_back(source);
    _source_hash[name] = source;
    for (int i = 0; i < GQ_NUM_VERTEX_BUFFER_TYPES; i++)
    {
        if (name == GQVertexBufferNames[i])
        {
            _semantic_list[i] = source;
            break;
        }
    }
    _vertex_buffer_set.add(name, source->width(), *source);
}

void NPRGeometry::addData( GQVertexBufferType semantic, int width )
{
    addData( GQVertexBufferNames[semantic], width );
}

void NPRGeometry::addData( GQVertexBufferType semantic, NPRDataSource* source )
{
    addData( GQVertexBufferNames[semantic], source );
}

void NPRGeometry::bind() const 
{ 
    assert(_currently_bound_geom == 0);
    _vertex_buffer_set.bind();
    _currently_bound_geom = this;
}

void NPRGeometry::unbind() const 
{ 
    assert(_currently_bound_geom == this);
    _vertex_buffer_set.unbind();
    _currently_bound_geom = 0;
}


void NPRGeometry::copyToVBO()
{
    _vertex_buffer_set.copyToVBOs();
}

void NPRGeometry::deleteVBO()
{
    _vertex_buffer_set.deleteVBOs();
}

void NPRGeometry::convertFromCdaGeometry( NPRGeometry* geom, const CdaGeometry* cda_geometry )
{
    int cda2ln_prims[CDA_NUM_PRIMITIVES] = { NPR_TRIANGLES, NPR_LINES, NPR_PROFILES, NPR_LINE_STRIP };

    NPRDataSource* verts = new NPRDataSource(3);
    NPRDataSource* normals = new NPRDataSource(3);
    int vertex_count = 0;

    geom->clear();

    // Use a hash table to store a unique single-index-storage vertex for each 
    // distinct combination of indices in the CdaGeometry.
    QHash< int, int > vertex_indices;
    QHash< QString, int > vertex_normal_indices;
    bool any_prims_have_normals = false;

    // Process primitives that have normals first.
    for (int primtype = 0; primtype < CDA_NUM_PRIMITIVES; primtype++) {
        const CdaPrimitiveList& prims = cda_geometry->primList((CdaPrimitiveType)primtype);
        for (int i = 0; i < prims.size(); i++) {
            const CdaPrimitive& cdaprim = prims[i];
            bool has_normals = cdaprim.indices(CDA_NORMAL).size() > 0;
            
            if (has_normals) {
                any_prims_have_normals = true;
                NPRPrimitiveType type = (NPRPrimitiveType)(cda2ln_prims[primtype]);
                NPRPrimitive* newprim = new NPRPrimitive( type );
                newprim->setMaterialSymbol( cdaprim.material_symbol() );

                for (int j = 0; j < cdaprim.size(); j++) {
                    int vertind = cdaprim.indices(CDA_VERTEX)[j];
                    int normalind = cdaprim.indices(CDA_NORMAL)[j];

                    QString vert_norm_key = QString::number(vertind) + "_" + QString::number(normalind);

                    int index;
                    if (!vertex_normal_indices.contains(vert_norm_key)) {
                        // Add a new vertex / normal pair.
                        index = vertex_count;
                        vertex_normal_indices[vert_norm_key] = index;
                        
                        const vec& v = cda_geometry->data(CDA_VERTEX).asVec3()[vertind];
                        verts->appendVec3(v);
                        const vec& n = cda_geometry->data(CDA_NORMAL).asVec3()[normalind];
                        normals->appendVec3(n);

                        vertex_count++;
                        
                        // Record the vertex index by itself so that
                        // primitives without normals can use it too.
                        if (!vertex_indices.contains(vertind)) {
                            vertex_indices[vertind] = index;
                        }
                    }
                    else {
                        // Reuse the existing pair.
                        index = vertex_normal_indices.value(vert_norm_key);
                    }
                    newprim->push_back(index);
                }
                geom->_primitives[type].push_back(newprim);
            }
        }
    }

    // Process the primitives that do not have normals.
    for (int primtype = 0; primtype < CDA_NUM_PRIMITIVES; primtype++) {
        const CdaPrimitiveList& prims = cda_geometry->primList((CdaPrimitiveType)primtype);
        for (int i = 0; i < prims.size(); i++) {
            const CdaPrimitive& cdaprim = prims[i];
            bool has_normals = cdaprim.indices(CDA_NORMAL).size() > 0;
            
            if (!has_normals) {
                NPRPrimitiveType type = (NPRPrimitiveType)(cda2ln_prims[primtype]);
                NPRPrimitive* newprim = new NPRPrimitive( type );
                
                for (int j = 0; j < cdaprim.size(); j++) {
                    int vertind = cdaprim.indices(CDA_VERTEX)[j];
                    
                    int index;
                    if (!vertex_indices.contains(vertind)) {
                        // Add a new vertex.
                        index = vertex_count;
                        vertex_indices[vertind] = index;

                        const vec& v = cda_geometry->data(CDA_VERTEX).asVec3()[vertind];
                        verts->appendVec3(v);
                        
                        if (any_prims_have_normals) {
                            // Fill in a dummy normal value to maintain the
                            // correct array lengths. 
                            normals->appendVec3(vec(0.0f, 0.0f, 0.0f));
                        }
                        
                        vertex_count++;
                    }
                    else {
                        // Reuse the existing vertex.
                        index = vertex_indices.value(vertind);
                    }
                    newprim->push_back(index);
                }
                geom->_primitives[type].push_back(newprim);                
            }
        }
    }
    geom->addData(GQ_VERTEX, verts);

    assert( normals->length() == 0 || normals->length() == verts->length() );
    
    if (any_prims_have_normals)
    {
        geom->addData(GQ_NORMAL, normals);
    }

    geom->findBoundingSphere();
}

void NPRGeometry::findBoundingSphere()
{
    const NPRDataSource* verts = _semantic_list[GQ_VERTEX];
    if (verts && verts->size() > 0)
    {
        // Use the trimesh library's miniball code to 
        // compute the exact bounding sphere.
        Miniball<3,float> mb;
        for (int i = 0; i < verts->length(); i++)
            mb.check_in(verts->asVec3()[i]);
        mb.build();
        _bsphere_center = mb.center();
        _bsphere_radius = sqrt(mb.squared_radius());
    }
    else
    {
        _bsphere_center = vec(0,0,0);
        _bsphere_radius = -1;
    }
}

void NPRGeometry::debugDump(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);

    QHashIterator<QString, NPRDataSource*> iter(_source_hash);
    while (iter.hasNext()) {
        iter.next();
        out << iter.key() << "\n";
        NPRDataSource* source = iter.value();
        for (int i = 0; i < source->length(); i++)
        {
            float* entry = source->entry(i);
            for (int j = 0; j < source->width(); j++)
            {
                out << entry[j] << " ";
            }
            out << "\n";
        }
        out << "\n";
    } 

    for (int i = 0; i < NPR_NUM_PRIMITIVES; i++)
    {
        out << "prim type " << i << "\n";
        for (int j = 0; j < _primitives[i].size(); j++)
        {
            NPRPrimitive* prim = _primitives[i][j];
            out << "prim " << j << "\n";
            for (int k = 0; k < prim->size(); k++)
            {
                out << (*prim)[k] << " ";
                if ( ((NPRPrimitiveType)i == NPR_TRIANGLES && k % 3 == 2) ||
                     ((NPRPrimitiveType)i == NPR_LINES && k % 2 == 1) )
                     out << "\n";
            }
            out << "\n";
        }
        out << "\n";
    }

    file.close();
}

