// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef VERSIONANDLOCATIONINFO_H
#define VERSIONANDLOCATIONINFO_H

#include "versioninfo.h"
#include "locationinfo.h"

struct VersionAndLocationInfo
{
protected:
    VersionInfo m_Version;
    LocationInfo m_Location;
public:
    VersionAndLocationInfo();
    VersionAndLocationInfo(const VersionInfo& version, const LocationInfo location);
    VersionInfo Version()  const;
    LocationInfo Location() const ;
};


#include "versionandlocationinfo.inl"

#endif // VERSIONANDLOCATIONINFO_H

