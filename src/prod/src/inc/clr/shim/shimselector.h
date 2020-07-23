// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef SHIMSELECTOR_H
#define SHIMSELECTOR_H

// does not currently need to add anything to VersionAndLocationInfo
#include "versionandlocationinfo.h"
typedef VersionAndLocationInfo ShimInfo;

class ShimSelector
{
protected:
    VersionInfo m_Baseline;   // base line to compare against
    ShimInfo m_Best;             // best found so far
    bool m_bHasSomething;  // has any data
public:

    //constructor
    ShimSelector();

    // whether the given info is better than the base line
    bool IsAcceptable(const ShimInfo& shimInfo)  const;

    // add shim info
    HRESULT Add(const ShimInfo& shimInfo);

    //set the base line
    void SetBaseline(const VersionInfo& version);

    // get the best found
    ShimInfo GetBest();

    // has any useful data
    bool HasUsefulShimInfo();

    // is 1st better than 2nd
    static bool IsBetter(const ShimInfo& ri1, const ShimInfo& ri2);
};


#include "shimselector.inl"

#endif // SHIMSELECTOR_H
