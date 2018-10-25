// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Transport;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("RolloutContext");

RolloutContext::RolloutContext(RolloutContextType::Enum type)
    : StoreData()
    , status_(RolloutStatus::Pending)
    , contextType_(type)
    , clientRequest_()
    , retryTimer_()
    , timerLock_()
    , timeout_(TimeSpan::Zero)
    , error_(ErrorCodeValue::Success)
    , cachedMapKey_(L"")
    , isExternallyFailed_(false)
    , shouldKeepInQueue_(false)
    , operationRetryStopwatch_()
    , timeoutCount_(0)
{
}

RolloutContext::RolloutContext(RolloutContext const & other)
    : StoreData(other)
    , status_(other.status_)
    , contextType_(other.contextType_)
    , clientRequest_(other.clientRequest_)
    , retryTimer_(other.retryTimer_)
    , timerLock_()
    , timeout_(other.timeout_)
    , error_(other.error_)
    , cachedMapKey_(other.cachedMapKey_)
    , isExternallyFailed_(false)
    , shouldKeepInQueue_(false)
    , operationRetryStopwatch_()
    , timeoutCount_(0)
{
}

RolloutContext::RolloutContext(RolloutContext && other)
    : StoreData(move(other))
    , status_(move(other.status_))
    , contextType_(move(other.contextType_))
    , clientRequest_(move(other.clientRequest_))
    , retryTimer_(move(other.retryTimer_))
    , timerLock_()
    , timeout_(move(other.timeout_))
    , error_(move(other.error_))
    , cachedMapKey_(move(other.cachedMapKey_))
    , isExternallyFailed_(false)
    , shouldKeepInQueue_(false)
    , operationRetryStopwatch_()
    , timeoutCount_(0)
{
}

RolloutContext & RolloutContext::operator =(RolloutContext && other)
{
    if (this != &other)
    {
        __super::operator = (move(other));
        status_ = move(other.status_);
        contextType_ = move(other.contextType_);
        clientRequest_ = move(other.clientRequest_);
        retryTimer_ = move(other.retryTimer_);
        // do not move: timerLock_()
        timeout_ = move(other.timeout_);
        error_.Overwrite(move(other.error_));
        cachedMapKey_ = move(other.cachedMapKey_);
        isExternallyFailed_ = false;
        shouldKeepInQueue_ = false;
    }

    return *this;
}

RolloutContext::RolloutContext(
    RolloutContextType::Enum type,
    ComponentRoot const & replica,
    ClientRequestSPtr const & clientRequest)
    : StoreData(
        clientRequest->ReplicaActivityId)
    , replicaSPtr_(replica.CreateComponentRoot())
    , status_(RolloutStatus::Pending)
    , contextType_(type)
    , clientRequest_(clientRequest)
    , retryTimer_()
    , timerLock_()
    , timeout_(TimeSpan::Zero)
    , error_(ErrorCodeValue::Success)
    , cachedMapKey_(L"")
    , isExternallyFailed_(false)
    , shouldKeepInQueue_(false)
    , operationRetryStopwatch_()
    , timeoutCount_(0)
{
    if (clientRequest_)
    {
        this->TrySetOperationTimeout(clientRequest_->Timeout);
    }
}

RolloutContext::RolloutContext(
    RolloutContextType::Enum type,
    Store::ReplicaActivityId const & replicaActivityId)
    : StoreData(replicaActivityId)
    , replicaSPtr_()
    , status_(RolloutStatus::Pending)
    , contextType_(type)
    , clientRequest_()
    , retryTimer_()
    , timerLock_()
    , timeout_(TimeSpan::Zero)
    , error_(ErrorCodeValue::Success)
    , cachedMapKey_(L"")
    , isExternallyFailed_(false)
    , shouldKeepInQueue_(false)
    , operationRetryStopwatch_()
    , timeoutCount_(0)
{
}

void RolloutContext::ReInitializeContext(ComponentRoot const & replica, ClientRequestSPtr const & clientRequest)
{
    replicaSPtr_ = replica.CreateComponentRoot();
    clientRequest_ = clientRequest;
    
    if (clientRequest_)
    {
        this->ReInitializeTracing(clientRequest_->ReplicaActivityId);
        this->TrySetOperationTimeout(clientRequest_->Timeout);
    }
}

TimeSpan RolloutContext::get_OperationTimeout() const
{
    if (timeout_ < ManagementConfig::GetConfig().MinOperationTimeout)
    {
        return ManagementConfig::GetConfig().MinOperationTimeout;
    }

    return timeout_;
}

TimeSpan RolloutContext::get_CommunicationTimeout() const
{
    auto timeout = this->OperationTimeout;

    if (timeout > ManagementConfig::GetConfig().MaxCommunicationTimeout)
    {
        return ManagementConfig::GetConfig().MaxCommunicationTimeout;
    }

    return timeout;
}

bool RolloutContext::TrySetOperationTimeout(TimeSpan const timeout)
{
    auto newTimeout = timeout;

    auto minTimeout = ManagementConfig::GetConfig().MinOperationTimeout;
    if (newTimeout < minTimeout)
    {
        newTimeout = minTimeout;
    }

    // timeout for internally processing any context is capped, but retries may occur indefinitely
    auto maxTimeout = ManagementConfig::GetConfig().MaxOperationTimeout;
    if (newTimeout > maxTimeout)
    {
        newTimeout = maxTimeout;
    }

    if (timeout_ != newTimeout)
    {
        timeout_ = newTimeout;

        return true;
    }
    else
    {
        return false;
    }
}

wstring const & RolloutContext::get_MapKey() const
{
    if (cachedMapKey_.empty())
    {
        wstring temp;
        StringWriter writer(temp);
        writer.Write("{0}:{1}", this->Type, this->Key);

        cachedMapKey_ = temp;
    }

    return cachedMapKey_;
}

void RolloutContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("RolloutContext[{0}, {1}, {2}, {3}]", this->Type, this->Key, this->SequenceNumber, this->Status);
}

ErrorCode RolloutContext::InsertCompletedIfNotFound(StoreTransaction const & storeTx)
{
    this->Status = RolloutStatus::Completed;

    WriteNoise(TraceComponent, "{0} InsertCompletedIfNotFound()", this->TraceId);

    return storeTx.InsertIfNotFound(*this);
}

ErrorCode RolloutContext::UpdateStatus(StoreTransaction const & storeTx, RolloutStatus::Enum status)
{
    if (status == RolloutStatus::Unknown)
    {
        Assert::TestAssert("{0} should not persist an unknown status");
        return ErrorCodeValue::OperationFailed;
    }

    WriteNoise(TraceComponent, "{0} UpdateStatus({1}, {2})", this->TraceId, status, this->SequenceNumber);

    this->Status = status;
    return storeTx.Update(*this);
}

ErrorCode RolloutContext::Refresh(StoreTransaction const & storeTx)
{
    ErrorCode error = storeTx.ReadExact(*this);

    if (!error.IsSuccess())
    {
        this->Status = RolloutStatus::Unknown;
    }

    return error;
}

ErrorCode RolloutContext::Complete(StoreTransaction const & storeTx)
{
    shouldKeepInQueue_ = false;
    return this->UpdateStatus(storeTx, RolloutStatus::Completed);
}

ErrorCode RolloutContext::Fail(StoreTransaction const & storeTx)
{
    // Clear derived class in-memory state for potential revert logic
    this->OnFailRolloutContext();

    // Fail unconditionally
    this->SetSequenceNumber(0);
    return this->UpdateStatus(storeTx, RolloutStatus::Failed);
}

void RolloutContext::CompleteClientRequest()
{
    CompleteClientRequest(this->Error);
}

void RolloutContext::CompleteClientRequest(ErrorCode const & error)
{
    if (clientRequest_)
    {
        WriteNoise(TraceComponent, "{0} completing client request with {1}", this->TraceId, error);
        clientRequest_->CompleteRequest(error);
    }
    else
    {
        WriteNoise(TraceComponent, "{0} no client request to complete", this->TraceId);
    }
}

void RolloutContext::SetRetryTimer(ThreadpoolCallback const & callback, TimeSpan const retryDelay)
{
    AcquireExclusiveLock lock(timerLock_);

    retryTimer_ = Timer::Create(TimerTagDefault, [this, callback](TimerSPtr const & timer) 
        { 
            timer->Cancel();
            callback();
        },
        true); // allow concurrency
    retryTimer_->Change(retryDelay);
}

void RolloutContext::SetCompletionError(ErrorCode const & error)
{
    error_ = ErrorCode::FirstError(error_, error);
}

ErrorCode RolloutContext::SwitchReplaceToCreate(StoreTransaction const & storeTx)
{
    shouldKeepInQueue_ = true;
    this->Status = RolloutStatus::Enum::Pending;
    return storeTx.Update(*this);
}
