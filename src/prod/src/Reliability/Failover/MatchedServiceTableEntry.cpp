// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Transport;
using namespace ServiceModel;

StringLiteral const TraceComponent("MatchedServiceTableEntry");

MatchedServiceTableEntry::MatchedServiceTableEntry(
    CachedServiceTableEntrySPtr const & cachedEntry,
    bool checkPrimaryChangeOnly,
    bool matchedPrimaryOnly)
    : cachedEntry_(cachedEntry->Clone())
    , checkPrimaryChangeOnly_(checkPrimaryChangeOnly)
    , matchedPrimaryOnly_(matchedPrimaryOnly)
{
}

shared_ptr<MatchedServiceTableEntry> MatchedServiceTableEntry::CreateOnUpdateMatch(
    CachedServiceTableEntrySPtr const & cachedEntry,
    bool checkPrimaryChangeOnly)
{
    auto shouldCheck = cachedEntry->Partition->ServiceReplicaSet.IsStateful ? checkPrimaryChangeOnly : false;

    return shared_ptr<MatchedServiceTableEntry>(new MatchedServiceTableEntry(
        cachedEntry,
        //
        // Flags should simply be the same if the match results from
        // processing a broadcast
        //
        shouldCheck,
        shouldCheck));
}

shared_ptr<MatchedServiceTableEntry> MatchedServiceTableEntry::CreateOnConnectionMatch(
    CachedServiceTableEntrySPtr const & cachedEntry,
    bool matchedPrimaryOnly)
{
    return shared_ptr<MatchedServiceTableEntry>(new MatchedServiceTableEntry(
        cachedEntry,
        //
        // We've already performed the primary-only filtering during
        // connection matching so there's no need to check again later 
        // before sending (see ServiceNotificationSender)
        //
        false,
        //
        // Inform client that this notification resulted from
        // a primary-only match
        //
        cachedEntry->Partition->ServiceReplicaSet.IsStateful ? matchedPrimaryOnly : false));
}

void MatchedServiceTableEntry::UpdateCheckPrimaryChangeOnly(
    bool checkPrimaryChangeOnly)
{
    // The primary-only filter is more restrictive, so
    // we only allow making the match result less restrictive
    // if multiple filters are matched.
    //
    if (checkPrimaryChangeOnly_ && !checkPrimaryChangeOnly)
    {
        checkPrimaryChangeOnly_ = checkPrimaryChangeOnly;
        matchedPrimaryOnly_ = checkPrimaryChangeOnly_;
    }
}

void MatchedServiceTableEntry::UpdateMatchedPrimaryOnly(
    bool matchedPrimaryOnly)
{
    if (matchedPrimaryOnly_ && !matchedPrimaryOnly)
    {
        matchedPrimaryOnly_ = matchedPrimaryOnly;
    }
}

bool MatchedServiceTableEntry::TryGetServiceTableEntry(
    ActivityId const & activityId,
    VersionRangeCollectionSPtr const & clientVersions,
    __out ServiceTableEntryNotificationSPtr & result)
{
    if (checkPrimaryChangeOnly_)
    {
        ServiceTableEntrySPtr ste;
        bool matched = cachedEntry_->TryGetServiceTableEntry(
            activityId,
            clientVersions,
            checkPrimaryChangeOnly_,
            ste);

        if (matched)
        {
            result = make_shared<ServiceTableEntryNotification>(
                ste, 
                matchedPrimaryOnly_);
        }

        return matched;
    }
    else
    {
        result = make_shared<ServiceTableEntryNotification>(
            cachedEntry_->Partition, 
            matchedPrimaryOnly_);

        return true;
    }
}
