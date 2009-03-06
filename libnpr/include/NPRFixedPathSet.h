#ifndef _NPR_FIXED_PATH_SET_H_
#define _NPR_FIXED_PATH_SET_H_

#include <QVector>
#include <QHash>
#include <QList>
#include "GQInclude.h"
#include "NPRGeometry.h"
#include "NPRDrawable.h"

// Enums

enum NPRLineType    { 
                    NPR_CONTOUR=0,       // Zero contour of n * v (Hertzmann 2000). 
                    NPR_SCONTOUR,        // Suggestive contour (DeCarlo 2003).
					NPR_RIDGE,			// Geometric ridge (Ohtake 2004).
					NPR_VALLEY,			// Geometric valley (Ohtake 2004).
                    NPR_APP_RIDGE,       // Apparent ridges (Judd 2007).
                    NPR_ISOPHOTE,        // Iso-contour of luminance.
                    NPR_BOUNDARY,        // Boundary of hole in the mesh (an edge with one bordering face).
                    NPR_CREASE,          // Discontinuity in the surface normal.
                    NPR_PROFILE,         // Mesh edge that lies on a silhouette boundary.
                    NPR_NUM_LINE_TYPES 
                    };

// NPRFixedPathAttr
                  
class NPRFixedPathAttr
{
  public:
    NPRFixedPathAttr() : type(NPR_CONTOUR), static_id(0), model_id(0), static_length(0) {};

    // 
    // the attributes 
    //
   
    NPRLineType      type;      // the line extraction type 
    uint32          static_id; // path id of the original path in the model
    uint32          model_id;  // id of the model the path came from
    float           static_length;
};

// NPRFixedPath

class NPRFixedPath : public NPRPrimitive
{
    public:
        NPRFixedPath( const NPRFixedPath& path);
        NPRFixedPath( const NPRGeometry* geom );
        NPRFixedPath( const NPRGeometry* geom, 
                      const NPRFixedPathAttr& attr );
        NPRFixedPath( const NPRGeometry* geom, 
                      const NPRDrawable* drawable,
                      const NPRFixedPathAttr& attr );

        const NPRGeometry* geometry() const { return _const_geom; }
        const NPRDrawable* drawable() const { return _drawable; }

        NPRFixedPathAttr& attributes() { return _attributes; }
        const NPRFixedPathAttr& attributes() const { return _attributes; }

        inline const vec3f& vert( int which ) const { return _const_geom->data(GQ_VERTEX)->asVec3()[value(which)]; }
        inline const vec3f& normal( int which ) const { return _const_geom->data(GQ_NORMAL)->asVec3()[value(which)]; }

        void appendPath( const NPRFixedPath* path );
        void reverse();
        void orientToMajorAxis();

        float computeLength();
        float cachedLength() const { return _attributes.static_length; }

        void debugDump() const;

    protected:
        const NPRGeometry* _const_geom;
        const NPRDrawable* _drawable;
        NPRFixedPathAttr   _attributes;
};

typedef QList<NPRFixedPath*> NPRFixedPathPointerList;

// NPRFixedPathSet

class NPRFixedPathSet
{
    public:
        NPRFixedPathSet( const NPRGeometry* geom );
        NPRFixedPathSet( const NPRDrawable* drawable );
        virtual ~NPRFixedPathSet() { clear(); }

        NPRFixedPath* operator[]( int i ) { return _paths[i]; }
        const NPRFixedPath* operator[]( int i ) const { return _paths[i]; }
        NPRFixedPath* at( int i ) { return _paths[i]; }
        const NPRFixedPath* at( int i ) const { return _paths[i]; }

        NPRFixedPath* newPath();
        NPRFixedPath* newPath( const NPRFixedPathAttr& attr );
        void    addPath( NPRFixedPath* path );
        void    deletePath( int which );

        const NPRDataSource* data( const QString& name ) const 
            { return _const_geom->data(name); }
      
        int size() const { return _paths.size(); }

        void clear(); 

        // iterate over the paths creating IDs for each 
        void assignStaticIDs();
        void assignStaticLengths();
        void orientPaths();

        void sort();

        const NPRGeometry* geometry() const { return _const_geom; }

    protected:
        void init();
        void stitchLinesIntoPaths( const NPRPrimitive* lines, const NPRFixedPathAttr& attr );

    protected:
        NPRFixedPathPointerList   _paths;
        const NPRDrawable*        _drawable;
        const NPRGeometry*        _const_geom;
};

#endif


