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

StringLiteral const TraceComponent("CachedServiceTableEntry");

CachedServiceTableEntry::CachedServiceTableEntry(
    CachedServiceTableEntry const & other)
    : partition_(make_shared<ServiceTableEntry>(*other.partition_))
    , lastPrimaryChangeVersion_(other.lastPrimaryChangeVersion_)
    , lastPrimaryLocation_(other.lastPrimaryLocation_)
    , owner_(other.owner_)
{
}

CachedServiceTableEntry::CachedServiceTableEntry(
    ActivityId const & activityId,
    ServiceTableEntrySPtr const & partition,
    OwnerSPtr const & owner)
    : partition_(partition)
    , lastPrimaryChangeVersion_(0)
    , lastPrimaryLocation_()
    , owner_(owner)
{
    this->UpdateLastPrimaryLocation(activityId);
}

CachedServiceTableEntrySPtr CachedServiceTableEntry::Clone() const
{
    return shared_ptr<CachedServiceTableEntry>(new CachedServiceTableEntry(*this));
}

void CachedServiceTableEntry::UpdateServiceTableEntry(
    ActivityId const & activityId,
    ServiceTableEntrySPtr const & partition)
{
    partition_ = partition;

    this->UpdateLastPrimaryLocation(activityId);
}

void CachedServiceTableEntry::UpdateLastPrimaryLocation(ActivityId const & activityId)
{
    auto const & replicaSet = partition_->ServiceReplicaSet;

    if (replicaSet.IsStateful)
    {
        if (replicaSet.IsPrimaryLocationValid)
        {
            if (lastPrimaryLocation_ != replicaSet.PrimaryLocation)
            {
                lastPrimaryLocation_ = replicaSet.PrimaryLocation;
                lastPrimaryChangeVersion_ = partition_->Version;

                WriteNoise(
                    TraceComponent, 
                    "{0}: updated last primary location {1}: '{2}' vers={3}",
                    activityId,
                    partition_->ServiceName,
                    lastPrimaryLocation_,
                    lastPrimaryChangeVersion_);
            }
        }
        else
        {
            if (!lastPrimaryLocation_.empty())
            {
                lastPrimaryLocation_.clear();
                lastPrimaryChangeVersion_ = partition_->Version;

                WriteNoise(
                    TraceComponent, 
                    "{0}: cleared last primary location {1}: vers={2}",
                    activityId,
                    partition_->ServiceName,
                    lastPrimaryChangeVersion_);
            }
        }
    }
}

bool CachedServiceTableEntry::TryGetServiceTableEntry(
    ActivityId const & activityId,
    VersionRangeCollectionSPtr const & clientVersions,
    bool primaryChangeOnly,
    __out ServiceTableEntrySPtr & result)
{
    bool matched = true;

    if (clientVersions)
    {
        // Note that primary-change filtering here is only an optimization and
        // not guaranteed since the gateway cache is cleared when the node
        // restarts. If the client re-connects to a node that has restarted,
        // then it may still get notifications in which the primary has
        // not changed.
        //
        // Furthermore, replica sets are versioned by the FM as whole and not
        // individual endpoints. Suppose there are two versions of a replica
        // set:
        //
        // v0 = {P0} v1 = {P1 S1 S2} v2 = {P1 S1 S3}
        //
        // Suppose the broadcasts are received in v1, v2 order, but the client
        // gets the notification for v2 before v1. If the client reconnects
        // at this point, its version knowledge contains v2 but not v1
        // (since they are out of notification order). Since the primary endpoint 
        // changed at v1, the client will be delivered a notification for v1 to 
        // fill in the client's missing knowledge. 
        //
        // For both cases, the client must either perform additional duplicate detection 
        // on the endpoint itself or just accept the extra notification.
        //
        if (primaryChangeOnly && partition_->ServiceReplicaSet.IsStateful)
        {
            matched = !clientVersions->Contains(lastPrimaryChangeVersion_);

            if (!matched)
            {
                WriteInfo(
                    TraceComponent, 
                    "{0}: {1} ({2}) (primary.vers={3}) [stale]",
                    activityId,
                    partition_->ServiceName,
                    partition_->ConsistencyUnitId,
                    lastPrimaryChangeVersion_);
            }
        }
        else
        {
            matched = !clientVersions->Contains(partition_->Version);

            if (!matched)
            {
                WriteInfo(
                    TraceComponent, 
                    "{0}: {1} ({2}) (vers={3}) [stale]",
                    activityId,
                    partition_->ServiceName,
                    partition_->ConsistencyUnitId,
                    partition_->Version);
            }
        }
    }

    if (matched)
    {
        WriteNoise(
            TraceComponent, 
            "{0}: {1} ({2}) (primary.vers={3}) (vers={4}) [match]",
            activityId,
            partition_->ServiceName,
            partition_->ConsistencyUnitId,
            lastPrimaryChangeVersion_,
            partition_->Version);

        result = partition_;
    }

    return matched;
}
