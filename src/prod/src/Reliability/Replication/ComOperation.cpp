// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::Stopwatch;
using Common::TimeSpan;
using Common::ComUtility;

ComOperation::ComOperation(
    FABRIC_OPERATION_METADATA const & metadata,
    FABRIC_EPOCH const & epoch,
    FABRIC_SEQUENCE_NUMBER lastOperationInBatch)
    :   IFabricOperation(), 
        ComUnknownBase(),
        metadata_(metadata),
        epoch_(epoch),
        lastOperationInBatch_(lastOperationInBatch),
        dataSize_(0),
        enqueueTime_(Stopwatch::Now()),
        commitTime_(0),
        completeTime_(0),
        cleanupTime_(0)
{
}

ComOperation::~ComOperation()
{
}

const FABRIC_OPERATION_METADATA * STDMETHODCALLTYPE ComOperation::get_Metadata(void)
{
    return &metadata_;
}
            
ULONGLONG ComOperation::get_DataSize()
{
    if (dataSize_ == 0)
    {
        dataSize_ = GetDataSize();
    }

    return dataSize_;
}
        
// The operation is committed, but not completed.
 TimeSpan ComOperation::Commit()
{
    if (commitTime_.Ticks == 0)
        commitTime_ = Stopwatch::Now();
    
    auto timeSpan = TimeSpan::FromTicks(commitTime_.Ticks - enqueueTime_.Ticks);

    OperationQueueEventSource::Events->OperationCommit(
        metadata_.SequenceNumber,
        timeSpan.TotalMilliseconds());

    return timeSpan;
}

// The operation is completed - the user doesn't need it anymore.
// Based on the purpose of the container, the operation may still be needed
// until Cleanup is called.
TimeSpan ComOperation::Complete()
{
    if (completeTime_.Ticks == 0)
        completeTime_ = Stopwatch::Now();

    auto timeSpan = TimeSpan::FromTicks(completeTime_.Ticks - enqueueTime_.Ticks);

    OperationQueueEventSource::Events->OperationComplete(
        metadata_.SequenceNumber,
        timeSpan.TotalMilliseconds());

    return timeSpan;
}

// The operation is not needed anymore, so it can be cleaned up.
// Once this is called, the replication engine should remove it from the queue
// that is holding it.
TimeSpan ComOperation::Cleanup()
{
    if (cleanupTime_.Ticks == 0)
        cleanupTime_ = Stopwatch::Now();

    auto timeSpan = TimeSpan::FromTicks(cleanupTime_.Ticks - enqueueTime_.Ticks);

    OperationQueueEventSource::Events->OperationCleanup(
        metadata_.SequenceNumber,
        timeSpan.TotalMilliseconds());

    return timeSpan;
}

ULONGLONG ComOperation::GetDataSize()
{
    ULONGLONG totalSize = 0;
    ULONG count = 0;
    FABRIC_OPERATION_DATA_BUFFER const * buffers = NULL;

    if (SUCCEEDED(GetData(&count, &buffers)) && NULL != buffers)
    {
        for (ULONG i = 0; i< count; i++)
        {
            totalSize += buffers[i].BufferSize;
        }
    }

    return totalSize;
}
    
} // end namespace ReplicationComponent
} // end namespace Reliability
