
/******************************************************************************\
 *                                                                            *
 *  filename : NPRRenderer.h                                                  *
 *  authors  : Forrester Cole												  *
 *																			  *
 *  NPR rendering engine designed for NPR line drawing. Based on the original *
 *  libnpr by R. Keith Morley and Adam Finkelstein.							  *
 *                                                                            *
\******************************************************************************/

#ifndef _NPR_RENDERER_H_
#define _NPR_RENDERER_H_

#include "NPRPathSet.h"

class NPRScene;
class NPRStyle;

class NPRRenderer
{
  public:
    virtual ~NPRRenderer() { }
    virtual void clear() = 0;

    virtual void drawScene( const NPRScene& scene ) = 0;
    virtual void drawSceneDepth( const NPRScene& scene ) = 0;

    virtual void resize( int width, int height ) = 0;
};

#endif // _NPR_RENDERER_H_

