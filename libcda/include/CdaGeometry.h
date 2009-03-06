// CdaGeometry.h
// Currently this class only supports the mesh datatype

#ifndef _CDA_GEOMETRY_H_
#define _CDA_GEOMETRY_H_

#include "CdaTypes.h"

#include <QString>
#include <QDomElement>
#include <QList>
#include <QVector>
#include <QHash>

class CdaSource : public QVector<float>
{
    public:
        CdaSource(const QDomElement& element);
        const int length() const { return size() / _width; }
        const int width() const { return _width; }
        void setWidth(int width) { _width = width; }
        const float* entry(int i) const { return &constData()[i * _width]; }
        const CdaVec3* asVec3() const { return (CdaVec3*)data(); }
        const CdaVec2* asVec2() const { return (CdaVec2*)data(); }
        const float* asFloat() const { return (float*)data(); }
        
    public:
        QString _id;
        
    private:
        int _width;
};

enum CdaSourceSemantic { CDA_VERTEX, CDA_NORMAL, CDA_TEXCOORD, CDA_NUM_SEMANTICS };
enum CdaPrimitiveType { CDA_TRIANGLES, CDA_LINES, CDA_CONTOURS, CDA_LINESTRIPS, CDA_NUM_PRIMITIVES };

struct CdaPrimitiveDefinition
{
    QString      _type;
    int          _num_vertices;
    unsigned int _valid_semantics;
};

class CdaPrimitive
{
    public:
        CdaPrimitive(int size, int count, int stride, const QString& material_symbol) :
        _size(size),
        _count(count),
        _stride(stride),
        _material_symbol(material_symbol)
        { }
                
        const int *index(CdaSourceSemantic semantic, int i) const { return &_indices[semantic][_stride * i]; }
        const int size() const { return _size; }
        const int count() const { return _count; }
        const QString& material_symbol() const { return _material_symbol; }
        QVector<int>& indices(CdaSourceSemantic semantic) { return _indices[semantic]; }
        const QVector<int>& indices(CdaSourceSemantic semantic) const { return _indices[semantic]; }
        
    private:
        QVector<int> _indices[CDA_NUM_SEMANTICS];
        int          _size;
        int          _count;
        int          _stride;
        QString      _material_symbol;
    };

typedef QList<CdaPrimitive> CdaPrimitiveList;

class CdaGeometry
    {
    public:
        CdaGeometry() { clear(); }
        CdaGeometry(const QDomElement& element);
        
        void clear();
        
    public:
        const QString& id() const { return _id; }
        QString _id;

        bool hasData(CdaSourceSemantic semantic) const { return _data[semantic] != NULL; }
        const CdaSource& data(CdaSourceSemantic semantic) const { return *_data[semantic]; }
        const CdaPrimitiveList& primList(CdaPrimitiveType primitiveType) const { return _primitives[primitiveType]; }

    protected:
        CdaSource* findSource(const QString &id);
        void parsePrimitive(QDomElement &element, CdaPrimitiveType primitive_type);

        static const QString _source_semantics[];
        static const CdaPrimitiveDefinition _primitive_definitions[];
        
        CdaPrimitiveList  _primitives[CDA_NUM_PRIMITIVES];
        QList<CdaSource*> _sources;
        CdaSource*        _data[CDA_NUM_SEMANTICS];
    };

#endif
