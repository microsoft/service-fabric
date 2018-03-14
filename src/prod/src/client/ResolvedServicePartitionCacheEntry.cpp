// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Client;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;

StringLiteral const TraceComponent("ResolvedServicePartitionCacheEntry");

ResolvedServicePartitionCacheEntry::ResolvedServicePartitionCacheEntry(
    ResolvedServicePartitionSPtr const & rsp)
    : rsp_(rsp)
    , memberRsps_()
    , memberRspsLock_()
{
}

bool ResolvedServicePartitionCacheEntry::TryGetParsedRsp(
    NamingUri const & fullName,
    __out ResolvedServicePartitionSPtr & rspResult)
{
    // Not a service group. Return original RSP.
    if (fullName.Fragment.empty()) 
    { 
        rspResult = rsp_;
        return true; 
    }

    // Parse and cache service group member RSP from
    // original RSP
    //
    {
        AcquireReadLock lock(memberRspsLock_);

        auto it = memberRsps_.find(fullName.Fragment);
        if (it != memberRsps_.end())
        {
            rspResult = it->second;

            return true;
        }
    }

    {
        AcquireWriteLock lock(memberRspsLock_);

        ResolvedServicePartitionSPtr parsedRsp;
        if (ServiceGroup::Utility::ExtractMemberServiceLocation(
            rsp_, 
            fullName.ToString(),
            parsedRsp))
        {
            // Ignore return value. Either way the result is the same.
            //
            memberRsps_.insert(make_pair(
                fullName.Fragment,
                parsedRsp));

            rspResult = move(parsedRsp);

            return true;
        }
        else
        {
            this->WriteWarning(
                TraceComponent, 
                "Failed to parse service group member addresses: {1}",
                *rsp_);
        }
    }

    return false;
}
