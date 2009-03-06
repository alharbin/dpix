// CdaGeometry.cc

#include <QTextStream>
#include <QStringList>
#include <QDebug>
#include "CdaGeometry.h"
#include "CdaUtility.h"
#include <assert.h>
// the data source node

CdaSource::CdaSource(const QDomElement& element)
{
    _id = element.attribute("id");
    
    QDomElement fa_e = element.firstChildElement("float_array");
    assert(!fa_e.isNull());

    QStringList fa_list = fa_e.text().split(QRegExp("\\s+"), QString::SkipEmptyParts);

    int float_count = fa_e.attribute("count").toInt();
    assert(float_count == fa_list.size());
    
    QDomElement tech_e = element.firstChildElement("technique_common");
    assert(!tech_e.isNull());
    QDomElement acc_e = tech_e.firstChildElement("accessor");
    assert(!acc_e.isNull());
    
    int stride = acc_e.attribute("stride").toInt();
#ifndef QT_NO_DEBUG
    int data_count = acc_e.attribute("count").toInt();
    assert(float_count == stride * data_count);
#endif
    
    setWidth(stride);
    
    for (int i = 0; i < float_count; i++) {
        push_back(fa_list[i].toFloat());
    }
}

// the actual geometry node

void CdaGeometry::clear()
{
    _id = QString("");
    
    for (int i = 0; i < _sources.size(); i++) {
        delete _sources[i];
    }
    _sources.clear();

    for (int i = 0; i < CDA_NUM_SEMANTICS; i++) {
        _data[i] = NULL;
    }
    
    for (int i = 0; i < CDA_NUM_PRIMITIVES; i++) {
        _primitives[i].clear();
    }
}

CdaSource* CdaGeometry::findSource(const QString &id)
{
    for (int i = 0; i < _sources.size(); i++) {
        if (id == _sources[i]->_id)
            return _sources[i];
    }
    return NULL;
}

CdaGeometry::CdaGeometry(const QDomElement& element)
{
    for (int i = 0; i < CDA_NUM_SEMANTICS; i++) {
        _data[i] = NULL;
    }
    
    // The collada mesh storage format is incredibly overengineered...
    _id = element.attribute("id");
    
    QDomElement mesh = element.firstChildElement("mesh");
    assert(!mesh.isNull());
    
    // first find and read all the data sources
    QDomElement source_e = mesh.firstChildElement("source");
    while (!source_e.isNull())
    {
        CdaSource *source = new CdaSource(source_e);
        assert(!findSource(source->_id));
        _sources << source;
        source_e = source_e.nextSiblingElement("source");
    }
    
    // find the useless "vertices" node
    QDomElement vertices_e = mesh.firstChildElement("vertices");
    assert(!vertices_e.isNull());

    QString vertices_id = vertices_e.attribute("id");
    assert(!vertices_id.isNull());

    // find the vertex position source from the vertices node
    QDomElement input_e = vertices_e.firstChildElement("input");
    if (!input_e.isNull() && input_e.attribute("semantic") == "POSITION") {
        _data[CDA_VERTEX] = findSource(trimHash(input_e.attribute("source")));
        assert(_data[CDA_VERTEX]);
        _data[CDA_VERTEX]->_id = vertices_id;
    }
    
    // parse the primitive nodes
    for (int i = 0; i < CDA_NUM_PRIMITIVES; i++) {
        const QString &type = _primitive_definitions[i]._type;
        QDomElement element = mesh.firstChildElement(type);
        while(!element.isNull()) {
            parsePrimitive(element, (CdaPrimitiveType)i);
            element = element.nextSiblingElement(type);
        }
    }
    
    QDomElement extra_e = mesh.firstChildElement("extra");
    if (!extra_e.isNull()) {
        QDomElement dpix = extra_e.firstChildElement("technique");
        if (!dpix.isNull() && dpix.attribute("profile") == "DPIX") {
            int i = CDA_CONTOURS;
            const QString &type = _primitive_definitions[i]._type;
            QDomElement element = dpix.firstChildElement(type);
            while(!element.isNull()) {
                parsePrimitive(element, (CdaPrimitiveType)i);
                element = element.nextSiblingElement(type);
            }
        }
    }
}

const QString CdaGeometry::_source_semantics[] = { QString("VERTEX"), 
                                                   QString("NORMAL"), 
                                                   QString("TEXCOORD") };
const CdaPrimitiveDefinition CdaGeometry::_primitive_definitions[] = {
    // _type, _num_vertices, _valid_semantics
    { QString("triangles"), 3, 1 << CDA_VERTEX | 1 << CDA_NORMAL | 1 << CDA_TEXCOORD },
    { QString("lines"), 2, 1 << CDA_VERTEX | 1 << CDA_TEXCOORD },
    { QString("contours"), 2, 1 << CDA_VERTEX | 1 << CDA_NORMAL },
    { QString("linestrips"), -1, 1 << CDA_VERTEX | 1 << CDA_TEXCOORD },
};


void CdaGeometry::parsePrimitive(QDomElement& element, CdaPrimitiveType primitiveType)
{
    // Maps an offset to a semantic
    int offset_semantic_map[CDA_NUM_SEMANTICS];
    int num_offsets;
    
    // Get the first input node
    num_offsets = 0;
    QDomElement input_e = element.firstChildElement("input");
    
    // Repeat over all sibling input nodes
    while (!input_e.isNull()) {
        // Get the offset, which should be less than the number of semantics
        int offset = input_e.attribute("offset").toInt();
        assert(offset < CDA_NUM_SEMANTICS);
        
        // Find the semantic num for the semantic string
        const QString &semantic_str = input_e.attribute("semantic");
        assert(!semantic_str.isEmpty());
        int semantic;
        for (semantic = 0; semantic < CDA_NUM_SEMANTICS; semantic++)
            if (semantic_str == _source_semantics[semantic])
                break;
        
        if (semantic == CDA_NUM_SEMANTICS)
            qFatal("Unrecognized input semantic %s", qPrintable(semantic_str));
        
        // Verify semantic is valid for this primitive
        assert(_primitive_definitions[primitiveType]._valid_semantics & (1 << semantic));
        
        // Store semantic num in the offset map
        offset_semantic_map[offset] = semantic;
        
        // Get the source ID string
        const QString &source_str = trimHash(input_e.attribute("source"));
        assert(!source_str.isEmpty());
        
        // Check to see if it has been found previously
        if (_data[semantic]) {
            // Verify we're using the same semantic
            assert(_data[semantic]->_id == source_str);
        }
        else {
            // Otherwise find the source
            _data[semantic] = findSource(source_str);
            assert(_data[semantic]);
        }
        
        // Update the number of offsets in the map
        num_offsets++;
        
        // Move to next sibling
        input_e = input_e.nextSiblingElement("input");
    }
    
    // If num_vertices > 0, primitive has only one p node and primitives are packed, otherwise, has multiple p nodes
    bool packed = _primitive_definitions[primitiveType]._num_vertices > 0;

    QDomElement p_e = element.firstChildElement("p");
    assert(!p_e.isNull());

    do {
        // Split string into lists
        QStringList p_list = p_e.text().split(QRegExp("\\s+"), QString::SkipEmptyParts);
        
        int size, count, stride;        
        if (packed) {
            // Size is total length of primitive, 
            // Count is number of packed primitives, 
            // Stride is number of vertices per packed primitive
            count = element.attribute("count").toInt();
            stride = _primitive_definitions[primitiveType]._num_vertices;
            size = count * stride;
        }
        else {
            // Size is length of primitive, count is one, stride is one
            size = p_list.size() / num_offsets;
            count = 1;
            stride = 1;
        }

        // Verify total number of entries is correct
        assert(p_list.size() == size * num_offsets);
        
        // Create a primitive
        const QString &material_symbol = element.attribute("material");        
        _primitives[primitiveType] << CdaPrimitive(size, count, stride, material_symbol);
        CdaPrimitive &primitive = _primitives[primitiveType].last();
        
        
        // For each input number
        for (int i = 0; i < p_list.size();) {
            // Append it to the appropriate entry in the indices
            for (int j = 0; j < num_offsets; j++, i++) {
                primitive.indices((CdaSourceSemantic)offset_semantic_map[j]) << p_list[i].toInt();
            }
        }
        
        // Get the next element and repeat if not a single element primitive
        p_e = element.nextSiblingElement("p");
    } while (!p_e.isNull() && !packed);
}
