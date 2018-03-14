// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

RemoteReplicatorStatus::RemoteReplicatorStatus()
    : replicaId_(-1)
    , lastAckProcessedTime_(DateTime::Zero)
    , lastReceivedReplicationSeqNumber_(-1)
    , lastAppliedReplicationSeqNumber_(-1)
    , isInBuild_(false)
    , lastReceivedCopySeqNumber_(-1)
    , lastAppliedCopySeqNumber_(-1)
    , remoteReplicatorAckStatus_()
{
}

RemoteReplicatorStatus::RemoteReplicatorStatus(
    FABRIC_REPLICA_ID replicaId,
    DateTime lastAckProcessedTime,
    FABRIC_SEQUENCE_NUMBER lastReceivedReplicationSeqNumber,
    FABRIC_SEQUENCE_NUMBER lastAppliedReplicationSeqNumber,
    bool isInBuild,
    FABRIC_SEQUENCE_NUMBER lastReceivedCopySeqNumber,
    FABRIC_SEQUENCE_NUMBER lastAppliedCopySeqNumber,
    RemoteReplicatorAcknowledgementStatus && remoteReplicatorAcknowledgementStatus)
    : replicaId_(replicaId)
    , lastAckProcessedTime_(lastAckProcessedTime)
    , lastReceivedReplicationSeqNumber_(lastReceivedReplicationSeqNumber)
    , lastAppliedReplicationSeqNumber_(lastAppliedReplicationSeqNumber)
    , isInBuild_(isInBuild)
    , lastReceivedCopySeqNumber_(lastReceivedCopySeqNumber)
    , lastAppliedCopySeqNumber_(lastAppliedCopySeqNumber)
    , remoteReplicatorAckStatus_(move(remoteReplicatorAcknowledgementStatus))
{
}

RemoteReplicatorStatus::RemoteReplicatorStatus(RemoteReplicatorStatus && other)
    : replicaId_(other.replicaId_)
    , lastAckProcessedTime_(move(other.lastAckProcessedTime_))
    , lastReceivedReplicationSeqNumber_(other.lastReceivedReplicationSeqNumber_)
    , lastAppliedReplicationSeqNumber_(other.lastAppliedReplicationSeqNumber_)
    , isInBuild_(other.isInBuild_)
    , lastReceivedCopySeqNumber_(other.lastReceivedCopySeqNumber_)
    , lastAppliedCopySeqNumber_(other.lastAppliedCopySeqNumber_)
    , remoteReplicatorAckStatus_(move(other.remoteReplicatorAckStatus_))
{
}

RemoteReplicatorStatus & RemoteReplicatorStatus::operator=(RemoteReplicatorStatus && other)
{
    if (this != &other)
    {
        this->replicaId_ = other.replicaId_;
        this->lastAckProcessedTime_ = move(other.lastAckProcessedTime_);
        this->lastReceivedReplicationSeqNumber_ = other.lastReceivedReplicationSeqNumber_;
        this->lastAppliedReplicationSeqNumber_ = other.lastAppliedReplicationSeqNumber_;
        this->isInBuild_ = other.isInBuild_;
        this->lastReceivedCopySeqNumber_ = other.lastReceivedCopySeqNumber_;
        this->lastAppliedCopySeqNumber_ = other.lastAppliedCopySeqNumber_;
        this->remoteReplicatorAckStatus_ = move(other.remoteReplicatorAckStatus_);
    }

    return *this;
}

ErrorCode RemoteReplicatorStatus::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_REMOTE_REPLICATOR_STATUS & publicResult) const 
{    
	publicResult.ReplicaId = this->replicaId_;
    publicResult.LastAcknowledgementProcessedTimeUtc = this->lastAckProcessedTime_.AsFileTime;
    publicResult.LastReceivedReplicationSequenceNumber = this->lastReceivedReplicationSeqNumber_;
    publicResult.LastAppliedReplicationSequenceNumber = this->lastAppliedReplicationSeqNumber_;
    publicResult.LastReceivedCopySequenceNumber = this->lastReceivedCopySeqNumber_;
    publicResult.LastAppliedCopySequenceNumber = this->lastAppliedCopySeqNumber_;
    publicResult.IsInBuild = this->isInBuild_ ? TRUE : FALSE;

    auto ackTimes = heap.AddItem<FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_STATUS>();
    publicResult.Reserved = ackTimes.GetRawPointer();
    
    this->remoteReplicatorAckStatus_.ToPublicApi(heap, *ackTimes);

    return ErrorCode::Success();
}

ErrorCode RemoteReplicatorStatus::FromPublicApi(
    __in FABRIC_REMOTE_REPLICATOR_STATUS & publicResult)
{
    this->isInBuild_ = publicResult.IsInBuild ? true : false;
    this->lastAckProcessedTime_ = DateTime(publicResult.LastAcknowledgementProcessedTimeUtc);
    this->lastAppliedCopySeqNumber_ = publicResult.LastAppliedCopySequenceNumber;
    this->lastAppliedReplicationSeqNumber_ = publicResult.LastAppliedReplicationSequenceNumber;
    this->lastReceivedCopySeqNumber_ = publicResult.LastReceivedCopySequenceNumber;
    this->lastReceivedReplicationSeqNumber_ = publicResult.LastReceivedReplicationSequenceNumber;
    this->replicaId_ = publicResult.ReplicaId;

    if(publicResult.Reserved != nullptr)
    {
        auto ack_status = static_cast<FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_STATUS*>(publicResult.Reserved);
        this->remoteReplicatorAckStatus_.FromPublicApi(*ack_status);
    }

    return ErrorCode();
}

void RemoteReplicatorStatus::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring RemoteReplicatorStatus::ToString() const
{
    return wformatString("ReplicaId = {0}, LastAcknowledgementProcessedTime = {1}, LastReceivedReplicationSequenceNumber = {2}, LastAppliedReplicationSequenceNumber = {3}, IsInBuild = {4}, LastReceivedCopySequenceNumber = {5}, LastAppliedCopySequenceNumber = {6}, RemoteReplicatorAcknowledgementStatus:{7}",
        this->replicaId_,
        this->lastAckProcessedTime_,
        this->lastReceivedReplicationSeqNumber_,
        this->lastAppliedReplicationSeqNumber_,
        this->isInBuild_,
        this->lastReceivedCopySeqNumber_,
        this->lastAppliedCopySeqNumber_,
        this->remoteReplicatorAckStatus_);
}
