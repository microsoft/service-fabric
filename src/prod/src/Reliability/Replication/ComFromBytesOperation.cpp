// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Reliability::ReplicationComponent;
using namespace Common;

ComFromBytesOperation::ComFromBytesOperation(
    std::vector<Common::const_buffer> const & buffers,
    std::vector<ULONG> const & segmentSizes,
    FABRIC_OPERATION_METADATA const & metadata,
    OperationAckCallback const & ackCallback,
    FABRIC_SEQUENCE_NUMBER lastOperationInBatch)
    :   ComOperation(metadata, Constants::InvalidEpoch, lastOperationInBatch),
        data_(),
        segmentSizes_(segmentSizes),
        ackCallback_(ackCallback),
        callbackState_(CallbackState::NotCalled)
{
    InitBuffer(buffers);
    InitData();
}

ComFromBytesOperation::ComFromBytesOperation(
    std::vector<Common::const_buffer> const & buffers,
    std::vector<ULONG> const & segmentSizes,
    FABRIC_OPERATION_METADATA const & metadata,
    FABRIC_EPOCH const & epoch,
    OperationAckCallback const & ackCallback,
    FABRIC_SEQUENCE_NUMBER lastOperationInBatch)
    :   ComOperation(metadata, epoch, lastOperationInBatch),
        data_(),
        segmentSizes_(segmentSizes),
        ackCallback_(ackCallback),
        callbackState_(CallbackState::NotCalled)
{
    InitBuffer(buffers);
    InitData();
}

ComFromBytesOperation::ComFromBytesOperation(
    std::vector<BYTE> && data,
    std::vector<ULONG> const & segmentSizes,
    FABRIC_OPERATION_METADATA const & metadata,
    FABRIC_EPOCH const & epoch,
    OperationAckCallback const & ackCallback,
    FABRIC_SEQUENCE_NUMBER lastOperationInBatch)
    :   ComOperation(metadata, epoch, lastOperationInBatch),
        data_(std::move(data)),
        segmentSizes_(segmentSizes),
        ackCallback_(ackCallback),
        callbackState_(CallbackState::NotCalled)
{
    InitData();
}

ComFromBytesOperation::ComFromBytesOperation(
    FABRIC_OPERATION_METADATA const & metadata,
    OperationAckCallback const & ackCallback,
    FABRIC_SEQUENCE_NUMBER lastOperationInBatch,
    Common::ComponentRoot const & root)
    :   ComOperation(metadata, Constants::InvalidEpoch, lastOperationInBatch),
        ackCallback_(ackCallback),
        callbackState_(CallbackState::NotCalled)
{
    rootSPtr_ = root.CreateComponentRoot();
}

ComFromBytesOperation::~ComFromBytesOperation()
{
}

void ComFromBytesOperation::InitData()
{
    size_t size = data_.size();

    if (this->segmentSizes_.size() != 0)
    {
        size_t totalSegmentSize = 0;
        for (ULONG segmentSize : this->segmentSizes_)
        {
            totalSegmentSize += segmentSize;
        }

        ASSERT_IF(totalSegmentSize != size, "{0} does not match {1}", totalSegmentSize, size);

        ULONG segmentWalker = 0;
        for (ULONG segmentSize : this->segmentSizes_)
        {
            FABRIC_OPERATION_DATA_BUFFER replicaBuffer;
            // for 0 size buffer, point the Buffer to any valid non-NULL address with BufferSize=0
            replicaBuffer.Buffer = (0 == size) ? (BYTE*)&data_ : (this->data_.data() + segmentWalker);
            replicaBuffer.BufferSize = segmentSize;

            segmentWalker += segmentSize;

            this->segments_.push_back(replicaBuffer);
        }
    }
}

void ComFromBytesOperation::InitBuffer(
    std::vector<Common::const_buffer> const & buffers)
{
    size_t size = 0;
    for (auto buffer : buffers)
    {
        size += buffer.len;
    }

    data_.reserve(size);
    for (auto buffer : buffers)
    {
        data_.insert(data_.end(), buffer.buf, buffer.buf + buffer.len);
    }
}

bool ComFromBytesOperation::IsEmpty() const 
{
    return data_.empty();
}

bool ComFromBytesOperation::get_NeedsAck() const
{
    Common::AcquireExclusiveLock lock(this->lock_);
    bool ret = false;

    switch (this->callbackState_)
    {
    case CallbackState::NotCalled:
        ret = true;
        break;
    case CallbackState::ReadyToRun:
        // #4780473 - pending ops on secondary with EOS do not keep root alive. hence wait for the running callback to finish
        // Scenario without EOS is that LSN 1 could be completing, with 2 and 3 waiting on the queue lock in secondaryreplicator onackreplicationoperation
        // 2 and 3 are in running state, which makes OQ believe that they are ack'd and hence releases all objects to finish any pending async op and destruct
        // when 2 and 3's callbacks run, they AV
        ret = true;
        break;
    case CallbackState::Running:
    case CallbackState::Completed:
    case CallbackState::Cancelled:
        ret = false;
        break;
    default:
        Assert::CodingError("Unknown callbackstate {0}", static_cast<int>(callbackState_));
    }

    return ret;
}

void ComFromBytesOperation::OnAckCallbackStartedRunning()
{
    Common::AcquireExclusiveLock lock(this->lock_);
    
    ASSERT_IFNOT(
        this->callbackState_ == CallbackState::ReadyToRun, 
        "Callback must be in ready to run state before running. It is {0}", 
        static_cast<int>(callbackState_));

    callbackState_ = CallbackState::Running;
}

void ComFromBytesOperation::SetIgnoreAckAndKeepParentAlive(Common::ComponentRoot const & root) 
{
    Common::AcquireExclusiveLock lock(this->lock_);
    
    if (this->IsEndOfStreamOperation)
    {
        // For an end of stream operation that could have been enqueued into the operation queue (happens during Copy only),
        // we cannot ignore the ACK. So we will keep the root alive, but not ignore the ACK
        if (this->callbackState_ == CallbackState::NotCalled ||
            this->callbackState_ == CallbackState::Running ||
            this->callbackState_ == CallbackState::ReadyToRun)
        {
            rootSPtr_ = root.CreateComponentRoot();
        }
    }
    else
    {
        if (this->callbackState_ == CallbackState::NotCalled)
        {
            this->callbackState_ = CallbackState::Cancelled;
        }
        else if (this->callbackState_ == CallbackState::Running || this->callbackState_ == CallbackState::ReadyToRun)
        {
            // Ack was required and now is ignored;
            // increase the ref count on the parent,
            // so in case ACK comes it will still be alive
            rootSPtr_ = root.CreateComponentRoot();
        }
        // else it must be completed, we don't care
    }
}

HRESULT ComFromBytesOperation::Acknowledge()
{
    bool isCallbackRunning = false;

    {
        Common::AcquireExclusiveLock lock(this->lock_);

        TESTASSERT_IF(this->callbackState_ == CallbackState::Completed, "Callback is already completed");
        TESTASSERT_IF(this->callbackState_ == CallbackState::Running, "Callback is already running");
        ASSERT_IF(this->callbackState_ == CallbackState::Cancelled && this->IsEndOfStreamOperation, "End of stream operation should never have its callbackstate cancelled");
        
        if (this->callbackState_ == CallbackState::Completed ||
            this->callbackState_ == CallbackState::Running ||
            this->callbackState_ == CallbackState::ReadyToRun)
        {
            return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
        }

        if (this->callbackState_ == CallbackState::NotCalled)
        {
            this->callbackState_ = CallbackState::ReadyToRun;
            isCallbackRunning = true;
        }
    }

    if (ackCallback_ && isCallbackRunning)
    {
        ackCallback_(*this);
    }
    else
    {
        OperationQueueEventSource::Events->OperationAck(
            Metadata.SequenceNumber);
    }

    if (isCallbackRunning)
    {
        // If the callback finished running, set it to completed and reset the root in case the secondary had closed
        // and created the root while the callback was running
        Common::AcquireExclusiveLock lock(this->lock_);

        ASSERT_IF(
            (this->callbackState_ != CallbackState::Running) && ackCallback_, 
            "Callback must be in running state before completed when there is a valid ack callback. It is {0}", 
            static_cast<int>(callbackState_));

        this->callbackState_ = CallbackState::Completed;

        rootSPtr_.reset();
    }
    else
    {
        TESTASSERT_IFNOT(
            this->rootSPtr_ == nullptr,
            "If callback did not run, root must be nullptr");
    }

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComFromBytesOperation::GetData(
    /*[out]*/ ULONG * count, 
    /*[out, retval]*/ FABRIC_OPERATION_DATA_BUFFER const ** buffers)
{
    if ((count == NULL) || (buffers == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    *count = static_cast<ULONG>(this->segments_.size());
    *buffers = this->segments_.data();

    return ComUtility::OnPublicApiReturn(S_OK);
}
