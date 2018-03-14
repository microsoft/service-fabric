// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

RemoteReplicatorAcknowledgementDetail::RemoteReplicatorAcknowledgementDetail()
    : avgReceiveAckDuration_(-1)
    , avgApplyAckDuration_(-1)
    , notReceivedCount_(-1)
    , receivedAndNotAppliedCount_(-1)
{
}

RemoteReplicatorAcknowledgementDetail::RemoteReplicatorAcknowledgementDetail(
    int64 avgReplicationReceiveAckDuration,
    int64 avgReplicationApplyAckDuration,
    FABRIC_SEQUENCE_NUMBER notReceivedReplicationCount,
    FABRIC_SEQUENCE_NUMBER receivedAndNotAppliedReplicationCount)
    : avgReceiveAckDuration_(avgReplicationReceiveAckDuration)
    , avgApplyAckDuration_(avgReplicationApplyAckDuration)
    , notReceivedCount_(notReceivedReplicationCount)
    , receivedAndNotAppliedCount_(receivedAndNotAppliedReplicationCount)
{
}

RemoteReplicatorAcknowledgementDetail::RemoteReplicatorAcknowledgementDetail(RemoteReplicatorAcknowledgementDetail && other)
    : avgReceiveAckDuration_(other.avgReceiveAckDuration_)
    , avgApplyAckDuration_(other.avgApplyAckDuration_)
    , notReceivedCount_(other.notReceivedCount_)
    , receivedAndNotAppliedCount_(other.receivedAndNotAppliedCount_)
{
}

RemoteReplicatorAcknowledgementDetail & RemoteReplicatorAcknowledgementDetail::operator=(RemoteReplicatorAcknowledgementDetail && other)
{
    if (this != &other)
    {
        this->avgReceiveAckDuration_ = other.avgReceiveAckDuration_;
        this->avgApplyAckDuration_ = move(other.avgApplyAckDuration_);
        this->notReceivedCount_ = other.notReceivedCount_;
        this->receivedAndNotAppliedCount_ = other.receivedAndNotAppliedCount_;
    }

    return *this;
}

ErrorCode RemoteReplicatorAcknowledgementDetail::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_DETAIL & publicResult) const
{
    UNREFERENCED_PARAMETER(heap);

    publicResult.AverageReceiveDurationMilliseconds = this->avgReceiveAckDuration_;
    publicResult.AverageApplyDurationMilliseconds = this->avgApplyAckDuration_;
    publicResult.NotReceivedCount = this->notReceivedCount_;
    publicResult.ReceivedAndNotAppliedCount = this->receivedAndNotAppliedCount_;

    return ErrorCode::Success();
}

ErrorCode RemoteReplicatorAcknowledgementDetail::FromPublicApi(
    __in FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_DETAIL & publicResult)
{
    this->avgReceiveAckDuration_ = publicResult.AverageReceiveDurationMilliseconds;
    this->avgApplyAckDuration_ = publicResult.AverageApplyDurationMilliseconds;
    this->notReceivedCount_ = publicResult.NotReceivedCount;
    this->receivedAndNotAppliedCount_ = publicResult.ReceivedAndNotAppliedCount;
        
    return ErrorCode();
}

void RemoteReplicatorAcknowledgementDetail::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring RemoteReplicatorAcknowledgementDetail::ToString() const
{
    return wformatString("AverageReceiveAcknowledgementDuration = {0}, AverageApplyAcknowledgementDuration = {1}, NotReceivedCount = {2}, ReceivedAndNotAppliedCount = {3}",
        this->avgReceiveAckDuration_,
        this->avgApplyAckDuration_,
        this->notReceivedCount_,
        this->receivedAndNotAppliedCount_);
}
