// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

OperationStatusMap::OperationStatusMap()
    : lock_()
{
}

void OperationStatusMap::Initialize(wstring const & id)
{
    {
        AcquireWriteLock lock(lock_);
        auto status = OperationStatus();
        status.State = OperationState::NotStarted;
        map_[id] = status;
    }
}

bool OperationStatusMap::TryInitialize(wstring const & id, __out OperationStatus & initializedStatus)
{
    {
        AcquireWriteLock lock(lock_);
        auto iter = map_.find(id);
        if (iter == map_.end())
        {
            initializedStatus = OperationStatus();
            initializedStatus.State = OperationState::NotStarted;
            map_[id] = initializedStatus;
            return true;
        }
        else
        {
            initializedStatus = iter->second;
            return false;
        }
    }
}

OperationStatus OperationStatusMap::Get(wstring const & id) const
{
    {
        AcquireReadLock lock(lock_);
        auto iter = map_.find(id);
        if (iter != map_.end())
        {
            return iter->second;
        }
        else
        {
            return OperationStatus();
        }
    }
}

bool OperationStatusMap::TryGet(wstring const & id, __out OperationStatus & status) const
{
    {
        AcquireReadLock lock(lock_);
        auto iter = map_.find(id);
        if (iter != map_.end())
        {
            status = iter->second;
            return true;
        }
        else
        {
            return false;
        }
    }
}

void OperationStatusMap::Set(wstring const & id, OperationStatus const status)
{
    {
        AcquireWriteLock lock(lock_);
        auto iter = map_.find(id);
        if (iter != map_.end())
        {
            iter->second = status;
        }
    }
}

void OperationStatusMap::Set(wstring const & id, ErrorCode const & error, OperationState::Enum const & state, ULONG const failureCount, ULONG const internalFailureCount)
{
    {
        AcquireWriteLock lock(lock_);
        auto iter = map_.find(id);
        if (iter != map_.end())
        {
            iter->second.State = state;
            iter->second.LastError = error;
            iter->second.FailureCount = failureCount;
            iter->second.InternalFailureCount = internalFailureCount;
        }
    }
}

void OperationStatusMap::SetState(wstring const & id, OperationState::Enum const & state)
{
    {
        AcquireWriteLock lock(lock_);
        auto iter = map_.find(id);
        if (iter != map_.end())
        {
            iter->second.State = state;
        }
    }
}

void OperationStatusMap::SetFailureCount(wstring const & id, ULONG const failureCount)
{
    {
        AcquireWriteLock lock(lock_);
        auto iter = map_.find(id);
        if (iter != map_.end())
        {
            iter->second.FailureCount = failureCount;
        }
    }
}

void OperationStatusMap::SetError(wstring const & id, ErrorCode const & error)
{
    {
        AcquireWriteLock lock(lock_);
        auto iter = map_.find(id);
        if (iter != map_.end())
        {
            iter->second.LastError = error;
        }
    }
}

bool OperationStatusMap::TryRemove(std::wstring const & id, __out OperationStatus & status)
{
    {
        AcquireWriteLock lock(lock_);
        auto iter = map_.find(id);
        if (iter != map_.end())
        {
            status = iter->second;
            map_.erase(iter);
            return true;
        }
        else
        {
            status = OperationStatus();
            return false;
        }
    }
}

void OperationStatusMap::WriteTo(__in TextWriter & w,FormatOptions const &) const
{
    {
        AcquireReadLock lock(lock_);
        for(auto iter = map_.begin(); iter != map_.end(); ++iter)
        {
            w.Write("Id={0}, Status={1}", iter->first, iter->second);
        }
    }
}
