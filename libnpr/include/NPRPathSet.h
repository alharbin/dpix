#ifndef NPR_PATH_SET_H_
#define NPR_PATH_SET_H_

#include <QVector>
#include <QHash>
#include <QList>
#include "NPRFixedPathSet.h"
#include "NPRUtility.h"

class NPRPathAttr
{
    public:
        NPRPathAttr() : type(NPR_CONTOUR), static_id(0), model_id(0), source_path(0) {}

        void copyFrom( const NPRFixedPath* source );

        NPRLineType    type;
        int static_id;
        int model_id;
        const NPRFixedPath* source_path;
};

class NPRVertAttr
{
    public:
        NPRVertAttr() : visibility(1), priority(1), source1(-1), source2(-1), interp_t(-1) {}
        float visibility;
        float priority;
        float focus;
        int   source1, source2;
        float interp_t;
};

class NPRPath 
{
    public:
        NPRPathAttr& attributes() { return _attributes; }
        const NPRPathAttr& attributes() const { return _attributes; }

        void clear();
        void reset(); // Removes the vertices, but leaves the attribute structure intact.

        inline int size() const { return _verts.size(); }

        inline vec& vert( int which )               { return _verts[which]; }
        inline vec& lastVert()                      { return _verts.last(); }
        inline NPRVertAttr& vertAttr( int which )   { return _attrs[which]; }
        inline NPRVertAttr& lastVertAttr()          { return _attrs.last(); }

        inline const vec& vert( int which )             const { return _verts[which]; }
        inline const vec& lastVert()                    const { return _verts.last(); }
        inline const NPRVertAttr& vertAttr( int which ) const { return _attrs[which]; }
        inline const NPRVertAttr& lastVertAttr()        const { return _attrs.last(); }

        void addVert( const vec& v );
        void addVert( const vec& v, float visibility );
        void addVert( const NPRPath* source, int which );
        void addInterpolatedVert( const NPRPath* source, int v1, int v2, float t );

        float length() const;

        void transform( const xform& xf );

        static NPRPath* newPath();
        static NPRPath* newPath(const NPRPathAttr& attr);
        static NPRPath* newPath(const NPRFixedPath* source);

        static void     deletePath(NPRPath* path);

    protected:
        NPRPath() : _ref_count(0) {}
        ~NPRPath() {}
        void copyFromFixedPath( const NPRFixedPath* source );

        void addRef();
        void decRef();

    protected:
        int                     _ref_count;
        NPRPathAttr             _attributes;
        QVector<vec>            _verts;
        QVector<NPRVertAttr>    _attrs;

        static QList<NPRPath*>  _free_pool;

    friend class NPRPathSet;
};

class NPRPathSet
{
    public:
        NPRPathSet() {}
        ~NPRPathSet() { clear(); }

        NPRPath* operator[]( int i ) { return _paths[i]; }
        const NPRPath* operator[]( int i ) const { return _paths[i]; }
    
        NPRPathSet& operator=(NPRPathSet& orig);

        void addPath( NPRPath* path );

        int size() const { return _paths.size(); }

        void sort();

        void clear(); 

    protected:
        QList<NPRPath*> _paths;
};


#endif


