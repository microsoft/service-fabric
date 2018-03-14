// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ReplicatorQueueStatus::ReplicatorQueueStatus()
    : queueUtilizationPercentage_(0)
    , queueMemorySize_(0)
    , firstSequenceNumber_(-1)
    , completedSequenceNumber_(-1)
    , committedSequenceNumber_(-1)
    , lastSequenceNumber_(-1)
{
}

ReplicatorQueueStatus::ReplicatorQueueStatus(
    uint queueUtilizationPercentage,
    LONGLONG queueMemorySize,
    FABRIC_SEQUENCE_NUMBER firstSequenceNumber,
    FABRIC_SEQUENCE_NUMBER completedSequenceNumber,
    FABRIC_SEQUENCE_NUMBER committedSequenceNumber,
    FABRIC_SEQUENCE_NUMBER lastSequenceNumber)
    : queueUtilizationPercentage_(queueUtilizationPercentage)
    , queueMemorySize_(queueMemorySize)
    , firstSequenceNumber_(firstSequenceNumber)
    , completedSequenceNumber_(completedSequenceNumber)
    , committedSequenceNumber_(committedSequenceNumber)
    , lastSequenceNumber_(lastSequenceNumber)
{
}

ReplicatorQueueStatus::ReplicatorQueueStatus(ReplicatorQueueStatus && other)
    : queueUtilizationPercentage_(other.queueUtilizationPercentage_)
    , queueMemorySize_(other.queueMemorySize_)
    , firstSequenceNumber_(other.firstSequenceNumber_)
    , committedSequenceNumber_(other.committedSequenceNumber_)
    , completedSequenceNumber_(other.completedSequenceNumber_)
    , lastSequenceNumber_(other.lastSequenceNumber_)
{
}

ReplicatorQueueStatus const & ReplicatorQueueStatus::operator = (ReplicatorQueueStatus && other)
{
    if (this != &other)
    {
        this->queueUtilizationPercentage_ = other.queueUtilizationPercentage_;
        this->queueMemorySize_ = other.queueMemorySize_;
        this->firstSequenceNumber_ = other.firstSequenceNumber_;
        this->committedSequenceNumber_ = other.committedSequenceNumber_;
        this->completedSequenceNumber_ = other.completedSequenceNumber_;
        this->lastSequenceNumber_ = other.lastSequenceNumber_;
    }

    return *this;
}

ErrorCode ReplicatorQueueStatus::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_REPLICATOR_QUEUE_STATUS & publicResult) const
{
    UNREFERENCED_PARAMETER(heap);
    publicResult.QueueUtilizationPercentage = this->queueUtilizationPercentage_;
    publicResult.CommittedSequenceNumber = this->committedSequenceNumber_;
    publicResult.CompletedSequenceNumber = this->completedSequenceNumber_;
    publicResult.FirstSequenceNumber = this->firstSequenceNumber_;
    publicResult.LastSequenceNumber = this->lastSequenceNumber_;
    publicResult.QueueMemorySize = this->queueMemorySize_;
    
    return ErrorCodeValue::Success;
}

ErrorCode ReplicatorQueueStatus::FromPublicApi(
    __in FABRIC_REPLICATOR_QUEUE_STATUS & publicResult)
{
    this->committedSequenceNumber_ = publicResult.CommittedSequenceNumber;
    this->completedSequenceNumber_ = publicResult.CompletedSequenceNumber;
    this->firstSequenceNumber_ = publicResult.FirstSequenceNumber;
    this->lastSequenceNumber_ = publicResult.LastSequenceNumber;
    this->queueMemorySize_ = publicResult.QueueMemorySize;
    this->queueUtilizationPercentage_ = publicResult.QueueUtilizationPercentage;

    return ErrorCode();
}

void ReplicatorQueueStatus::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ReplicatorQueueStatus::ToString() const
{
    return wformatString(
        "queueUtilizationPercentage = '{0}', FirstSequenceNumber = '{1}', committedSequenceNumber = '{2}', completedSequenceNumber = '{3}', lastSequenceNumber = '{4}', QueueMemorySize = '{5}'",
       queueUtilizationPercentage_, firstSequenceNumber_, committedSequenceNumber_, completedSequenceNumber_, lastSequenceNumber_, queueMemorySize_);
}
