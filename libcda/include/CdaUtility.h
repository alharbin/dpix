/*****************************************************************************\

CdaUtility.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Utility functions.

libcda is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/


#ifndef _CDA_UTILITY_H_
#define _CDA_UTILITY_H_

#include <QString>

inline QString trimHash( const QString& str )
{
    if (str[0] == '#')
        return str.right(str.size() - 1);
    else
        return str;
};

#endif