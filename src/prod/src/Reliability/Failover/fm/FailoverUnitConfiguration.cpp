// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

size_t FailoverUnitConfiguration::get_UpCount() const
{
    size_t result = 0;

    for (ReplicaConstIterator it : replicas_)
    {
        if (it->IsUp)
        {
            ++result;
        }
    }

    return result;
}

size_t FailoverUnitConfiguration::get_OfflineCount() const
{
    size_t result = 0;

    for (ReplicaConstIterator it : replicas_)
    {
        if (it->IsOffline)
        {
            ++result;
        }
    }

    return result;
}

size_t FailoverUnitConfiguration::get_AvailableCount() const
{
    size_t result = 0;

    for (ReplicaConstIterator it : replicas_)
    {
        if (it->IsAvailable)
        {
            ++result;
        }
    }

    return result;
}

// Get the number of Replicas that are Down.
size_t FailoverUnitConfiguration::get_DownSecondaryCount() const
{
    size_t result = 0;

    for (ReplicaConstIterator it : replicas_)
    {
        if (it->IsCurrentConfigurationSecondary && !it->IsUp)
        {
            ++result;
        }
    }

    return result;
}

size_t FailoverUnitConfiguration::get_DroppedCount() const
{
    size_t result = 0;

    for (ReplicaConstIterator it : replicas_)
    {
        if (it->IsDropped)
        {
            ++result;
        }
    }

    return result;
}

size_t FailoverUnitConfiguration::get_DeletedCount() const
{
    size_t result = 0;

    for (ReplicaConstIterator it : replicas_)
    {
        if (it->IsDeleted)
        {
            ++result;
        }
    }

    return result;
}

size_t FailoverUnitConfiguration::get_StableReplicaCount() const
{
    size_t result = 0;

    for (ReplicaConstIterator it : replicas_)
    {
        if (it->IsStable)
        {
            ++result;
        }
    }

    return result;
}

size_t FailoverUnitConfiguration::get_StandByReplicaCount() const
{
	size_t result = 0;

	for (ReplicaConstIterator it : replicas_)
	{
		if (it->IsStandBy)
		{
			++result;
		}
	}

	return result;
}

FailoverUnitConfiguration::FailoverUnitConfiguration(bool isCurrentConfiguration, ReplicaConstIterator endIt)
    : isCurrentConfiguration_(isCurrentConfiguration), primary_(endIt), end_(endIt)
{
}

ReplicaConstIterator FailoverUnitConfiguration::get_Primary() const
{
    ASSERT_IF(IsEmpty != (primary_ == end_), "Inconsistent configuration status {0}-{1}", IsEmpty, primary_ == end_);
    return primary_;
}

void FailoverUnitConfiguration::AddReplica(ReplicaConstIterator replicaIt)
{
    ASSERT_IFNOT(InConfiguration(*replicaIt), "replica to add doesn't belong to this configuration");

    Reliability::ReplicaRole::Enum role = GetRole(*replicaIt);

    if (role == ReplicaRole::Primary)
    {
        ASSERT_IFNOT(primary_ == end_, "Primary replica to be added to configuration already exists: {0}", *replicaIt);
        primary_ = replicaIt;
    }

    for (ReplicaConstIterator it : replicas_)
    {
        ASSERT_IF(it == replicaIt, "Replica to be added to the configuration already exists");
    }
    replicas_.push_back(replicaIt);
}

void FailoverUnitConfiguration::RemoveReplica(ReplicaConstIterator replicaIt)
{
    ASSERT_IFNOT(InConfiguration(*replicaIt) && !IsEmpty, "replica to remove doesn't belong to this configuration");

    Reliability::ReplicaRole::Enum role = GetRole(*replicaIt);

    if (role == ReplicaRole::Primary)
    {
        ASSERT_IFNOT(primary_ == replicaIt, "primary replica to remove doesn't exist in this configuration");
        primary_ = end_;
    }

    for (auto it = replicas_.begin(); it != replicas_.end(); it++)
    {
        if (*it == replicaIt)
        {
            replicas_.erase(it);
            return;
        }
    }

    Assert::CodingError("Replica to remove doesn't exist in this configuration");
}

void FailoverUnitConfiguration::Clear()
{
    replicas_.clear();
}

Reliability::ReplicaRole::Enum FailoverUnitConfiguration::GetRole(Replica const& replica)
{
    return isCurrentConfiguration_ ? replica.CurrentConfigurationRole : replica.PreviousConfigurationRole;
}

bool FailoverUnitConfiguration::InConfiguration(Replica const& replica)
{
    return (isCurrentConfiguration_ && replica.IsInCurrentConfiguration) || (!isCurrentConfiguration_ && replica.IsInPreviousConfiguration);
}

void FailoverUnitConfiguration::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    if (primary_ != end_)
    {
        w.Write(primary_->FederationNodeId);
    }

    for (auto it = replicas_.begin(); it != replicas_.end(); it++)
    {
        if (*it != end_)
        {
            w.Write(",");
            w.Write((*it)->FederationNodeId);
        }
    }
}
