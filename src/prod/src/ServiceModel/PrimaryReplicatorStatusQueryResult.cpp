// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ReplicatorStatusQueryResultSPtr PrimaryReplicatorStatusQueryResult::Create(
      shared_ptr<ReplicatorQueueStatus> && replicationQueueStatus,
      vector<RemoteReplicatorStatus> && remoteReplicators)
{
    auto result = shared_ptr<PrimaryReplicatorStatusQueryResult>(
        new PrimaryReplicatorStatusQueryResult(
            move(replicationQueueStatus),
            move(remoteReplicators)));

    return move(result);
}

PrimaryReplicatorStatusQueryResult::PrimaryReplicatorStatusQueryResult()
    : ReplicatorStatusQueryResult(FABRIC_REPLICA_ROLE_UNKNOWN)
    , replicationQueueStatus_()
    , remoteReplicators_()
{
}

PrimaryReplicatorStatusQueryResult::PrimaryReplicatorStatusQueryResult(
    shared_ptr<ReplicatorQueueStatus> && replicationQueueStatus,
    vector<RemoteReplicatorStatus> && remoteReplicators)
    : ReplicatorStatusQueryResult(FABRIC_REPLICA_ROLE_PRIMARY)
    , replicationQueueStatus_(move(replicationQueueStatus))
    , remoteReplicators_(move(remoteReplicators))
{
}

PrimaryReplicatorStatusQueryResult::PrimaryReplicatorStatusQueryResult(PrimaryReplicatorStatusQueryResult && other)
    : ReplicatorStatusQueryResult(move(other))
    , replicationQueueStatus_(move(other.replicationQueueStatus_))
    , remoteReplicators_(move(other.remoteReplicators_))
{
}

PrimaryReplicatorStatusQueryResult & PrimaryReplicatorStatusQueryResult::operator=(PrimaryReplicatorStatusQueryResult && other)
{
    if (this != &other)
    {
        this->replicationQueueStatus_ = move(other.replicationQueueStatus_);
        this->remoteReplicators_ = move(other.remoteReplicators_);

        ReplicatorStatusQueryResult::operator=(move(other));
    }

    return *this;
}

ErrorCode PrimaryReplicatorStatusQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_REPLICATOR_STATUS_QUERY_RESULT & publicResult) const 
{
    ErrorCode ec;

    if (this->replicationQueueStatus_ != nullptr)
    {
        auto queueStatus = heap.AddItem<FABRIC_REPLICATOR_QUEUE_STATUS>();
        ec = this->replicationQueueStatus_->ToPublicApi(heap, *queueStatus);

        if (ec.IsSuccess())
        {
            auto list = heap.AddItem<FABRIC_REMOTE_REPLICATOR_STATUS_LIST>();
            ec = PublicApiHelper::ToPublicApiList<RemoteReplicatorStatus, FABRIC_REMOTE_REPLICATOR_STATUS, FABRIC_REMOTE_REPLICATOR_STATUS_LIST>(heap, this->remoteReplicators_, *list);

            if (ec.IsSuccess())
            {
                auto primaryResult = heap.AddItem<FABRIC_PRIMARY_REPLICATOR_STATUS_QUERY_RESULT>();
                primaryResult->RemoteReplicators = list.GetRawPointer();
                primaryResult->ReplicationQueueStatus = queueStatus.GetRawPointer();

                publicResult.Role = this->role_;
                publicResult.Value = primaryResult.GetRawPointer();
            }
        }
    }
    else 
    {
        ec = ErrorCodeValue::InvalidArgument;
    }

    return ec;
}

ErrorCode PrimaryReplicatorStatusQueryResult::FromPublicApi(
    __in FABRIC_REPLICATOR_STATUS_QUERY_RESULT const & publicResult)
{
    if (publicResult.Value == nullptr ||
        publicResult.Role != FABRIC_REPLICA_ROLE_PRIMARY)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    ErrorCode ec;
    auto primaryStatus = reinterpret_cast<FABRIC_PRIMARY_REPLICATOR_STATUS_QUERY_RESULT*>(publicResult.Value);

    this->replicationQueueStatus_ = make_shared<ReplicatorQueueStatus>();
    ec = this->replicationQueueStatus_->FromPublicApi(*primaryStatus->ReplicationQueueStatus);

    if (ec.IsSuccess())
    {
        ec = PublicApiHelper::FromPublicApiList(primaryStatus->RemoteReplicators, this->remoteReplicators_);
    }

    if (!ec.IsSuccess())
    {
        this->role_ = FABRIC_REPLICA_ROLE_UNKNOWN;
        this->replicationQueueStatus_.reset();
        this->remoteReplicators_.clear();
    }
    else
    {
        this->role_ = publicResult.Role;
    }

    return ec;
}

void PrimaryReplicatorStatusQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring PrimaryReplicatorStatusQueryResult::ToString() const
{
    return wformatString("ReplicationQueueStatus = {0}, RemoteReplicatorsCount = {1}",
        this->replicationQueueStatus_->ToString(),
        this->remoteReplicators_.size());
}
