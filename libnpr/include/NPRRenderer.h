/*****************************************************************************\

NPRRenderer.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

An abstract interface for the npr renderer.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/


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

