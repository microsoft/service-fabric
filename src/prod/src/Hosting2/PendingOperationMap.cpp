// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

// ********************************************************************************************************************
// PendingOperationMap::PendingOperation implementation
//
struct PendingOperationMap::PendingOperation
{
     DENY_COPY(PendingOperation)

public:
    PendingOperation(wstring const & operationId)
        : Status(operationId),
        Operation()
    {
    }

public:
    OperationStatus Status;
    AsyncOperationSPtr Operation;
    uint64 InstanceId;
};

// ********************************************************************************************************************
// PendingOperationMap implementation
//
PendingOperationMap::PendingOperationMap()
    : map_(),
    lock_(),
    isClosed_(false)
{
}

PendingOperationMap::~PendingOperationMap()
{
}

ErrorCode PendingOperationMap::Get(
    std::wstring const & operationId,
    __out uint64 & instanceId,
    __out Common::AsyncOperationSPtr & operation,
    __out OperationStatus & status) const
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(operationId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }
        else
        {
            status = iter->second->Status;
            operation = iter->second->Operation;
            instanceId = iter->second->InstanceId;

            return ErrorCode(ErrorCodeValue::Success);
        }
    }
}

ErrorCode PendingOperationMap::GetStatus(
    wstring const & operationId,
    __out OperationStatus & status) const
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(operationId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }
        else
        {
            status = iter->second->Status;
            return ErrorCode(ErrorCodeValue::Success);
        }
    }
}

ErrorCode PendingOperationMap::GetPendingAsyncOperations(__out vector<AsyncOperationSPtr> & pendingAyncOperations) const
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        for(auto iter = map_.begin(); iter != map_.end(); ++iter)
        {
            if(iter->second->Operation)
            {
                pendingAyncOperations.push_back(iter->second->Operation);
            }
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode PendingOperationMap::Start(
    wstring const & operationId,
    uint64 instanceId,
    AsyncOperationSPtr const & operation,
    __out OperationStatus & status)
{
    return this->Start(operationId, instanceId, operation, nullptr, status);
}

ErrorCode PendingOperationMap::Start(
    wstring const & operationId,
    uint64 instanceId,
    AsyncOperationSPtr const & operation,
    ShouldReplaceCallback const & shouldReplaceCallback,
    __out OperationStatus & status)
{
    AsyncOperationSPtr cancelOperation = nullptr;

    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(operationId);
        if (iter != map_.end())
        {
            if ((shouldReplaceCallback != 0) && (shouldReplaceCallback(iter->second->Operation, iter->second->InstanceId, iter->second->Status)))
            {
                cancelOperation = iter->second->Operation;
                map_.erase(iter);
            }
            else
            {
                status = iter->second->Status;
                return ErrorCode(ErrorCodeValue::AlreadyExists);
            }
        }

        {
            auto pendingOperation = make_shared<PendingOperation>(operationId);
            pendingOperation->Status.State = OperationState::InProgress;
            pendingOperation->Operation = operation;
            pendingOperation->InstanceId = instanceId;

            status = pendingOperation->Status;
            map_.insert(make_pair(operationId, move(pendingOperation)));
        }
    }

    if (cancelOperation != nullptr)
    {
        cancelOperation->Cancel();
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode PendingOperationMap::UpdateStatus(wstring const & operationId, uint64 instanceId, OperationStatus const status)
{
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(operationId);
        if ((iter == map_.end()) || (iter->second->InstanceId != instanceId))
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }
        else
        {
            if(iter->second->Status.State != OperationState::Completed)
	        {
                iter->second->Status = status;
                return ErrorCode(ErrorCodeValue::Success);
            }
            else
            {
                return ErrorCode(ErrorCodeValue::InvalidState);
            }
        }
    }
}

ErrorCode PendingOperationMap::Complete(wstring const & operationId, uint64 instanceId, OperationStatus const status)
{
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(operationId);
        if ((iter == map_.end()) || (iter->second->InstanceId != instanceId))
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }
        else
        {
            iter->second->Status = status;
            iter->second->Operation.reset();
            return ErrorCode(ErrorCodeValue::Success);
        }
    }
}

void PendingOperationMap::Remove(wstring const & operationId, uint64 instanceId)
{
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return; }

        auto iter = map_.find(operationId);
        if ((iter != map_.end()) && (iter->second->InstanceId == instanceId))
        {
            map_.erase(iter);
        }
    }
}

vector<AsyncOperationSPtr> PendingOperationMap::Close()
{
    vector<AsyncOperationSPtr> removed;
    {
        AcquireWriteLock lock(lock_);
        if (!isClosed_)
        {
            for(auto iter = map_.begin(); iter != map_.end(); ++iter)
            {
                if (iter->second->Operation)
                {
                    removed.push_back(iter->second->Operation);
                }
            }
            map_.clear();
            isClosed_ = true;
        }
    }

    return removed;
}

bool PendingOperationMap::IsEmpty() const
{
    AcquireWriteLock lock(lock_);
    return map_.empty();
}
