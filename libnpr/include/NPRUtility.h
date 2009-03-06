/*****************************************************************************\
 *                                                                           *
 *  filename : NPRUtility.h                                                  *
 *  authors  : R. Keith Morley                                               *
 *                                                                           *
 *  Common utility functions used by multiple modules within NPRlib          *
 *                                                                           *
\*****************************************************************************/

#ifndef _NPR_UTIILITY_H_
#define _NPR_UTIILITY_H_

#include <Vec.h>
#include <XForm.h>
#include <stdlib.h>
#include <stdarg.h>

#include "GQInclude.h"

// returns a random double in [0, 1)
double nprRand();
// seeds nprRand()
void nprSeedRand(unsigned int x);

// returns the 2D distance between two pixels
float nprPixelDist(const vec& v0, const vec& v1);
// returns the squared 2D distance between two pixels
float nprPixelDist2(const vec& v0, const vec& v1);

// loads the viewport, modelview, and projection matrices
void nprGetGLMatrices( GLint viewport[4], GLdouble mv[16], GLdouble proj[16]);

// calls glMultMatrixf with the input xform
void nprMultMatrix( const xform& xf );

inline
double nprRand()
{
#ifdef HAVE_DRAND48
    return drand48();
#else
    return (double)rand() / (double)RAND_MAX;
#endif
}

inline
void nprSeedRand(unsigned int seed)
{
#ifdef HAVE_DRAND48
    srand48(seed);
#else
    srand(seed);
#endif
}

inline 
float nprPixelDist(const vec& v0, const vec& v1)
{
    float dx = v1[0]-v0[0];
    float dy = v1[1]-v0[1];
    return  sqrtf(dx*dx + dy*dy) ;
}

inline 
float nprPixelDist2(const vec& v0, const vec& v1)
{
    float dx = v1[0]-v0[0];
    float dy = v1[1]-v0[1];
    return  dx*dx + dy*dy ;
}

inline 
void nprGetGLMatrices( GLint viewport[4], GLdouble mv[16], GLdouble proj[16])
{
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, mv);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
}

inline void nprMultMatrix( const xform& xf )
{
    glMultMatrixd( (const double *)xf );
}

inline void nprProject(const vec& world, vec& screen, 
                        GLint viewport[4], GLdouble mvmatrix[16], GLdouble projmatrix[16] )
{
    double sx, sy, sz;
    gluProject( world[0], world[1], world[2],  
            mvmatrix, projmatrix, viewport,
            &sx, &sy, &sz);

    screen[0] = (float)sx;
    screen[1] = (float)sy;
    screen[2] = (float)sz;
}


inline void nprUnProject(const vec& screen, vec& world, 
                        GLint viewport[4], GLdouble mvmatrix[16], GLdouble projmatrix[16] )
{
    double sx, sy, sz;
    gluUnProject( screen[0], screen[1], screen[2],  
            mvmatrix, projmatrix, viewport,
            &sx, &sy, &sz);

    world[0] = (float)sx;
    world[1] = (float)sy;
    world[2] = (float)sz;
}

inline float nprClamp( float f, float min, float max )
{
    if (f > max)
        return max;
    else if (f < min)
        return min;
    else
        return f;
}

// dan bernstein's string hash from comp.lang.c (k = 33)
inline uint32 nprStrHash( const char* string )
{
    uint32 hash = 5381;
    int c;
    while ((c = *string++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

// robert jenkins' 7 shift int hash
inline uint32 nprIntHash( uint32 a )
{
    a -= (a<<6);
    a ^= (a>>17);
    a -= (a<<9);
    a ^= (a<<4);
    a -= (a<<3);
    a ^= (a<<10);
    a ^= (a>>15);
    return a;
}

#endif // _NPR_UTIILITY_H_
