// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef RUNTIMESELECTOR_H
#define RUNTIMESELECTOR_H

// does not currently need to add anything to VersionAndLocationInfo
#include "versionandlocationinfo.h"
typedef VersionAndLocationInfo RuntimeInfo;

class RuntimeSelector
{
protected:
    VersionInfo m_RequestedVersion; // requested version
    RuntimeInfo m_Best;       // the best option so far
    bool m_bHasSomething;             // has any data
public:
    //constructor
    RuntimeSelector();

    // whether the given info compatible with the request
    bool IsAcceptable(const RuntimeInfo& runtimeInfo)  const;

    // add runtime info
    HRESULT Add(const RuntimeInfo& runtimeInfo);

    // set the version requested
    void SetRequestedVersion(const VersionInfo& version);

    // get the best found
    RuntimeInfo GetBest();

    // has any useful data
    bool HasUsefulRuntimeInfo();
    
    // is 1st better than 2nd
    static bool IsBetter(const RuntimeInfo& ri1, const RuntimeInfo& ri2);
};

#include "runtimeselector.inl"

#endif // RUNTIMESELECTOR_H

