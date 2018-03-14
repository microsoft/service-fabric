// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;


ReplicatorStatusQueryResultSPtr SecondaryReplicatorStatusQueryResult::Create(
    ReplicatorQueueStatusSPtr && replicationQueueStatus,
    Common::DateTime && lastReplicationOperationReceivedTime,
    bool isInBuild,
    ReplicatorQueueStatusSPtr && copyQueueStatus,
    Common::DateTime && lastCopyOperationReceivedTime,
    Common::DateTime && lastAcknowledgementSentTime)
{
    auto result = shared_ptr<SecondaryReplicatorStatusQueryResult>(
        new SecondaryReplicatorStatusQueryResult(
            move(replicationQueueStatus),
            move(lastReplicationOperationReceivedTime),
            isInBuild,
            move(copyQueueStatus),
            move(lastCopyOperationReceivedTime),
            move(lastAcknowledgementSentTime)));

    return move(result);
}
         
SecondaryReplicatorStatusQueryResult::SecondaryReplicatorStatusQueryResult()
    : ReplicatorStatusQueryResult(FABRIC_REPLICA_ROLE_UNKNOWN)
    , replicationQueueStatus_()
    , lastReplicationOperationReceivedTime_(DateTime::Zero)
    , isInBuild_(false)
    , copyQueueStatus_()
    , lastCopyOperationReceivedTime_(DateTime::Zero)
    , lastAcknowledgementSentTime_(DateTime::Zero)
{
}

SecondaryReplicatorStatusQueryResult::SecondaryReplicatorStatusQueryResult(
    ReplicatorQueueStatusSPtr && replicationQueueStatus,
    DateTime && lastReplicationOperationReceivedTime,
    bool isInBuild,
    ReplicatorQueueStatusSPtr && copyQueueStatus,
    DateTime && lastCopyOperationReceivedTime,
    DateTime && lastAcknowledgementSentTime)
    : ReplicatorStatusQueryResult(FABRIC_REPLICA_ROLE_UNKNOWN)
    , replicationQueueStatus_(move(replicationQueueStatus))
    , lastReplicationOperationReceivedTime_(move(lastReplicationOperationReceivedTime))
    , isInBuild_(isInBuild)
    , copyQueueStatus_(move(copyQueueStatus))
    , lastCopyOperationReceivedTime_(move(lastCopyOperationReceivedTime))
    , lastAcknowledgementSentTime_(move(lastAcknowledgementSentTime))
{
    if (isInBuild_)
    {
        this->role_ = FABRIC_REPLICA_ROLE_IDLE_SECONDARY;
    }
    else
    {
        this->role_ = FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY;
    }
}

SecondaryReplicatorStatusQueryResult::SecondaryReplicatorStatusQueryResult(SecondaryReplicatorStatusQueryResult && other)
    : ReplicatorStatusQueryResult(move(other))
    , replicationQueueStatus_(move(other.replicationQueueStatus_))
    , lastReplicationOperationReceivedTime_(move(other.lastReplicationOperationReceivedTime_))
    , isInBuild_(other.isInBuild_)
    , copyQueueStatus_(move(other.copyQueueStatus_))
    , lastCopyOperationReceivedTime_(move(other.lastCopyOperationReceivedTime_))
    , lastAcknowledgementSentTime_(move(other.lastAcknowledgementSentTime_))
{
}

SecondaryReplicatorStatusQueryResult const & SecondaryReplicatorStatusQueryResult::operator = (SecondaryReplicatorStatusQueryResult && other)
{
    if (this != &other)
    {
        this->replicationQueueStatus_ = move(other.replicationQueueStatus_);
        this->lastReplicationOperationReceivedTime_ = move(other.lastReplicationOperationReceivedTime_);
        this->isInBuild_ = other.isInBuild_;
        this->copyQueueStatus_ = move(other.copyQueueStatus_);
        this->lastCopyOperationReceivedTime_ = move(other.lastCopyOperationReceivedTime_);
        this->lastAcknowledgementSentTime_ = move(other.lastAcknowledgementSentTime_);

        ReplicatorStatusQueryResult::operator=(move(other));
    }

    return *this;
}

ErrorCode SecondaryReplicatorStatusQueryResult::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_REPLICATOR_STATUS_QUERY_RESULT & publicResult) const
{
    ErrorCode ec;

    if (this->replicationQueueStatus_ != nullptr &&
        this->copyQueueStatus_ != nullptr)
    {
        auto result1 = heap.AddItem<FABRIC_REPLICATOR_QUEUE_STATUS>();
        ec = this->replicationQueueStatus_->ToPublicApi(heap, *result1);

        if (ec.IsSuccess())
        {
            auto result2 = heap.AddItem<FABRIC_REPLICATOR_QUEUE_STATUS>();
            ec = this->copyQueueStatus_->ToPublicApi(heap, *result2);

            if (ec.IsSuccess())
            {
                auto secondaryResult = heap.AddItem<FABRIC_SECONDARY_REPLICATOR_STATUS_QUERY_RESULT>();
                secondaryResult->ReplicationQueueStatus = result1.GetRawPointer();
                secondaryResult->CopyQueueStatus = result2.GetRawPointer();

                secondaryResult->LastReplicationOperationReceivedTimeUtc = this->lastReplicationOperationReceivedTime_.AsFileTime;
                secondaryResult->LastCopyOperationReceivedTimeUtc = this->lastCopyOperationReceivedTime_.AsFileTime;
                secondaryResult->LastAcknowledgementSentTimeUtc = this->lastAcknowledgementSentTime_.AsFileTime;
                secondaryResult->IsInBuild = this->isInBuild_ ? TRUE : FALSE;

                publicResult.Role = this->role_;
                publicResult.Value = secondaryResult.GetRawPointer();
            }
        }
    }
    else 
    {
        ec = ErrorCodeValue::InvalidArgument;
    }
    
    return ec;
}

ErrorCode SecondaryReplicatorStatusQueryResult::FromPublicApi(
    __in FABRIC_REPLICATOR_STATUS_QUERY_RESULT const & publicResult)
{
    if (publicResult.Value == nullptr)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    if (publicResult.Role != FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY &&
        publicResult.Role != FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    auto secondaryStatus = reinterpret_cast<FABRIC_SECONDARY_REPLICATOR_STATUS_QUERY_RESULT*>(publicResult.Value);

    if (secondaryStatus->CopyQueueStatus == nullptr ||
        secondaryStatus->ReplicationQueueStatus == nullptr)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    ErrorCode ec;

    this->copyQueueStatus_ = make_shared<ReplicatorQueueStatus>();
    ec = this->copyQueueStatus_->FromPublicApi(*secondaryStatus->CopyQueueStatus);
    
    if (ec.IsSuccess())
    {
        this->replicationQueueStatus_ = make_shared<ReplicatorQueueStatus>();
        ec = this->replicationQueueStatus_->FromPublicApi(*secondaryStatus->ReplicationQueueStatus);
    }

    if (!ec.IsSuccess())
    {
        this->role_ = FABRIC_REPLICA_ROLE_UNKNOWN;
        this->copyQueueStatus_.reset();
        this->replicationQueueStatus_.reset();
    }
    else
    {
        this->role_ = publicResult.Role;
        this->isInBuild_ = secondaryStatus->IsInBuild ? true : false;
        this->lastAcknowledgementSentTime_ = DateTime(secondaryStatus->LastAcknowledgementSentTimeUtc);
        this->lastCopyOperationReceivedTime_ = DateTime(secondaryStatus->LastCopyOperationReceivedTimeUtc);
        this->lastReplicationOperationReceivedTime_ = DateTime(secondaryStatus->LastReplicationOperationReceivedTimeUtc);
    }

    return ec;
}

void SecondaryReplicatorStatusQueryResult::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring SecondaryReplicatorStatusQueryResult::ToString() const
{
    return wformatString(
        "ReplicationQueueStatus = '{0}', LastReplicationOperationReceivedTimeUtc = '{1}', IsInBuild = '{2}', CopyQueueStatus = '{3}', LastCopyOperationReceivedTime = '{4}', LastAcknowledgementSentTime = '{5}'",
       replicationQueueStatus_->ToString(), lastReplicationOperationReceivedTime_, isInBuild_, copyQueueStatus_->ToString(), lastCopyOperationReceivedTime_, lastAcknowledgementSentTime_);
}
