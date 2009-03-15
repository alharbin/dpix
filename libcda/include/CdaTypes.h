/*****************************************************************************\

CdaTypes.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Some helpful type definitions for libcda.

libcda is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _CDA_TYPES_H_
#define _CDA_TYPES_H_

#include <Vec.h>
#include <XForm.h>

typedef unsigned int   uint32;
typedef unsigned char  uint8;

typedef Vec<4,float> CdaColor4;
typedef Vec<3,float> CdaColor3;

typedef Vec<2,int>	 CdaInt2;
typedef Vec<3,int>	 CdaInt3;

typedef vec			 CdaVec3;
typedef Vec<2,float> CdaVec2;
typedef xform		 CdaXform;

#endif

