// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef VERSIONINFO_H
#define VERSIONINFO_H


struct VersionInfo
{
protected:
    unsigned short m_wMajor;
    unsigned short m_wMinor;
    unsigned short m_wBuild;
    unsigned short m_wRevision;
public:
    VersionInfo();
    VersionInfo( const unsigned short major,
                            const unsigned short minor,
                            const unsigned short build,
                            const unsigned short revision
                            );
    unsigned short Major() const;
    unsigned short Minor() const;
    unsigned short Build() const;
    unsigned short Revision() const;

    // 1 if "this" is bigger, -1 if "this' is smaller 
    int Compare(const VersionInfo& version) const;

    // on success returns count of  numbers read (<=4), otherwise -1
    static int Parse(   LPCTSTR szString,  
                                VersionInfo* result);
};

#include "versioninfo.inl"

#endif	
