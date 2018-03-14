// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Reliability::ReplicationComponent;

// SecondaryReplicatorTraceHandler
// Class used to batch trace information on ack
SecondaryReplicatorTraceHandler::SecondaryReplicatorTraceHandler(ULONG64 const & batchSize)
    : secondaryReplicatorBatchTracingArraySize_(batchSize)
    , pendingTraceMessageIndex_(0)
    , pendingTraceMessages_()
{
    pendingTraceMessages_.reserve(secondaryReplicatorBatchTracingArraySize_);
}

void SecondaryReplicatorTraceHandler::AddBatchOperation(
    __in FABRIC_SEQUENCE_NUMBER const & lowLSN,
    __in FABRIC_SEQUENCE_NUMBER const & highLSN)
{
    this->AddOperationPrivate(lowLSN, highLSN);
}

void SecondaryReplicatorTraceHandler::AddOperation(__in FABRIC_SEQUENCE_NUMBER const & lsn)
{
    this->AddOperationPrivate(lsn, lsn);
}

void SecondaryReplicatorTraceHandler::AddOperationPrivate(
    __in FABRIC_SEQUENCE_NUMBER const & lowLSN,
    __in FABRIC_SEQUENCE_NUMBER const & highLSN)
{
    // Populate the messages vector until the maximum configured size is reached
    if (pendingTraceMessages_.size() < secondaryReplicatorBatchTracingArraySize_)
    {
        pendingTraceMessages_.push_back(SecondaryReplicatorTraceInfo(lowLSN, highLSN));
    }
    else
    {
        // Update entries in the messages vector
        // If the total # of messages is greater than SecondaryReplicatorBatchTracingArraySize the earliest entry will be overwritten with newer message information
        pendingTraceMessages_[pendingTraceMessageIndex_].Update(lowLSN, highLSN);
    }

    IncrementPendingTraceIndex();
}

std::vector<SecondaryReplicatorTraceInfo> SecondaryReplicatorTraceHandler::GetTraceMessages()
{
    std::vector<SecondaryReplicatorTraceInfo> rv;

    // Return empty vector if no messages are pending
    if(pendingTraceMessages_.size() == 0)
    {
        return rv;
    }

    // Swap current vector with new empty
    rv.reserve(secondaryReplicatorBatchTracingArraySize_);
    std::swap(rv, pendingTraceMessages_);

    // Reset vector index
    pendingTraceMessageIndex_ = 0;

    // Return previous vector value
    return rv;
}

void SecondaryReplicatorTraceHandler::IncrementPendingTraceIndex()
{
    if(pendingTraceMessageIndex_ < secondaryReplicatorBatchTracingArraySize_ - 1)
    {
        pendingTraceMessageIndex_++;
    }
    else
    {
        pendingTraceMessageIndex_ = 0;
    }
}

ErrorCode SecondaryReplicatorTraceHandler::Test_GetLsnAtIndex(
    __in ULONG64 index,
    __out LONG64 & lowLsn,
    __out LONG64 & highLsn)
{
    if(index > pendingTraceMessages_.size() + 1)
    {
        return Common::ErrorCodeValue::InvalidArgument;
    }

    lowLsn = pendingTraceMessages_[index].LowLsn;
    highLsn = pendingTraceMessages_[index].HighLsn;
    return Common::ErrorCodeValue::Success;
}
