// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

RemoteReplicatorAcknowledgementStatus::RemoteReplicatorAcknowledgementStatus()
    : replicationStreamAcknowledgementDetail_()
    , copyStreamAcknowledgementDetail_()
{
}

RemoteReplicatorAcknowledgementStatus::RemoteReplicatorAcknowledgementStatus(
    RemoteReplicatorAcknowledgementDetail && replicationStreamAcknowledgementDetail,
    RemoteReplicatorAcknowledgementDetail && copyStreamAcknowledgementDetail)
    : replicationStreamAcknowledgementDetail_(move(replicationStreamAcknowledgementDetail))
    , copyStreamAcknowledgementDetail_(move(copyStreamAcknowledgementDetail))
{
}

RemoteReplicatorAcknowledgementStatus::RemoteReplicatorAcknowledgementStatus(RemoteReplicatorAcknowledgementStatus && other)
    : replicationStreamAcknowledgementDetail_(move(other.replicationStreamAcknowledgementDetail_))
    , copyStreamAcknowledgementDetail_(move(other.copyStreamAcknowledgementDetail_))
{
}

RemoteReplicatorAcknowledgementStatus & RemoteReplicatorAcknowledgementStatus::operator=(RemoteReplicatorAcknowledgementStatus && other)
{
    if (this != &other)
    {
        this->replicationStreamAcknowledgementDetail_ = move(other.replicationStreamAcknowledgementDetail_);
        this->copyStreamAcknowledgementDetail_ = move(other.copyStreamAcknowledgementDetail_);
    }

    return *this;
}

ErrorCode RemoteReplicatorAcknowledgementStatus::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_STATUS & publicResult) const 
{
    auto replicationAckDetails = heap.AddItem<FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_DETAIL>();
    auto copyAckDetails = heap.AddItem<FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_DETAIL>();

    publicResult.ReplicationStreamAcknowledgementDetails = replicationAckDetails.GetRawPointer();
    publicResult.CopyStreamAcknowledgementDetails = copyAckDetails.GetRawPointer();

    this->replicationStreamAcknowledgementDetail_.ToPublicApi(heap, *publicResult.ReplicationStreamAcknowledgementDetails);
    this->copyStreamAcknowledgementDetail_.ToPublicApi(heap, *publicResult.CopyStreamAcknowledgementDetails);

    return ErrorCode::Success();
}

ErrorCode RemoteReplicatorAcknowledgementStatus::FromPublicApi(
    __in FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_STATUS & publicResult)
{
    this->replicationStreamAcknowledgementDetail_.FromPublicApi(*publicResult.ReplicationStreamAcknowledgementDetails);
    this->copyStreamAcknowledgementDetail_.FromPublicApi(*publicResult.CopyStreamAcknowledgementDetails);

    return ErrorCode();
}

void RemoteReplicatorAcknowledgementStatus::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring RemoteReplicatorAcknowledgementStatus::ToString() const
{
    return wformatString("\nReplicationStreamAcknowledgementDetails = {0}\nCopyStreamAcknowledgementDetails = {1}\n",
        this->replicationStreamAcknowledgementDetail_,
        this->copyStreamAcknowledgementDetail_);
}
