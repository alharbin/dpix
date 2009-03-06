// CdaUtility.h

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